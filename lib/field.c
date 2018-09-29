#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fnv.h>
#include <zed_dbg.h>
#include <field.h>
#include "offset_defs.h"

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

/*	xdpk_field_shiftval()
 * Return the number of bits to shift the value to be used
 * for hashing where (masklen % 8 != 0).
 */
uint8_t xdpk_field_shiftval(uint16_t mlen)
{
	if (!mlen)
		return 0x0;
	
	const uint8_t shifts[] = {
		0,
		7,
		6,
		5,
		4,
		3,
		2,
		1
	};
	return shifts[(mlen & 0x7)];
}

/*	xdpk_hash()
 * Return 64-bit fnv1a hash of all bits described 'mlen''
 * Return 0 if nothing to hash or insane parameters.
 * TODO: This duplicates much of xdpk_field_hash - factor?
 */
uint64_t xdpk_hash(uint16_t mlen, const void *start, 
				size_t plen, uint64_t *hashp)
{
	/* sane length */
	size_t flen = xdpk_field_len(mlen);
	if (!flen || (flen > plen))
		return 0;

	uint64_t hash = fnv_hash64(hashp, start, flen-1);
	Z_log(Z_inf, "hash == 0x%lu", hash);
	uint8_t trailing = ((uint8_t*)start)[flen-1]
				& xdpk_field_tailmask(mlen);
	return fnv_hash64(&hash, &trailing, sizeof(trailing));
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
	Z_log(Z_inf, "hash == 0x%lu", hash);
	uint8_t trailing = ((uint8_t*)start)[flen-1]
				& xdpk_field_tailmask(field.mlen);
	return fnv_hash64(&hash, &trailing, sizeof(trailing));
}

/*	parse_data_type()
 * The starting characters of a value determine whether it's hexadecimal ('0x'),
 * binary ('b'), decimal ('[-]0-9'), or ASCII (all other).
 */	
enum data_type parse_data_type(const char *begin, const char * end) {
	int len = end - begin;
	switch (*begin) {
	case '0':
		if (len == 1)
			return DECIMAL_TYPE;
		else if ((len > 1) && *(begin+1) == 'x')
			return HEX_TYPE;
		break;
	case 'b':
		if ((len > 1) && (*(begin+1) == '0' || *(begin+1) == '1'))
			return BIN_TYPE;
		break;
	case '-':
		if ((len > 1) && isdigit(*(begin+1)))
			return DECIMAL_TYPE;
		break;
	default:
		if (isdigit(*begin))
			return DECIMAL_TYPE;
		break;
	}

	return ASCII_TYPE;
}


/*	parse_binary
 *
 */
uint64_t parse_binary(const char *begin, const char *end, 
						uint64_t max, bool *err) {
	uint64_t val = 0;
	int len = (end - begin) < 64 ? (end - begin) : 64;
	const char *cp = begin;
	for (int i = 0; (i < len) && (val <= max); i++, cp++) {	
		switch(*cp) {
		case '0':
			val = (val << 1);
			break;
		case '1':
			val = (val << 1) + 1;
			break;
		default:
			*err = true;
			goto error;
		}
	}

error:
	return val;
}

/*	parse_decimal
 *
 */
int64_t parse_decimal(const char *begin, const char *end, 
						uint64_t max, bool *err) {
	bool neg = false;
	if (*begin == '-') {
		neg = true;
		begin++;
	}

	int64_t val = 0;
	int len = (end - begin) <  19? (end - begin) : 19;
	const char *cp = begin;
	for (int i = 0; (i < len) && (val <= max); i++, cp++) {	
		switch(*cp) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			val = (val * 10) + (*cp - '0');
			break;
		default:
			*err = true;
			goto error;
		}
	}
	if (neg) val = -val;

error:
	return val;
}

/*	hex_value
 */
uint8_t hex_value(const char c) {
	return (c > '9') ? ('A' - c + 10) : (c - '0');
}

/*	parse_hex
 *
 */
 uint64_t parse_hex(const char *begin, const char *end, 
						uint64_t max, bool *err) {
	uint64_t val = 0;
	int len = (end - begin) < 8 ? (end - begin) : 8;
	if ((len % 2) != 0) {
		*err = true;
		goto error;
	}
	int i = 0;
	for (const char *cp = begin; cp < end; cp += 2, i++) {
		char c1 = *cp;
		char c2 = *(cp + 1);
		if (!isxdigit(c1) || !isxdigit(c2)) {
			*err = true;
			goto error;
		}
		val = (val << 8) | (hex_value(toupper(c1)) << 4) 
			| hex_value(toupper(c2));
	}

error:
	return val;
}

/*	parse_long_hex
 *
 */
void parse_long_hex(const char *begin, const char *end, 
						uint8_t *buf, bool *err)
{
	int len = end - begin;
	if ((len % 2) != 0) {
		*err = true;
		goto error;
	}
	int i = 0;
	for (const char *cp = begin; cp < end; cp += 2, i++) {
		char c1 = *cp;
		char c2 = *(cp + 1);
		if (!isxdigit(c1) || !isxdigit(c2)) {
			*err = true;
			goto error;
		}
		buf[i] = (hex_value(toupper(c1)) << 4) 
			| hex_value(toupper(c2));
	}

error:
	return;
}

/*	parse_value
 * Get the value of a field, third part in the grammar "mlen@offt=value",
 * and return its hash value.
 */
uint64_t parse_value(const char *begin, const char *end, int mlen, bool *err) {
	size_t flen = xdpk_field_len(mlen);
	int slen = end - begin;
	int64_t dval = 0;
	uint64_t hval = 0;
	uint64_t hash = 0;

	uint8_t * buf = (uint8_t*) calloc(flen, sizeof(uint8_t));
	
	switch (parse_data_type(begin, end))
	{
	case DECIMAL_TYPE:
		if (flen > 16) {
			*err = true;
			goto error;
		}
		dval = parse_decimal(begin, end, 0x7FFFFFFFFFFFFFFFL, err);
		if (*err) goto error;
		dval <<= xdpk_field_shiftval(mlen);
		for (int i = 0; i < flen; i++) {
			// Put into network byte order
			buf[flen-i-1] = (dval >> (8*i)) & 0xFF;
		}
		hash = fnv_hash64(NULL, (void*)buf, flen);
		break;
	case HEX_TYPE:
		if ((slen - 2) <= 16) {
			hval = parse_hex(begin+2, end, 0xFFFFFFFFFFFFFFFF, err);
			if (*err) goto error;
			hval <<= xdpk_field_shiftval(mlen);
			for (int i = 0; i < flen; i++) {
				// Put into network byte order
				buf[flen-i-1] = (hval >> (8*i)) & 0xFF;
			}
			hash  = fnv_hash64(NULL, (void*)buf, flen);
		}
		else {
			if (xdpk_field_shiftval(mlen) != 0) goto error;
			parse_long_hex(begin+2, end, buf, err);
			if (*err) goto error;
			hash  = fnv_hash64(NULL, (void*)buf, flen);
		}
		break;
	case BIN_TYPE:
		if ((flen > 16) || ((slen - 1) > 64)) {
			*err = true;
			goto error;
		}
		hval = parse_binary(begin + 1, end, 0xFFFFFFFFFFFFFFFF, err);
		if (*err) goto error;
		for (int i = 0; i < flen; i++) {
			// Put into network byte order
			buf[flen-i-1] = (hval >> (8*i)) & 0xFF;
		}
		hash  = fnv_hash64(NULL, (void*)buf, flen);
		break;
	case ASCII_TYPE:
		break;
	default:
		*err = true;
		return 0;
	}

error:
	if (buf) free(buf);

	return hash;
}

/*	parse_16bit
 * Parse a 16-bit integer from an ASCII string.  Numeric value may be
 * decimal, binary or hex.  This determines which and calls the appropriate
 * sub-parsing function.  64-bits is returned so that this doesn't have to 
 * account for overflow.  Caller can truncate.
 */
int64_t parse_16bit(const char *begin, const char *end, bool *err) {
	switch (parse_data_type(begin, end))
	{
	case DECIMAL_TYPE:
		return parse_decimal(begin, end, 0xFFFF, err);
		break;
	case HEX_TYPE:
		return parse_hex(begin+2, end, 0xFFFF, err);
		break;
	case BIN_TYPE:
		return parse_binary(begin + 1, end, 0xFFFF, err);
		break;
	default:
		*err = true;
		return 0;
	}
}

/*	parse_int16()
 * Parse an unsigned 16-bit integer from an ASCII string.  32-bits is returned
 * so that this doesn't have to account for overflow.  Caller can truncate.
 */
uint16_t parse_uint16(const char *begin, const char *end, bool *err) {
	int64_t num = parse_16bit(begin, end, err);
	if ((num < 0) || (num > 0xFFFF)) *err = true;
	
	return (uint16_t)num;
}

/*	parse_int16()
 * Parse a signed 16-bit integer from an ASCII string.  32-bits is returned
 * so that this doesn't have to account for overflow.  Caller can truncate.
 */
int16_t parse_int16(const char *begin, const char *end, bool *err) {
	int64_t num = parse_16bit(begin, end, err);
	if ((num < -32768) || (num > 32767)) *err = true;
	
	return (int16_t)num;
}

/*	lookup_offset	
 * Lookup the numeric value for an ASCII represented offset.  This does
 * a linear search on the assumption that the list of symbols is
 * short and it doesn't matter for UI speeds.
 */	
int32_t lookup_offset(const char *begin, const char *end, bool *err) {
	int slen = end - begin;
	for (int i = 0; i < nsyms; i++) {
		if (!strncmp(begin, xdpk_offt_sym[i].symbol, slen))
			return xdpk_offt_sym[i].offt;
	}
	*err = true;
	return 0;
}

/*	parse_offset()
 * Parse the offset field of an xdpk grammar string, which may be
 * numeric or ASCII valued.  Max value is max(mlen)/8 == 65536/8 == 8192.
 */
int32_t parse_offset(const char *begin, const char *end, bool *err) {
	int16_t offt = (int16_t)parse_int16(begin, end, err);
	if (*err) {
		*err = false;
		offt = (int16_t)lookup_offset(begin, end, err);
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
struct xdpk_field xdpk_field_parse(const char *grammar,
				uint64_t *expected_hash)
{
	struct xdpk_field fld = {0, 0};
	bool err = false;

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
	*expected_hash = parse_value(epos + 1, tpos, mlen, &err);
	if (err) goto error;

	fld.offt = offt;
	fld.mlen = mlen;

error:
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