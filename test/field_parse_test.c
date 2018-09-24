/*	field_parse_test.c
 * Verify field matching
 * (c) 2018 Sirio Balmelli
 * (c) 2018 Alan Morrissett
 */
#include <stdlib.h>
#include <stdio.h>
#include <nonlibc.h>
#include <binhex.h>
#include <zed_dbg.h>
#include "field.h"

/* Keep definitions separately to make more readable */
#include "field_parse_test_set.h"

/*	parse_check()
 * Validate a matcher can find packets with matching fields.
 */
int parse_check()
{
	int err_cnt = 0;

	for (int i = 0; i < NLC_ARRAY_LEN(parse_tests); i++) {
		const struct ptuple *pt = &parse_tests[i];
		uint64_t hash;
		struct xdpk_field fld = xdpk_field_parse(pt->grammar, &hash);

		Z_log(Z_inf, "offt == %d, mlen == %d, hash == %llu", 
				fld.offt, fld.mlen, hash);
		bool is_match = (fld.offt == pt->expected_fld.offt) &&
			     (fld.mlen == pt->expected_fld.mlen) &&
			     (hash == pt->expected_hash);

		if (pt->pos_test) {
			Z_err_if(fld.offt != pt->expected_fld.offt, 
				"Tag %s: offt %d != %d",
				pt->tag, fld.offt, pt->expected_fld.offt);
			Z_err_if(fld.mlen != pt->expected_fld.mlen, 
				"Tag %s: mlen %u != %u",
				pt->tag, fld.mlen, pt->expected_fld.mlen);
			Z_err_if(hash != pt->expected_hash, 
				"Tag %s: hash 0x%llu != 0x%llu",
				pt->tag, hash, pt->expected_hash);
		} else {
			
		}
	}
	Z_log(Z_inf, "number of matcher tests == %ld",
			NLC_ARRAY_LEN(parse_tests));

	return err_cnt;
}


/*	main()
 * Run all test routines in this file.
 * Expects test routines to return err_cnt (0 on success).
 */
int main()
{
	int err_cnt = 0;
	err_cnt += parse_check();
	return err_cnt;
}
