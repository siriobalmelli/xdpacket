/*	node2.c
 * (c) 2018 Sirio Balmelli
 */

#include <node2.h>
#include <ndebug.h>
#include <judyutils.h>


static Pvoid_t	node_JS = NULL; /* (char *node_name) -> (struct node *node) */
static Pvoid_t	node_J = NULL; /* (uint64_t seq) -> (struct node *node) */


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
	JL_LOOP(&ne->writes_JQ,
		fval_free(val);
	       );

	free(ne);
}

/*	node_new()
 * Create a new node.
 */
struct node *node_new(const char *name, uint64_t seq,
		struct iface *in, struct iface *out,
		Pvoid_t matches_JQ, Pvoid_t writes_JQ)
{
	struct node *ret = NULL;
	NB_die_if(!name || !in, "no name or input iface given for node");

	/* return already existing ONLY if identical */
	if ((ret = js_get(&node_JS, name))) {
		if (ret->in == in && ret->out == out)
			return ret;
		NB_wrn("node '%s' already exists but not identical: deleting", name);
		node_free(ret);
	}

	NB_die_if(!(
		ret = calloc(1, sizeof(*ret))
		), "fail alloc size %zu", sizeof(*ret));
	NB_die_if((
		snprintf(ret->name, sizeof(ret->name), "%s", name)
		) >= sizeof(ret->name), "node name overflow '%s'", name);

	NB_die_if(!(
		ret->in = in
		), "node '%s' does not have a valid input interface", ret->name);
	ret->out = out;
	ret->matches_JQ = matches_JQ;
	ret->writes_JQ = writes_JQ;

	/* TODO: add to sequence */
	/* TODO: register with interface */

	js_insert(&node_JS, ret->name, ret, true);
	return ret;
die:
	node_free(ret);
	return NULL;
}
