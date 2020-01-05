#include <rule.h>
#include <operations.h>
#include <parse2.h>

/* expose internals of parse2.h just for this test */
extern int parse(const unsigned char *buf, size_t buf_len, int outfd);


/* one-time declaration of fields used by all tests */
const char *fields = "\
xdpk:\n\
  - field: any\n\
    offt: 0\n\
    len: 0\n\
  - field: mac dst\n\
    offt: 0\n\
    len: 6\n\
  - field: mac src\n\
    offt: 6\n\
    len: 6\n\
  - field: ttl\n\
    offt: 22\n\
    len: 1\n\
  - field: ip src\n\
    offt: 26\n\
    len: 4\n\
  - field: ip dst\n\
    offt: 30\n\
    len: 4\n\
";


struct test {
	const char	*yaml;
	uint8_t		*pkt_in; /* packet input: it is expected that match rules match this */
	size_t		pkt_len;
	const uint8_t	*pkt_ex; /* expected result after write rules are applied */
};

struct test tests[] = {
	{	/* nop should _not_ access memory */
		.yaml = "\
xdpk:\n\
  - rule: rule\n\
    match:\n\
      - dst: {field: \"any\"}\n\
        src: {field: \"any\"}\n\
    write:\n\
      - dst: {field: \"any\"}\n\
        src: {field: \"any\"}\n\
",
		.pkt_in = NULL,
		.pkt_len = 0,
		.pkt_ex = NULL
	},
	{	/* value -> field */
		.yaml = "\
xdpk:\n\
  - rule: rule\n\
    match:\n\
      - dst: {field: \"any\"}\n\
        src: {field: \"any\"}\n\
    write:\n\
      - dst: {field: \"mac dst\"}\n\
        src: {value: \"0a:00:27:00:00:00\"}\n\
",
		.pkt_in = (uint8_t []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		.pkt_len = 6,
		.pkt_ex = (uint8_t []){0x0a, 0x00, 0x27, 0x00, 0x00, 0x00}
	}
};


/*	main()
 */
int main()
{
	int err_cnt = 0;
	struct rule *rl = NULL;

	/* parse all fields only once */
	NB_die_if(
		parse((const unsigned char *)fields, strlen(fields), fileno(stdout))
		, "failed to parse YAML:\n%s", fields);

	for (unsigned int i = 0; i < NLC_ARRAY_LEN(tests); i++) {
		struct test *tc = &tests[i];
		/* set up environment for test to run */
		NB_die_if(
			parse((const unsigned char *)tc->yaml, strlen(tc->yaml), fileno(stdout))
			, "failed to parse YAML:\n%s", tc->yaml);

		/* we expect only one rule named 'rule' */
		rl = rule_get("rule");
		NB_die_if(!rl, "");

		/* all 'match' rules expected to test positive */
		JL_LOOP(&rl->match_JQ,
			struct op *op = val;
			NB_die_if(
				op_match(&op->set, tc->pkt_in, tc->pkt_len)
				, "");
		);
		/* all 'write' rules expected to succeed */
		JL_LOOP(&rl->write_JQ,
			struct op *op = val;
			NB_die_if(
				op_write(&op->set, tc->pkt_in, tc->pkt_len)
				, "");
		);

		/* final packet state should match expectations */
		NB_die_if(
			memcmp(tc->pkt_in, tc->pkt_ex, tc->pkt_len)
			, "");

		rule_release(rl);
		rule_free(rl);
		rl = NULL;
	}

die:
	rule_release(rl);
	rule_free(rl);
	field_free_all();
	return err_cnt;
}
