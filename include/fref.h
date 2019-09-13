#ifndef fref_h_
#define fref_h_

/*	fref.h
 * Representation of a user-supplied (fieldname, state/reference) tuple.
 * Semantics are much the same as for fval.h with the key distinction that
 * "value" is not user-supplied but is a memory area which:
 * - can be written to from a packet using a "store" (state)
 * - can be read from (and written to a packet) using a "copy" (reference)
 */

#include <field.h>
#include <field_flags.h>
#include <state.h>
#include <yaml.h>


/*	fref_set
 * @where	: location in packet where to read ("copy) or write ("store")
 * @bytes	: pointer to 'bytes' in corresponding 'struct state'
 */
struct fref_set {
	struct field_set	where;
	uint8_t			*bytes;
};


/*	fref_set_store()
 * Copy from packet to global state variable.
 */
NLC_INLINE int fref_set_store(struct fref_set *ref, void *pkt, size_t plen)
{
	struct field_set set = ref->where;
	FIELD_PACKET_INDEXING

	memcpy(ref->bytes, start, flen-1);
	ref->bytes[flen-1] = ((uint8_t*)start)[flen-1] & set.mask;

	return 0;
}

/*	fref_set_copy()
 * Copy from global state variable to packet.
 */
NLC_INLINE int fref_set_copy(struct fref_set *ref, void *pkt, size_t plen)
{
	struct field_set set = ref->where;
	FIELD_PACKET_INDEXING

	memcpy(start, ref->bytes, flen-1);
	((uint8_t*)start)[flen-1] = ref->bytes[flen-1] & set.mask;

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
				enum field_flags type);

int		fref_emit	(struct fref *fref,
				yaml_document_t *outdoc,
				int outlist);


#endif /* fref_h_ */
