/*	fref.c
 * (c) 2018 Sirio Balmelli
 */

#include <fref.h>
#include <yamlutils.h>


static Pvoid_t fref_JS = NULL; /* (char *ref_name) -> (struct fref *state) */
static Pvoid_t fref_state_JS = NULL; /* (char *state_name) -> (struct fref_state *state) */


/*	fref_free()
 */
void fref_free(void *arg)
{
	if (!arg)
		return;
	struct fref *ff = arg;
	NB_wrn("erase ff %s", ff->ref_name);

	/* we may be freeing a partially allocated object with no 'set' */
	uint8_t flags = ff->set_ref ? ff->set_ref->where.flags : 0;

	/* if we are state (pointed to by ref), check refcount and deregister */
	if (flags & FREF_STATE) {
		NB_err_if(ff->refcnt, "fref '%s' free with non-zero refcount", ff->ref_name);
		js_delete(&fref_JS, ff->ref_name);
		free(ff->set_state);

	/* otherwise, we are ref (pointer to state), decrement their refcount */
	} else if (flags & FREF_REF) {
		struct fref *state = js_get(&fref_JS, ff->ref_name);
		if (state)
			state->refcnt--;
		free(ff->set_ref);
	}

	field_release(ff->field);
	free(ff->ref_name);
	free(ff);
}


/*	fref_new()
 */
struct fref *fref_new(const char *field_name, const char *ref_name, enum fref_type type)
{
	struct fref *ret = NULL;
	NB_die_if(!field_name || !ref_name, "fref requires 'field_name' and 'ref_name'");

	NB_die_if(!(
		ret = calloc(1, sizeof(*ret))
		), "failed malloc size %zu", sizeof(*ret));

	NB_die_if(!(
		ret->field = field_get(field_name)
		), "could not get field '%s'", field_name);

	/* ref_name.
	 * do not allow a ref name longer than MAXLINELEN
	 */
	size_t vlen = strnlen(ref_name, MAXLINELEN);
	NB_die_if(vlen == MAXLINELEN, "ref_name truncated:\n%.*s", MAXLINELEN, ref_name);
	NB_die_if(!(
		ret->ref_name = malloc(vlen +1) /* add \0 terminator */
		), "fail alloc size %zu", vlen +1);
	snprintf(ret->ref_name, vlen +1, "%s", ref_name);

	/* State/field-specific set.
	 * Use field 'flags' to store whether "state" or "ref".
	 */
	switch (type) {
	case FREF_STATE:
	{
		/* Rely on free() *not* running js_delete()
		 * if ret->set_state is NULL.
		 */
		NB_die_if((
			js_get(&fref_JS, ret->ref_name)
			) != NULL, "store (state) '%s' already exists", ret->ref_name);

		struct fref_set_state *state; /* for legibility only */
		size_t len = ret->field->set.len;
		NB_die_if(!(
			state = ret->set_state = calloc(1, sizeof(*state) + len)
			), "fail malloc size %zu", sizeof(*state) + len);
		state->where = ret->field->set;
		state->where.flags |= FREF_STATE;
		js_insert(&fref_JS, ret->ref_name, ret, false);
		break;
	}

	case FREF_REF:
	{
		struct fref *state;
		NB_die_if(!(
			state = js_get(&fref_JS, ret->ref_name)
			), "cannot find state for ref_name '%s'", ret->ref_name);
		state->refcnt++;

		struct fref_set_ref *ref; /* for legibility only */
		NB_die_if(!(
			ref = ret->set_ref = malloc(sizeof(*ref))
			), "fail malloc size %zu", sizeof(*ref));
		ref->bytes = (uint8_t *)&state->set_state->bytes;
		ref->where = ret->field->set;
		ref->where.flags |= FREF_REF;
		NB_die_if(state->set_state->where.len < ref->where.len,
			"illegal ref of len %d to '%s' with len %d",
			ref->where.len, ret->ref_name, state->set_state->where.len);
		break;
	}

	default:
		NB_die("invalid fref_type %d", type);
	}

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
		y_pair_insert(outdoc, reply, fref->field->name, fref->ref_name)
		, "");
	NB_die_if(!(
		yaml_document_append_sequence_item(outdoc, outlist, reply)
		), "");
die:
	return err_cnt;
}
