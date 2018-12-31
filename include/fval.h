#ifndef fval_h_
#define fval_h_

/*	fval.h
 * Representation of user-supplied (fieldname, value) tuple.
 * NOTE that the user gave 'fieldname' as a string, but we store a pointer so that:
 * - we clearly show that 'fieldname' _must_ refer to a valid field
 * - we save a string lookup to get the field's 'set'
 * - this struct stays the size of a cacheline
 * - we are forced to clean up dependant field_value structs when fields are freed
 *   or else have dangling pointers
 *
 * (c) 2018 Sirio Balmelli
 */

#include <xdpacket.h>
#include <field2.h>


struct fval {
	char		value[MAXLINELEN];
	struct field	*field;
};

void	fval_free(void *arg);

struct fval *fval_new(const char *field_name, const char *value);

#endif /* fval_h_ */
