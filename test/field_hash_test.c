/*	field_test.c
 * Verify invariant bitfield and hashing manipulation math
 * (c) 2018 Sirio Balmelli
 * (c) 2018 Alan Morrissett
 */
#include <stdlib.h>
#include <stdio.h>
#include <nonlibc.h>
#include <unistd.h>
#include <getopt.h>
#include <binhex.h>
#include <arpa/inet.h>
#include <zed_dbg.h>
#include <fnv.h>
#include "field.h"
#include "packet.h"

/* Keep definitions separately to make more readable */
#include "field_hash_test_set.h"
#include "test_pkts.h"

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
				pkt->data, pkt->len, NULL);
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

/*	baseline_hash_check()
 */
 void baseline_hash_check()
 {
	uint64_t h = 0;
	uint8_t byte = 0;
	h = fnv_hash64(NULL, (void*)&byte, sizeof(byte));
	printf("fnv_hash64 1-bit hash of 0 == 0x%lx\n", h);
	uint16_t udp_sport = 6501;
	uint16_t n_udp_sport = htons(udp_sport);
	printf("udp_sport == 0x%04x, n_udp_sport == 0x%04x\n", udp_sport, n_udp_sport);
	h = fnv_hash64(NULL, (void*)&n_udp_sport, sizeof(udp_sport));
	printf("fnv_hash64 16-bit hash of 6501 == 0x%lx\n", h);
 }

/*	interactive_hash_check()
 */
void interactive_hash_check()
{
	char buf[80];
	char mlenb[80];
	char numb[80];
	uint16_t mlen = 0;
	uint64_t hash = 0;
	bool err;

	while (true) {
		memset(mlenb, 0, sizeof(mlenb));
		memset(numb, 0, sizeof(numb));
		printf("Enter <mlen> <number>: ");
		if (fgets(buf, sizeof(buf), stdin) == NULL)
			break;
		if (sscanf(buf, "%s %s", mlenb, numb) != 2) {
			printf("Error: two parameters needed.\n");
			continue;
		}
		mlen = parse_uint16(mlenb, strchr(mlenb, '\0'), &err);
		printf("mlen == 0x%x, err = %d\n", mlen, err);
		hash = parse_value(numb, strchr(numb, '\0'), mlen, &err);
		printf("hash == 0x%lx, err = %d\n", hash, err);
	}
}

/*	interactive_parse_check()
 */
void interactive_parse_check()
{
	char buf[1024];
	struct xdpk_field fld;
	uint64_t hash = 0;

	while (true) {
		memset(buf, 0, sizeof(buf));
		printf("Enter <field grammar>: ");
		if (fgets(buf, sizeof(buf), stdin) == NULL)
			break;
		if (sscanf(buf, "%s", buf) != 1) {
			printf("Error: one field grammar string needed.\n");
			continue;
		}
		fld = xdpk_field_parse(buf, &hash);

		printf("fld.mlen == %u, field.offt = %d\n", fld.mlen, fld.offt);
		printf("hash == 0x%lx\n", hash);
	}
}

/*	main()
 * Run all test routines in this file.
 * Expects test routines to return err_cnt (0 on success).
 */
int main(int argc, char *argv[])
{
	int err_cnt = 0;
	int opt = 0;
	bool interactive = false;
	bool baseline = false;
	bool parse_check = false;
	bool hash_check = false;

	if (isatty(0)) {
		while ((opt = getopt(argc, argv, "bph")) != -1) {
			interactive = true;
			switch (opt) {
			case 'b':
				baseline = true;
				break;
			case 'h':
				hash_check = true;
				break;
			case 'p':
				parse_check = true;
				break;
			default:
				fprintf(stderr, "Usage: %s [-n]\n");
				exit(EXIT_FAILURE);
			}
		}
		if (baseline)
			baseline_hash_check();
		if (interactive) {
			if (parse_check)
				interactive_parse_check();
			else
				interactive_hash_check();
		}
	}

	if (!interactive)
		err_cnt += field_check();

	return err_cnt;
}
