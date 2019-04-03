/*	rule.c
 * (c) 2018 Sirio Balmelli
 */

#include <rule.h>
#include <ndebug.h>
#include <judyutils.h>
#include <yamlutils.h>
#include <fnv.h>
#include <nstring.h>
#include <refcnt.h>


static Pvoid_t	rule_JS = NULL; /* (char *rule_name) -> (struct rule *rule) */

/*	rule_release_refs()
 * All references that a rule takes should be freed here.
 * This is a separate function because it may, in dire situations,
 * be called by rule_new().
 */
static void rule_release_refs(Pvoid_t writes_JQ,
				Pvoid_t copies_JQ,
				Pvoid_t stores_JQ,
				Pvoid_t matches_JQ)
{
	/* free in reverse order, for reference reasons */
	JL_LOOP(&writes_JQ,	fval_free(val);		);
	JL_LOOP(&copies_JQ,	fref_free(val);		);
	JL_LOOP(&stores_JQ,	fref_free(val);		);
	JL_LOOP(&matches_JQ,	fval_free(val);		);

	int __attribute__((unused)) rc;
	JLFA(rc, writes_JQ);
	JLFA(rc, copies_JQ);
	JLFA(rc, stores_JQ);
	JLFA(rc, matches_JQ);
}

/*	rule_free()
 */
void rule_free(void *arg)
{
	if (!arg)
		return;
	struct rule *rule = arg;
	NB_wrn("erase rule %s", rule->name);
	NB_die_if(rule->refcnt,
		"rule '%s' free with non-zero recfnt == leak", rule->name);

	/* we may be a dup: only remove from rule_JS if it points to us */
	if (js_get(&rule_JS, rule->name) == rule)
		js_delete(&rule_JS, rule->name);

	rule_release_refs(rule->writes_JQ, rule->copies_JQ, rule->stores_JQ, rule->matches_JQ);

	free(rule->name);
	free(rule);
die:
	return;
}

/*	rule_free_all()
 */
void __attribute__((destructor(101))) rule_free_all()
{
	JS_LOOP(&rule_JS,
		rule_free(val);
	);
}

/*	rule_new()
 * Create a new rule.
 */
struct rule *rule_new(const char *name,
			Pvoid_t matches_JQ, Pvoid_t stores_JQ,
			Pvoid_t copies_JQ, Pvoid_t writes_JQ)
{
	/* Create an object _first_ so that later failures can be passed
	 * to _free() which will do the right thing (tm).
	 * The corner case the calloc() failure, where we explicitly
	 * release references (which is otherwise done by rule_free().
	 */
	struct rule *ret = NULL;
	if(!(ret = calloc(1, sizeof(*ret)))) {
		rule_release_refs(writes_JQ, copies_JQ, stores_JQ, matches_JQ);
		NB_die("fail alloc size %zu", sizeof(*ret));
	}
	ret->matches_JQ = matches_JQ;
	ret->stores_JQ = stores_JQ;
	ret->copies_JQ = copies_JQ;
	ret->writes_JQ = writes_JQ;

	NB_die_if(!name, "no name given for rule");
	errno = 0;
	NB_die_if(!(
		ret->name = nstralloc(name, MAXLINELEN, NULL)
		), "string alloc fail");
	NB_die_if(errno == E2BIG, "value truncated:\n%s", ret->name);

#ifdef XDPACKET_DISALLOW_CLOBBER
	NB_die_if(js_insert(&rule_JS, ret->name, ret, false)
		, "rule '%s' already exists", name);
#else
	if ((ret = js_get(&rule_JS, name))) {
		NB_wrn("rule '%s' already exists: deleting", name);
		rule_free(ret);
		js_insert(&rule_JS, ret->name, ret, true);
	}
#endif

	NB_inf("%s", ret->name);
	return ret;

die:
	rule_free(ret);
	return NULL;
}


/*	rule_release()
 */
void rule_release(struct rule *rule)
{
	if (!rule)
		return;
	refcnt_release(rule);
}

/*	rule_get()
 * Increments refcount, don't call from inside this file.
 */
struct rule *rule_get(const char *name)
{
	struct rule *ret = js_get(&rule_JS, name);
	if (ret)
		refcnt_take(ret);
	return ret;
}


/*	rule_parse()
 */
int rule_parse (enum parse_mode mode,
		yaml_document_t *doc, yaml_node_t *mapping,
		yaml_document_t *outdoc, int outlist)
{
	int err_cnt = 0;
	struct rule *rule = NULL;

	const char *name = NULL;
	Pvoid_t matches_JQ = NULL;
	Pvoid_t stores_JQ = NULL;
	Pvoid_t copies_JQ = NULL;
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

			if (!strcmp("rule", keyname) || !strcmp("r", keyname))
				name = valtxt;
			else
				NB_err("'rule' does not implement '%s'", keyname);

		} else if (val->type == YAML_SEQUENCE_NODE) {
			if (!strcmp("match", keyname) || !strcmp("m", keyname)) {
				Y_SEQ_MAP_PAIRS_EXEC(doc, val,
					/* rely on enqueue() to test 'fv' (a NULL datum is invalid) */
					NB_err_if(
						jl_enqueue(&matches_JQ, fval_new(keyname, valtxt))
						, "");
					);
			} else if (!strcmp("store", keyname) || !strcmp("s", keyname)) {
				Y_SEQ_MAP_PAIRS_EXEC(doc, val,
					NB_err_if(
						jl_enqueue(&stores_JQ, fref_new(keyname, valtxt, FREF_STORE))
						, "");
					);
			} else if (!strcmp("copy", keyname) || !strcmp("c", keyname)) {
				Y_SEQ_MAP_PAIRS_EXEC(doc, val,
					NB_err_if(
						jl_enqueue(&copies_JQ, fref_new(keyname, valtxt, FREF_COPY))
						, "");
					);
			} else if (!strcmp("write", keyname) || !strcmp("w", keyname)) {
				Y_SEQ_MAP_PAIRS_EXEC(doc, val,
					NB_err_if(
						jl_enqueue(&writes_JQ, fval_new(keyname, valtxt))
						, "");
					);
			} else {
				NB_err("'rule' does not implement '%s'", keyname);
			}

		} else {
			NB_die("'%s' in rule not a scalar or sequence", keyname);
		}
	}

	/* process based on 'mode' */
	switch (mode) {
	case PARSE_ADD:
	{
		NB_die_if(!(
			rule = rule_new(name, matches_JQ, stores_JQ, copies_JQ, writes_JQ)
			), "could not create new rule '%s'", name);
		NB_die_if(rule_emit(rule, outdoc, outlist), "");
		break;
	}

	case PARSE_DEL:
		NB_die_if(!(
			rule = js_get(&rule_JS, name)
			), "could not get rule '%s'", name);
		NB_die_if(rule->refcnt, "rule '%s' still in use", name);
		NB_die_if(
			rule_emit(rule, outdoc, outlist)
			, "");
		rule_free(rule);
		break;

	case PARSE_PRN:
		/* if nothing is given, print all */
		if (!strcmp("", name)) {
			JS_LOOP(&rule_JS,
				NB_die_if(
					rule_emit(val, outdoc, outlist)
					, "");
			);
		/* otherwise, search for a literal match */
		} else if ((rule = js_get(&rule_JS, name))) {
			NB_die_if(rule_emit(rule, outdoc, outlist), "");
		}
		break;

	default:
		NB_err("unknown mode %s", parse_mode_prn(mode));
	};

die:
	return err_cnt;
}


/*	rule_emit()
 * Emit a rule as a mapping under 'outlist' in 'outdoc'.
 */
int rule_emit(struct rule *rule, yaml_document_t *outdoc, int outlist)
{
	int err_cnt = 0;
	int reply = yaml_document_add_mapping(outdoc, NULL, YAML_BLOCK_MAPPING_STYLE);

	int matches = yaml_document_add_sequence(outdoc, NULL, YAML_BLOCK_SEQUENCE_STYLE);
	JL_LOOP(&rule->matches_JQ,
		NB_die_if(
			fval_emit(val, outdoc, matches)
			, "fail to emit match");
	);
	int stores = yaml_document_add_sequence(outdoc, NULL, YAML_BLOCK_SEQUENCE_STYLE);
	JL_LOOP(&rule->stores_JQ,
		NB_die_if(
			fref_emit(val, outdoc, stores)
			, "fail to emit match");
	);
	int copies = yaml_document_add_sequence(outdoc, NULL, YAML_BLOCK_SEQUENCE_STYLE);
	JL_LOOP(&rule->copies_JQ,
		NB_die_if(
			fref_emit(val, outdoc, copies)
			, "fail to emit match");
	);
	int writes = yaml_document_add_sequence(outdoc, NULL, YAML_BLOCK_SEQUENCE_STYLE);
	JL_LOOP(&rule->writes_JQ,
		NB_die_if(
			fval_emit(val, outdoc, writes)
			, "fail to emit write");
	);

	NB_die_if(
		y_pair_insert(outdoc, reply, "rule", rule->name)
		|| y_pair_insert_obj(outdoc, reply, "match", matches)
		|| y_pair_insert_obj(outdoc, reply, "store", stores)
		|| y_pair_insert_obj(outdoc, reply, "copy", copies)
		|| y_pair_insert_obj(outdoc, reply, "write", writes)
		, "");
	NB_die_if(!(
		yaml_document_append_sequence_item(outdoc, outlist, reply)
		), "");
die:
	return err_cnt;
}
