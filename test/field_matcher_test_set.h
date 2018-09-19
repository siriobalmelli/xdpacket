/*	matcher_test_set.c
 * Define test set for hashing
 * (c) 2018 Sirio Balmelli
 * (c) 2018 Alan Morrissett
 */
#include <nonlibc.h>
#include <stdio.h>
#include <zed_dbg.h>
#include "field.h"
#include "matcher.h"

/* mtuple: test-only data structure
 * @matcher = set of field specification to check for hash match
 * @npkt = index of packet structure in packet test array
 * @hash = expected fnv1a result
 * @tag = human readable test identifier
 * @pos_test = if test supposed to succeed or fail
 */

 /* TODO: size XDPK_MATCH_FIELD_MAX might change.  
  * Make this resistant to that by creating local structure
  * size and only copying in what will fit.
  */

struct mtuple {
	struct xdpk_matcher	matcher;
	int			npkt;
	uint64_t		hash;
	char			*tag;
	int			pos_test;
};

const struct mtuple matcher_tests[] = {
	// Straightforward "blaar" test
	{
		{{
			{ 0, 40 },
			{ 0, 0 },
			{ 0, 0 },
			{ 0, 0 },
		}},
		.npkt = 0,
		.hash = 0x4b64e9abbc760b0d,
		"1",
		1
	},
	// Match on the second
	{
		{{
			{ 10, 40 },
			{ 0, 40 },
			{ 0, 0 },
			{ 0, 0 },
		}},
		.npkt = 0,
		.hash = 0x4b64e9abbc760b0d,
		"2",
		1
	},
	// Match on the third
	{
		{{
			{ 20, 40 },
			{ -10, 40 },
			{ 0, 40 },
			{ 0, 0 },
		}},
		.npkt = 0,
		.hash = 0x4b64e9abbc760b0d,
		"3",
		1
	},
	// Match on the fourth with negative offset
	{
		{{
			{ 20, 40 },
			{ -10, 40 },
			{ 0, 30 },
			{ -5, 40 },
		}},
		.npkt = 0,
		.hash = 0x4b64e9abbc760b0d,
		"4",
		1
	},
	// Fail on invalid field
	{
		{{
			{ 0, 0 },
			{ 0, 40 },
			{ 0, 0 },
			{ 0, 0 },
		}},
		.npkt = 0,
		.hash = 0x4b64e9abbc760b0d,
		"5",
		0
	},
	// Bigger packet
	{
		{{
			{ 0, 1 },
			{ 0, 40 },
			{ 9, 40 },
			{ 0, 0 },
		}},
		.npkt = 4,
		.hash = 0x4b64e9abbc760b0d,
		"6",
		1
	},
	// Check within 0's packet
	{
		{{
			{ 0, 8 },
			{ 4, 40 },
			{ 0, 0 },
			{ 0, 0 },
		}},
		.npkt = 3,
		.hash =  0x4b64e9abbc760b0d,
		"7",
		1
	},
};