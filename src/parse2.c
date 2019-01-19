#include <parse2.h>
#include <ndebug.h>
#include <unistd.h> /* read() */
#include <yamlutils.h>

/* subsystems which implement parsers */
#include <iface.h>
#include <field.h>
#include <rule.h>
#include <process.h>


/*	private parse functions
 */
static void parse(const unsigned char *buf,
			size_t buf_len,
			int outfd);
static int parse_mapping(yaml_document_t *doc,
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
void parse_free(void *arg)
{
	if (!arg)
		return;
	struct parse *ps = arg;

	NB_wrn("close parse fd %d", ps->fdin);
	if (ps->fdin != -1)
		close(ps->fdin);
	if (ps->fdout != -1 && ps->fdout != ps->fdin)
		close(ps->fdout);

	free(ps->buf);
	free(ps);
}

/*	parse_new()
 */
struct parse *parse_new(int fdin, int fdout)
{
	struct parse *ret = NULL;
	NB_die_if(fdin < 0 || fdout < 0,
		"input '%d' output '%d' not sane", fdin, fdout);
	NB_die_if(!(
		ret = calloc(sizeof(struct parse), 1)
		), "alloc %zu", sizeof(struct parse));
	ret->fdin = fdin;
	ret->fdout = fdout;
	NB_wrn("open parse fd %d", ret->fdin);
	return ret;
die:
	free(ret);
	return NULL;
}

/*	parse_cmptail()
 * strcmp() the _tail_ end of 'buf' against 'check', with some sanity checks for buf_len.
 * Return semantics identical to strcmp().
 */
static int parse_cmptail(const char *check, const unsigned char *buf, size_t buf_pos)
{
	size_t check_len = strlen(check);
	if (check_len > buf_pos)
		return -1;
	return strncmp(check, (const char *)&buf[buf_pos-check_len], check_len);
}

/*	parse_callback()
 */
int parse_callback(int fd, uint32_t events, void *context)
{
	struct parse *ps = context;

	/* read all available bytes into our contiguous buffer without blocking */
	ssize_t res = 0;
	do {
		/* extend memory as needed */
		if (ps->buf_pos + PIPE_BUF > ps->buf_len) {
			NB_die_if(ps->buf_len >= PARSE_BUF_MAX,
				"parse buffer exceeds max %zuB", PARSE_BUF_MAX);
			ps->buf_len += PIPE_BUF;
			NB_die_if(!(
				ps->buf = realloc(ps->buf, ps->buf_len)
				), "size %zu", ps->buf_len);
		}
		res = read(ps->fdin, &ps->buf[ps->buf_pos], PIPE_BUF);
		if (res > 0)
			ps->buf_pos += res;
	} while (res == PIPE_BUF); /* a read of < PIPE_BUF implies no data left */

	/* Read of 0 means end of doc when piping from file (or stdin)
	 * or client closed if this is a net socket.
	 * We return non-0 to indicate we should be deregistered from epoll
	 * and our destructor (parse_free)executed.
	 */
	if (res == 0)
		return 1;

	/* Possible conditions for "document complete":
	 * - two newlines in a row (and buffer not empty)
	 * - the standard YAML termination sequence '...' on it's own line
	 * Until then, wait for more data.
	 * NOTE complication because some inputs terminate lines with LF
	 * while others with CRLF.
	 *
	 * TODO: this seems to give a false positive when a single line of space
	 * is followed by a comment line which DOES begin with spaces
	 */
	if (parse_cmptail("\n\n", ps->buf, ps->buf_pos)
			&& parse_cmptail("\r\n\r\n", ps->buf, ps->buf_pos)
			&& parse_cmptail("...\n", ps->buf, ps->buf_pos)
			&& parse_cmptail("...\r\n", ps->buf, ps->buf_pos))
		return 0;

	/* force string termination before parsing */
	ps->buf[ps->buf_pos] = '\0';
	parse(ps->buf, ps->buf_pos, ps->fdout);

die:
	ps->buf_pos = 0; /* reset input buffer */
	return 0;
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
 * PROTIP for those who aren't intimately familiar with the YAML spec:
 * 0. a "scalar" is a string
 * 1. a "sequence" is a list
 * 2. a "mapping" is an object
 * 3. "flow style" is JSONish and "block style" is YAMLish
 *
 * TODO: parse multiple documents in a text stream
 */
static void parse(const unsigned char *buf, size_t buf_len, int outfd)
{
	int err_cnt = 0;

	yaml_document_t doc = {{0}};
	yaml_node_t *root = NULL;
	yaml_parser_t parser = {0};

	yaml_document_t outdoc = {{0}};
	int outroot = 0; /* use to test successful init of 'outdoc' */
	yaml_emitter_t emitter = {0};

	/* Init emitter and output doc first:
	 * error count can then be dumped if there are later failures.
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
	root = yaml_document_get_root_node(&doc);

	/* If document parses correctly but doesn't yield the expected
	 * structure, ignore it silently.
	 * By "silently" is meant DO NOT EMIT (in 'die' block below).
	 */
	if (root && root->type == YAML_MAPPING_NODE)
		err_cnt += parse_mapping(&doc, root, &outdoc, outroot);
	else
		outroot = 0;

die:
	/* serialize err_cnt and dump outdoc if at all possible */
	if (outroot) {
		NB_err_if(
			y_pair_insert_nf(&outdoc, outroot, "errors", "%d", err_cnt)
			, "error emitting 'errors'");
		/* This will destroy 'outdoc', don't call yaml_document_delete() */
		yaml_emitter_dump(&emitter, &outdoc);

	} else {
		yaml_document_delete(&outdoc);
	}

	yaml_emitter_delete(&emitter);
	yaml_parser_delete(&parser);
	yaml_document_delete(&doc);
	return;
}

/*	parse_mapping()
 * Parse a top-level mapping should contain one or more "mode" mappings,
 * e.g. "add || prn || del".
 * Each mode-mapping should contain a list of "directive" mappings.
 * Example:
 * ```
 * add: [iface: enp0s3]
 * ```
 */
static int parse_mapping(yaml_document_t *doc, yaml_node_t *root,
		yaml_document_t *outdoc, int outroot)
{
	int err_cnt = 0;

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
			NB_err("node '%s' not a sequence (list)", keyname);
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

		/* child functions will append replies here */
		int reply_list = yaml_document_add_sequence(outdoc, NULL,
					YAML_BLOCK_SEQUENCE_STYLE);

		/* process children list objects */
		for (yaml_node_item_t *child = val->data.sequence.items.start;
			child < val->data.sequence.items.top;
			child++)
		{
			yaml_node_t *mapping = yaml_document_get_node(doc, *child);
			if (mapping->type != YAML_MAPPING_NODE) {
				NB_err("node not a map");
				continue;
			}

			/* peek into node:
			 * first pair _must_ contain "subsystem: name" tuple.
			 */
			yaml_node_pair_t *pair = mapping->data.mapping.pairs.start;
			yaml_node_t *key = yaml_document_get_node(doc, pair->key);
			const char *keyval = (const char *)key->data.scalar.value;

			/* match 'subsystem' and hand off to the relevant parser */
			if (!strcmp("iface", keyval) || !strcmp("i", keyval))
				err_cnt += iface_parse(mode, doc, mapping, outdoc, reply_list);

			else if (!strcmp("field", keyval) || !strcmp("f", keyval))
				err_cnt += field_parse(mode, doc, mapping, outdoc, reply_list);

			else if (!strcmp("rule", keyval) || !strcmp("r", keyval))
				err_cnt += rule_parse(mode, doc, mapping, outdoc, reply_list);

			else if (!strcmp("process", keyval) || !strcmp("p", keyval))
				err_cnt += process_parse(mode, doc, mapping, outdoc, reply_list);

			else
				NB_err("subsystem '%s' unknown", keyval);
		}

		/* emit sequence of "reply mappings" from subroutines */
		NB_err_if(
			y_pair_insert_obj(outdoc, outroot, keyname, reply_list)
			, "");
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
	ssize_t ret = write((int)(uintptr_t)data, buffer, size);
	return (ret == size);
}
