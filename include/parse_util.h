#ifndef parse_util_h_
#define parse_util_h_

/*	parse_util.h
 * Utilities used by parse, broken out for simplicity only
 */

#include <yaml.h>
#include <stdbool.h>
#include <string.h>
#include <nonlibc.h>
#include <ndebug.h>
#include <stdint.h>


enum data_type { ASCII_TYPE = 0, DECIMAL_TYPE = 1, HEX_TYPE = 2, BIN_TYPE = 3};


enum data_type parse_data_type(const char *begin, size_t len);
uint64_t parse_binary(const char *begin, size_t len, uint64_t max, bool *err);
int64_t parse_decimal(const char *begin, size_t len, uint64_t max, bool *err);
uint8_t hex_value(const char c);
uint64_t parse_hex(const char *begin, size_t len, uint64_t max, bool *err);
void parse_long_hex(const char *begin, size_t len, uint8_t *buf, size_t flen, bool *err);
void parse_ascii(const char *begin, size_t len, uint8_t *buf, size_t flen, bool *err);
int64_t parse_16bit(const char *begin, size_t len, bool *err);
int64_t parse_32bit(const char *begin, size_t len, bool *err);
uint8_t parse_uint8(const char *begin, size_t len, bool *err);
int32_t parse_int16(const char *begin, size_t len, bool *err);

#endif /* parse_util_h_ */
