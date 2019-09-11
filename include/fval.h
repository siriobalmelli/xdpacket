#ifndef fval_h_
#define fval_h_

/*	fval.h
 * Representation of user-supplied (fieldname, value) tuple,
 * given to us as strings.
 *
 * NOTES:
 * 1. We should always be able to reproduce the exact user input,
 *    which is why 'value' is stored as given.
 * 2. The user gave 'fieldname' as a string, but we store a pointer so that:
 *    - we clearly show that 'fieldname' _must_ refer to a valid field
 *    - we save a string lookup to get the field's 'set'
 *    - we are forced to clean up dependant field_value structs when fields are freed
 *      or else have dangling pointers
 * 3. 'value' is an "intermediate" representation; it must then be parsed
 *    into a big-endian (network-order) array of bytes which can be:
 *    - written literally to a packet when writing/mangling
 *    - hashed to verify a match.
 *    'fval_set' is this parsed array of bytes.
 *
 * (c) 2018 Sirio Balmelli
 */

#include <field.h>
#include <field_flags.h>
#include <yaml.h>


/*	fval_set
 * A parsed/compiled fval, consisting of the field-set location of bytes
 * and a big-endian representation of the value, suitable for hashing
 * or writing directly into the packet.
 */
struct fval_set {
	struct field_set	where;
	uint8_t			bytes[];
};


void		fval_set_free	(void * arg);

struct fval_set	*fval_set_new	(const char *value,
				size_t value_len,
				struct field_set set);

char		*fval_set_print	(struct fval_set *fvb);

/*	fval_set_hash()
 * Return the hash of 'where' and 'bytes'.
 * This is the hash that incoming packets will have to match.
 * NOTES:
 * - The semantics are different from field_hash():
 *   we need to ignore the offset but need to hash the 'set' _with_ the offset.
 * - We elide all the length checking etc: fval instantiation should
 *   have sanity-checked this.
 * - Don't forget to initialize the hash before the first call to this function,
 *   by e.g. `ret->bytes_hash = fnv_hash64(NULL, NULL, 0)`
 */
NLC_INLINE
int		fval_set_hash	(const struct fval_set *fvb,
				uint64_t *outhash)
{
	struct field_set set = fvb->where;
	size_t flen = set.len;
	void *start = (void *)fvb->bytes;

	/* see field.h */
	FIELD_PACKET_HASHING
	return 0;
}

int		fval_set_write	(struct fval_set *fvb,
				void *pkt,
				size_t plen);



/*	fval
 */
struct fval {
	struct field		*field;
	char			*val;

	struct fval_set		*set;
	char			*bytes_prn;
	uint64_t		set_hash;
};


void		fval_free	(void *arg);

struct fval	*fval_new	(const char *field_name,
				const char *value);

int		fval_emit	(struct fval *fval,
				yaml_document_t *outdoc,
				int outlist);


#endif /* fval_h_ */
