#include <ndebug.h>
#include "field.h"

/*	field_valid_check()
 * Verify validity of field structures.
 */
int field_valid_check()
{
	int err_cnt = 0;

	/* tuple: test-only data structure
	 * @mlen = number of ones in the mask
	 * @size = number of bytes to hash (field length)
	 */
	struct tuple {
		struct xdpk_field field;
		bool expect;
	};

	const struct tuple tests[] = {
		{{0, 2, 0xff}, 1},
		{{-1, 1, 0xff}, 1},
		{{1000, 0, 0xff}, 0},
		{{0, 0, 0xff}, 0},
	};

	for (int i=0; i < NLC_ARRAY_LEN(tests); i++) {
		bool valid = xdpk_field_valid(tests[i].field);
		NB_err_if((valid ^ tests[i].expect),
			"xpdk_field_valid({%d, %u}) == %d, expected %d",
			tests[i].field.offt, tests[i].field.len, valid, tests[i].expect);
	}

	NB_inf("number of field_valid tests == %ld",
			NLC_ARRAY_LEN(tests));

	return err_cnt;
}


/* Run all tests for this file.
 * Expects test routines to return err_cnt (0 on success).
 */
int main()
{
	int err_cnt = 0;
	err_cnt += field_valid_check();
	return err_cnt;
}
