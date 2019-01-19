#ifndef field2_h_
#define field2_h_

/*	field2.h
 * A "field" is a descriptor, which can be used to specify:
 * - what to match in a packet (when filtering incoming packets).
 * - where to write to in a packet (when mangling packet contents).
 *
 * (c) 2018 Sirio Balmelli
 */

#include <xdpacket.h>
#include <stdint.h>
#include <stdlib.h>
#include <nonlibc.h>
#include <judyutils.h>
#include <yaml.h>
#include <parse2.h>
#include <fnv.h>
#include <regex.h>


/*	field_set
 * The set of parameters defining the extent of a field.
 * NOTE: this is expressly a uint64_t/uintptr_t size and meant to be passed
 * by _value_ (copied, not referenced) see e.g. fval.h which uses this.
 * @offt	: offset into the packet where field matching should begin.
 * @len		: how many bytes to match.
 * @mask	: mask to apply to last byte of match (0xff == entire last byte).
 * @flags	: opaque byte that can be used by others when handling copies
 *		  of field_set structures.
 *		  Must always be zero inside a 'struct field'.
 */
struct field_set {
	int32_t			offt;
	uint16_t		len;
	uint8_t			mask;
	uint8_t			flags;
}__attribute__((packed));


/*	field
 * User-supplied parameters describing a field.
 */
struct field {
	char			*name;
	struct field_set	set;
	size_t			refcnt;
};


void		field_free	(void *arg);
void		field_free_all	();
struct field	*field_new	(const char *name,
				long offt,
				long len,
				long mask);

void		field_release	(struct field *field);
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


/* Common code for sanitizing and computing lengths when a 'set' needs to
 * deal with (hash, write to) a packet.
 * Avoid copies since this code is critical.
 * Declares the following variables for use by later code:
 * - 'start'
 * - 'flen'
 */
#define FIELD_PACKET_INDEXING							\
	/* Parameter sanity */							\
	if NLC_UNLIKELY(!pkt || !plen)						\
		return 1;							\
										\
	/* Offset sanity.							\
	 * 'offt' may be negative, in which case it denotes offset from		\
	 * the end of the packet.						\
	 */									\
	__typeof__(pkt) start = pkt + set.offt;					\
	if (set.offt < 0)							\
		start += plen;							\
	if (start < pkt)							\
		return 1;							\
										\
	/* Size sanity.								\
	 * We rely on the invariant that no set may have length 0		\
	 */									\
	size_t flen = set.len;							\
	if ((start + flen) > (pkt + plen))					\
		return 1;

/* Common code for calculating a packet hash given an fval_set.
 * Expects to see the following variables:
 * - (struct field_set) set
 * - (size_t) flen
 * - (void *) start
 * NOTES:
 * - a 0-length packet simply hashes the 'set' itself
 * - a 0-offt and 0-len 'set' will match _all_ packets.
 * - a 0-length packet should with offt X should match all packets where
 *   (len >= X)
 */
#define FIELD_PACKET_HASHING							\
	/* must not error after we start changing 'outhash' */			\
	*outhash = fnv_hash64(outhash, &set, sizeof(set));			\
	if (flen) {								\
		*outhash = fnv_hash64(outhash, start, flen-1);			\
		/* last byte must be run through the mask */			\
		uint8_t trailing = ((uint8_t*)start)[flen-1] & set.mask;	\
		*outhash = fnv_hash64(outhash, &trailing, sizeof(trailing));	\
	}


#endif /* field2_h_ */
