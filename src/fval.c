#include <fval.h>
#include <ndebug.h>
#include <yamlutils.h>
#include <judyutils.h>


/*	fval_free()
 */
void fval_free(void *arg)
{
	free(arg);
}

/*	fval_new()
 */
struct fval *fval_new(const char *field_name, const char *value)
{
	struct fval *ret = NULL;
	NB_die_if(!field_name || !value, "fval requires 'field_name' and 'value'");

	struct field *field = NULL;
	NB_die_if(!(
		field = field_get(field_name)
		), "could not get field '%s'", field_name);

	/* do not allow a length longer than the maximum length value of a field-set */
	size_t max = ((size_t)1 << (sizeof(field->set.len) * 8)) -1;
	size_t vlen = strnlen(value, max);
	NB_die_if(!vlen, "zero-length 'value' invalid");
	NB_die_if(vlen == max, "value truncated:\n%.*s", (int)max, value);
	vlen++; /* \0 terminator */

	NB_die_if(!(
		ret = malloc(sizeof(*ret) + vlen)
		), "fail alloc size %zu", sizeof(*ret) + vlen);
	ret->field = field;
	ret->vlen = vlen;
	memcpy(ret->val, value, ret->vlen-1);
	ret->val[ret->vlen-1] = '\0';

	/* TODO: verify it will parse? */

	return ret;
die:
	fval_free(ret);
	return NULL;
}


/*	fval_emit()
 * Emit an interface as a mapping under 'outlist' in 'outdoc'.
 */
int fval_emit(struct fval *fval, yaml_document_t *outdoc, int outlist)
{
	int err_cnt = 0;
	int reply = yaml_document_add_mapping(outdoc, NULL, YAML_BLOCK_MAPPING_STYLE);
	NB_die_if(
		y_pair_insert(outdoc, reply, fval->field->name, fval->val)
		, "");
	NB_die_if(!(
		yaml_document_append_sequence_item(outdoc, outlist, reply)
		), "");
die:
	return err_cnt;
}
