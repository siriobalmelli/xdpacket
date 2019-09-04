#ifndef fref_h_
#define fref_h_

/*	fref.h
 * Representation of a user-supplied (fieldname, state/reference) tuple.
 * Semantics are much the same as for fval.h with the key distinction that
 * "value" is not user-supplied but is a memory area which:
 * - can be written to from a packet using a "store" (state)
 * - can be read from (and written to a packet) using a "copy" (reference)
 *
 * TODO: STATE: coalesce this with fval.h, use _type_ field to distinguish.
 */

#include <field.h>
#include <state.h>
#include <yaml.h>


/*	fref_type
 * Use the 'flags' byte of 'struct field_set' to store fref_type
 */
enum fref_type {
	FREF_INVALID	= 0x0,
	FREF_STORE	= 0x1,
	FREF_COPY	= 0x2
};


/*	fref_set
 * @where	: location in packet to write bytes TO
 * @bytes	: pointer to 'bytes' in corresponding 'struct state'
 */
struct fref_set {
	struct field_set	where;
	uint8_t			*bytes;
};


/*	fref_set_exec()
 * Either'store' (copy from packet) or 'copy' (copy to packet)
 * depending on fref_set type (encoded in flags variable).
 */
NLC_INLINE int fref_set_exec(struct fref_set *ref, void *pkt, size_t plen)
{
	struct field_set set = ref->where;
	FIELD_PACKET_INDEXING

	if (set.flags & FREF_STORE) {
		memcpy(ref->bytes, start, flen-1);
		ref->bytes[flen-1] = ((uint8_t*)start)[flen-1] & set.mask;

	/* we assume: (set.flags & FREF_COPY) */
	} else {
		memcpy(start, ref->bytes, flen-1);
		((uint8_t*)start)[flen-1] = ref->bytes[flen-1] & set.mask;

	}

	return 0;
}


/*	fref
 */
struct fref {
	struct field		*field;
	struct state		*state;
	struct fref_set		*ref;
};


void		fref_free	(void *arg);

struct fref	*fref_new	(const char *field_name,
				const char *state_name,
				enum fref_type type);

int		fref_emit	(struct fref *fref,
				yaml_document_t *outdoc,
				int outlist);


#endif /* fref_h_ */
