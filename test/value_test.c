/*	value_test.c
 * (c) 2019 Sirio Balmelli
 */

#include <value.h>
#include <nonlibc.h>
#include <ndebug.h>

struct test_case {
	size_t	field_len;	/* length of referenced field being parsed */
	char	*value;		/* user input text */
	uint8_t	*parsed;	/* expected parsing result (big-endian byte array) */
	char	*rendered;	/* expected human-readable rendering */
};

const static struct test_case tests[] = {
	{
		.field_len = 4,
		.value = "1.1.1.1",
		.parsed = (uint8_t []){0x01, 0x01, 0x01, 0x01},
		.rendered = "0x01010101"
	}
};

/* TODO: implement the following test cases:
 * 1.1.1.1                                     # ipv4 yes
 * 192.168.1.1                                 # ipv4 yes
 * 192.168.1                                   # ipv4 no
 * 192.168                                     # ipv4 no
 * 192.                                        # ipv4 no
 * 192                                         # ipv4 no
 * 00:11:aa:bb:cc:dd                           # mac yes
 * 00:11:aa:bb:cc                              # mac no
 * 00:11:aa:bb:                                # mac no
 * 00:                                         # mac no
 * 00                                          # mac no
 * 1                                           # int yes
 * 12                                          # int yes
 * x12134                                      # int yes
 * 0x1                                         # int yes
 * b01                                         # int yes
 * 01                                          # int yes
 * 2001:0db8:85a3:0000:0000:8a2e:0370:7334     # ipv6 yes
 * 2001:db8:85a3:0:0:8a2e:370:7334             # ipv6 yes
 * 2001:db8:85a3::8a2e:370:7334                # ipv6 yes
 * ::1                                         # ipv6 yes
 * ::                                          # ipv6 yes
 */

#define varprint(BUF, LEN)

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
			, "");
		NB_err_if(memcmp(parsed, tc->parsed, tc->field_len),
			"%s parsing error", tc->value);
		/* render */
		NB_err_if(!(
			rendered = value_render(parsed, tc->field_len)
			), "failed to render");
		NB_err_if(strcmp(rendered, tc->rendered),
			"rendered '%s' != expected '%s'", rendered, tc->rendered);
	}

die:
	free(parsed);
	free(rendered);
	return err_cnt;
}
