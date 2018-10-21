#ifndef field_h_
#define field_h_

#include <xdpk.h> /* must always be first */

/*	field.h
 * A data structure describing a contiguous series of bits to be matched,
 * replated manipulation functions.
 *
 * (c) 2018 Alan Morrissett and Sirio Balmelli
 */
#include <nonlibc.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

/* xdpk_field
 * @offt : positive offset from start of packet, negative from end; in bytes.
 * @len  : byte length of field being hashed
 * @mask : bitwise &-mask applied to last byte of field before hashing
 * @pad  : unused (*not* guaranteed to remain, don't store ad-hoc data here)
 */
struct xdpk_field {
	int32_t		offt;
	uint16_t	len;
	uint8_t		mask;
	uint8_t		pad;
}__attribute__((packed));

/*	xdpk_field_valid()
 * Used to test whether a field is "valid".
 * Functions may return 'struct xdpk_field' objects
 * (they are 32b ergo will fit in a register)
 * and if so should use XDPK_FIELD_INVALID to signal
 * an error.
 * 
 * A field may have 0 offset, but a 0 len or a 0 mask are invalid. 
 */
NLC_INLINE bool xdpk_field_valid(struct xdpk_field field)
{
	return ((field.len) != 0 && (field.mask != 0));
}

NLC_PUBLIC __attribute__((pure))
uint64_t xdpk_hash(const void *start, uint16_t len, 
				uint8_t mask, uint64_t *hashp);

NLC_PUBLIC __attribute__((pure))
	uint64_t xdpk_field_hash(struct xdpk_field field,
			const void *pkt, size_t plen, uint64_t *hashp);

/*	xdpk_field_create()
 * 
 */
NLC_PUBLIC
struct xdpk_field xdpk_field_create(size_t offt,
					size_t len,
					uint8_t mask,
					char *val, size_t val_len,
					uint64_t *expected_hash);

/*	xdpk_field_print()
 * Allocate and return a string containing
 * the definition of a field in the proper grammar (see preceding).
 * TODO: @alan please implement
 */
NLC_PUBLIC
	char *xdpk_field_print(struct xdpk_field field);

NLC_LOCAL __attribute__((const))
	uint8_t xdpk_field_tailmask(uint16_t mlen);
NLC_LOCAL  uint8_t xdpk_field_start_byte(struct xdpk_field field, const void *pkt, size_t plen);

#endif /* field_h_ */
