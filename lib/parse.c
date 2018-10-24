#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fnv.h>
#include <zed_dbg.h>
#include <yaml.h>
#include <field.h>
#include <parse.h>
#include "offset_defs.h"


int parse_if(yaml_document_t *document, yaml_node_t *node, xdpk_command_t **cmd, int *numcmd) 
{
	Z_log(Z_inf, "parse node 'if'");
	Z_die_if(node->type != YAML_MAPPING_NODE, "Top node not a map: '%s'", yaml_node_type(node));

out:
	return err_cnt;
}

int parse_field(yaml_document_t *document, yaml_node_t *node, xdpk_command_t **cmd, int *numcmd) 
{
	int err_cnt = 0;
	Z_log(Z_inf, "parse node 'field'");
	Z_die_if(node->type != YAML_MAPPING_NODE, "Top node not a map: '%s'", yaml_node_type(node));
	char *value = NULL;
	xdpk_command_t command;
	memset((void*)&command, 0, sizeof(command));
	struct xdpk_field *fld = &command.field.fld;
	fld->mask = 0xff;
	command.type = CMD_FLD_TYPE;

	yaml_node_pair_t *i_node_p;
	yaml_node_t *key_node_p;
	yaml_node_t *val_node_p;
	char *key;
	char *val;
	bool err = false;
	for (i_node_p = node->data.mapping.pairs.start; i_node_p < node->data.mapping.pairs.top; i_node_p++) {
		key_node_p = yaml_document_get_node(document, i_node_p->key);
		key = (char*)key_node_p->data.scalar.value;
		val_node_p = yaml_document_get_node(document, i_node_p->value);
		val = (char*)val_node_p->data.scalar.value;
		if (!strncmp("offt", key, strlen(key))) {
			fld->offt = (int32_t)parse_offset((char*)val, strlen(val), &err);
			Z_die_if(err, "Bad offset");
		} else if (!strncmp("length", key, strlen(key))) {
			fld->len = (uint16_t)parse_uint16(val, strlen(val), &err);
			Z_die_if(err, "Bad length");
		} else if (!strncmp("mask", key, strlen(key))) {
			fld->mask = (uint8_t)parse_uint8(val, strlen(val), &err);
			Z_die_if(err, "Bad mask");
		} else if (!strncmp("value", key, strlen(key))) {
			value = val;
		} else if (!strncmp("add", key, strlen(key))) {
			command.field.name = val;
		} else {
			Z_die_if(true, "Bad field: '%s'", key);
		}
	}
	char * fldstr = xdpk_field_print(*fld);
	Z_log(Z_inf, "field: %s", fldstr);
	free(fldstr);
	Z_die_if(!xdpk_field_valid(*fld), "Invalid field");
	Z_die_if(!command.field.name || !strlen(command.field.name), "No name given for field");

	*cmd = calloc(sizeof(xdpk_command_t), 1);
	memcpy((void*)*cmd, (void*)&command, sizeof(xdpk_command_t));
	(*cmd)->field.value = (uint8_t*)strdup(value);
	*numcmd = 1;
out:
	Z_log(Z_inf, "parse_field err_cnt: %d\n", err_cnt);
	
	return err_cnt;
}

int parse_action(yaml_document_t *document, yaml_node_t *node, xdpk_command_t **cmd, int *numcmd) 
{
	Z_log(Z_inf, "parse node 'action'");
	Z_die_if(false, "");

out:
	return err_cnt;
}

int parse_match(yaml_document_t *document, yaml_node_t *node, xdpk_command_t **cmd, int *numcmd) 
{
	Z_log(Z_inf, "parse node 'match'");
	Z_die_if(false, "");

out:
	return err_cnt;
}

int parse_xdpk(yaml_document_t *document, yaml_node_t *node, xdpk_command_t **cmd, int *numcmd) 
{
	Z_log(Z_inf, "parse node 'xdpk'");
	Z_die_if(false, "");

out:
	return err_cnt;
}

/*	parse_commands
 * Parses a string and returns an array of xdpk_command.  Caller
 * owns the storage.  Returns 0 on success.
 */
int parse_commands(char *cmdstr, size_t cmdlen, xdpk_command_t *cmds[], int *numcmds) 
{
	yaml_parser_t parser;
	yaml_document_t document;
	yaml_node_t *node;
	yaml_node_t *key_node_p;
	yaml_node_t *val_node_p;
	bool err = false;
	*numcmds = 0;
	*cmds = NULL;
	yaml_parser_initialize(&parser);
	yaml_parser_set_input_string(&parser, (yaml_char_t*)cmdstr, cmdlen);
	if (!yaml_parser_load(&parser, &document)) {
		yaml_mark_t *mark = &parser.context_mark;
		err = true;
		Z_die_if(true, "Parse failed at position %d", mark->column);
	}
	node = yaml_document_get_root_node(&document);
	Z_die_if(node->type != YAML_MAPPING_NODE, "Top node not a map: '%s'", yaml_node_type(node));
	int topcnt = yaml_map_count(node);
	Z_die_if(topcnt > 1, "Top node count > 1: %d", topcnt);

	yaml_node_pair_t *i_node_p;
	char *key;
	char *val;
	for (i_node_p = node->data.mapping.pairs.start; i_node_p < node->data.mapping.pairs.top; i_node_p++) {
		key_node_p = yaml_document_get_node(&document, i_node_p->key);
		key = (char*)key_node_p->data.scalar.value;
		val_node_p = yaml_document_get_node(&document, i_node_p->value);
		val = (char*)val_node_p->data.scalar.value;
		
		int keylen = strlen(key);
		if (!strncmp("xdpk", key, keylen)) {
			Z_die_if(parse_xdpk(&document, val_node_p, cmds, numcmds), "Failed parse of xdpk");
		} else if (!strncmp("if", key, keylen)) {
			Z_die_if(parse_if(&document, val_node_p, cmds, numcmds), "Failed parse of if");
		} else if (!strncmp("field", key, keylen)) {
			Z_die_if(parse_field(&document, val_node_p, cmds, numcmds), "Failed parse of field");
		} else if (!strncmp("action", key, keylen)) {
			Z_die_if(parse_action(&document, val_node_p, cmds, numcmds), "Failed parse of action");
		} else if (!strncmp("match", key, keylen)) {
			Z_die_if(parse_match(&document, val_node_p, cmds, numcmds), "Failed parse of match");
		} else {
			Z_die_if(true, "Bad key on top node: '%s'", key);
		}
	}
	char *cmdtmp = xdpk_command_print(cmds[0]);
	Z_log(Z_inf, "command: %s\n", cmdtmp);
	if (cmdtmp) free(cmdtmp);
out:
	return (int)err;
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
		sprintf(buf, "IF_TYPE: { name: %s }", cmd->intf.name);
		break;
	case CMD_FLD_TYPE:
		fldstr = xdpk_field_print(cmd->field.fld);
		sprintf(buf, "FLD_TYPE: { name: %s, field: %s, value: '%s', hash: 0x%0lx }",
			cmd->field.name, fldstr, cmd->field.value, cmd->field.hash);
		free(fldstr);
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

	Z_log(Z_inf, "command_print: %s\n", buf);
	char *rv = malloc(strlen(buf)+1);
	strcpy(rv, buf);
	
	return rv;
}

void delete_commands(xdpk_command_t **cmds, int numcmds)
{
	Z_die_if(cmds == NULL, "");
	for (int i = 0; i < numcmds; i++) {
		Z_die_if(cmds[i] == NULL, "");
		switch (cmds[i]->type) {
		case CMD_IF_TYPE:
			if (cmds[i]->intf.name != NULL)
				free(cmds[i]->intf.name);
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
			break;
		case CMD_MATCH_TYPE:
			if (cmds[i]->match.name != NULL)
				free(cmds[i]->match.name);
			break;
		default:
			continue;
		}
	}

out:
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

/*	yaml_map_count
 *
 */
 int yaml_map_count(const yaml_node_t *node) 
 {
	int cnt = 0;
	Z_die_if(node->type != YAML_MAPPING_NODE, "Not a mapping node");
	yaml_node_pair_t *i_node_p;
	for (i_node_p = node->data.mapping.pairs.start; i_node_p < node->data.mapping.pairs.top; i_node_p++)
		cnt++;

out:
	return cnt;
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


/*	parse_data_type()
 * The starting characters of a value determine whether it's hexadecimal ('0x'),
 * binary ('b'), decimal ('[-]0-9'), or ASCII (all other).
 */	
enum data_type parse_data_type(const char *begin, size_t len) {
	switch (*begin) {
	case '0':
		if (len == 1)
			return DECIMAL_TYPE;
		else if ((len > 1) && *(begin+1) == 'x')
			return HEX_TYPE;
		break;
	case 'b':
		if ((len > 1) && (*(begin+1) == '0' || *(begin+1) == '1'))
			return BIN_TYPE;
		break;
	case '-':
		if ((len > 1) && isdigit(*(begin+1)))
			return DECIMAL_TYPE;
		break;
	default:
		if (isdigit(*begin))
			return DECIMAL_TYPE;
		break;
	}

	return ASCII_TYPE;
}


/*	parse_binary
 *
 */
uint64_t parse_binary(const char *begin, size_t len, uint64_t max, bool *err) 
{
	uint64_t val = 0;
	if (len > 64)
		len = 64;
	const char *cp = begin;
	for (int i = 0; (i < len) && (val <= max); i++, cp++) {	
		switch(*cp) {
		case '0':
			val = (val << 1);
			break;
		case '1':
			val = (val << 1) + 1;
			break;
		default:
			*err = true;
			goto error;
		}
	}

error:
	return val;
}

/*	parse_decimal
 *
 */
int64_t parse_decimal(const char *begin, size_t len, uint64_t max, bool *err) 
{
	bool neg = false;
	if (*begin == '-') {
		neg = true;
		begin++;
	}

	int64_t val = 0;
	if (len > 19)
		len = 19;
	const char *cp = begin;
	for (int i = 0; (i < len) && (val <= max); i++, cp++) {	
		switch(*cp) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			val = (val * 10) + (*cp - '0');
			break;
		default:
			*err = true;
			goto error;
		}
	}
	if (neg) val = -val;

error:
	return val;
}

/*	hex_value
 */
uint8_t hex_value(const char c) {
	return (c > '9') ? (c - 'A' + 10) : (c - '0');
}

/*	parse_hex
 *
 */
 uint64_t parse_hex(const char *begin, size_t len, uint64_t max, bool *err) 
 {
	uint64_t val = 0;
	if (len > 8)
		len = 8;
	if ((len % 2) != 0) {
		*err = true;
		goto error;
	}
	const char *end = begin + len;
	for (const char *cp = begin; cp < end; cp += 2) {
		int c1 = *cp;
		int c2 = *(cp + 1);
		if (!isxdigit(c1) || !isxdigit(c2)) {
			*err = true;
			goto error;
		}
		val = (val << 8) | (hex_value(toupper(c1)) << 4) 
			| hex_value(toupper(c2));
	}

error:
	return val;
}

/*	parse_long_hex
 * Parse fields longer than will fit in a 64-bit integer.
 */
void parse_long_hex(const char *begin, size_t len, 
			uint8_t *buf, size_t flen, bool *err)
{
	if ((len % 2) != 0) {
		*err = true;
		goto error;
	}
	int i = 0;
	for (const char *cp = begin; (i < len) && (i < flen); cp += 2, i++) {
		char c1 = *cp;
		char c2 = *(cp + 1);
		if (!isxdigit(c1) || !isxdigit(c2)) {
			*err = true;
			goto error;
		}
		buf[i] = (hex_value(toupper(c1)) << 4) 
			| hex_value(toupper(c2));
	}

error:
	return;
}

/*	parse_ascii
 *
 */
void parse_ascii(const char *begin, size_t len, uint8_t *buf, 
						size_t flen, bool *err)
{
	const char *cp = begin;
	for (int i = 0; (i < len) && (i < flen); cp++, i++) {
		buf[i] = *cp;
	}

	return;
}

/*	parse_16bit
 * Parse a 16-bit integer from an ASCII string.  Numeric value may be
 * decimal, binary or hex.  This determines which and calls the appropriate
 * sub-parsing function.  64-bits is returned so that this doesn't have to 
 * account for overflow.  Caller can truncate.
 */
int64_t parse_16bit(const char *begin, size_t len, bool *err) {
	switch (parse_data_type(begin, len))
	{
	case DECIMAL_TYPE:
		return parse_decimal(begin, len, 0xFFFF, err);
		break;
	case HEX_TYPE:
		return parse_hex(begin+2, len-2, 0xFFFF, err);
		break;
	case BIN_TYPE:
		return parse_binary(begin+1, len-1, 0xFFFF, err);
		break;
	default:
		*err = true;
		return 0;
	}
}


/*	parse_32bit
 * Parse a 32-bit integer from an ASCII string.  Numeric value may be
 * decimal, binary or hex.  This determines which and calls the appropriate
 * sub-parsing function.  64-bits is returned so that this doesn't have to 
 * account for overflow.  Caller can truncate.
 */
int64_t parse_32bit(const char *begin, size_t len, bool *err) {
	switch (parse_data_type(begin, len))
	{
	case DECIMAL_TYPE:
		return parse_decimal(begin, len, 0xFFFFFFFF, err);
		break;
	case HEX_TYPE:
		return parse_hex(begin+2, len-2, 0xFFFFFFFF, err);
		break;
	case BIN_TYPE:
		return parse_binary(begin + 1, len-1, 0xFFFFFFFF, err);
		break;
	default:
		*err = true;
		return 0;
	}
}

/*	parse_uint8()
 * Parse an unsigned 8-bit integer from an ASCII string.  32-bits is returned
 * so that this doesn't have to account for overflow.  Caller can truncate.
 */
uint32_t parse_uint8(const char *begin, size_t len, bool *err) {
	int64_t num = parse_16bit(begin, len, err);
	if ((num < 0) || (num > 0x00FF)) *err = true;
	
	return (uint32_t)num;
}

/*	parse_uint16()
 * Parse an unsigned 16-bit integer from an ASCII string.  32-bits is returned
 * so that this doesn't have to account for overflow.  Caller can truncate.
 */
uint32_t parse_uint16(const char *begin, size_t len, bool *err) {
	int64_t num = parse_16bit(begin, len, err);
	if ((num < 0) || (num > 0xFFFF)) *err = true;
	
	return (uint32_t)num;
}

/*	parse_int16()
 * Parse a signed 16-bit integer from an ASCII string.  32-bits is returned
 * so that this doesn't have to account for overflow.  Caller can truncate.
 */
int32_t parse_int16(const char *begin, size_t len, bool *err) {
	int64_t num = parse_16bit(begin, len, err);
	if ((num < -32768) || (num > 32767)) *err = true;
	
	return (int32_t)num;
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