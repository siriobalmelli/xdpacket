#ifndef parse_h_
#define parse_h_

/*	parse.h
 * Routines for parsing YAML into xdpk data structures
 *
 * (c) 2018 Alan Morrissett and Sirio Balmelli
 */
#include <xdpk.h> /* must always be first */
#include <nonlibc.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <yaml.h>

enum data_type { ASCII_TYPE = 0, DECIMAL_TYPE = 1, HEX_TYPE = 2, BIN_TYPE = 3};

int64_t parse_16bit(const char *begin, size_t len, bool *err);

uint32_t parse_uint16(const char *begin, size_t len, bool *err);

int32_t parse_int16(const char *begin, size_t len, bool *err);

uint64_t parse_value(const char *begin, size_t blen, 
				size_t flen, uint8_t mask, bool *err);

struct xdpk_field xdpk_field_parse(char *fldstr, size_t len, uint64_t *hash);

void printl_utf8(unsigned char *str, size_t len, FILE *stream);

void print_yaml_node(yaml_document_t *document_p, yaml_node_t *node);

int tree_depth(yaml_document_t *document_p, yaml_node_t *node);

void print_yaml_document(yaml_document_t *document_p);

#endif /* parse_h_ */
