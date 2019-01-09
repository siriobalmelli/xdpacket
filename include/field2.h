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
#include <fnv.h>


/*	field_set
 * The set of parameters defining the extent of a field.
 * This is expressly a uint64_t size and meant to be passed by _value_.
 * See match2.h which uses this.
 */
struct field_set {
	int32_t			offt;
	uint16_t		len;
	uint8_t			mask;
	uint8_t			setff; /* must be ff (disambiguate from a pointer!) */
}__attribute__((packed));


/*	field
 * User-supplied parameters describing a field.
 */
struct field {
	char			name[MAXLINELEN];
	struct field_set	set;
};


void		field_free	(void *arg);
struct field	*field_new	(const char *name,
				long offt,
				long len,
				long mask);

struct field	*field_get	(const char *field_name);

int		field_hash	(struct field_set set,
				const void *pkt,
				size_t plen,
				uint64_t *outhash);

/* integrates into parse2.h
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
