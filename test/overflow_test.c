/*	overflow_test.c
 * Test that GCC overflow-checking builtin add works when assigning (by adding 0)
 * a long type to a smaller (e.g. uint8_t) type.
 */
#include <ndebug.h>

int main()
{
	int err_cnt = 0;

	int16_t res;
	uint8_t res_u;
	long input;

	/* signed -> signed */
	input = (long)INT16_MAX;
	NB_die_if(__builtin_add_overflow(input, 0, &res)
		, "not expecting overflow");
	NB_die_if(res != INT16_MAX,
		"assigned yielded %d instead of %d", res, INT16_MAX);
	input++;
	NB_die_if(!__builtin_add_overflow(input, 0, &res)
		, "expecting overflow");
	input = (long)INT16_MIN -1;
	NB_die_if(!__builtin_add_overflow(input, 0, &res)
		, "expecting underflow");

	/* signed -> unsigned */
	input = (long)UINT8_MAX;
	NB_die_if(__builtin_add_overflow(input, 0, &res_u)
		, "not expecting overflow");
	NB_die_if(res_u != UINT8_MAX,
		"assigned yielded %d instead of %d", res_u, UINT8_MAX);
	input++;
	NB_die_if(!__builtin_add_overflow(input, 0, &res_u)
		, "expecting overflow");
	input = -1;
	NB_die_if(!__builtin_add_overflow(input, 0, &res_u)
		, "expecting underflow");
	input = (long)INT8_MIN;
	NB_die_if(!__builtin_add_overflow(input, 0, &res_u)
		, "expecting underflow");

	/* throw an alignment curve-ball at the compiler */
	struct test {
		uint8_t		u8;
		int32_t		i32;
		int16_t		i16;
		uint8_t		pad_8;
	}__attribute__((packed));
	struct test stack = { 0 };

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wpragmas"  /* GCC ignoring own option */
	#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
	input = (long)INT32_MAX;
	NB_die_if(__builtin_add_overflow(input, 0, &stack.i32)
		, "not expecting overflow");
	NB_die_if(stack.i32 != INT32_MAX,
		"assigned yielded %d instead of %d", stack.i32, INT32_MAX);
	input++;
	NB_die_if(!__builtin_add_overflow(input, 0, &stack.i32)
		, "expecting overflow");
	input = (long)INT32_MIN -1;
	NB_die_if(!__builtin_add_overflow(input, 0, &stack.i32)
		, "expecting underflow");
	#pragma GCC diagnostic pop

die:
	return err_cnt;
}
