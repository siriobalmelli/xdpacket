#include <field.h>
#include <stdint.h>
#include <stdio.h>
#include <fnv.h>
#include <zed_dbg.h>

/*	xdpk_field_tailmask()
 * Return a mask to apply against the final byte of a field.
 * NOTE that mask is considered to "grow" from the MSb (think CIDR netmask);
 *	e.g. (mlen=3) == 0xe0 == b1110000
 */
uint8_t xdpk_field_tailmask(uint16_t mlen)
{
	if (!mlen)
		return 0x0;
	/* no bit-twiddling, do a single lookup */
	const uint8_t masks[] = {
		0xff, /* because mlen itself is masked at lookup */
		0x80,
		0xc0,
		0xe0,
		0xf0,
		0xf8,
		0xfc,
		0xfe
	};
	return masks[mlen & 0x7];
}

/*	xdpk_field_hash()
 * Return 64-bit fnv1a hash of all bits described by 'field'
 * Return 0 if nothing to hash or insane parameters.
 */
uint64_t xdpk_field_hash(struct xdpk_field field, const void *pkt, size_t plen)
{
	/* sane addressing */
	const void *start = pkt + field.offt;
	if (field.offt < 0)
		start += plen;
	if (start < pkt)
		return 0;

	/* sane length */
	size_t flen = xdpk_field_len(field.mlen);
	Z_log(Z_inf, "flen  == %lu, start == %p, plen == %lu", flen, start, plen);
	if (!flen || (start + flen) > (pkt + plen))
		return 0;

	uint64_t hash = fnv_hash64(NULL, start, flen-1);
	Z_log(Z_inf, "hash1 == %lu", hash);
	uint8_t trailing = ((uint8_t*)start)[flen-1]
				& xdpk_field_tailmask(field.mlen);
	return fnv_hash64(&hash, &trailing, sizeof(trailing));
}

/*
 * Return address of field start
 */
uint8_t xdpk_field_start_byte(struct xdpk_field field, const void *pkt, size_t plen)
{
	/* sane addressing */
	const void *start = pkt + field.offt;
	if (field.offt < 0)
		start += plen;
	if (start < pkt)
		return (uint8_t)0;
	return *((uint8_t*)start);
}
