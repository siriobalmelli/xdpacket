#include <memref.h>
#include <yamlutils.h>
#include <refcnt.h>
#include <nstring.h>


static Pvoid_t state_JS = NULL; /* (char *field_name) -> (struct memref *ref) */

/*	state_check_exit()
 * Catch memory leaks.
 */
static void __attribute__((destructor)) memref_check_exit()
{
	JS_LOOP(&state_JS,
		struct memref *ref = val;
		NB_err("memref '%s' was not freed/released == leak", ref->name);
	);
}


/*	memref_release()
 * If a global state value, release a reference.
 * If dropping last reference, or if a literal value, free() underlying memory.
 */
void memref_release(void *arg)
{
	if (!arg)
		return;
	struct memref *ref = arg;

	if (memref_is_value(ref)) {
		free(ref->input);
		free(ref->rendered);
		free(ref);
		return;
	}

	/* cases where we are being released/freed:
	 * - errors in initialization (was never published)
	 * - last consumer is dropping a reference to us (being de-published)
	 * - one of several consumers is dropping a reference to us
	 */
	if (ref->refcnt) {
		refcnt_release(ref);
		if (ref->refcnt)
			return;
	}

	/* _only_ remove from state_JS if _we_ are there:
	 * we may never have been added or may be a dup.
	 */
	if (js_get(&state_JS, ref->name) == ref)
		js_delete(&state_JS, ref->name);

	free(ref);
}


/*	memref_value_new()
 */
struct memref *memref_value_new(const struct field *field, const char *value)
{
	return NULL; /* TODO: implement */
}


/*	memref_state_get()
 */
struct memref *memref_state_get(const struct field *field, const char *state_name)
{
	struct memref *ret = NULL;
	NB_die_if(!field || !state_name, "missing arguments");

	/* is there an existing state? */
	ret = js_get(&state_JS, state_name);

	/* if a state exists, make sure the field is the same */
	if (ret) {
		/* Unorthodox usage of short-circuit evalutation so that
		 * memref_release() does not drop someone else's ref on error.
		 */
		NB_die_if(ret->set.bytes != field->set.bytes && !(ret = NULL),
			"field '%s' does not match existing field for '%s'",
			field->name, state_name);

	/* otherwise, allocate a new one */
	} else {
		size_t len = field->set.len;
		NB_die_if(!(
			ret = calloc(1, sizeof(*ret) + len)
			), "fail alloc size %zu", sizeof(*ret) + len);
		ret->set = field->set;

		errno = 0;
		NB_die_if(!(
			ret->name = nstralloc(state_name, MAXLINELEN, NULL)
			), "string alloc fail");
		NB_die_if(errno == E2BIG, "value truncated:\n%s", ret->name);
		NB_die_if(
			js_insert(&state_JS, ret->name, ret, false)
			, "failed to insert new state '%s'", state_name);
	}

	/* always take a ref before returning */
	refcnt_take(ret);
	return ret;
die:
	memref_release(ret);
	return NULL;
}


/*	memref_emit()
 */
int memref_emit (struct memref *ref, yaml_document_t *outdoc, int outmapping)
{
	/* TODO: implement */
	return 0;
}
