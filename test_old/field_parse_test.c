/*	field_parse_test.c
 * Verify field matching
 * (c) 2018 Sirio Balmelli
 * (c) 2018 Alan Morrissett
 */
#include <stdlib.h>
#include <stdio.h>
#include <nonlibc.h>
#include <binhex.h>
#include <ndebug.h>
#include <parse.h>
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
		uint64_t hash = 0;
		struct xdpk_field fld = xdpk_field_parse(pt->grammar,
						strlen(pt->grammar), &hash);

		NB_inf("offt == %d, len == %d, mask == 0x%0x, hash == 0x%0lx",
				fld.offt, fld.len, fld.mask, hash);

		bool match = (	(fld.offt == pt->expected_fld.offt)
			     &&	(fld.len = pt->expected_fld.len)
			     && (fld.mask = pt->expected_fld.mask)
			     &&	(hash == pt->expected_hash));

		if (pt->pos_test) {
			NB_err_if(!match,
				"Tag %s: (offt %d, len %u, mask 0x%0x, hash 0x%0lx)"
				" != expected (offt %d, len %u, mask %0x, hash 0x%0lx)\n",
				pt->tag, fld.offt, fld.len, fld.mask, hash,
				pt->expected_fld.offt, pt->expected_fld.len,
				pt->expected_fld.mask, pt->expected_hash);
		} else {
			NB_err_if(match,
				"Tag %s: (offt %d, len %u, mask 0x%0x, hash 0x%0lx)"
				" == expected (offt %d, len %u, mask %0x, hash 0x%0lx)\n",
				pt->tag, fld.offt, fld.len, fld.mask, hash,
				pt->expected_fld.offt, pt->expected_fld.len,
				pt->expected_fld.mask, pt->expected_hash);
		}
		printf("match == %d\n", match);
	}
	NB_inf("number of matcher tests == %ld",
			NLC_ARRAY_LEN(parse_tests));

	return err_cnt;
}


/*	main()
 * Run all test routines in this file.
 * Expects test routines to return err_cnt (0 on success).
 */
int main()
{
	printf("field_parse_test\n");
	int err_cnt = 0;
	err_cnt += parse_check();
	return err_cnt;
}
