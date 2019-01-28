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
 */
enum fref_type {
	FREF_INVALID	= 0x0,
	FREF_STATE	= 0x1,
	FREF_REF	= 0x2
};


/*	fref_set_state
 */
struct fref_set_state {
	struct field_set	where;
	uint8_t			bytes[];
};
/*	fref_set_ref
 */
struct fref_set_ref {
	struct field_set	where;
	void			*bytes;
};


/*	fref
 */
struct fref {
	struct field		*field;
	char			*ref;
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
