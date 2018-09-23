/*	parse_test_set.c
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

struct ptuple {
	char			*grammar;
	struct xdpk_field	expected_fld;
	uint64_t		expected_hash;
};

const struct ptuple parse_tests[] = {
	//
	{
		"16@udp.sport=6501",
		{0, 16},
		0
	},
	{
		"16@udp.sport=6501",
		{0, 22},
		0
	},
};