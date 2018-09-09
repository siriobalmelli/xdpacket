/*	field_test.c
 * Verify invariant bitfield and hashing manipulation math
 * (c) 2018 Sirio Balmelli
 * (c) 2018 Alan Morrissett
 */
#include <stdlib.h>
#include <nonlibc.h>
#include <binhex.h>
#include <zed_dbg.h>
#include "field.h"

/* Keep definitions separately to make more readable */
#include "hash_sets.h"
#include "packets.h"

struct pkt {
	size_t len;
	uint8_t * data;
};

static struct pkt *pkts;

/*	init_pkts()
Initializes the ASCII expressed packets into binary blocks.
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
*/
void free_pkts() {
	int npkts = (sizeof(cpkts)/sizeof(char*));

	for (int i = 0; i < npkts; i++)
		if (pkts[i].data != NULL) free(pkts[i].data);
	free(pkts);
	pkts = NULL;

	return;
}

void dump_pkt(struct pkt *pkt)
{
	Z_log(Z_inf, "dump_pkt len == %lu", pkt->len);
	Z_log(Z_inf, "packet == '");
	for (int i = 0; i < pkt->len; i++)
		Z_log(Z_inf, "%c", ((char*)pkt->data)[i]);
	Z_log(Z_inf, "");
}


/*	field_check()
 * Validate that field hashing matches known values.
 */
int field_check()
{
	init_pkts();

	int err_cnt = 0;

	for (int i=0; i < NLC_ARRAY_LEN(hash_tests); i++) {
		const struct htuple *tt = &hash_tests[i];
		struct pkt *pkt = &pkts[tt->npkt];
		uint64_t hash = xdpk_field_hash(tt->matcher,
				pkt->data, pkt->len);
		if (tt->pos_test) {
			Z_err_if(hash != tt->hash, "Tag %s: 0x%lx != 0x%lx len %zu",
				tt->tag, hash, tt->hash, pkt->len);
		} else {
			Z_err_if(hash == tt->hash, "Tag %s: 0x%lx == 0x%lx len %zu",
				tt->tag, hash, tt->hash, pkt->len);
		}
	}
	Z_log(Z_inf, "number of field_check tests == %ld",
			NLC_ARRAY_LEN(hash_tests));

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
	err_cnt += field_check();
	return err_cnt;
}
