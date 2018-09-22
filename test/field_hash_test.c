/*	field_test.c
 * Verify invariant bitfield and hashing manipulation math
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
#include "field_hash_test_set.h"
#include "packets.h"

static struct pkt *pkts;
const static int npkts = (sizeof(cpkts)/sizeof(char*));

/*	field_check()
 * Validate that field hashing matches known values.
 */
int field_check()
{
	init_pkts(cpkts, npkts, &pkts);

	int err_cnt = 0;

	for (int i=0; i < NLC_ARRAY_LEN(hash_tests); i++) {
		const struct htuple *tt = &hash_tests[i];
		struct pkt *pkt = &pkts[tt->npkt];
		uint64_t hash = xdpk_field_hash(tt->matcher,
				pkt->data, pkt->len);
		if (tt->pos_test) {
			Z_err_if(hash != tt->hash, "Tag %s: 0x%lx != 0x%lx, len %zu, '%c'",
				tt->tag, hash, tt->hash, pkt->len, 
				(char)xdpk_field_start_byte(tt->matcher, pkt->data, pkt->len));
		} else {
			Z_err_if(hash == tt->hash, "Tag %s: 0x%lx == 0x%lx, len %zu, '%c'",
				tt->tag, hash, tt->hash, pkt->len, 
				(char)xdpk_field_start_byte(tt->matcher, pkt->data, pkt->len));
		}
	}
	Z_log(Z_inf, "number of field_check tests == %ld",
			NLC_ARRAY_LEN(hash_tests));

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
	err_cnt += field_check();
	return err_cnt;
}
