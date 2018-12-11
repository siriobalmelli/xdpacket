/*	field_test.c
 * Verify field matching
 * (c) 2018 Sirio Balmelli
 * (c) 2018 Alan Morrissett
 */
#include <stdlib.h>
#include <stdio.h>
#include <nonlibc.h>
#include <binhex.h>
#include <ndebug.h>
#include "field.h"
#include "packet.h"

/* Keep definitions separately to make more readable */
#include "field_matcher_test_set.h"
#include "test_pkts.h"

static struct pkt *pkts;
const static int npkts = (sizeof(cpkts)/sizeof(char*));

/*	matcher_check()
 * Validate a matcher can find packets with matching fields.
 */
int matcher_check()
{
	init_pkts(cpkts, npkts, &pkts);

	int err_cnt = 0;

	for (int i = 0; i < NLC_ARRAY_LEN(matcher_tests); i++) {
		const struct mtuple *mt = &matcher_tests[i];
		const struct xdpk_matcher *match = &mt->matcher;
		struct pkt *pkt = &pkts[mt->npkt];
		const uint64_t hash = mt->hash;
		bool is_match = xdpk_match(match, pkt->data, pkt->len, hash);
		NB_inf("test %d match == %d, len == %ld", i, is_match, pkt->len);

		if (mt->pos_test) {
			NB_err_if(is_match != true, "Tag %s: %d != %d, len %zu",
				mt->tag, is_match, true, pkt->len);
		} else {
			NB_err_if(is_match == true, "Tag %s: %d != %d, len %zu",
				mt->tag, is_match, true, pkt->len);
		}
	}
	NB_inf("number of matcher tests == %ld",
			NLC_ARRAY_LEN(matcher_tests));

	free_pkts(npkts, &pkts);

	return err_cnt;
}


/*	main()
 * Run all test routines in this file.
 * Expects test routines to return err_cnt (0 on success).
 */
int main()
{
	int err_cnt = 0;
	err_cnt += matcher_check();
	return err_cnt;
}
