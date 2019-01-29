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
#include <yaml.h>


/*	fref_type
 * Use the 'flags' byte of 'struct field_set' to store fref_type
 */
enum fref_type {
	FREF_INVALID	= 0x0,
	FREF_STATE	= 0x1,
	FREF_REF	= 0x2
};


/*	fref_set_state
 * @where	: location in packets to pull bytes FROM
 * @bytes	: buffer where bytes are stored
 */
struct fref_set_state {
	struct field_set	where;
	uint8_t			bytes[];
};

/*	fref_set_store()
 * Store bytes from 'pkt' into 'state->bytes', obeying offset and mask values in 'state->where'.
 * Return 0 on success, non-0 on failure (e.g. packet no long enough).
 */
NLC_INLINE int fref_set_store(struct fref_set_state *state, void *pkt, size_t plen)
{
	struct field_set set = state->where;
	FIELD_PACKET_INDEXING
	memcpy(state->bytes, start, flen-1);
	state->bytes[flen-1] = ((uint8_t*)start)[flen-1] & set.mask;
	return 0;
}


/*	fref_set_ref
 * @where	: location in packet to write bytes TO
 * @bytes	: pointer to 'bytes' of corresponding 'struct fref_set_state'
 */
struct fref_set_ref {
	struct field_set	where;
	uint8_t			*bytes;
};

/*	fref_set_copy()
 * Copy bytes from 'ref->bytes' into 'pkt', obeying offset and mask values in 'ref->where'.
 * Return 0 on success, non-0 on failure (e.g. packet no long enough).
 */
NLC_INLINE int fref_set_copy(struct fref_set_ref *ref, void *pkt, size_t plen)
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
	char			*ref_name;
union {
	struct fref_set_state	*set_state;
	struct fref_set_ref	*set_ref;
};
	size_t			refcnt;
};


void		fref_free	(void *arg);

struct fref	*fref_new	(const char *field_name,
				const char *ref_name,
				enum fref_type type);

int		fref_emit	(struct fref *fref,
				yaml_document_t *outdoc,
				int outlist);


#endif /* fref_h_ */
