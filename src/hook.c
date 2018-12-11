#include <hook.h>
#include <ndebug.h>

/*	hook_free()
 */
void hook_free(struct hook *hk)
{
	if (!hk)
		return;

	/* clean up matchers */
	hook_iter(hk, matcher_free);
	int Rc_word;
	JSLFA(Rc_word, hk->JS_match);

	free(hk);
}


/*	hook_new()
 */
struct hook *hook_new ()
{
	struct hook *ret = NULL;
	NB_die_if(!(
		ret = calloc(sizeof(*ret), 1)
		), "alloc size %zu", sizeof(*ret));

die:
	free(ret);
	return NULL;
}


/*	hook_callback()
 */
void hook_callback(struct hook *hk, void *buf, size_t len)
{
	hk->cnt_pkt++;

	Word_t *PValue;
	uint8_t index[MAXLINELEN] = { '\0' };
	JSLF(PValue, hk->JS_match, index);
	while (PValue) {
		if (matcher_do((struct matcher *)(*PValue), buf, len))
			break;
		JSLN(PValue, hk->JS_match, index);
	}
	if (!PValue)
		hk->cnt_drop++;
}


/*	hook_insert()
 * Insert 'mch' into 'hk' as 'name'.
 *
 * Returns 0 on success, -1 on error;
 * attempting to insert a duplicate item is an error.
 */
Word_t hook_insert(struct hook *hk, struct matcher *mch, const char *name)
{
	Pvoid_t *array = hk->JS_match;

	Word_t *PValue;
	JSLI(PValue, *array, (uint8_t*)name);
	if (*PValue)
		return -1;
	*PValue = (Word_t)mch;
	return 0;
}

/*	hook_delete()
 * Delete 'name' from 'hk' list.
 *
 * Returns the matcher at 'name' or NULL if nothing found to delete.
 * Caller is responsible for use or free() of returned matcher.
 */
struct matcher *hook_delete (struct hook *hk, const char *name)
{
	Pvoid_t *array = hk->JS_match;

	Word_t *PValue;
	JSLG(PValue, *array, (uint8_t *)name);
	if (!PValue)
		return NULL;

	struct matcher *ret = (void *)(*PValue);
	int Rc_int; /* we already know it exists, ignore this */
	JSLD(Rc_int, *array, (uint8_t*)name);
	return ret;
}

/*	hook_iter()
 * Iterate through all matchers of 'hk' list and call 'exec' on each.
 * Returns number of matchers processed.
 */
Word_t hook_iter (struct hook *hk, void(*exec)(struct matcher *mch))
{
	Pvoid_t *array = hk->JS_match;

	Word_t *PValue;
	uint8_t index[MAXLINELEN] = { '\0' };
	JSLF(PValue, *array, index);
	Word_t ret = 0;
	while (PValue) {
		exec((struct matcher *)(*PValue));
		JSLN(PValue, *array, index);
		ret++;
	}
	return ret;
}
