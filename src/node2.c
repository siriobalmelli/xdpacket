/*	node2.c
 * (c) 2018 Sirio Balmelli
 */

#include <node2.h>
#include <ndebug.h>
#include <judyutils.h>
#include <yamlutils.h>


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

/*	node_free_all()
 */
static void __attribute__((destructor)) node_free_all()
{
	JS_LOOP(&node_JS,
		node_free(val);
		);
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

	js_insert(&node_JS, ret->name, ret, true);
	return ret;
die:
	node_free(ret);
	return NULL;
}


/*	node_get()
 */
struct node *node_get(const char *name)
{
	return js_get(&node_JS, name);
}


/*	node_parse()
 */
int node_parse (enum parse_mode mode,
		yaml_document_t *doc, yaml_node_t *mapping,
		yaml_document_t *outdoc, int outlist)
{
	/* TODO: implement */
	return 0;
}

/*	node_emit()
 * Emit a node as a mapping under 'outlist' in 'outdoc'.
 */
int node_emit(struct node *node, yaml_document_t *outdoc, int outlist)
{
	int err_cnt = 0;
	int reply = yaml_document_add_mapping(outdoc, NULL, YAML_BLOCK_MAPPING_STYLE);

	int matches = yaml_document_add_sequence(outdoc, NULL, YAML_BLOCK_SEQUENCE_STYLE);
	JL_LOOP(&node->matches_JQ,
		NB_die_if(
			fval_emit(val, outdoc, matches)
			, "fail to emit match");
	       );
	int mangles = yaml_document_add_sequence(outdoc, NULL, YAML_BLOCK_SEQUENCE_STYLE);
	JL_LOOP(&node->mangles_JQ,
		NB_die_if(
			fval_emit(val, outdoc, mangles)
			, "fail to emit mangle");
	       );

	NB_die_if(
		y_insert_pair(outdoc, reply, "node", node->name)
		|| y_insert_pair_obj(outdoc, reply, "match", matches)
		|| y_insert_pair_obj(outdoc, reply, "mangle", mangles)
		, "");
	NB_die_if(!(
		yaml_document_append_sequence_item(outdoc, outlist, reply)
		), "");
die:
	return err_cnt;
}
