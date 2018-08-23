/*	field_test.c
 * Verify invariant bitfield and hashing manipulation math
 * (c) 2018 Sirio Balmelli
 */
#include <nonlibc.h>
#include <stdio.h>
#include <zed_dbg.h>
#include "field.h"


/* simulated data packets, "field" matchers and expected 64-bit fnv1a hash results */
uint8_t			*packets[] = {
	"blaar"
};
struct xdpk_field	matchers[] = {
	{ .offt = 0, mlen = 40 }
};
uint64_t		expected[] = {
	0x4b64e9abbc760b0d
};


/*	main()
 */
int main()
{
	for (int i=0; i < NLC_ARRAY_LEN(packets); i++) {
		uint64_t hash = xdpk_field_hash(matchers[i], packets[i], sizeof(packets[i]));
		Z_err_if(hash != expected[i], "");
	}
	return 0;
}
