#include <hook.h>
#include <ndebug.h>

/*	hook_free()
 */
void hook_free(struct hook *hk)
{
	if (!hk)
		return;
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
	return ret;
die:
	free(ret);
	return NULL;
}


/*	hook_callback()
 */
void hook_callback(struct hook *hk, void *buf, size_t len)
{
	hk->cnt_pkt++;
	hk->cnt_drop++;
	/* TODO implement matching */
}
