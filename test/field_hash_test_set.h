/*	hash_sets.h
 * Define test set for hashing
 * (c) 2018 Sirio Balmelli
 * (c) 2018 Alan Morrissett
 */
#include <nonlibc.h>
#include <stdio.h>
#include <zed_dbg.h>
#include "field.h"

/* htuple: test-only data structure
 * @matcher = field specification (what portion of 'packet' to hash)
 * @npkt = index of packet structure in pkts.
 * @hash = expected fnv1a result
 */
struct htuple {
	struct xdpk_field	matcher;
	int			npkt;
	uint64_t		hash;
	char			*tag;
	int			pos_test;
};

const struct htuple hash_tests[] = {
	// Straightforward "blaar" test
	{
	.matcher = { .offt = 0, .len = 5,  .mask = 0xff},
	.npkt = 0,
	.hash = 0x4b64e9abbc760b0d,
	"1",
	1
	},
	// Negative offset "blaar" test
	{
	.matcher = { .offt = -5, .len = 5, .mask = 0xff },
	.npkt = 0,
	.hash = 0x4b64e9abbc760b0d,
	"2",
	1
	},
	// Negative offset too big
	{
	.matcher = { .offt = -6, .len = 5, .mask = 0xff },
	.npkt = 0,
	.hash = 0x0,
	"3",
	1
	},
/*
	// Mask length too big
	{
	.matcher = { .offt = 0, .len = 6 },
	.npkt = 0,
	.hash = 0x0,
	"4",
	1
	},
	// "blaar" test with mask too short
	{
	.matcher = { .offt = 0, .len = 4 },
	.npkt = 0,
	.hash = 0x4b64e9abbc760b0d,
	"5",
	0
	},
	// Offset, with mask length too short but matching real end
	{
	.matcher = { .offt = 1, .len =  4},
	.npkt = 0,
	.hash = 0x0,
	"6",
	0
	},
	// Offset in "Xblaar"
	{
	.matcher = { .offt = 1, .len = 5 },
	.npkt = 1,
	.hash = 0x4b64e9abbc760b0d,
	"7",
	1
	},
	// Offset in "XXXXXXXblaarYYYYYY"
	{
	.matcher = { .offt = 7, .len = 5 },
	.npkt = 2,
	.hash = 0x4b64e9abbc760b0d,
	"8",
	1
	},
	// Negative offset in "XXXXXXXblaarYYYYYY"
	{
	.matcher = { .offt = -11, .len = 5 },
	.npkt = 2,
	.hash = 0x4b64e9abbc760b0d,
	"9",
	1
	},
	// This one shouldn't pass (?) but does
	// Mask off last bit of "blaar" and then take hash
	//{
	//.matcher = { .offt = 7, .len = 5 },
	//.npkt = 2,
	//.hash = 0x4b64e9abbc760b0d,
	//"10",
	//0
	// Mask off last two bits of "blaar" and then take hash
	{
	.matcher = { .offt = 7, .len = 5 },
	.npkt = 2,
	.hash = 0x4b64e9abbc760b0d,
	"11",
	0
	},
	// Check embedded "blaar" within 0's packet
	{
	.matcher = { .offt = 4, .len = 5 },
	.npkt = 3,
	.hash = 0x4b64e9abbc760b0d,
	"12",
	1
	},
	// Bigger mask for "blaar" within 0's packet
	{
	.matcher = { .offt = 4, .len = 6 },
	.npkt = 3,
	.hash = 0x4b64e9abbc760b0d,
	"13",
	0
	},
	// Negative indexed "blaar" within 0's packet
	{
	.matcher = { .offt = -96, .len = 5 },
	.npkt = 3,
	.hash = 0x4b64e9abbc760b0d,
	"14",
	1
	},
	// Check "blaar" shifted by 4 bits off byte boundary
	{
	.matcher = { .offt = 90, .len = 6 },
	.npkt = 3,
	.hash = 0x4b64e9abbc760b0d,
	"15",
	0
	},
	// Bigger packet
	{
	.matcher = { .offt = 9, .len = 5 },
	.npkt = 4,
	.hash = 0x4b64e9abbc760b0d,
	"16",
	1
	},
	// Bigger packet, negative offset
	{
	.matcher = { .offt = -1431, .len = 5 },
	.npkt = 4,
	.hash = 0x4b64e9abbc760b0d,
	"17",
	1
	},
	// Bigger packet, last instance
	{
	.matcher = { .offt = -29, .len = 5 },
	.npkt = 4,
	.hash = 0x4b64e9abbc760b0d,
	"18",
	1
	},
*/
};