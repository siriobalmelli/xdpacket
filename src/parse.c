#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fnv.h>
#include <ndebug.h>
#include <yaml.h>
#include <field.h>
#include <parse.h>
#include <matcher.h>
#include "offset_defs.h"
#include <yamlutils.h>

static const char *cmd_usage =
	"[xdpk:|if:|field:|action:|match:] { <command body> }";

static const char *if_usage =
	"if: { [add: <name>|show: <ere>|del: <if>] }";

static const char *field_usage =
	"field: { add: <name>, offt: <offt>, len: <len>, [mask: <mask>] }"
	"\n"
	"field: { show: <ere> }";



/*	parse_commands()
 * Parses a string and returns an array of xdpk_command.
 * Caller owns the storage.
 * Returns 0 on success.
 */
int parse_commands(char *cmdstr, size_t cmdlen,
				xdpk_command_t **cmds, int *numcmds)
{
	int err_cnt = 0;
	yaml_parser_t parser;

	yaml_parser_initialize(&parser);
	yaml_parser_set_input_string(&parser, (yaml_char_t*)cmdstr, cmdlen);

	yaml_document_t document;
	if (!yaml_parser_load(&parser, &document)) {
		yaml_mark_t *mark = &parser.context_mark;
		NB_die_if(true, "Invalid YAML, "
				"parse failed at position %zu", mark->column);
	}

	yaml_node_t *root = yaml_document_get_root_node(&document);
	NB_die_if((root->type != YAML_MAPPING_NODE)
			|| (y_map_count(root) > 1)
		, "Usage: '%s'", cmd_usage);

	for (yaml_node_pair_t *i_node_p = root->data.mapping.pairs.start;
		i_node_p < root->data.mapping.pairs.top;
		i_node_p++)
	{
		yaml_node_t *key_node_p =
			yaml_document_get_node(&document, i_node_p->key);
		yaml_node_t *val_node_p =
			yaml_document_get_node(&document, i_node_p->value);
		char *key = (char*)key_node_p->data.scalar.value;
		int keylen = strlen(key);

		if (!strncmp("xdpk", key, keylen)) {
			NB_die_if(parse_xdpk(&document, val_node_p,
				cmds, numcmds), "Failed parse of xdpk");
		} else if (!strncmp("if", key, keylen)) {
			NB_die_if(parse_if(&document, val_node_p,
				cmds, numcmds), "Failed parse of if");
		} else if (!strncmp("field", key, keylen)) {
			NB_die_if(parse_field(&document, val_node_p,
				cmds, numcmds), "Failed parse of field");
		} else if (!strncmp("action", key, keylen)) {
			NB_die_if(parse_action(&document, val_node_p,
				cmds, numcmds), "Failed parse of action");
		} else if (!strncmp("match", key, keylen)) {
			NB_die_if(parse_match(&document, val_node_p,
				cmds, numcmds), "Failed parse of match");
		} else {
			NB_die_if(true, "Usage: '%s'", cmd_usage);
		}
	}

die:
	return err_cnt;
}


/*	parse_if()
 */
int parse_if(yaml_document_t *document, yaml_node_t *node,
					xdpk_command_t **cmds, int *numcmd)
{
	int err_cnt = 0;
	xdpk_command_t *cmd = NULL;
	NB_die_if(node->type != YAML_MAPPING_NODE, "Usage: '%s'", if_usage);

	cmd = calloc(sizeof(xdpk_command_t), 1);
	cmd->type = CMD_IF_TYPE;

	for (yaml_node_pair_t *i_node_p = node->data.mapping.pairs.start;
		i_node_p < node->data.mapping.pairs.top;
		i_node_p++)
	{
		yaml_node_t *key_node_p =
			yaml_document_get_node(document, i_node_p->key);
		char *key = (char*)key_node_p->data.scalar.value;
		yaml_node_t *val_node_p =
			yaml_document_get_node(document, i_node_p->value);
		char *val = (char*)val_node_p->data.scalar.value;

		if (!strncmp("add", key, strlen(key))) {
			cmd->intf.cmdtype = IF_CMD_ADD;
			cmd->intf.name = strdup(val);
		} else if (!strncmp("show", key, strlen(key))) {
			cmd->intf.cmdtype = IF_CMD_SHOW;
			cmd->intf.value = (uint8_t*)strdup(val);
		} else if (!strncmp("del", key, strlen(key))) {
			cmd->intf.cmdtype = IF_CMD_DEL;
			cmd->intf.name = strdup(val);
		} else {
			NB_die_if(true, "Bad field: '%s'", key);
		}
	}

	cmds[0] = cmd;
	*numcmd = 1;

	return err_cnt;
die:
	delete_commands(&cmd, 1);
	*cmds = NULL;
	numcmd = 0;
	return err_cnt;
}

int parse_field(yaml_document_t *document, yaml_node_t *node,
					xdpk_command_t **cmds, int *numcmd)
{
	int err_cnt = 0;
	xdpk_command_t *cmd = NULL;
	NB_die_if(node->type != YAML_MAPPING_NODE, "Usage: '%s'", field_usage);

	cmd = calloc(sizeof(xdpk_command_t), 1);
	cmd->type = CMD_FLD_TYPE;
	struct xdpk_field *fld = &(cmd->field.fld);
	fld->mask = 0xff;

	for (yaml_node_pair_t *i_node_p = node->data.mapping.pairs.start;
			i_node_p < node->data.mapping.pairs.top; i_node_p++) {
		yaml_node_t *key_node_p =
			yaml_document_get_node(document, i_node_p->key);
		char *key = (char*)key_node_p->data.scalar.value;
		yaml_node_t *val_node_p =
			yaml_document_get_node(document, i_node_p->value);
		char *val = (char*)val_node_p->data.scalar.value;
		bool err = false;

		if (!strncmp("offt", key, strlen(key))) {
			fld->offt = parse_offset((char*)val, strlen(val), &err);
			NB_die_if(err, "Invalid offset");
		} else if (!strncmp("length", key, strlen(key))) {
			fld->len = parse_uint16(val, strlen(val), &err);
			NB_die_if(err, "Invalid length");
		} else if (!strncmp("mask", key, strlen(key))) {
			fld->mask = parse_uint8(val, strlen(val), &err);
			NB_die_if(err, "Invalid mask");
		} else if (!strncmp("value", key, strlen(key))) {
			cmd->field.value = (uint8_t*)strdup(val);
		} else if (!strncmp("add", key, strlen(key))) {
			cmd->field.cmdtype = FLD_CMD_ADD;
			cmd->field.name = strdup(val);
		} else if (!strncmp("show", key, strlen(key))) {
			cmd->field.cmdtype = FLD_CMD_SHOW;
			cmd->field.value = (uint8_t*)strdup(val);
		} else {
			NB_die_if(true, "Bad field: '%s'", key);
		}
	}

	switch(cmd->field.cmdtype) {
	case FLD_CMD_ADD:
		NB_die_if(!xdpk_field_valid(*fld), "Invalid field");
		NB_die_if(!cmd->field.name || !strlen(cmd->field.name),
						"No name given for field");
		break;
	case FLD_CMD_SHOW:
		break;
	default:
		NB_die_if(true, "Bad field command type");
	}

	cmds[0] = cmd;
	*numcmd = 1;

	return err_cnt;
die:
	delete_commands(&cmd, 1);
	*cmds = NULL;
	numcmd = 0;
	return err_cnt;
}

static const char *action_usage =
	"action: { [add: <action>, out: <target>|add: <action>, "
		"offt: <offt>, len: <len>, val:, <val>] }";

int parse_action(yaml_document_t *document, yaml_node_t *node,
					xdpk_command_t **cmds, int *numcmd)
{
	int err_cnt = 0;
	xdpk_command_t *cmd = NULL;
	NB_die_if(node->type != YAML_MAPPING_NODE, "Usage: '%s'", action_usage);

	cmd = calloc(sizeof(xdpk_command_t), 1);
	cmd->type = CMD_ACTION_TYPE;
	struct xdpk_field *fld = &(cmd->field.fld);
	fld->mask = 0xff;

	for (yaml_node_pair_t *i_node_p = node->data.mapping.pairs.start;
			i_node_p < node->data.mapping.pairs.top; i_node_p++) {
		yaml_node_t *key_node_p =
			yaml_document_get_node(document, i_node_p->key);
		char *key = (char*)key_node_p->data.scalar.value;
		yaml_node_t *val_node_p =
			yaml_document_get_node(document, i_node_p->value);
		char *val = (char*)val_node_p->data.scalar.value;
		bool err = false;

		if (!strncmp("add", key, strlen(key))) {
			cmd->action.cmdtype = ACTION_CMD_ADD;
			cmd->action.name = strdup(val);
		} else if (!strncmp("out", key, strlen(key))) {
			cmd->action.value = (uint8_t*)strdup(val);
		} else if (!strncmp("offt", key, strlen(key))) {
			fld->offt = parse_offset((char*)val, strlen(val), &err);
			NB_die_if(err, "Invalid offset");
		} else if (!strncmp("length", key, strlen(key))) {
			fld->len = parse_uint16(val, strlen(val), &err);
			NB_die_if(err, "Invalid length");
		} else if (!strncmp("mask", key, strlen(key))) {
			fld->mask = parse_uint8(val, strlen(val), &err);
			NB_die_if(err, "Invalid mask");
		} else if (!strncmp("val", key, strlen(key))) {
			cmd->action.value = (uint8_t*)strdup(val);
			NB_die_if(err, "Invalid value");
		} else {
			NB_die_if(true, "Bad field: '%s'", key);
		}
	}

	cmds[0] = cmd;
	*numcmd = 1;

	return err_cnt;
die:
	delete_commands(&cmd, 1);
	*cmds = NULL;
	numcmd = 0;
	return err_cnt;
}

int parse_match(yaml_document_t *document, yaml_node_t *node,
					xdpk_command_t **cmds, int *numcmd)
{
	int err_cnt = 0;
	xdpk_command_t *cmd = NULL;
	NB_die_if(node->type != YAML_MAPPING_NODE, "Usage: '%s'", action_usage);

	cmd = calloc(sizeof(xdpk_command_t), 1);
	cmd->type = CMD_MATCH_TYPE;

	for (yaml_node_pair_t *i_node_p = node->data.mapping.pairs.start;
			i_node_p < node->data.mapping.pairs.top; i_node_p++) {
		yaml_node_t *key_node_p =
			yaml_document_get_node(document, i_node_p->key);
		char *key = (char*)key_node_p->data.scalar.value;
		yaml_node_t *val_node_p =
			yaml_document_get_node(document, i_node_p->value);
		char *val = (char*)val_node_p->data.scalar.value;

		if (!strncmp("add", key, strlen(key))) {
			cmd->match.cmdtype = MATCH_CMD_ADD;
			cmd->match.name = strdup(val);
		} else if (!strncmp("if", key, strlen(key))) {
			cmd->match.intf = strdup(val);
		} else if (!strncmp("dir", key, strlen(key))) {
			if (!strncmp("in", val, strlen(val)))
				cmd->match.dir = DIRECTION_IN;
			else if (!strncmp("out", val, strlen(val)))
				cmd->match.dir = DIRECTION_OUT;
			else
				NB_die_if(true, "Invalid direction '%s'", val);
		} else if (!strncmp("sel", key, strlen(key))) {
			cmd->match.sel = NULL;
		} else if (!strncmp("act", key, strlen(key))) {
			cmd->match.action = strdup(val);
		} else {
			NB_die_if(true, "Bad field: '%s'", key);
		}
	}

	cmds[0] = cmd;
	*numcmd = 1;

	return err_cnt;
die:
	delete_commands(&cmd, 1);
	*cmds = NULL;
	numcmd = 0;
	return err_cnt;
}

int parse_xdpk(yaml_document_t *document, yaml_node_t *node, xdpk_command_t **cmd, int *numcmd)
{
	NB_inf("parse node 'xdpk'");
	NB_die_if(false, "");

die:
	return err_cnt;
}


/*	xdpk_command_print()
 * Allocate and return a string containing
 * the definition of a xdpk_command_t.
 */
char *xdpk_command_print(xdpk_command_t *cmd)
{
	char buf[4096];
	memset(buf, 0, sizeof(buf));
	char * fldstr;
	switch(cmd->type) {
	case CMD_IF_TYPE:
		switch(cmd->intf.cmdtype) {
		case IF_CMD_ADD:
			sprintf(buf, "IF_TYPE: { add: %s }",
				cmd->intf.name ? cmd->intf.name : "None");
			break;
		case IF_CMD_SHOW:
			sprintf(buf, "IF_TYPE: { show: %s }", cmd->intf.value
				? (char*)cmd->intf.value : "None");
			break;
		case IF_CMD_DEL:
			sprintf(buf, "IF_TYPE: { del: %s }",
				cmd->intf.name ? cmd->intf.name : "None");
			break;
		default:
			sprintf(buf, "IF_TYPE: { BAD COMMAND TYPE }");
			break;
		}
		break;
	case CMD_FLD_TYPE:
		switch(cmd->field.cmdtype) {
		case FLD_CMD_ADD:
			fldstr = xdpk_field_print(cmd->field.fld);
			sprintf(buf, "FLD_TYPE: { name: %s, field: %s, value:"
				"'%s', hash: 0x%0lx }", cmd->field.name, fldstr,
				cmd->field.value, cmd->field.hash);
			free(fldstr);
			break;
		case FLD_CMD_SHOW:
			sprintf(buf, "FLD_TYPE: { show: %s }", cmd->field.value);
			break;
		default:
			sprintf(buf, "FLD_TYPE: { BAD COMMAND TYPE }");
			break;
		}
		break;
	case CMD_ACTION_TYPE:
		sprintf(buf, "ACTION_TYPE: { name: %s }", cmd->action.name);
		break;
	case CMD_MATCH_TYPE:
		sprintf(buf, "MATCH_TYPE: { name: %s }", cmd->match.name);
		break;
	default:
		sprintf(buf, "BOGUS_TYPE");
		break;
	}

	char *rv = malloc(strlen(buf)+1);
	strcpy(rv, buf);

	return rv;
}

void delete_commands(xdpk_command_t **cmds, int numcmds)
{
	NB_die_if(cmds == NULL, "");

	for (int i = 0; i < numcmds; i++) {
		if (cmds[i] == NULL)
			continue;
		switch (cmds[i]->type) {
		case CMD_IF_TYPE:
			if (cmds[i]->intf.name != NULL)
				free(cmds[i]->intf.name);
			if (cmds[i]->intf.value != NULL)
				free(cmds[i]->intf.value);
			break;
		case CMD_FLD_TYPE:
			if (cmds[i]->field.name != NULL)
				free(cmds[i]->field.name);
			if (cmds[i]->field.value != NULL)
				free(cmds[i]->field.value);
			break;
		case CMD_ACTION_TYPE:
			if (cmds[i]->action.name != NULL)
				free(cmds[i]->action.name);
			if (cmds[i]->action.value != NULL)
				free(cmds[i]->action.value);
			break;
		case CMD_MATCH_TYPE:
			if (cmds[i]->match.name != NULL)
				free(cmds[i]->match.name);
			if (cmds[i]->match.intf != NULL)
				free(cmds[i]->match.intf);
			if (cmds[i]->match.sel != NULL)
				// delete the selector;
			if (cmds[i]->match.action != NULL)
				free(cmds[i]->match.action);
			break;
		default:
			continue;
		}

		free(cmds[i]);
	}

die:
	return;
}

/* Obsolete (probably) */
const char *yaml_usage = "Usage: { offset: <offset>, len: <len>, mask: <mask> }";

/* Obsolete (probably) */
struct xdpk_field xdpk_field_parse(char *fldstr, size_t len, uint64_t *hash)
{
	struct xdpk_field fld = {0, 0, 0xff, 0};
	char *value = NULL;

	yaml_parser_t parser;
	yaml_document_t document;
	yaml_node_t *node;
	yaml_node_t *key_node_p;
	yaml_node_t *val_node_p;
	bool err = false;

	yaml_parser_initialize(&parser);
	yaml_parser_set_input_string(&parser, (yaml_char_t*)fldstr, len);
	if (!yaml_parser_load(&parser, &document)) {
		yaml_mark_t *mark = &parser.context_mark;
		fprintf(stderr, "parse failed: %s\n", parser.problem);
		fprintf(stderr, "%s\n", fldstr);
		for (int i = 0; i <= mark->column; i++)
			fprintf(stderr, " ");
		fprintf(stderr, "^\n");
		goto error;
	}

	node = yaml_document_get_root_node(&document);
	if (node->type != YAML_MAPPING_NODE) {
		fprintf(stdout, "%s\n", yaml_usage);
		fprintf(stdout, "%s\n", fldstr);
		goto error;
	}
	yaml_node_pair_t *i_node_p;
	char *key;
	char *val;
	for (i_node_p = node->data.mapping.pairs.start; i_node_p < node->data.mapping.pairs.top; i_node_p++) {
		key_node_p = yaml_document_get_node(&document, i_node_p->key);
		key = (char*)key_node_p->data.scalar.value;
		val_node_p = yaml_document_get_node(&document, i_node_p->value);
		val = (char*)val_node_p->data.scalar.value;
		if (!strncmp("offset", key, strlen(key))) {
			fld.offt = (int32_t)parse_offset((char*)val, strlen(val), &err);
			if (err) goto error;
		} else if (!strncmp("length", key, strlen(key))) {
			fld.len = (uint16_t)parse_uint16(val, strlen(val), &err);
			if (err) goto error;
		} else if (!strncmp("mask", key, strlen(key))) {
			fld.mask = (uint8_t)parse_uint8(val, strlen(val), &err);
			if (err) printf("ERROR on parse_uint8\n");
			if (err) goto error;
		} else if (!strncmp("value", key, strlen(key))) {
			value = val;
		} else {
			fprintf(stdout, "%s\n", yaml_usage);
			fprintf(stdout, "%s\n", fldstr);
			goto error;
		}
	}

	if ((xdpk_field_valid(fld)) && (value != NULL))
		*hash = parse_value(value, strlen(val), fld.len, fld.mask, &err);

error:
	return fld;
}


void printl_utf8(unsigned char *str, size_t len, FILE *stream)
{
	fwrite(str, 1, len, stream);
}

/*	yaml_node_type
 *
 */
const char * yaml_node_type(const yaml_node_t *node)
{
	switch (node->type) {
	case YAML_NO_NODE:
		return "YAML_NO_NODE";
	case YAML_SCALAR_NODE:
		return "YAML_SCALAR_NODE";
	case YAML_SEQUENCE_NODE:
		return "YAML_SEQUENCE_NODE";
	case YAML_MAPPING_NODE:
		return "YAML_MAPPING_NODE";
	default:
		return "UNKNOWN_NODE";
	}
}

/* Get the depth of the tree starting at the given yaml_node_item_t
*/
int tree_depth(yaml_document_t *document_p, yaml_node_t *node)
{
	int depth = 1;
	int subnode_depth = 0;

	yaml_node_t *next_node_p;
	yaml_node_item_t *i_node;
	yaml_node_pair_t *i_node_p;

	switch (node->type) {
	case YAML_NO_NODE:
	case YAML_SCALAR_NODE:
		break;
	case YAML_SEQUENCE_NODE:
		for (i_node = node->data.sequence.items.start; i_node < node->data.sequence.items.top; i_node++) {
			int d;
			next_node_p = yaml_document_get_node(document_p, *i_node);
			if (next_node_p) {
				d = tree_depth(document_p, next_node_p);
				if (d > subnode_depth)
					subnode_depth = d;
			}
		}
		break;
	case YAML_MAPPING_NODE:
		for (i_node_p = node->data.mapping.pairs.start; i_node_p < node->data.mapping.pairs.top; i_node_p++) {
			int d;
			next_node_p = yaml_document_get_node(document_p, i_node_p->value);
			if (next_node_p) {
				d = tree_depth(document_p, next_node_p);
				if (d > subnode_depth)
					subnode_depth = d;
			}
		}
		break;
	default:
		fputs("Unknown node type\n", stderr);
		exit(1);
	}

	return depth + subnode_depth;
}

void print_yaml_node(yaml_document_t *document_p, yaml_node_t *node)
{
	static int x = 0;
	x++;
	int node_n = x;

	yaml_node_t *next_node_p;

	switch (node->type) {
	case YAML_NO_NODE:
		printf("Empty node(%d):\n", node_n);
		break;
	case YAML_SCALAR_NODE:
		printf("Scalar node(%d):\n", node_n);
		printl_utf8(node->data.scalar.value, node->data.scalar.length, stdout);
		puts("");
		break;
	case YAML_SEQUENCE_NODE:
		printf("Sequence node(%d):\n", node_n);
		yaml_node_item_t *i_node;
		for (i_node = node->data.sequence.items.start; i_node < node->data.sequence.items.top; i_node++) {
			next_node_p = yaml_document_get_node(document_p, *i_node);
			if (next_node_p)
				print_yaml_node(document_p, next_node_p);
		}
		break;
	case YAML_MAPPING_NODE:
		printf("Mapping node(%d):\n", node_n);
		yaml_node_pair_t *i_node_p;
		for (i_node_p = node->data.mapping.pairs.start; i_node_p < node->data.mapping.pairs.top; i_node_p++) {
			next_node_p = yaml_document_get_node(document_p, i_node_p->key);
			if (next_node_p) {
				puts("Key:");
				print_yaml_node(document_p, next_node_p);
			} else {
				fputs("Couldn't find next node\n", stderr);
				exit(1);
			}

			next_node_p = yaml_document_get_node(document_p, i_node_p->value);
			if (next_node_p) {
				puts("Value:");
				print_yaml_node(document_p, next_node_p);
			} else {
				fputs("Couldn't find next node\n", stderr);
				exit(1);
			}
		}
		break;
	default:
		fputs("Unknown node type\n", stderr);
		exit(1);
	}

	printf("END NODE(%d)\n", node_n);
}

void print_yaml_document(yaml_document_t *document_p)
{
	puts("NEW DOCUMENT");

	print_yaml_node(document_p, yaml_document_get_root_node(document_p));

	puts("END DOCUMENT");
}

/*	parse_value()
 * Get the for an xdpk_field and return its hash value.
 */
uint64_t parse_value(const char *begin, size_t blen, size_t flen, uint8_t mask, bool *err)
{
	int64_t dval = 0;
	uint64_t hval = 0;
	uint64_t hash = 0;

	uint8_t *buf = (uint8_t*) calloc(flen, sizeof(uint8_t));

	switch (parse_data_type(begin, blen))
	{
	case DECIMAL_TYPE:
		if (flen > 16) {
			*err = true;
			goto error;
		}
		dval = parse_decimal(begin, blen, 0x7FFFFFFFFFFFFFFFL, err);
		if (*err) goto error;
		for (int i = 0; i < flen; i++) {
			// Put into network byte order
			buf[flen-i-1] = (dval >> (8*i)) & 0xFF;
		}
		hash = xdpk_hash((const void*)buf, flen, mask, NULL);
		//hash = fnv_hash64(NULL, (void*)buf, flen);
		break;
	case HEX_TYPE:
		if ((blen - 2) <= 16) {
			hval = parse_hex(begin+2, blen-2, 0xFFFFFFFFFFFFFFFFL, err);
			if (*err) goto error;
			for (int i = 0; i < flen; i++) {
				// Put into network byte order
				buf[flen-i-1] = (hval >> (8*i)) & 0xFF;
			}
			hash = xdpk_hash((const void*)buf, flen, mask, NULL);
			//hash  = fnv_hash64(NULL, (void*)buf, flen);
		}
		else {
			parse_long_hex(begin+2, blen-2, buf, flen, err);
			if (*err) goto error;
			hash = xdpk_hash((const void*)buf, flen, mask, NULL);
			//hash = fnv_hash64(NULL, (void*)buf, flen);
		}
		break;
	case BIN_TYPE:
		if ((flen > 16) || ((blen - 1) > 64)) {
			*err = true;
			goto error;
		}
		hval = parse_binary(begin + 1, blen-1, 0xFFFFFFFFFFFFFFFF, err);
		if (*err) goto error;
		for (int i = 0; i < flen; i++) {
			// Put into network byte order
			buf[flen-i-1] = (hval >> (8*i)) & 0xFF;
		}
		hash = xdpk_hash((const void*)buf, flen, mask, NULL);
		//hash  = fnv_hash64(NULL, (void*)buf, flen);
		break;
	case ASCII_TYPE:
		parse_ascii(begin, blen, buf, flen, err);
		if (*err) goto error;
		hash = fnv_hash64(NULL, (void*)buf, flen);
		break;
	default:
		*err = true;
		return 0;
	}

error:
	if (buf) free(buf);


	return hash;
}

/*	lookup_offset
 * Lookup the numeric value for an ASCII represented offset.  This does
 * a linear search on the assumption that the list of symbols is
 * short and it doesn't matter for UI speeds.
 */
int32_t lookup_offset(const char *begin, size_t len, bool *err) {
	for (int i = 0; i < nsyms; i++) {
		if (!strncmp(begin, xdpk_offt_sym[i].symbol, len))
			return xdpk_offt_sym[i].offt;
	}
	*err = true;
	return 0;
}

/*	parse_offset()
 * Parse the offset field of an xdpk grammar string, which may be
 * numeric or ASCII valued.  Max value is max(mlen)/8 == 65536/8 == 8192.
 */
int32_t parse_offset(const char *begin, size_t len, bool *err) {
	int32_t offt = (int32_t)parse_32bit(begin, len, err);
	if (*err) {
		*err = false;
		offt = (int32_t)lookup_offset(begin, len, err);
	}

	return offt;
}
