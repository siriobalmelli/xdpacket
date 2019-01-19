/*	field_test.c
 * (c) 2018 Sirio Balmelli
 */

#include <field.h>
#include <ndebug.h>


/* test all field hashing against this set of bytes.
 * TODO: how to test hashing?
 */
uint8_t some_bytes[] = {
	0x00, 0x01, 0x02, 0x03, 0x04,
	0xff, 0xfe, 0xfd, 0xfc, 0xfb
};


/*	field_test
 */
struct field_test {
	/* user input field description */
	char			*yaml;
	/* expected resulting field parameters */
	char			*name; /* NULL expected 'name' means we expect parsing to fail */
	struct field_set	set;
};


struct field_test tests[] = {
	{
		.yaml = "{ field: wildcard, offt: 0, len: 0, mask: 0xff }",
		.name = "wildcard",
		.set = { .offt = 0, .len = 0, .mask = 0xff },
	},
	{
		.yaml = "{ field: wildcard elided }",
		.name = "wildcard elided",
		.set = { .offt = 0, .len = 0, .mask = 0xff }
	}
};


/*	main()
 */
int main()
{
	int err_cnt = 0;
	/* TODO: implement */
	return err_cnt;
}
