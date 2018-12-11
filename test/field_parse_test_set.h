/*	parse_test_set.c
 * Define test set for hashing
 * (c) 2018 Sirio Balmelli
 * (c) 2018 Alan Morrissett
 */
#include <nonlibc.h>
#include <stdio.h>
#include "field.h"
#include "matcher.h"

/* ptuple: test-only data structure
 * @grammar = single field specification grammar
 * @expected_fld =  expected xdpk_field generated from grammar
 * @expected_hash = expected hash of value
 * @tag = human readable test identifier
 * @pos_test = if test supposed to succeed or fail
 */

struct ptuple {
	char			*grammar;
	struct xdpk_field	expected_fld;
	uint64_t		expected_hash;
	char			*tag;
	bool			pos_test;
};

const struct ptuple parse_tests[] = {
	{
		"{ offset: 34, len: 2, val: 6501 }",
		{34, 16, 0xff, 0},
		0x8804f07b52d3063,
		"1",
		true
	},
	{
		"{ offset: tcp.flags, len: 1, mask: 0x80, val: 0 }",
		{46, 1, 0x80, 0},
		0xaf63bd4c8601b7df,
		"2",
		true
	},
/*
	{
		"80@0=0x00000000000000000001",
		{0, 80},
		0x69d306cc20f6edda,
		"3",
		true
	},
*/
};
