#ifndef field2_h_
#define field2_h_

/*	field2.h
 * A "field" is a range of bytes, with an optional mask, which can be used to:
 * - specify which parts of a packet to hash, when matching incoming packets.
 * - specify which bytes to write to (and possibly also read from),
 *	when mangling packet contents.
 * (c) 2018 Sirio Balmelli
 */

#include <xdpacket.h>
#include <stdint.h>
#include <stdlib.h>
#include <nonlibc.h>
#include <Judy.h>
#include <yaml.h>
#include <parse2.h>


/*	field_matcher
 * This is expressly a uint64_t size and meant to be passed by _value_.
 */
struct field_matcher {
	int32_t			offt;
	uint16_t		len;
	uint8_t			mask;
	uint8_t			setff; /* must be 00 (disambiguate from a pointer!) */
}__attribute__((packed));


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
	struct field_matcher	arr[];
}__attribute__((packed));

/* FIELD_IS_ARRAY_P()
 * A pointer to a struct field_array (indeed, any pointer on a sane system)
 * will have the low 4B as 0x0, while a field will have the low 8B as 0xff.
 */
#define FIELD_IS_ARRAY_(field_or_array_p) \
	(((struct field_matcher)field_or_array_p).setff == 0)



/*	field
 * User-supplied parameters describing a field.
 */
struct field {
	char			name[MAXLINELEN];
	struct field_matcher	matcher;
};


void		field_free	(void *arg);
struct field	*field_new	(const char *name,
				ssize_t offt,
				size_t len,
				uint8_t mask);


/* integrate into parse2.h
 */
int		field_parse	(enum parse_mode mode,
				yaml_document_t *doc,
				yaml_node_t *mapping,
				yaml_document_t *outdoc,
				int outlist);

int		field_emit	(struct field *field,
				yaml_document_t *outdoc,
				int outlist);

#endif /* field2_h_ */
