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


/*	fref_state
 * Memory buffer storing actual state
 */
struct fref_state {
	uint32_t	refcnt;
	uint32_t	len;
	uint8_t		bytes[];
}__attribute((aligned(4)));


/*	fref_set
 */
struct fref_set {
	struct field_set	where;
	struct fref_state	*ref;
};



/* Combining 'state' and 'ref' saves 8B in 'struct rout_set'
 * by combining all 'store' and 'copy' operations into a single JQ.
 * TODO: remove after this has proved out in testing.
 */
#define FREF_COMBINED_STATE_REF


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

/*	fref_set_ref
 * @where	: location in packet to write bytes TO
 * @bytes	: pointer to 'bytes' of corresponding 'struct fref_set_state'
 */
struct fref_set_ref {
	struct field_set	where;
	uint8_t			*bytes;
};


/*	fref_set_exec()
 * Either'store' (copy from packet) or 'copy' (copy to packet)
 * depending on fref_set type (encoded in flags variable).
 */
NLC_INLINE int fref_set_exec(void *fref_set, void *pkt, size_t plen)
{
	struct fref_set_state *state = fref_set;
	struct field_set set = state->where;
	FIELD_PACKET_INDEXING

	if (set.flags & FREF_STATE) {
		memcpy(state->bytes, start, flen-1);
		state->bytes[flen-1] = ((uint8_t*)start)[flen-1] & set.mask;

	} else {
		struct fref_set_ref *ref = fref_set;
		memcpy(start, ref->bytes, flen-1);
		((uint8_t*)start)[flen-1] = ref->bytes[flen-1] & set.mask;

	}

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
