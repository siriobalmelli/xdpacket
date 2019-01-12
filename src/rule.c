/*	rule.c
 * (c) 2018 Sirio Balmelli
 */

#include <rule.h>
#include <ndebug.h>
#include <judyutils.h>
#include <yamlutils.h>
#include <fnv.h>


static Pvoid_t	rule_JS = NULL; /* (char *rule_name) -> (struct rule *rule) */


/*	rule_set_free()
 */
void rule_set_free (void *arg)
{
	free(arg);
}

/*	rule_set_new()
 * Compile a JQ (queue) of matches into a packed format for handling packets.
 * @matches_JQ		: (uint64_t seq) -> (struct fval *mch)
 */
struct rule_set *rule_set_new(Pvoid_t matches_JQ)
{
	struct rule_set *ret = NULL;
	size_t match_cnt = jl_count(&matches_JQ);
	size_t alloc_size = sizeof(*ret) + (sizeof(struct field_set) * match_cnt);

	NB_die_if(!(
		ret = malloc(alloc_size)
		), "fail malloc size %zu", alloc_size);
	ret->match_cnt = match_cnt;
	ret->hash = fnv_hash64(NULL, NULL, 0); /* seed with initializer */

	/* Get only the field 'set' (necessary for matching against packets);
	 * accumulate the hash of all fields.
	 */
	JL_LOOP(&matches_JQ,
		struct fval *current = val;
		ret->matches[i] = current->field->set;
		NB_die_if(fval_bytes_hash(current->bytes, &ret->hash), "");
		);

	return ret;
die:
	rule_set_free(ret);
	return NULL;
}


/*	rule_set_match()
 * Attempt to match 'pkt' of 'plen' Bytes against all rules in 'set'.
 * Return 'true' if matching, else 'false'.
 */
bool  __attribute__((hot)) rule_set_match(struct rule_set *set, void *pkt, size_t plen)
{
	uint64_t hash = fnv_hash64(NULL, NULL, 0);
	for (unsigned int i=0; i < set->match_cnt; i++) {
		/* a failing hash indicates packet too short, etc */
		if (field_hash(set->matches[i], pkt, plen, &hash))
			return false;
	}

	if (hash != set->hash)
		return false;

	return true;
}


/*	rule_free()
 */
void rule_free(void *arg)
{
	if (!arg)
		return;
	struct rule *ne = arg;

	js_delete(&rule_JS, ne->name);

	rule_set_free(ne->set);

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

/*	rule_free_all()
 */
static void __attribute__((destructor)) rule_free_all()
{
	JS_LOOP(&rule_JS,
		rule_free(val);
		);
}


/*	rule_new()
 * Create a new rule.
 */
struct rule *rule_new(const char *name, Pvoid_t matches_JQ, Pvoid_t writes_JQ)
{
	struct rule *ret = NULL;
	NB_die_if(!name, "no name given for rule");

	/* no easy way of knowing if dups are identical, kill them */
	if ((ret = js_get(&rule_JS, name))) {
		NB_wrn("rule '%s' already exists: deleting", name);
		rule_free(ret);
	}

	NB_die_if(!(
		ret = calloc(1, sizeof(*ret))
		), "fail alloc size %zu", sizeof(*ret));
	NB_die_if((
		snprintf(ret->name, sizeof(ret->name), "%s", name)
		) >= sizeof(ret->name), "rule name overflow '%s'", name);

	ret->matches_JQ = matches_JQ;
	ret->writes_JQ = writes_JQ;

	NB_die_if(!(
		ret->set = rule_set_new(ret->matches_JQ)
		), "");

	js_insert(&rule_JS, ret->name, ret, true);
	return ret;
die:
	rule_free(ret);
	return NULL;
}


/*	rule_get()
 */
struct rule *rule_get(const char *name)
{
	return js_get(&rule_JS, name);
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

			if (!strcmp("rule", keyname) || !strcmp("n", keyname))
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

			} else if (!strcmp("write", keyname) || !strcmp("w", keyname)) {
				Y_SEQ_MAP_PAIRS_EXEC(doc, val,
					/* rely on enqueue() to test 'fv' (a NULL datum is invalid) */
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
			rule = rule_new(name, matches_JQ, writes_JQ)
			), "could not create new rule '%s'", name);
		NB_die_if(rule_emit(rule, outdoc, outlist), "");
		break;
	}

	case PARSE_DEL:
		NB_die_if(!(
			rule = rule_get(name)
			), "could not get rule '%s'", name);
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
		} else if ((rule = rule_get(name))) {
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
	int writes = yaml_document_add_sequence(outdoc, NULL, YAML_BLOCK_SEQUENCE_STYLE);
	JL_LOOP(&rule->writes_JQ,
		NB_die_if(
			fval_emit(val, outdoc, writes)
			, "fail to emit write");
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
