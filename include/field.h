#ifndef field_h_
#define field_h_

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
 * @mlen : mask length in bits (and implicitly: length of field being masked).
 */
struct xdpk_field {
	int16_t		offt;
	uint16_t	mlen;
}__attribute__((packed));

/*	xdpk_field_valid()
 * Used to test whether a field is "valid".
 * Functions may return 'struct xdpk_field' objects
 * (they are 32b ergo will fit in a register)
 * and if so should use XDPK_FIELD_INVALID to signal
 * an error.
 * The rationale is that a field with no offset and no mask
 * cannot possibly be valid; this also makes anything
 * obtained by calloc() invalid by default.
 *
 * Alan note: My thought about this is that 'mlen' cannot be zero,
 *	 but 'offt' can be.  I think it's sufficient to check for mlen != 0.
 */
NLC_INLINE bool xdpk_field_valid(struct xdpk_field field)
{
	return (field.mlen != 0);
}

/*	xdpk_field_len()
 * Use the mask bitcount to extrapolate field length.
 * 0 is a valid length.
 */
NLC_INLINE size_t xdpk_field_len(uint16_t mlen)
{
	/* bitmask -> bytecount
	 * - `>> 3` == `/ 8`
	 * - any fractional bits (< 8) are treated as one byte
	 */
	return ((mlen & 0x7) != 0) + (mlen >> 3);
}

NLC_PUBLIC __attribute__((pure))
	uint64_t xdpk_field_hash(struct xdpk_field field,
				const void *pkt, size_t plen);

/*	xdpk_field_parse()
 * Parse text field definition in 'grammar' (e.g. '16@tcp.src_port=80').
 * Output the expected hash to 'expected_hash'.
 * Return a field struct.
 * On error:
 * - returns `(!xdpk_field_valid(field)`
 * - leaves 'expected_hash' in a mangled/garbage state
 *
 * TODO: @alan please implement
 *
 * NOTE:
 * - grammar is mlen@offt=value
 * - integers may legally be decimal ('80'), hex ('0x50') or binary ('b01010000')
 * - need to come up with a clean approach to statically represent the offset grammar,
 *	so that this same single definition can be referenced:
 *	1. by xdpk_field_parse() to parse input
 *	2. by xdpk_field_print() to generate grammar from fields
 *	3. by xdpk in printing help/debug information (listing the offsets)
 *	4. in the documentation
 */
NLC_PUBLIC
	struct xdpk_field xdpk_field_parse(const char *grammar,
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
