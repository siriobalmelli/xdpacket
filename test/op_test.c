#include <operations.h>
#include <parse2.h>

/* expose internals of parse2.h just for this test */
extern int parse(const unsigned char *buf, size_t buf_len, int outfd);


struct test {
	const char	*yaml;
};

struct test tests[] = {
	{	.yaml = "\
xdpk:\n\
  - field: mac src\n\
    offt: 6\n\
    len: 6\n\
  - rule: check src\n\
    match:\n\
      - dst: {field: \"mac src\"}\n\
        src: {value: \"0a:00:27:00:00:00\"}\n\
"
	}
};


/*	main()
 */
int main()
{
	int err_cnt = 0;

	for (unsigned int i = 0; i < NLC_ARRAY_LEN(tests); i++) {
		struct test *tc = &tests[i];
		/* set up environment for test to run */
		NB_die_if(
			parse((const unsigned char *)tc->yaml, strlen(tc->yaml), fileno(stdout))
			, "failed to parse YAML:\n%s", tc->yaml);
	}

die:
	return err_cnt;
}
