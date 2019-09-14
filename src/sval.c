#include <sval.h>
#include <ndebug.h>
#include <value.h>
#include <nstring.h>
#include <yamlutils.h>


/*	sval_set_free()
 */
void sval_set_free(void * arg)
{
	free(arg);
}

/*	sval_set_new()
 */
struct sval_set	*sval_set_new (struct state *state, const char *value)
{
	struct sval_set *ret = NULL;

	/* 'len' may be zero for zero-length fields */
	size_t alloc_size = sizeof(*ret) + state->len;
	NB_die_if(!(
		ret = calloc(alloc_size, 1)
		), "fail malloc size %zu", alloc_size);

	ret->state = state;
	NB_die_if(
		value_parse(value, ret->bytes, state->len)
		, "");

	return ret;
die:
	sval_set_free(ret);
	return NULL;
};



/*	sval_free()
 */
void sval_free(void *arg)
{
	if (!arg)
		return;
	struct sval *sv = arg;

	/* Counterintuitively, use this to only print info when:
	 * - we are compiled with debug flags
	 * - 'field' and 'val' are non-NULL
	 */
	NB_wrn_if(sv->state && sv->val, "%s: %s", sv->state->name, sv->val);

	state_release(sv->state);
	free(sv->val);
	free(sv->rendered);
	sval_set_free(sv->set);
	free(sv);
}

/*	sval_new()
 */
struct sval *sval_new (const char *state_name, const char *value)
{
	struct sval *ret = NULL;
	NB_die_if(!state_name || !value, "sval requires 'state_name' and 'value'");

	NB_die_if(!(
		ret = calloc(1, sizeof(*ret))
		), "failed malloc size %zu", sizeof(*ret));

	NB_die_if(!(
		ret->state = state_get(state_name, 0)
		), "could not get state '%s'", state_name);
	/* We rely on state to tell us the length to parse 'value' into,
	 * so a 0-length state is nonsensical even though it may grow in future.
	 */
	NB_die_if(!ret->state->len, "state '%s' is zero-length", state_name);

	/* guard against insane values from the user */
	errno = 0;
	ret->val = nstralloc(value, MAXVALUELEN, NULL); /* a zero-length value is valid */
	NB_die_if(errno == EINVAL, "");
	NB_die_if(errno == E2BIG, "value truncated:\n%s", ret->val);

	NB_die_if(!(
		ret->set = sval_set_new(ret->state, ret->val)
		), "");
	NB_die_if(!(
		ret->rendered = value_render(ret->set->bytes, ret->state->len)
		), "");

	NB_inf("%s: %s", ret->state->name, ret->val);
	return ret;
die:
	sval_free(ret);
	return NULL;
}


/*	sval_emit()
 * Emit an interface as a mapping under 'outlist' in 'outdoc'.
 */
int sval_emit(struct sval *sval, yaml_document_t *outdoc, int outlist)
{
	int err_cnt = 0;
	int reply = yaml_document_add_mapping(outdoc, NULL, YAML_BLOCK_MAPPING_STYLE);
	NB_die_if(
		y_pair_insert(outdoc, reply, sval->state->name, sval->val)
#ifndef NDEBUG
		|| y_pair_insert(outdoc, reply, "bytes", sval->rendered)
#endif
		// || y_pair_insert_nf(outdoc, reply, "hash", "0x%"PRIx64, sval->set_hash)
		, "");
	NB_die_if(!(
		yaml_document_append_sequence_item(outdoc, outlist, reply)
		), "");
die:
	return err_cnt;
}
