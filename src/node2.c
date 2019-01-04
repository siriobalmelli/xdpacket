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
	JL_LOOP(&ne->writes_JQ,
		fval_free(val);
	       );
	int rc;
	JLFA(rc, ne->matches_JQ);
	JLFA(rc, ne->writes_JQ);

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
struct node *node_new(const char *name, Pvoid_t matches_JQ, Pvoid_t writes_JQ)
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
	ret->writes_JQ = writes_JQ;

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
	int err_cnt = 0;
	struct node *node = NULL;

	const char *name = NULL;
	Pvoid_t matches_JQ = NULL;
	Pvoid_t writes_JQ = NULL;

	/* parse mapping */
	for (yaml_node_pair_t *pair = mapping->data.mapping.pairs.start;
		pair < mapping->data.mapping.pairs.top;
		pair++)
	{
		/* loop boilerplate */
		yaml_node_t *key = yaml_document_get_node(doc, pair->key);
		const char *keyname = (const char *)key->data.scalar.value;
		yaml_node_t *val = yaml_document_get_node(doc, pair->value);

		if (val->type == YAML_SCALAR_NODE) {
			const char *valtxt = (const char *)val->data.scalar.value;

			if (!strcmp("node", keyname) || !strcmp("n", keyname))
				name = valtxt;
			else
				NB_err("'node' does not implement '%s'", keyname);

		} else if (val->type == YAML_SEQUENCE_NODE) {
			if (!strcmp("match", keyname) || !strcmp("m", keyname)) {
				Y_SEQ_MAP_PAIRS_EXEC(doc, val,
					/* rely on enqueue() to test 'fv' (a NULL datum is invalid) */
					NB_err_if(
						jl_enqueue(&matches_JQ, fval_new(keyname, valtxt))
						, "");
					);

			} else if (!strcmp("write", keyname) || !strcmp("w", keyname)) {
				Y_SEQ_MAP_PAIRS_EXEC(doc, val,
					/* rely on enqueue() to test 'fv' (a NULL datum is invalid) */
					NB_err_if(
						jl_enqueue(&writes_JQ, fval_new(keyname, valtxt))
						, "");
					);

			} else {
				NB_err("'node' does not implement '%s'", keyname);
			}

		} else {
			NB_die("'%s' in node not a scalar or sequence", keyname);
		}
	}

	/* process based on 'mode' */
	switch (mode) {
	case PARSE_ADD:
	{
		NB_die_if(!(
			node = node_new(name, matches_JQ, writes_JQ)
			), "could not create new node '%s'", name);
		NB_die_if(node_emit(node, outdoc, outlist), "");
		break;
	}

	case PARSE_DEL:
		NB_die_if(!(
			node = node_get(name)
			), "could not get node '%s'", name);
		NB_die_if(
			node_emit(node, outdoc, outlist)
			, "");
		node_free(node);
		break;

	case PARSE_PRN:
		/* if nothing is given, print all */
		if (!strcmp("", name)) {
			JS_LOOP(&node_JS,
				NB_die_if(
					node_emit(val, outdoc, outlist)
					, "");
				);
		/* otherwise, search for a literal match */
		} else if ((node = node_get(name))) {
			NB_die_if(node_emit(node, outdoc, outlist), "");
		}
		break;

	default:
		NB_err("unknown mode %s", parse_mode_prn(mode));
	};

die:
	return err_cnt;
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
	int writes = yaml_document_add_sequence(outdoc, NULL, YAML_BLOCK_SEQUENCE_STYLE);
	JL_LOOP(&node->writes_JQ,
		NB_die_if(
			fval_emit(val, outdoc, writes)
			, "fail to emit write");
	       );

	NB_die_if(
		y_pair_insert(outdoc, reply, "node", node->name)
		|| y_pair_insert_obj(outdoc, reply, "match", matches)
		|| y_pair_insert_obj(outdoc, reply, "write", writes)
		, "");
	NB_die_if(!(
		yaml_document_append_sequence_item(outdoc, outlist, reply)
		), "");
die:
	return err_cnt;
}
