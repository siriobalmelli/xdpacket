/*	fref.c
 * (c) 2018 Sirio Balmelli
 */

#include <fref.h>
#include <yamlutils.h>


/*	fref_free()
 */
void fref_free(void *arg)
{
	if (!arg)
		return;
	struct fref *fref = arg;
	NB_wrn("erase fref %s: %s", fref->field->name, fref->state->name);

	state_release(fref->state);
	field_release(fref->field);
	free(fref->ref);
	free(fref);
}


/*	fref_new()
 */
struct fref *fref_new(const char *field_name, const char *state_name, enum field_flags type)
{
	struct fref *ret = NULL;
	NB_die_if(!field_name || !state_name, "fref requires 'field_name' and 'ref_name'");
	NB_die_if(type != FIELD_FREF_STORE && type != FIELD_FREF_COPY,
		"invalid fref_type %d", type);

	NB_die_if(!(
		ret = calloc(1, sizeof(*ret))
		), "failed malloc size %zu", sizeof(*ret));

	NB_die_if(!(
		ret->field = field_get(field_name)
		), "could not get field '%s'", field_name);
	NB_die_if(!(
		ret->state = state_get(state_name, ret->field->set.len)
		), "could not get state '%s' size %d", state_name, ret->field->set.len);
	NB_die_if(!(
		ret->ref = malloc(sizeof(*ret->ref))
		), "fail malloc size %zu", sizeof(*ret->ref));

	 /* Use 'flags' to store whether "store" or "copy" */
	ret->ref->where = ret->field->set;
	ret->ref->where.flags |= type;
	ret->ref->bytes = ret->state->bytes;

	return ret;
die:
	fref_free(ret);
	return NULL;
}


/*	fref_emit()
 */
int fref_emit (struct fref *fref, yaml_document_t *outdoc, int outlist)
{
	int err_cnt = 0;
	int reply = yaml_document_add_mapping(outdoc, NULL, YAML_BLOCK_MAPPING_STYLE);
	NB_die_if(
		y_pair_insert(outdoc, reply, fref->field->name, fref->state->name)
		, "");
	NB_die_if(!(
		yaml_document_append_sequence_item(outdoc, outlist, reply)
		), "");
die:
	return err_cnt;
}
