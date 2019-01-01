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

	free(ne->match_list);
	free(ne->write_list);
	free(ne);
}

/*	node_new()
 * Create a new node.
 */
struct node *node_new(const char *name, struct iface *in, uint64_t seq, struct iface *out)
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

	/* TODO: register with interface */

	js_insert(&node_JS, ret->name, ret);
	return ret;
die:
	node_free(ret);
	return NULL;
}

/*	node_add_match()
 */
struct node *node_add_match(struct node *ne, const char *value, struct field *field)
{
	/* Basic sanity checking.
	 * Past this point, any error will cause us to abort and free 'ne' entirely.
	 */
	if (!ne || !value || !field)
		return ne;

	size_t alloc = ++ne->match_count * sizeof(struct field_value);
	NB_die_if(!(
		ne->match_list = realloc(ne->match_list, alloc)
		), "failed to realloc size %zu", alloc);

	struct field_value *fv = &ne->match_list[ne->match_count-1];
	fv->field = field;
	NB_die_if((
		snprintf(fv->value, sizeof(fv->value), "%s", value)
		) >= sizeof(fv->value), "value overflow '%s'", value);

	return ne;
die:
	node_free(ne);
	return NULL;
}

/*	node_add_write()
 */
struct node *node_add_write(struct node *ne, const char *value, struct field *field)
{
	/* TODO: generalize with preceding case and implement */
}
