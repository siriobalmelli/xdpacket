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
};

/* TODO: test zero, odd masks, negative offsets, etc */
const struct htuple hash_pos_tst[] = {
	{
	.matcher = { .offt = 0, .mlen = 40 },
	.npkt = 0,
	.hash = 0x4b64e9abbc760b0d
	},
	{
	.matcher = { .offt = 1, .mlen = 40 },
	.npkt = 1,
	.hash = 0x4b64e9abbc760b0d
	},
	{
	.matcher = { .offt = 7, .mlen = 40 },
	.npkt = 2,
	.hash = 0x4b64e9abbc760b0d
	},
};

const struct htuple hash_neg_tst[] = {
	{
	.matcher = { .offt = 0, .mlen = 32 },
	.npkt = 0,
	.hash = 0x4b64e9abbc760b0d
	},
	{
	.matcher = { .offt = 0, .mlen = 41},
	.npkt = 0,
	.hash = 0x4b64e9abbc760b0d
	},
};
