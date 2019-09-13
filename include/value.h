#ifndef value_h_
#define value_h_

/*	value.h
 * Common code for parsing a user-supplied "value string".
 *
 * 1. value: the verbatim string supplied by the user,
 *    so that it can be printed back as-is when outputting YAML.
 * 2. parsed: a big-endian (network-order) array of bytes which can be:
 *    - written literally to a packet when writing/mangling
 *    - hashed to verify a match.
 * 3. rendered: a human-readable reprentation of 'parsed'
 *    as a series of hex digits.
 */

#include <stdint.h>
#include <stdlib.h>


int value_parse		(const char *value,
	       		 uint8_t *parsed,
			 size_t len);

char *value_render	(const uint8_t *parsed,
			size_t len);


#endif /* value_h_ */
