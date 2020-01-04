/*	value_test.c
 * (c) 2019 Sirio Balmelli
 */

#include <value.h>
#include <nonlibc.h>
#include <ndebug.h>
#include <stdbool.h>

struct test_case {
	size_t	field_len;	/* length of referenced field being parsed */
	char	*value;		/* user input text */
	char	*rendered;	/* expected hex string representing parsed value (big-endian byte array) */
	bool	expect_fail;	/* parsing _should_ trigger an error */
};

const static struct test_case tests[] = {
	{	/* parse an IP */
		.field_len = 4,
		.value = "192.168.1.1",
		.rendered = "0xc0a80101"
	},
	{	/* all-zero is a valid IP */
		.field_len = 4,
		.value = "0.0.0.0",
		.rendered = "0x00000000"
	},
	{	/* invalid IP: parses as ASCII and truncates to "192" */
		.field_len = 3,
		.value = "192.168.1",
		.rendered = "0x313932"
	},
	{	/* parses as valid IP, then truncates to the first 3 bytes */
		.field_len = 3,
		.value = "192.168.1.0",
		.rendered = "0xc0a801"
	},
	{	/* valid MAC address */
		.field_len = 6,
		.value = "00:11:aa:bb:cc:dd",
		.rendered = "0x0011aabbccdd"
	},
	{	/* invalid MAC: parses as ASCII and truncates to "00:11" */
		.field_len = 5,
		.value = "00:11:aa:bb:cc",
		.rendered = "0x30303a3131"
	},
	{	/* parses as ASCII and truncates to "00:" */
		.field_len = 3,
		.value = "00:",
		.rendered = "0x30303a"
	},
	{	/* uint64_t */
		.field_len = 8,
		.value = "1",
		.rendered = "0x0000000000000001"
	},
	{	/* uint32_t */
		.field_len = 4,
		.value = "12",
		.rendered = "0x0000000c"
	},
	{	/* uint32_t */
		.field_len = 4,
		.value = "0x12134",
		.rendered = "0x00012134"
	},
	{	/* uint16_t (overflow) */
		.field_len = 2,
		.value = "0x12134",
		.rendered = "0x0000", /* out-of-bounds == value not set */
		.expect_fail = true
	},
	{	/* positive prefix */
		.field_len = 1,
		.value = "+11",
		.rendered = "0x0b"
	},
	{	/* negative numbers */
		.field_len = 2,
		.value = "-1",
		.rendered = "0xffff"
	},
	{	/* leading spaces, large 0X, extra zeroes */
		.field_len = 1,
		.value = "  0X000000d",
		.rendered = "0x0d"
	},
	{	/* valid IPv6*/
		.field_len = 16,
		.value = "2001:0db8:85a3:0000:0000:8a2e:0370:7334",
		.rendered = "0x20010db885a3000000008a2e03707334"
	},
	{	/* valid IPv6: collapse '0000' to '0' */
		.field_len = 16,
		.value = "2001:db8:85a3:0:0:8a2e:370:7334",
		.rendered = "0x20010db885a3000000008a2e03707334"
	},
	{	/* valid IPv6: '::' to elide sequences of '0' */
		.field_len = 16,
		.value = "2001:db8:85a3::8a2e:370:7334",
		.rendered = "0x20010db885a3000000008a2e03707334"
	},
	{	/* valid IPv6: loopback */
		.field_len = 16,
		.value = "::1",
		.rendered = "0x00000000000000000000000000000001"
	},
	{	/* valid IPv6: any */
		.field_len = 16,
		.value = "::",
		.rendered = "0x00000000000000000000000000000000"
	},
	{	/* arbitrary length of hex digits */
		.field_len = 14,
		.value = "0x0102030405060708090a0b0c0d0e",
		.rendered = "0x0102030405060708090a0b0c0d0e"
	},
	{	/* ASCII: space, then tab */
		.field_len = 2,
		.value = " 	",
		.rendered = "0x2009"
	}
};


/*	main()
 */
int main()
{
	int err_cnt = 0;

	uint8_t *parsed = NULL;
	char *rendered = NULL;

	for (unsigned int i=0; i < NLC_ARRAY_LEN(tests); i++) {
		const struct test_case *tc = &tests[i];
		NB_die_if(!(
			parsed = malloc(tc->field_len)
			), "failed alloc len %zu", tc->field_len);
		/* parse */
		NB_err_if(
			value_parse(tc->value, parsed, tc->field_len)
			&& !tc->expect_fail /* some tests are designed to fail */
			, "");
		/* render and compare */
		NB_err_if(!(
			rendered = value_render(parsed, tc->field_len)
			), "failed to render");
		NB_err_if(strcmp(rendered, tc->rendered),
			"'%s' rendered as '%s' != expected '%s' (test %u)",
			tc->value, rendered, tc->rendered, i);

		free(parsed); parsed = NULL;
		free(rendered); rendered = NULL;
	}

die:
	free(parsed);
	free(rendered);
	return err_cnt;
}
