/*	command_test_set.c
 * Define test set for command parsing
 * (c) 2018 Sirio Balmelli
 * (c) 2018 Alan Morrissett
 */
#include <nonlibc.h>
#include <stdio.h>
#include <zed_dbg.h>
#include "field.h"
#include "matcher.h"

/* ctuple: test-only data structure
 * @grammar = single field specification grammar
 * @expected_fld =  expected xdpk_field generated from grammar
 * @expected_hash = expected hash of value
 * @tag = human readable test identifier
 * @pos_test = if test supposed to succeed or fail
 */

struct ctuple {
	char			*cmdstr;
	int			expected_num_cmds;
	char			*tag;
	bool			pos_test;
};

const struct ctuple cmd_tests[] = {
	{
		"if: { add: eth0 }",
		1,
		"if1",
		true
	},
	{
		"if: { show: eth.* }",
		1,
		"if2",
		true
	},
		{
		"if: { del: eth0 }",
		1,
		"if3",
		true
	},
	{
		"field: { add: joes_ip, offt: 34, len: 4, val: 192.168.1.1 }",
		1,
		"fld1",
		true
	},
	{
		"field: { show: .* }",
		1,
		"fld2",
		true
	},
	{
		"action: { add: reflect, out: in.if }",
		1,
		"act1",
		true
	},
	{
		"action: { add: set_flag, offt: tcp.flags, len: 1, val: 0xC0 }",
		1,
		"act2",
		true
	},
	{
		"match: { add: to_joe, if: eth0, dir: in, sel: [joes_ip, isp] }",
		1,
		"match1",
		true
	},
	{
		"match: { add: to_isp, if: eth1, dir: out, "
		"sel: [joes_ip, [joes_isp, flagged]], act: set_flag }",
		1,
		"match2",
		true
	},
};