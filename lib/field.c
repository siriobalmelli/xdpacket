#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fnv.h>
#include <zed_dbg.h>
#include <field.h>

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
uint64_t xdpk_field_hash(struct xdpk_field field, const void *pkt, 
				size_t plen, uint64_t *hashp)
{
	/* sane addressing */
	const void *start = pkt + field.offt;
	if (field.offt < 0)
		start += plen;
	if (start < pkt)
		return 0;

	/* sane length */
	size_t flen = xdpk_field_len(field.mlen);
	Z_log(Z_inf, "flen  == 0x%lu, start == %p, plen == %lu", flen, start, plen);
	if (!flen || (start + flen) > (pkt + plen))
		return 0;

	uint64_t hash = fnv_hash64(hashp, start, flen-1);
	Z_log(Z_inf, "hash == 0x%llu", hash);
	uint8_t trailing = ((uint8_t*)start)[flen-1]
				& xdpk_field_tailmask(field.mlen);
	return fnv_hash64(&hash, &trailing, sizeof(trailing));
}

uint8_t * parse_value(const char *begin, const char *end, bool *err) {
	// Caller owns the storage
	uint8_t * buf = (uint8_t*) calloc(10, sizeof(uint8_t));

	return buf;
}

uint32_t parse_16bit_binary(const char *begin, const char *end, bool *err) {
	return 0;
}

uint32_t parse_16bit_decimal(const char *begin, const char *end, bool *err) {
	bool neg = false;
	if (*begin == '-') {
		neg = true;
		begin++;
	}

	int32_t val = 0;
	for (const char *cp = begin; (cp < end) && (val <= 0xFFFF); cp++) {
		switch(*cp) {
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				val = (val * 10) + (*cp - '0');
				break;
			default:
				*err = true;
				goto error;
				break;
		}
	}
	if (neg) val = -val;

error:
	return val;
}

uint32_t parse_16bit_hex(const char *begin, const char *end, bool *err) {
	return 0;
}

int32_t parse_16bit(const char *begin, const char *end, bool *err) {
	if (*begin == '0') {
		if (*(begin+1) == 'x')
			return parse_16bit_hex(begin+2, end, err);
		goto error;
	}
	else if (*begin == 'b')
		return parse_16bit_binary(begin + 1, end, err);
	else if (isdigit(*begin) || (*begin == '-'))
		return parse_16bit_decimal(begin, end, err);

error:
	*err = true;
	return 0;
}

int32_t parse_uint16(const char *begin, const char *end, bool *err) {
	int32_t num = parse_16bit(begin, end, err);
	if ((num < 0) || (num > 0xFFFF)) *err = true;
	
	return (uint16_t)num;
}

int32_t parse_int16(const char *begin, const char *end, bool *err) {
	int32_t num = parse_16bit(begin, end, err);
	if ((num < -32768) || (num > 32767)) *err = true;
	
	return (uint16_t)num;
}

int32_t lookup(const char *begin, const char *end, bool *err) {
	return 34;
}

int32_t parse_offset(const char *begin, const char *end, bool *err) {
	int16_t offt = (int16_t)parse_int16(begin, end, err);
	if (*err) {
		*err = false;
		offt = (int16_t)lookup(begin, end, err);
	}

	return offt;
}

/*	xdpk_field_parse()
 * Parse text field definition in 'grammar' (e.g. '16@tcp.src_port=80').
 * Output the expected hash to 'expected_hash'.
 * Return a field struct.
 * On error:
 * - returns `(!xdpk_field_valid(field)`
 * - leaves 'expected_hash' in a mangled/garbage state
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
 #define PBUF_SIZE 20

struct xdpk_field xdpk_field_parse(const char *grammar,
				uint64_t *expected_hash)
{
	struct xdpk_field fld = {0, 0};
	bool err = false;
	uint8_t *val = NULL;

	char *apos = strchr(grammar, '@');
	if (apos == NULL) goto error;
	char *epos = strchr(apos+1, '=');
	if (epos == NULL) goto error;
	char *tpos = strchr(epos+1, '\0');
	if (tpos == NULL) goto error;

	uint16_t mlen = (uint16_t)parse_uint16(grammar, apos, &err);
	if (err) goto error;
	int16_t offt = (int16_t)parse_offset(apos + 1, epos, &err);
	if (err) goto error;
	val = parse_value(epos + 1, tpos, &err);
	if (err) goto error;

	fld.offt = offt;
	fld.mlen = mlen;
	*expected_hash = 0;

error:
	if (val != NULL) free(val);
	return fld;
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