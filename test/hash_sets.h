/*	tests.c
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

/* TODO: test zero, odd masks, negative offsets, etc */
const struct htuple hash_tests[] = {
	// Straightforward "blaar" test
	{
	.matcher = { .offt = 0, .mlen = 40 },
	.npkt = 0,
	.hash = 0x4b64e9abbc760b0d,
	"1",
	1
	},
	// Negative offset "blaar" test
	{
	.matcher = { .offt = -5, .mlen = 40 },
	.npkt = 0,
	.hash = 0x4b64e9abbc760b0d,
	"2",
	1
	},
	// Negative offset too big
	{
	.matcher = { .offt = -6, .mlen = 40 },
	.npkt = 0,
	.hash = 0x0,
	"3",
	1
	},
	// Mask length too big
	{
	.matcher = { .offt = 0, .mlen = 41 },
	.npkt = 0,
	.hash = 0x0,
	"4",
	1
	},
	// "blaar" test with mask too short
	{
	.matcher = { .offt = 0, .mlen = 32 },
	.npkt = 0,
	.hash = 0x4b64e9abbc760b0d,
	"5",
	0
	},
	// Offset, with mask length too big
	{
	.matcher = { .offt = 1, .mlen =  35},
	.npkt = 0,
	.hash = 0x0,
	"6",
	1
	},
	// Offset in "Xblaar"
	{
	.matcher = { .offt = 1, .mlen = 40 },
	.npkt = 1,
	.hash = 0x4b64e9abbc760b0d,
	"7",
	1
	},
	// Offset in "XXXXXXXblaarYYYYYY"
	{
	.matcher = { .offt = 7, .mlen = 40 },
	.npkt = 2,
	.hash = 0x4b64e9abbc760b0d,
	"8",
	1
	},
	// Negative offset in "XXXXXXXblaarYYYYYY"
	{
	.matcher = { .offt = -11, .mlen = 40 },
	.npkt = 2,
	.hash = 0x4b64e9abbc760b0d,
	"9",
	1
	},
	/* This one shouldn't pass (?) but does
	// Mask off last bit of "blaar" and then take hash
	{
	.matcher = { .offt = 7, .mlen = 39 },
	.npkt = 2,
	.hash = 0x4b64e9abbc760b0d,
	"10",
	0
	},*/
	// Mask off last two bits of "blaar" and then take hash
	{
	.matcher = { .offt = 7, .mlen = 38 },
	.npkt = 2,
	.hash = 0x4b64e9abbc760b0d,
	"11",
	0
	},
	// Check embedded "blaar" within 0's packet
	{
	.matcher = { .offt = 4, .mlen = 40 },
	.npkt = 3,
	.hash = 0x4b64e9abbc760b0d,
	"12",
	1
	},
	// Bigger mask for "blaar" within 0's packet
	{
	.matcher = { .offt = 4, .mlen = 48 },
	.npkt = 3,
	.hash = 0x4b64e9abbc760b0d,
	"13",
	0
	},
	// Negative indexed "blaar" within 0's packet
	{
	.matcher = { .offt = -96, .mlen = 40 },
	.npkt = 3,
	.hash = 0x4b64e9abbc760b0d,
	"14",
	1
	},
	// Check "blaar" shifted by 4 bits off byte boundary
	{
	.matcher = { .offt = 90, .mlen = 48 },
	.npkt = 3,
	.hash = 0x4b64e9abbc760b0d,
	"15",
	0
	},
	// Bigger packet
	{
	.matcher = { .offt = 9, .mlen = 40 },
	.npkt = 4,
	.hash = 0x4b64e9abbc760b0d,
	"16",
	1
	},
	/**/
	// Bigger packet, negative offset
	{
	.matcher = { .offt = -1431, .mlen = 40 },
	.npkt = 4,
	.hash = 0x4b64e9abbc760b0d,
	"17",
	1
	},
	// Bigger packet, last instance
	{
	.matcher = { .offt = -29, .mlen = 40 },
	.npkt = 4,
	.hash = 0x4b64e9abbc760b0d,
	"18",
	1
	},
	/**/
};