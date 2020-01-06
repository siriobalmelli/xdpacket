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
#include <operations.h>


static Pvoid_t	rule_JS = NULL; /* (char *rule_name) -> (struct rule *rule) */

/*	rule_release_refs()
 * All references that a rule takes should be freed here.
 * This is a separate function because it may, in dire situations,
 * be called by rule_new().
 */
static void rule_release_refs(Pvoid_t match_JQ, Pvoid_t write_JQ)
{
	/* free in reverse order, for reference reasons */
	JL_LOOP(&match_JQ,	op_free(val);		);
	JL_LOOP(&write_JQ,	op_free(val);		);

	int __attribute__((unused)) rc;
	JLFA(rc, match_JQ);
	JLFA(rc, write_JQ);
}

/*	rule_free()
 */
void rule_free(void *arg)
{
	if (!arg)
		return;
	struct rule *rule = arg;
	NB_wrn_if(rule->name != NULL,
		"erase rule %s", rule->name);
	NB_die_if(rule->refcnt,
		"rule '%s' free with non-zero recfnt == leak", rule->name);

	/* we may be a dup: only remove from rule_JS if it points to us */
	if (js_get(&rule_JS, rule->name) == rule)
		js_delete(&rule_JS, rule->name);

	rule_release_refs(rule->match_JQ, rule->write_JQ);

	free(rule->name);
	free(rule);
die:
	return;
}

/*	rule_free_all()
 */
void __attribute__((destructor(104))) rule_free_all()
{
	JS_LOOP(&rule_JS,
		rule_free(val);
	);
}

/*	rule_new()
 * Create a new rule.
 */
struct rule *rule_new(const char *name, Pvoid_t match_JQ, Pvoid_t write_JQ)
{
	/* Create an object _first_ so that later failures can be passed
	 * to _free() which will do the right thing (tm).
	 * The corner case the calloc() failure, where we explicitly
	 * release references (which is otherwise done by rule_free().
	 */
	struct rule *ret = NULL;
	if(!(ret = calloc(1, sizeof(*ret)))) {
		rule_release_refs(match_JQ, write_JQ);
		NB_die("fail alloc size %zu", sizeof(*ret));
	}
	ret->match_JQ = match_JQ;
	ret->write_JQ = write_JQ;

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

	const char *name = "";
	Pvoid_t match_JQ = NULL;
	Pvoid_t write_JQ = NULL;

	/* {
	 *   rule: rule_name		(scalar)
	 *   match:			(sequence)
	 *     - { } # op		(mapping)
	 *   write:			(sequence)
	 *     - { } # op		(mapping)
	 * } # rule			(mapping)
	 */
	Y_FOR_MAP(doc, mapping,
		if (type == YAML_SCALAR_NODE) {
			if (!strcmp("rule", keyname) || !strcmp("r", keyname))
				name = txt;
			else
				NB_err("'rule' does not implement '%s'", keyname);

		} else if (type == YAML_SEQUENCE_NODE) {
			if (mode != PARSE_ADD) {
				NB_err("'rule' does not support parsing rule operations when not adding");
				continue;
			}

			if (!strcmp("match", keyname) || !strcmp("m", keyname)) {
				Y_FOR_SEQ(doc, seq,
					if (type != YAML_MAPPING_NODE) {
						NB_err("'match' item not a mapping");
						continue;
					}
					/* rely on enqueue() to test 'fv' (a NULL datum is invalid) */
					NB_err_if(
						jl_enqueue(&match_JQ, op_parse_new(doc, map))
						, "");
				);

			} else if (!strcmp("write", keyname) || !strcmp("w", keyname)) {
				Y_FOR_SEQ(doc, seq,
					if (type != YAML_MAPPING_NODE) {
						NB_err("'match' item not a mapping");
						continue;
					}
					NB_err_if(
						jl_enqueue(&write_JQ, op_parse_new(doc, map))
						, "");
				);

			} else {
				NB_err("'rule' does not implement '%s'", keyname);
				continue;
			}

		} else {
			NB_die("'%s' in rule not a scalar or sequence", keyname);
		}
	);

	/* process based on 'mode' */
	switch (mode) {
	case PARSE_ADD:
	{
		NB_die_if(!(
			rule = rule_new(name, match_JQ, write_JQ)
			), "could not create new rule '%s'", name);
		NB_die_if(
			rule_emit(rule, outdoc, outlist)
			, "");
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
			NB_die_if(
				rule_emit_all(outdoc, outlist)
				, "");
		/* otherwise, search for a literal match */
		} else if ((rule = js_get(&rule_JS, name))) {
			NB_die_if(
				rule_emit(rule, outdoc, outlist)
				, "");
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
	JL_LOOP(&rule->match_JQ,
		NB_die_if(
			op_emit(val, outdoc, matches)
			, "fail to emit a match op");
	);
	int writes = yaml_document_add_sequence(outdoc, NULL, YAML_BLOCK_SEQUENCE_STYLE);
	JL_LOOP(&rule->write_JQ,
		NB_die_if(
			op_emit(val, outdoc, writes)
			, "fail to emit a write op");
	);

	NB_die_if(
		y_pair_insert(outdoc, reply, "rule", rule->name)
		|| y_pair_insert_obj(outdoc, reply, "match", matches)
		|| y_pair_insert_obj(outdoc, reply, "write", writes)
		, "");
	NB_die_if(!(
		yaml_document_append_sequence_item(outdoc, outlist, reply)
		), "");
die:
	return err_cnt;
}

/*	rule_emit_all()
 */
int rule_emit_all (yaml_document_t *outdoc, int outlist)
{
	int err_cnt = 0;

	JS_LOOP(&rule_JS,
		NB_die_if(
			rule_emit(val, outdoc, outlist)
			, "");
	);

die:
	return err_cnt;
}
