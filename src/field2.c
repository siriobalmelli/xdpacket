#include <field2.h>
#include <ndebug.h>
#include <judyutils.h>
#include <yamlutils.h>


static Pvoid_t field_parse_JS = NULL; /* (char *field_name) -> (struct field *field) */


/*	field_free()
 */
void field_free(void *arg)
{
	if (!arg)
		return;
	struct field *fl = arg;
	js_delete(&field_parse_JS, fl->name);
	NB_wrn("erase field %s", fl->name);
	free(fl);
}

/*	field_new()
 */
struct field *field_new	(const char *name, ssize_t offt, size_t len, uint8_t mask)
{
	struct field *ret = NULL;
	NB_die_if(!name || !len, "insane parameters");
	NB_die_if(!(
		ret = malloc(sizeof(struct field))
		), "fail alloc sz %zu", sizeof(struct field));
	/* TODO: implement */
	NB_die_if(
		js_insert(&field_parse_JS, ret->name, ret)
		, "");
die:
	field_free(ret);
	return NULL;
}

/*	field_parse()
 * Parse 'root' according to 'mode' (add | rem | prn).
 * Returns 0 on success.
 */
int field_parse(enum parse_mode	mode,
		yaml_document_t *doc, yaml_node_t *mapping,
		yaml_document_t *outdoc, int outlist)
{
	int err_cnt = 0;
	struct field *field = NULL;

	/* possible params for a new field */
	const char *name = NULL;
	ssize_t offt = 0;
	size_t len = 0;
	uint8_t mask = 0x0;

	/* parse mapping */
	for (yaml_node_pair_t *pair = mapping->data.mapping.pairs.start;
		pair < mapping->data.mapping.pairs.top;
		pair++)
	{
		/* loop boilerplate */
		yaml_node_t *key = yaml_document_get_node(doc, pair->key);
		const char *keyname = (const char *)key->data.scalar.value;

		yaml_node_t *val = yaml_document_get_node(doc, pair->value);
		if (val->type != YAML_SCALAR_NODE) {
			NB_err("'%s' in iface not a scalar", keyname);
			continue;
		}
		const char *valtxt = (const char *)val->data.scalar.value;

		/* Match field names and populate 'local' */
		if (!strcmp("field", keyname)
			|| !strcmp("f", keyname))
		{
			name = valtxt;
		} else if (!strcmp("offt", keyname)
			|| !strcmp("o", keyname))
		{
			offt = 0; /* TODO: implement */
		} else {
			NB_err("'iface' does not implement '%s'", keyname);
		}
	}

	/* process based on 'mode' */
	switch (mode) {
	case PARSE_ADD:
	{
		NB_die_if(!(
			field = field_new(name, offt, len, mask)
			), "could not create new field '%s'", name);
		NB_die_if(field_emit(field, outdoc, outlist), "");
		break;
	}

	case PARSE_DEL:
		NB_die_if(!(
			field = js_get(&field_parse_JS, name)
			), "could not get field '%s'", name);
		NB_die_if(
			field_emit(field, outdoc, outlist)
			, "");
		field_free(field);
		break;

	case PARSE_PRN:
		/* if nothing is given, print all */
		if (!strcmp("", name)) {
			JS_LOOP(&field_parse_JS,
				NB_die_if(
					field_emit(val, outdoc, outlist)
					, "");
				);
		/* otherwise, search for a literal match */
		} else if ((field = js_get(&field_parse_JS, name))) {
			NB_die_if(field_emit(field, outdoc, outlist), "");
		}
		break;

	default:
		NB_err("unknown mode %s", parse_mode_prn(mode));
	};

die:
	return err_cnt;
}

/*	field_emit()
 * Emit an interface as a mapping under 'outlist' in 'outdoc'.
 */
int field_emit(struct field *field, yaml_document_t *outdoc, int outlist)
{
	int err_cnt = 0;
	int reply = yaml_document_add_mapping(outdoc, NULL, YAML_BLOCK_MAPPING_STYLE);
	NB_die_if(
		y_insert_pair(outdoc, reply, "field", field->name)
		|| y_insert_pair_nf(outdoc, reply, "offt", "%zd", field->matcher.offt)
		|| y_insert_pair_nf(outdoc, reply, "len", "%zu", field->matcher.len)
		|| y_insert_pair_nf(outdoc, reply, "mask", "0x%x", field->matcher.mask)
		, "");
	NB_die_if(!(
		yaml_document_append_sequence_item(outdoc, outlist, reply)
		), "");
die:
	return err_cnt;
}
