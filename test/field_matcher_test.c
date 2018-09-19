/*	field_test.c
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
#include "packet.h"

/* Keep definitions separately to make more readable */
#include "field_matcher_test_set.h"
#include "packets.h"

static struct pkt *pkts;

/*	init_pkts()
Initializes the ASCII expressed packets into binary blocks.
TODO: replace this with library function
*/
void init_pkts() {
	int npkts = (sizeof(cpkts)/sizeof(char*));
	pkts = malloc(sizeof(struct pkt) * npkts);

	for (int i = 0; i < npkts; i++) {
		uint16_t slen = strlen(cpkts[i]);
		Z_die_if(slen & 0x1, "odd number of chars in cpkts #%d", i);
		pkts[i].len = slen/2;
		pkts[i].data = malloc(pkts[i].len);
		for (int j = 0; j < slen; j+=2) {
			uint8_t c1 = hex_parse_nibble(&(cpkts[i][j]));
			uint8_t c2 = hex_parse_nibble(&(cpkts[i][j+1]));
			uint8_t val = (c1 << 4) | c2;
			pkts[i].data[j/2] = val;
		}
	}

out:
	return;
}

/*	free_pkts()
Initializes the ASCII expressed packets into binary blocks.
TODO: replace this with library function
*/
void free_pkts() {
	int npkts = (sizeof(cpkts)/sizeof(char*));

	for (int i = 0; i < npkts; i++)
		if (pkts[i].data != NULL) free(pkts[i].data);
	free(pkts);
	pkts = NULL;

	return;
}

/*
TODO: replace this with library function
*/
void dump_pkt(struct pkt *pkt)
{
	Z_log(Z_inf, "dump_pkt len == %lu", pkt->len);
	Z_log(Z_inf, "packet == '");
	for (int i = 0; i < pkt->len; i++)
		Z_log(Z_inf, "%c", ((char*)pkt->data)[i]);
	Z_log(Z_inf, "");
}


/*	matcher_check()
 * Validate a matcher can find packets with matching fields.
 */
int matcher_check()
{
	init_pkts();	
	//new_init_pkts(cpkts, npkts, &pkts);

	int err_cnt = 0;

	for (int i = 0; i < NLC_ARRAY_LEN(matcher_tests); i++) {
		const struct mtuple *mt = &matcher_tests[i];
		const struct xdpk_matcher *match = &mt->matcher;
		struct pkt *pkt = &pkts[mt->npkt];
		const uint64_t hash = mt->hash;
		bool is_match = xdpk_match(match, pkt->data, pkt->len, hash);
		Z_log(Z_inf, "test %d match == %d, len == %d", i, is_match, pkt->len);

		if (mt->pos_test) {
			Z_err_if(is_match != true, "Tag %s: %d != %d, len %zu",
				mt->tag, is_match, true, pkt->len);
		} else {
			Z_err_if(is_match == true, "Tag %s: %d != %d, len %zu",
				mt->tag, is_match, true, pkt->len);
		}
	}
	Z_log(Z_inf, "number of matcher tests == %ld",
			NLC_ARRAY_LEN(matcher_tests));

	free_pkts();

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
