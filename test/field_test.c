/*	field_test.c
 * Verify invariant bitfield and hashing manipulation math
 * (c) 2018 Sirio Balmelli
 * (c) 2018 Alan Morrissett
 */
#include <nonlibc.h>
#include <stdio.h>
#include <zed_dbg.h>
#include "field.h"

/* Separate code for creating test packet set for clarity */
#include "test_pkt.c"


/*	len_check()
 * Validate length calculation from mask values.
 */
int len_check()
{
	int err_cnt = 0;

	/* tuple: test-only data structure
	 * @mlen = number of ones in the mask
	 * @size = number of bytes to hash (field length)
	 */
	struct tuple {
		uint16_t mlen;
		size_t size;
	};
	/* TODO: add some edge cases to tests list */
	const struct tuple tests[] = {
		{ .mlen = 0, .size = 0 },
		{ .mlen = 1, .size = 1 },
		{ .mlen = 8, .size = 1 },
		{ .mlen = 9, .size = 2 },
		{ .mlen = 40, .size = 5 }
	};

	for (int i=0; i < NLC_ARRAY_LEN(tests); i++) {
		size_t check = xdpk_field_len(tests[i].mlen);
		Z_err_if(check != tests[i].size, "mlen %d -> %ld != %ld",
			tests[i].mlen, check, tests[i].size);
	}

	return err_cnt;
}

/*	field_check()
 * Validate that field hashing matches known values.
 */
int field_check()
{
	printf("Calling init_pkts()\n");
	init_pkts();

	int err_cnt = 0;

	/* tuple: test-only data structure
	 * @matcher = field specification (what portion of 'packet' to hash)
	 * @packet = array of bytes simulating an actual packet
	 * @len = size of packet in bytes
	 * @hash = expected fnv1a result
	 */
	struct tuple {
		struct xdpk_field	matcher;
		uint8_t			*packet;
		int			plen;
		uint64_t		hash;
	};
	/* TODO: test zero, odd masks, negative offsets, etc
	 * NOTE we use strlen so no non-ASCII constructs please
	 */
	const struct tuple tst[] = {
		{
		.matcher = { .offt = 0, .mlen = 40 },
		.packet = _pkts[0].pkt,
		.plen = _pkts[0].len,
		.hash = 0x4b64e9abbc760b0d
		},
		{
		.matcher = { .offt = 1, .mlen = 40 },
		.packet = _pkts[1].pkt,
		.plen = _pkts[1].len,
		.hash = 0x4b64e9abbc760b0d
		},
		{
		.matcher = { .offt = 7, .mlen = 40 },
		.packet = _pkts[2].pkt,
		.plen = _pkts[2].len,
		.hash = 0x4b64e9abbc760b0d
		},
		/*
		{
		.matcher = { .offt = 0, .mlen = 40 },
		.packet = (uint8_t*)"blaar",
		.plen = 0,
		.hash = 0x4b64e9abbc760b0d
		},
		{
		.matcher = { .offt = 1, .mlen = 40 },
		.packet = "xblaar",
		.plen = 0,
		.hash = 0x4b64e9abbc760b0d
		},
		{
		.matcher = { .offt = -5, .mlen = 40 },
		.packet = "xblaar",
		.plen = 0,
		.hash = 0x4b64e9abbc760b0d
		},
		{
		.matcher = { .offt = -19, .mlen = 40 },
		.packet = "xblaarYYYYYYYYYYYYYY",
		.plen = 0,
		.hash = 0x4b64e9abbc760b0d
		},
		*/

	};

	for (int i=0; i < NLC_ARRAY_LEN(tst); i++) {
		const struct tuple *tt = &tst[i];
		size_t len = tt->plen;
		if (tt->plen == 0)
			len = strlen((char*)tt->packet);
		dump_pkt(tt->packet, len);
		uint64_t hash = xdpk_field_hash(tt->matcher, tt->packet, len);
		Z_err_if(hash != tt->hash, "0x%lx 0x%lx len %zu",
			hash, tt->hash, len);
	}

	printf("number of field_check tests == %d\n", NLC_ARRAY_LEN(tst));

	return err_cnt;
}

int main()
{
	int err_cnt = 0;
	err_cnt += len_check();
	err_cnt += field_check();
	return err_cnt;
}
