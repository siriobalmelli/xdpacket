/*	test_pkt.c
 * Create test packets
 * (c) 2018 Sirio Balmelli
 * (c) 2018 Alan Morrissett
 */
#include <stdlib.h>
#include <nonlibc.h>
#include <stdio.h>
#include <zed_dbg.h>

/* Keep definitions separately to make more readable */
#include "test_pkts.c"

struct pkt {
	size_t len;
	uint8_t * data;
};

static struct pkt *pkts;

/*	hex_parse_nibble_()
Returns value of parsed nibble, -1 on error.
TODO: Remove this copied function and link to nonlibc instead.
*/
uint8_t hex_parse_nibble_(const char *hex)
{
	/* We take advantage of the fact that sets of characters appear in
		the ASCII table in sequence ;)
	*/
	switch (*hex) {
		case '0'...'9':
			return (*hex) & 0xf;
		case 'A'...'F':
		case 'a'...'f':
			return 9 + ((*hex) & 0xf);
		default:
			return -1;
	}
}

/*	init_pkts()
Initializes the ASCII expressed packets into binary blocks.
*/
void init_pkts() {
	int npkts = (sizeof(cpkts)/sizeof(char*));
	pkts = malloc(sizeof(struct pkt) * npkts);

	for (int i = 0; i < npkts; i++) {
		uint16_t slen = strlen(cpkts[i]);
		Z_die_if((slen % 2 != 0),
			"odd number of chars in cpkts #%d", i);
		pkts[i].len = slen/2;
		pkts[i].data = malloc(pkts[i].len);
		for (int j = 0; j < slen; j+=2) {
			uint8_t c1 = hex_parse_nibble_(&(cpkts[i][j]));
			uint8_t c2 = hex_parse_nibble_(&(cpkts[i][j+1]));
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
