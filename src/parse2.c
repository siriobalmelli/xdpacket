#include <parse2.h>
#include <zed_dbg.h>
#include <unistd.h> /* read() */
#include <fcntl.h> /* fcntl() for nonblocking I/O */
#include <limits.h> /* PIPE_BUF */
#include <parse_util.h>

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
	/* if we can make input fd nonblocking, we are inputting from a terminal */
	Z_die_if(
		fcntl(fdin, F_SETFL, fcntl(fdin, F_GETFL) | O_NONBLOCK)
		, "")
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

	for (yaml_node_pair_t *i_node_p = root->data.mapping.pairs.start;
		i_node_p < root->data.mapping.pairs.top;
		i_node_p++)
	{
		yaml_node_t *key_node_p =
			yaml_document_get_node(&document, i_node_p->key);
		yaml_node_t *val_node_p =
			yaml_document_get_node(&document, i_node_p->value);

		Z_log(Z_inf, "%s: %s",
			key_node_p->data.scalar.value,
			val_node_p->data.scalar.value
			);
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

	/* a line with only '\n' is our cue to parse all accumulated text as one document */
	if (res != 1 || ps->buf[ps->buf_pos-1] != '\n')
		return;
	/* force string termination */
	ps->buf[ps->buf_pos] = '\0';

	parse_exec(ps->buf, ps->buf_pos);

out:
	ps->buf_pos = 0; /* always reset input buffer */
}


