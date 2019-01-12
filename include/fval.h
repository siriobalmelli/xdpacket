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
 *    'fval_bytes' is this parsed array of bytes.
 *
 * (c) 2018 Sirio Balmelli
 */

#include <field2.h>
#include <yaml.h>
#include <nmath.h>


/*	fval_bytes
 * A parsed/compiled fval, consisting of the field-set location of bytes
 * and a big-endian representation of the value, suitable for hashing
 * or writing directly into the packet.
 */
struct fval_bytes {
	struct field_set	where;
	uint8_t			bytes[];
};


void			fval_bytes_free(void * arg);

struct fval_bytes	*fval_bytes_new(const char *value,
					size_t value_len,
					struct field_set set);

char			*fval_bytes_print(struct fval_bytes *fvb);

/*	fval_bytes_hash()
 * Return the hash of 'where' and 'bytes'.
 * This is the hash that incoming packets will have to match.
 */
NLC_INLINE
int			fval_bytes_hash(const struct fval_bytes *fvb,
					uint64_t *out_hash)
{
	/* ignore offset when hashing */
	struct field_set temp = fvb->where;
	temp.offt = 0;
	return field_hash(temp, fvb->bytes, fvb->where.len, out_hash);
}

/*	fval_bytes_len()
 * Return the length of '*fvb'.
 * NOTE: struct fval_bytes is always aligned to 'void *'
 */
NLC_INLINE
size_t			fval_bytes_len(struct fval_bytes *fvb)
{
	return nm_next_mult64(sizeof(*fvb) + fvb->where.len, sizeof(void*));
}

int			fval_bytes_write(struct fval_bytes *fvb,
					void *pkt,
					size_t plen);



/*	fval
 */
struct fval {
	struct field		*field;

	char			*val;

	struct fval_bytes	*bytes;
	char			*bytes_prn;
};


void		fval_free	(void *arg);

struct fval	*fval_new	(const char *field_name,
				const char *value);

int		fval_emit	(struct fval *fval,
				yaml_document_t *outdoc,
				int outlist);


#endif /* fval_h_ */
