/*	field_test.c
 * (c) 2018 Sirio Balmelli
 */

#include <field.h>
#include <ndebug.h>


/*	field_test
 */
struct field_test {
	/* user input field description */
	char			*yaml;
	/* expected resulting field parameters */
	char			*name; /* NULL expected 'name' means we expect parsing to fail */
	struct field_set	set;
};


/*	tests[]
 * Array members will be run as tests.
 * Also serves to illustrate possible usage and expected results.
 */
struct field_test tests[] = {
	/* matches all packets */
	{
		.yaml = "{ field: wildcard, offt: 0, len: 0, mask: 0xff }",
		.name = "wildcard",
		.set = { .offt = 0, .len = 0, .mask = 0xff },
	},
	/* identical to 'wildcard': elided values should be given sane defaults */
	{
		.yaml = "{ field: wildcard elided }",
		.name = "wildcard elided",
		.set = { .offt = 0, .len = 0, .mask = 0xff }
	},
	/* match any packet at least 32B long */
	{
		.yaml = "{ field: length check, offt: 32 }",
		.name = "length check",
		.set = { .offt = 32, .len = 0, .mask = 0xff }
	},
	/* match the last (trailing) 4B of a packet */
	{
		.yaml = "{ field: end match, offt: -4, len: 4}",
		.name = "end match",
		.set = { .offt = -4, .len = 4, .mask = 0xff }
	},
	/* show maximum positive values for all fields; prove hex is parsed correctly */
	{
		.yaml = "{ field: max positive, offt: 0x7fffffff, len: 0xffff, mask: 0xff}",
		.name = "max positive",
		.set = { .offt = INT32_MAX, .len = UINT16_MAX, .mask = UINT8_MAX }
	},
	/* show minimum non-zero values for all fields */
	{
		.yaml = "{ field: max negative, offt: -2147483646, len: 1, mask: 1}",
		.name = "max negative",
		.set = { .offt = -2147483646, .len = 1, .mask = 1 }
	}
};


/*	test_field_parse()
 */
int test_field_parse(struct field_test *tst)
{
	yaml_document_t doc = {{0}};
	yaml_parser_t parser = {0};
	yaml_node_t *root = NULL;
	struct field *fld = NULL;
	int err_cnt = 0;

	/* treat a 'field' mapping as a standalone YAML document */
	NB_die_if(!
		yaml_parser_initialize(&parser)
		, "parser init");
	yaml_parser_set_input_string(&parser, (void *)tst->yaml, strlen(tst->yaml));
	NB_die_if(!
		yaml_parser_load(&parser, &doc)
		, "Invalid YAML, parse failed at position %zu",
		parser.context_mark.column);
	root = yaml_document_get_root_node(&doc);
	NB_die_if(!root || root->type != YAML_MAPPING_NODE,
		"malformed test yaml '%s'", tst->yaml);

#ifdef XDPACKET_DISALLOW_CLOBBER
	/* attempting to add duplicates _should_ fail */
	NB_die_if(
		field_parse(PARSE_ADD, &doc, root, NULL, 0)
		, "failed to add field from yaml '%s'", tst->yaml);
	NB_die_if(
		!field_parse(PARSE_ADD, &doc, root, NULL, 0)
		, "duplicate field did not yield error, yaml '%s'", tst->yaml);
#else
	/* parse YAML "add" - TWICE to verify duplicates are handled cleanly */
	NB_die_if(
		field_parse(PARSE_ADD, &doc, root, NULL, 0)
		, "failed to add field from yaml '%s'", tst->yaml);
	NB_die_if(
		field_parse(PARSE_ADD, &doc, root, NULL, 0)
		, "failed to add field from yaml '%s'", tst->yaml);
#endif

	/* test get/refcount mechanics */
	NB_die_if(!(
		fld = field_get(tst->name)
		), "fail to get field '%s' after parse", tst->name);
	NB_die_if(!fld || fld->refcnt != 1,
		"get/refcount mechanics broken");
	field_release(fld);

	/* verify that parsed field conforms to expectations */
	NB_die_if(strcmp(fld->name, tst->name),
		"naming broken '%s' != '%s'", fld->name, tst->name);
	NB_die_if(fld->set.offt != tst->set.offt,
		"offt mismatch: %d != %d", fld->set.offt, tst->set.offt);
	NB_die_if(fld->set.len != tst->set.len,
		"len mismatch: %d != %d", fld->set.len, tst->set.len);
	NB_die_if(fld->set.mask != tst->set.mask,
		"mask mismatch: 0x%hhx != 0x%hhx", fld->set.mask, tst->set.mask);
	NB_die_if(fld->set.flags != tst->set.flags,
		"flags mismatch: 0x%hhx != 0x%hhx", fld->set.flags, tst->set.flags);

	/* parse YAML "delete" - TWICE to verify duplicates are handled cleanly */
	NB_die_if(
		field_parse(PARSE_DEL, &doc, root, NULL, 0)
		, "failed to delete field from yaml '%s'", tst->yaml);
	/* verify field has been deleted */
	NB_die_if((
		fld = field_get(tst->name)
		) != 0, "field '%s' still available but should have been deleted", tst->name);

die:
	yaml_parser_delete(&parser);
	yaml_document_delete(&doc);
	field_free(fld);
	return err_cnt;
}


/*	main()
 */
int main()
{
	int err_cnt = 0;
	for (unsigned int i=0; i < NLC_ARRAY_LEN(tests); i++)
		err_cnt += test_field_parse(&tests[i]);
	return err_cnt;
}
