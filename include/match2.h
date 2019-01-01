#ifndef match2_h_
#define match2_h_

/*	match2.h
 * A matcher is an array of (field, value) tuples.
 * (c) 2018 Sirio Balmelli
 */

#include <field2.h>







#define FIELD_INVAL	0x0
#define FIELD_AND	0x1
#define FIELD_OR	0x2

/*	field_array
 * This is a variable-sized array and is meant to be passed by _reference_.
 * See FIELD_IS_ARRAY_P() below.
 */
struct field_array {
	uint32_t		len;
	uint16_t		flags; /* FIELD_AND, FIELD_OR, etc */
	uint16_t		pad16;
	struct field_set	arr[];
}__attribute__((packed));

/* FIELD_IS_ARRAY_P()
 * A pointer to a struct field_array (indeed, any pointer on a sane system)
 * will have the low 4B as 0x0, while a field will have the low 8B as 0xff.
 */
#define FIELD_IS_ARRAY_(field_or_array_p) \
	(((struct field_matcher)field_or_array_p).setff == 0)


#endif /* match2_h_ */
