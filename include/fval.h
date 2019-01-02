#ifndef fval_h_
#define fval_h_

/*	fval.h
 * Representation of user-supplied (fieldname, value) tuple,
 * given to us as strings.
 *
 * NOTES:
 * 1. We should always be able to reproduce the exact user input,
 *    which is why 'value' is stored as given.
 * 2. The user gave 'fieldname' as a string, but we store a pointer so that:
 *    - we clearly show that 'fieldname' _must_ refer to a valid field
 *    - we save a string lookup to get the field's 'set'
 *    - we are forced to clean up dependant field_value structs when fields are freed
 *      or else have dangling pointers
 * 3. This an "intermediate" representation; 'value' must then be parsed
 *    into a big-endian (network-order) array of bytes which can be:
 *    - written literally to a packet when writing/mangling
 *    - hashed to verify a match.
 *
 * (c) 2018 Sirio Balmelli
 */

#include <field2.h>

struct fval {
	struct field	*field;
	size_t		vlen;
	char		val[];
};


void		fval_free(void *arg);

struct fval	*fval_new(const char *field_name, const char *value);


#endif /* fval_h_ */
