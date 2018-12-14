#include <parse2.h>
#include <parse_util.h>
#include <ndebug.h>
#include <unistd.h> /* read() */
#include <limits.h> /* PIPE_BUF */

/* subsystems which implement parsers */
#include <iface.h>


/*	private parse functions
 */
static void parse(const unsigned char *buf,
			size_t buf_len,
			int outfd);
static int parse_exec(yaml_document_t *doc,
			yaml_node_t *root,
			yaml_document_t *outdoc,
			int outroot);
static int parse_write_handler(void *data,
			unsigned char *buffer,
			size_t size);


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
	NB_die_if(!(
		ret = calloc(sizeof(struct parse), 1)
		), "alloc %zu", sizeof(struct parse));
	ret->fdin = fdin;
	ret->fdout = fdout;
	return ret;
die:
	free(ret);
	return NULL;
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
			NB_die_if(!(
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

	//NB_inf("process %zu bytes", ps->buf_pos);

	/* force string termination */
	ps->buf[ps->buf_pos] = '\0';

	parse(ps->buf, ps->buf_pos, ps->fdout);

die:
	ps->buf_pos = 0; /* always reset input buffer */
}


/*	parse()
 * Parse and execute directives from 'doc'.
 * Outputs (emits) results to 'outfd' as a valid YAML document.
 *
 * NOTE that 'outfd' _may_ be stdout: do NOT use any print directives
 * (printf(), NB_inf(), NB_prn(), etc) which may poison 'outfd' (render it invalid YAML),
 * in this function OR in any child parser it calls(!).
 * Use e.g. NB_err(), NB_wrn() etc - they will write to stderr which is fair game.
 *
 * PROTIP for all those who aren't intimately familiar with the YAML spec:
 * 0. a "scalar" is a string (should *also* be e.g. a number, but doesn't seem to be the case)
 * 1. a "sequence" is a list
 * 2. a "mapping" is an object
 * 3. "flow style" is JSONish and "block style" is YAMLish
 */
static void parse(const unsigned char *buf, size_t buf_len, int outfd)
{
	int err_cnt = 0;
	int outroot = 0;
	yaml_document_t doc = {{0}};
	yaml_document_t outdoc = {{0}};
	yaml_document_t virgin_doc = {{0}};
	yaml_parser_t parser = {0};
	yaml_emitter_t emitter = {0};

	/* Init emitter and output doc first: error count can then be dumped
	 * if there are later failures.
	 */
	NB_die_if(!
		yaml_emitter_initialize(&emitter)
		, "emitter init");
	yaml_emitter_set_output(&emitter, parse_write_handler, (void*)(long)outfd);
	yaml_emitter_set_canonical(&emitter, 0);
	yaml_emitter_set_unicode(&emitter, 1);
	NB_die_if(!
		yaml_document_initialize(&outdoc, NULL, NULL, NULL, 0, 0)
		, "outdoc init");
	NB_die_if(!(
		outroot = yaml_document_add_mapping(&outdoc, NULL, YAML_BLOCK_MAPPING_STYLE)
		), "outdoc root node");

	/* parse the given YAML */
	NB_die_if(!
		yaml_parser_initialize(&parser)
		, "parser init");
	yaml_parser_set_input_string(&parser, buf, buf_len);
	NB_die_if(!
		yaml_parser_load(&parser, &doc)
		, "Invalid YAML, parse failed at position %zu",
		parser.context_mark.column);
	yaml_node_t *root = yaml_document_get_root_node(&doc);
	NB_die_if(!root || (root->type != YAML_MAPPING_NODE), "");

	/* execute directives */
	err_cnt = parse_exec(&doc, root, &outdoc, outroot);

die:
	/* serialize err_cnt and dump outdoc if at all possible */
	if (outroot && memcmp(&outdoc, &virgin_doc, sizeof(outdoc))) {
		int key, val;
		char tx[8];
		key = yaml_document_add_scalar(&outdoc, NULL,
			(yaml_char_t *)"parse errors", -1, YAML_PLAIN_SCALAR_STYLE);
		int sz = snprintf(tx, sizeof(tx), "%d", err_cnt);
		val = yaml_document_add_scalar(&outdoc, NULL,
			(yaml_char_t *)tx, sz, YAML_PLAIN_SCALAR_STYLE);
		NB_err_if(!key || !val, "serialize 'parse errors'");
		NB_err_if(!
			yaml_document_append_mapping_pair(&outdoc, outroot, key, val)
			, "append error to outdoc");
		/* This will destroy 'outdoc', don't call yaml_document_delete() */
		yaml_emitter_dump(&emitter, &outdoc);

	} else {
		NB_err("could not serialize 'parse errors: %d'", err_cnt);
		yaml_document_delete(&outdoc);
	}

	yaml_emitter_delete(&emitter);
	yaml_parser_delete(&parser);
	yaml_document_delete(&doc);
	return;
}

/*	parse_exec()
 */
static int parse_exec(yaml_document_t *doc, yaml_node_t *root,
		yaml_document_t *outdoc, int outroot)
{
	int err_cnt = 0;

	/* Process document.
	 * NOTE: only a 'mode' is a valid root node;
	 * under a mode we expect a list of directives. e.g.:
	 * Establish the mode; then handle each directive in the list.
	 */
	for (yaml_node_pair_t *pair = root->data.mapping.pairs.start;
		pair < root->data.mapping.pairs.top;
		pair++)
	{
		yaml_node_t *key = yaml_document_get_node(doc, pair->key);
		const char *keyname = (const char *)key->data.scalar.value;
		yaml_node_t *val = yaml_document_get_node(doc, pair->value);
		enum parse_mode mode = PARSE_INVALID;

		/* sanity */
		if (val->type != YAML_SEQUENCE_NODE) {
			NB_err("node '%s' not a sequence (list).\n"
				"Toplevel node should be a mode ('xdpk' || 'add' || 'del' || 'prn'),\n"
				"containing a list of directives e.g.\n"
				"```yaml\n"
				"add:\n"
				"  - iface: eth0\n"
				"```",
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
			NB_err("'%s' is not a valid mode", keyname);
			continue;
		}

		/* process children list objects */
		for (yaml_node_item_t *child = val->data.sequence.items.start;
			child < val->data.sequence.items.top;
			child++)
		{
			yaml_node_t *node = yaml_document_get_node(doc, *child);

			/* peek into node:
			 * first pair _must_ contain "subsystem: name" tuple.
			 */
			yaml_node_pair_t *pair = node->data.mapping.pairs.start;
			yaml_node_t *key = yaml_document_get_node(doc, pair->key);
			const char *subsystem = (const char *)key->data.scalar.value;

			/* match 'subsystem' and hand off to the relevant parser */
			if (!strcmp("iface", subsystem)) {
				err_cnt += iface_parse(mode, doc, node);

			} else if (!strcmp("field", subsystem)) {
				NB_err("'field' not implemented yet");

			} else if (!strcmp("node", subsystem)) {
				NB_err("'node' not implemented yet");

			} else {
				NB_err("subsystem '%s' unknown", subsystem);
			}
		}
	}
	return err_cnt;
}

/*	parse_write_handler()
 * Callback for parser emitter (replies being posted back to input).
 * This tactic chosen in lieu of allocating a buffer for emitter output
 * and checking for OOM etc.
 *
 * @data (context passed to yaml_emitter_set_output()) is actually output fd.
 *
 * NOTE that return semantics are "1=success; 0=error"
 * (clearly the libyaml folk are not familiar with POSIX).
 */
static int parse_write_handler(void *data, unsigned char *buffer, size_t size)
{
	ssize_t ret = write((int)data, buffer, size);
	return (ret == size);
}
