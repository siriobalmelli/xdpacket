/*	test_pkt.c
 * Create test packets
 * (c) 2018 Sirio Balmelli
 * (c) 2018 Alan Morrissett
 */
#include <stdlib.h>
#include <nonlibc.h>
#include <stdio.h>
#include <zed_dbg.h>

static char *_cpkts[] = 
{
	"626c616172",			/* "blaar" */
	"58626c616172",			/* "Xblaar" */
	"58585858585858"		/* XXXXXXXblaarYYYYYY */
	  "626c616172595959595959",
};

struct _pkt {
	size_t len;
	uint8_t * pkt;
};

static struct _pkt *_pkts;

/*	hex_parse_nibble_()
Returns value of parsed nibble, -1 on error.
TODO: Remove and link to nonlibc instead.
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
	int npkts = (sizeof(_cpkts)/sizeof(char*));
	printf("npkts = %d\n", npkts);
	_pkts = malloc(sizeof(struct _pkt) * npkts);

	for (int i = 0; i < sizeof(_cpkts)/sizeof(char *); i++) {
		uint16_t slen = strlen(_cpkts[i]);
		Z_die_if((slen % 2 != 0), 
			"odd number of chars in _cpkts #%d", i);
		_pkts[i].len = slen/2;
		//printf("packet %d malloc %d bytes\n", i, _pkts[i].len);
		_pkts[i].pkt = malloc(_pkts[i].len);
		//printf("pkt == '");
		for (int j = 0; j < slen; j+=2) {
			uint8_t c1 = hex_parse_nibble_(&(_cpkts[i][j]));
			uint8_t c2 = hex_parse_nibble_(&(_cpkts[i][j+1]));
			uint8_t val = (c1 << 4) | c2;
			_pkts[i].pkt[j/2] = val;
			//printf("%c", val);
		}
		//printf("'\n");
	}

out:
	return;
}

void dump_pkt(const void *pkt, size_t plen)
{
	printf("len == %lu\n", plen);
	printf("packet == '");
	for (int i = 0; i < plen; i++)
		printf("%c", ((char*)pkt)[i]);
	printf("'\n");
}
