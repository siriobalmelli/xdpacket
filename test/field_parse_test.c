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

		Z_log(Z_inf, "fld.offt == %d, fld.mlen == %d", fld.offt, fld.mlen);

		/*
		if (mt->pos_test) {
			Z_err_if(is_match != true, "Tag %s: %d != %d, len %zu",
				mt->tag, is_match, true, pkt->len);
		} else {
			Z_err_if(is_match == true, "Tag %s: %d != %d, len %zu",
				mt->tag, is_match, true, pkt->len);
		}
		*/
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
