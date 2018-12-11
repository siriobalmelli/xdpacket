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
#include <ndebug.h>
#include <fnv.h>
#include <parse.h>
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
			NB_err_if(hash != tt->hash, "Tag %s: 0x%lx != 0x%lx, len %zu, '%c'",
				tt->tag, hash, tt->hash, pkt->len,
				(char)xdpk_field_start_byte(tt->matcher, pkt->data, pkt->len));
		} else {
			NB_err_if(hash == tt->hash, "Tag %s: 0x%lx == 0x%lx, len %zu, '%c'",
				tt->tag, hash, tt->hash, pkt->len,
				(char)xdpk_field_start_byte(tt->matcher, pkt->data, pkt->len));
		}
	}
	NB_inf("number of field_check tests == %ld",
			NLC_ARRAY_LEN(hash_tests));

	free_pkts(npkts, &pkts);

	return err_cnt;
}

/*	baseline_hash_check()
 */
 void baseline_hash_check()
 {
	uint64_t h = 0;
	uint8_t byte = 0 & 0x80;
	h = fnv_hash64(NULL, (void*)&byte, sizeof(byte));
	printf("fnv_hash64 1-byte hash of 0, mask 0x80 == 0x%lx\n", h);
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
	char lenbuf[80];
	char buf2[80];
	char buf3[80];
	uint16_t len = 0;
	uint16_t mask = 0xff;
	uint64_t hash = 0;
	bool err;

	while (true) {
		int nparam;
		memset(lenbuf, 0, sizeof(lenbuf));
		memset(buf2, 0, sizeof(buf2));
		memset(buf3, 0, sizeof(buf3));
		printf("Enter <len> [<mask>==0xff] <value>: ");
		if (fgets(buf, sizeof(buf), stdin) == NULL)
			break;

		nparam = sscanf(buf, "%s %s %s", lenbuf, buf2, buf3);

		switch (nparam) {
		case 2:
			len = parse_uint16(lenbuf, strlen(lenbuf), &err);
			printf("len == %d (0x%04x), err = %d\n", len, len, err);
			mask = 0xff;
			printf("mask == %d (0x%02x)\n", mask, mask);
			//hash = parse_value(buf2, strlen(buf2), len, mask, &err);
			printf("hash == 0x%lx, err = %d\n", hash, err);
			break;
		case 3:
			len = parse_uint16(lenbuf, strlen(lenbuf), &err);
			printf("len == %d (0x%04x), err = %d\n", len, len, err);
			mask = parse_uint16(buf2, strlen(buf2), &err);
			printf("mask == %d (0x%02x), err = %d\n", mask, mask, err);
			//hash = parse_value(buf3, strlen(buf3), len, mask, &err);
			printf("hash == 0x%lx, err = %d\n", hash, err);
			break;
		default:
			printf("Error: 2 or 3 parameters needed.\n");
			continue;
		}
	}
}

/*	interactive_parse_check()
 */
void interactive_parse_check()
{
	char buf[1024];
	struct xdpk_field fld;
	int len;
	uint64_t hash;

	while (true) {
		memset(buf, 0, sizeof(buf));
		printf("Enter <YAML field expr>: ");
		if (fgets(buf, sizeof(buf), stdin) == NULL)
			break;
		len = strlen(buf);
		if (buf[len-1] == '\n')
			buf[--len] = '\0';
		fld = xdpk_field_parse(buf, strlen(buf), &hash);

		printf("offt == %u, len == %d, mask == 0x%02x, hash == 0x%016lx\n",
					fld.offt, fld.len, fld.mask, hash);
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
				fprintf(stderr, "Usage: %s [-n]\n", argv[0]);
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
