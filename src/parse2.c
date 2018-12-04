#include <parse2.h>
#include <zed_dbg.h>
#include <unistd.h> /* read() */
#include <fcntl.h> /* fcntl() for nonblocking I/O */
#include <limits.h> /* PIPE_BUF */
#include <parse_util.h>


const char *parse_modes[] = {
	"PARSE_INVALID",
	"PARSE_ADD",
	"PARSE_DEL",
	"PARSE_PRN"
};


/*	parse_free()
 */
void parse_free(struct parse *ps)
{
	if (!ps)
		return;
	free(ps->buf);
	free(ps);
}

/*	parse_new()
 */
struct parse *parse_new(int fdin, int fdout)
{
	struct parse *ret = NULL;
	Z_die_if(!(
		ret = calloc(sizeof(struct parse), 1)
		), "alloc %zu", sizeof(struct parse));
	ret->fdin = fdin;
	ret->fdout = fdout;
	return ret;
out:
	free(ret);
	return NULL;
}


/*	parse_exec()
 */
void parse_exec(const unsigned char *doc, size_t doc_len)
{
	yaml_parser_t parser = { 0 };
	yaml_document_t document = { {0} };

	Z_die_if(!
		yaml_parser_initialize(&parser)
		, "");
	yaml_parser_set_input_string(&parser, doc, doc_len);

	Z_die_if(!
		yaml_parser_load(&parser, &document)
		, "Invalid YAML, parse failed at position %zu",
		parser.context_mark.column);

	yaml_node_t *root = yaml_document_get_root_node(&document);
	Z_die_if(!root || (root->type != YAML_MAPPING_NODE), "");

	/* Only a 'mode' is a valid root node;
	 * under a mode we expect a list of directives. e.g.:
	 * Establish the mode; then handle each directive in the list.
	 */
	for (yaml_node_pair_t *node = root->data.mapping.pairs.start;
		node < root->data.mapping.pairs.top;
		node++)
	{
		yaml_node_t *key = yaml_document_get_node(&document, node->key);
		const char *keyname = (const char *)key->data.scalar.value;
		yaml_node_t *val = yaml_document_get_node(&document, node->value);
		enum parse_mode mode = PARSE_INVALID;

		/* sanity */
		if (val->type != YAML_SEQUENCE_NODE) {
			Z_log(Z_err, "node '%s' not a sequence (list)\n"
				"toplevel node should be a mode, e.g.\n"
				"'xdpk' || 'add' || 'del' || 'prn';\n"
				"containing a list of directives e.g.\n"
				"\n"
				"```yaml\n"
				"add:\n"
				"  - iface: eth0\n"
				"```\n",
				keyname);
			continue;
		}

		/* get mode */
		if (!strcmp("xdpk", keyname)
			|| !strcmp("add", keyname)
			|| !strcmp("a", keyname))
		{
			mode = PARSE_ADD;
		} else if (!strcmp("delete", keyname)
			|| !strcmp("del", keyname)
			|| !strcmp("d", keyname))
		{
			mode = PARSE_DEL;
		} else if (!strcmp("print", keyname)
			|| !strcmp("prn", keyname)
			|| !strcmp("p", keyname))
		{
			mode = PARSE_PRN;
		} else {
			Z_log(Z_err, "'%s' is not a valid mode", keyname);
			continue;
		}
		Z_log(Z_inf, "mode %s", parse_mode_prn(mode));

		/* what's this got to do with the price of fish? */
		if (val->data.sequence.style == YAML_BLOCK_SEQUENCE_STYLE) {
			Z_log(Z_inf, "block sequence");
		} else if (val->data.sequence.style == YAML_FLOW_SEQUENCE_STYLE) {
			Z_log(Z_inf, "flow sequence");
		}

		/* process children */
		for (yaml_node_item_t *child = val->data.sequence.items.start;
			child < val->data.sequence.items.start;
			child++)
		{
			Z_log(Z_inf, "item %d", *child);
#if 0
			child->
			yaml_node_t *ckey = yaml_document_get_node(&document, child->key);
			yaml_node_t *cval = yaml_document_get_node(&document, child->value);
			Z_log(Z_inf, "%s: %s",
				ckey->data.scalar.value,
				cval->data.scalar.value
				);
#endif
		}
	}

out:
	yaml_parser_delete(&parser);
	yaml_document_delete(&document);
	return;
}


/*	parse_callback()
 */
void parse_callback(int fd, uint32_t events, void *context)
{
	struct parse *ps = context;

	/* read all available bytes into our contiguous buffer without blocking */
	ssize_t res = 0;
	do {
		/* extend memory as needed */
		if (ps->buf_pos + PIPE_BUF > ps->buf_len) {
			ps->buf_len += PIPE_BUF;
			Z_die_if(!(
				ps->buf = realloc(ps->buf, ps->buf_len)
				), "size %zu", ps->buf_len);
		}
		res = read(ps->fdin, &ps->buf[ps->buf_pos], PIPE_BUF);
		if (res > 0)
			ps->buf_pos += res;
	} while (res == PIPE_BUF); /* a read of < PIPE_BUF implies no data left */

	/* read of 0 means end of doc when piping from file or stdin */
	if (res == 0)
		close(fd); /* we will never be called again */

	/* a line with only '\n' is our cue to parse all accumulated text as one document */
	else if (res != 1 || ps->buf[ps->buf_pos-1] != '\n')
		return;

	//Z_log(Z_inf, "process %zu bytes", ps->buf_pos);

	/* force string termination */
	ps->buf[ps->buf_pos] = '\0';

	parse_exec(ps->buf, ps->buf_pos);

out:
	ps->buf_pos = 0; /* always reset input buffer */
}


