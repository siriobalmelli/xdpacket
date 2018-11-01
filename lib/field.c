#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fnv.h>
#include <zed_dbg.h>
#include <yaml.h>
#include <field.h>
#include <parse.h>
#include "offset_defs.h"

/*	xdpk_hash()
 * Return 64-bit fnv1a hash of all bits described 'mlen''
 * Return 0 if nothing to hash or insane parameters.
 * TODO: This duplicates much of xdpk_field_hash - factor?
 */
uint64_t xdpk_hash(const void *start, uint16_t len, 
				uint8_t mask, uint64_t *hashp)
{
	/* sane length */
	if (len == 0)
		return 0;

	uint64_t hash = fnv_hash64(hashp, start, len-1);
	uint8_t trailing = ((uint8_t*)start)[len-1] & mask;
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
	size_t flen = field.len;
	Z_log(Z_inf, "flen  == 0x%lu, start == %p, plen == %lu", flen, start, plen);
	if (!flen || (start + flen) > (pkt + plen))
		return 0;

	uint64_t hash = fnv_hash64(hashp, start, flen-1);
	Z_log(Z_inf, "hash == 0x%lu", hash);
	uint8_t trailing = ((uint8_t*)start)[flen-1] & field.mask;
	return fnv_hash64(&hash, &trailing, sizeof(trailing));
}

/*	xdpk_field_create()
 * Create a field struct from component parts, and compute expected hash
 * from the value given.
 */

struct xdpk_field xdpk_field_create(size_t offt,
					size_t len,
					uint8_t mask,
					char *val, size_t val_len,
					uint64_t *expected_hash)
{
	bool err = false;
	struct xdpk_field fld = {offt, len, mask};

	Z_die_if(!xdpk_field_valid(fld), "");

	//*expected_hash = parse_value(val, val + len, len, fld.mask, &err);
	Z_die_if(err, "");

out:
	return fld;
}

/*	xdpk_field_print()
 * Allocate and return a string containing
 * the definition of a field in the proper grammar.
 */
char *xdpk_field_print(struct xdpk_field field)
{
	char buf[1024];
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "{ offt: %d, len: %d, mask: 0x%x }", 
				field.offt, field.len, field.mask);
	char *rv = malloc(strlen(buf)+1);
	strcpy(rv, buf);
	
	return rv;
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
