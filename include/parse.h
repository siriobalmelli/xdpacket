#ifndef parse_h_
#define parse_h_

/*	parse.h
 * Routines for parsing YAML into xdpk data structures
 *
 * (c) 2018 Alan Morrissett and Sirio Balmelli
 */
#include <xdpacket.h> /* must always be first */
#include <nonlibc.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <yaml.h>
#include <field.h>
#include <matcher.h>
#include <parse_util.h>

typedef enum cmd_type {
	CMD_NO_TYPE = 0,
	CMD_IF_TYPE = 1,
	CMD_FLD_TYPE = 2,
	CMD_ACTION_TYPE = 3,
	CMD_MATCH_TYPE = 4
} cmd_type_t;

typedef enum if_cmd_type {
	IF_CMD_NONE = 0,
	IF_CMD_ADD = 1,
	IF_CMD_SHOW = 2,
	IF_CMD_DEL = 3
} if_cmd_type_t;

typedef enum fld_cmd_type {
	FLD_CMD_NONE = 0,
	FLD_CMD_ADD = 1,
	FLD_CMD_SHOW = 2
} fld_cmd_type_t;

typedef enum action_cmd_type {
	ACTION_CMD_NONE = 0,
	ACTION_CMD_ADD = 1,
	ACTION_CMD_DEL = 2
} action_cmd_type_t;

typedef enum match_cmd_type {
	MATCH_CMD_NONE = 0,
	MATCH_CMD_ADD = 1,
	MATCH_CMD_DEL = 2
} match_cmd_type_t;

typedef enum direction_type {
	DIRECTION_NONE = 0,
	DIRECTION_IN = 1,
	DIRECTION_OUT = 2
} direction_type_t;

typedef struct xdpk_command {
	cmd_type_t type;
	union {
		struct {
			if_cmd_type_t cmdtype;
			char *name;
			uint8_t *value;
		} intf;

		struct {
			fld_cmd_type_t cmdtype;
			char *name;
			uint8_t *value;
			uint64_t hash;

			struct xdpk_field fld;
		} field;

		struct {
			action_cmd_type_t cmdtype;
			char *name;
			uint8_t *value;

			struct xdpk_field fld;
		} action;

		struct {
			match_cmd_type_t cmdtype;
			char *name;
			char *intf;
			direction_type_t dir;
			selector_t *sel;
			char *action;
		} match;
	};
} xdpk_command_t;

int parse_commands(char *cmdstr, size_t cmdlen, xdpk_command_t **cmds, int *numcmds);
int parse_if(yaml_document_t *document, yaml_node_t *node, xdpk_command_t **cmds, int *numcmd);
int parse_field(yaml_document_t *document, yaml_node_t *node, xdpk_command_t **cmds, int *numcmd);
int parse_action(yaml_document_t *document, yaml_node_t *node, xdpk_command_t **cmds, int *numcmd);
int parse_match(yaml_document_t *document, yaml_node_t *node, xdpk_command_t **cmds, int *numcmd);
int parse_xdpk(yaml_document_t *document, yaml_node_t *node, xdpk_command_t **cmds, int *numcmd);
void delete_commands(xdpk_command_t **cmds, int numcmds);
const char * yaml_node_type(const yaml_node_t *node);
char *xdpk_command_print(xdpk_command_t *cmd);

struct xdpk_field xdpk_field_parse(char *fldstr, size_t len, uint64_t *hash);

uint8_t parse_uint8(const char *begin, size_t len, bool *err);

int64_t parse_16bit(const char *begin, size_t len, bool *err);

uint16_t parse_uint16(const char *begin, size_t len, bool *err);

int32_t parse_int16(const char *begin, size_t len, bool *err);

uint64_t parse_value(const char *begin, size_t blen,
				size_t flen, uint8_t mask, bool *err);

int32_t parse_offset(const char *begin, size_t len, bool *err);


void printl_utf8(unsigned char *str, size_t len, FILE *stream);

void print_yaml_node(yaml_document_t *document_p, yaml_node_t *node);

int tree_depth(yaml_document_t *document_p, yaml_node_t *node);

void print_yaml_document(yaml_document_t *document_p);

#endif /* parse_h_ */
