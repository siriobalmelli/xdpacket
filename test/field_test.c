/*	field_test.c
 * Verify invariant bitfield and hashing manipulation math
 * (c) 2018 Sirio Balmelli
 */
#include <nonlibc.h>
#include <stdio.h>
#include <zed_dbg.h>
#include "field.h"


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
	int err_cnt = 0;

	/* tuple: test-only data structure
	 * @packet = array of bytes simulating an actual packet
	 * @matcher = field specification (what portion of 'packet' to hash)
	 * @hash = expected fnv1a result
	 */
	struct tuple {
		struct xdpk_field	matcher;
		uint64_t		hash;
		uint8_t			*packet;
	};
	/* TODO: test zero, odd masks, negative offsets, etc */
	const struct tuple tests[] = {
		{
		.matcher = { .offt = 0, .mlen = 40 },
		.hash = 0x4b64e9abbc760b0d,
		.packet = (uint8_t*)"blaar"
		}
	};

	for (int i=0; i < NLC_ARRAY_LEN(tests); i++) {
		const struct tuple *tt = &tests[i];
		uint64_t hash = xdpk_field_hash(tt->matcher, tt->packet, sizeof(tt->packet));
		Z_err_if(hash != tt->hash, "0x%lx 0x%lx", hash, tt->hash);
	}

	return err_cnt;
}


/*	main()
 */
int main()
{
	int err_cnt = 0;
	err_cnt += len_check();
	err_cnt += field_check();
	return err_cnt;
}
