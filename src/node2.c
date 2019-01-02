/*	node2.c
 * (c) 2018 Sirio Balmelli
 */

#include <node2.h>
#include <ndebug.h>
#include <judyutils.h>


static Pvoid_t	node_JS = NULL; /* (char *node_name) -> (struct node *node) */


/*	node_free()
 */
void node_free(void *arg)
{
	if (!arg)
		return;
	struct node *ne = arg;

	js_delete(&node_JS, ne->name);

	JL_LOOP(&ne->matches_JQ,
		fval_free(val);
	       );
	JL_LOOP(&ne->mangles_JQ,
		fval_free(val);
	       );

	free(ne);
}

/*	node_new()
 * Create a new node.
 */
struct node *node_new(const char *name, Pvoid_t matches_JQ, Pvoid_t mangles_JQ)
{
	struct node *ret = NULL;
	NB_die_if(!name, "no name given for node");

	/* no easy way of knowing if dups are identical, kill them */
	if ((ret = js_get(&node_JS, name))) {
		NB_wrn("node '%s' already exists: deleting", name);
		node_free(ret);
	}

	NB_die_if(!(
		ret = calloc(1, sizeof(*ret))
		), "fail alloc size %zu", sizeof(*ret));
	NB_die_if((
		snprintf(ret->name, sizeof(ret->name), "%s", name)
		) >= sizeof(ret->name), "node name overflow '%s'", name);

	ret->matches_JQ = matches_JQ;
	ret->mangles_JQ = mangles_JQ;

	/* TODO: register with interface */

	js_insert(&node_JS, ret->name, ret, true);
	return ret;
die:
	node_free(ret);
	return NULL;
}
