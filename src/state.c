/*	state.c
 * (c) 2018 Sirio Balmelli
 */

#include <state.h>
#include <judyutils.h>
#include <nstring.h>
#include <refcnt.h>


static Pvoid_t state_JS = NULL; /* (char *state_name) -> (struct state *state) */


/*	state_check_exit()
 * Catch memory leaks.
 */
static void __attribute__((destructor)) state_check_exit()
{
	JS_LOOP(&state_JS,
		struct state *state = val;
		NB_err("state '%s' was not freed/release (memory leak)", state->name);
	);
}


/*	state_release()
 * Decrement refcount and, if no more references, deregister and free.
 */
void state_release(struct state *state)
{
	if (!state)
		return;
	
	refcnt_release(state);
	if (state->refcnt)
		return;

	if (state->name) {
		js_delete(&state_JS, state->name);
		free(state->name);
	}
	free(state);
}

/*	state_get()
 * Get a pointer to 'name' state (or create a new one).
 * In the case an existing state is found (as opposed to a new one created),
 * guarantee that it is at least 'len' long.
 * Return state, incrementing ref count.
 * On failure (malloc), return NULL.
 */
struct state *state_get(const char *name, size_t len)
{
	struct state *ret = js_get(&state_JS, name);

	if (!ret) {
		NB_die_if(!(
			ret = calloc(1, sizeof(*ret) + len)
			), "fail alloc sz %zu", sizeof(*ret) + len);
		ret->len = len;

		errno = 0;
		NB_die_if(!(
			ret->name = nstralloc(name, MAXLINELEN, NULL)
			), "string alloc fail");
		NB_die_if(errno == E2BIG, "value truncated:\n%s", ret->name);

		js_insert(&state_JS, ret->name, ret, false);

	} else if (ret && (ret->len < len)) {
		ret = realloc(ret, sizeof(*ret) + len);
		ret->len = len;
		js_insert(&state_JS, ret->name, ret, true);
	}

	refcnt_take(ret);
	return ret;
die:
	state_release(ret);
	return NULL;
}
