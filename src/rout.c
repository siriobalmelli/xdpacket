/*	rout.c
 * (c) 2018 Sirio Balmelli
 */

#include <rout.h>
#include <yamlutils.h>


/*	rout_set_free()
 */
void rout_set_free (void *arg)
{
	if (!arg)
		return;
	struct rout_set *rts = arg;
	int __attribute__((unused)) rc;
	JLFA(rc, rts->actions_JQ);
	free(rts);
}


/*	rout_set_new()
 * @writes_JQ	: (uint64_t seq) -> (struct fval *wrt)
 */
struct rout_set *rout_set_new(struct rule *rule, struct iface *output)
{
	struct rout_set *ret = NULL;
	size_t match_cnt = jl_count(&rule->matches_JQ);
	size_t alloc_size = sizeof(*ret) + (sizeof(struct field_set) * match_cnt);

	NB_die_if(!(
		ret = calloc(1, alloc_size)
		), "fail malloc size %zu", alloc_size);
	ret->if_out = output;

	/* follow standard sequence: store, copy, write */
	JL_LOOP(&rule->stores_JQ,
		struct fref *fref = val;
		jl_enqueue(&ret->actions_JQ, fref->ref);
	);
	JL_LOOP(&rule->copies_JQ,
		struct fref *fref = val;
		jl_enqueue(&ret->actions_JQ, fref->ref);
	);
	JL_LOOP(&rule->writes_JQ,
		struct fval *write = val;
		jl_enqueue(&ret->actions_JQ, write->set);
	);

	ret->count_match = 0;
	ret->hash = fnv_hash64(NULL, NULL, 0); /* seed with initializer */
	ret->match_cnt = match_cnt;

	/* Get only the field 'set' (necessary for matching against packets);
	 * accumulate the hash of all fields.
	 */
	JL_LOOP(&rule->matches_JQ,
		struct fval *current = val;
		ret->matches[i] = current->field->set;
		NB_die_if(
			fval_set_hash(current->set, &ret->hash)
			, "");
	);

	return ret;
die:
	rout_set_free(ret);
	return NULL;
}


/*	rule_set_match()
 * Attempt to match 'pkt' of 'plen' Bytes against all rules in 'set'.
 * Return 'true' if matching (and increment matches counter),
 * otherwise return 'false'.
 */
bool  __attribute__((hot)) rout_set_match(struct rout_set *set, const void *pkt, size_t plen)
{
	uint64_t hash = fnv_hash64(NULL, NULL, 0);
	for (unsigned int i=0; i < set->match_cnt; i++) {
		/* a failing hash indicates packet too short, etc */
		if (field_hash(set->matches[i], pkt, plen, &hash))
			return false;
	}

	if (hash != set->hash)
		return false;

	/* TODO: STATE: walk through set of global state matches to perform,
	 * bail at the first sign that one is missing.
	 */

	set->count_match++;
	return true;
}


/*	rout_set_exec()
 * Execute all actions on 'pkt':
 * - stores
 * - copies
 * - writes
 * Returns true on success;
 * on failure returns false and '*pkt' will be in an inconsistent state.
 */
bool rout_set_exec(struct rout_set *rst, void *pkt, size_t plen)
{
	JL_LOOP(&rst->actions_JQ,
		struct fref_set *set = val;
		/* follow standard sequence: store, copy, write */
		if (set->where.flags & FIELD_FREF_STORE) {
			if (fref_set_store(set, pkt, plen))
				return false;
		} else if (set->where.flags & FIELD_FREF_COPY) {
			if (fref_set_copy(set, pkt, plen))
				return false;
		} else {
			if (fval_set_write((struct fval_set *)set, pkt, plen))
				return false;
		}
	);
	return true;
}



/*	rout_free()
 */
void rout_free(void *arg)
{
	if (!arg)
		return;
	struct rout *rt = arg;

	/* Counterintuitively, use this to only print info when:
	 * - we are compiled with debug flags
	 * - 'rule' and 'output' are non-NULL
	 */
	NB_wrn_if(rt->rule && rt->output, "%s: %s", rt->rule->name, rt->output->name);

	rule_release(rt->rule);
	iface_release(rt->output);
	rout_set_free(rt->set);
	free(rt);
}

/*	rout_new()
 */
struct rout *rout_new(const char *rule_name, const char *out_name)
{
	struct rout *ret = NULL;
	NB_die_if(!rule_name || !out_name, "rout requires 'rule_name' and 'out_name'");

	NB_die_if(!(
		ret = calloc(1, sizeof(*ret))
		), "fail alloc size %zu", sizeof(*ret));

	NB_die_if(!(
		ret->rule = rule_get(rule_name)
		), "could not get rule '%s'", rule_name);
	NB_die_if(!(
		ret->output = iface_get(out_name)
		), "could not get iface '%s'", out_name);
	NB_die_if(!(
		ret->set = rout_set_new(ret->rule, ret->output)
		), "could not create set for rout");

	NB_inf("%s: %s", ret->rule->name, ret->output->name);
	return ret;
die:
	rout_free(ret);
	return NULL;
}


/*	rout_emit()
 * Emit an interface as a mapping under 'outlist' in 'outdoc'.
 */
int rout_emit(struct rout *rout, yaml_document_t *outdoc, int outlist)
{
	int err_cnt = 0;
	int reply = yaml_document_add_mapping(outdoc, NULL, YAML_BLOCK_MAPPING_STYLE);
	NB_die_if(
		y_pair_insert(outdoc, reply, rout->rule->name, rout->output->name)
		// || y_pair_insert_nf(outdoc, reply, "hash", "0x%"PRIx64, rout->set->hash)
		|| y_pair_insert_nf(outdoc, reply, "matches", "%zu", rout->set->count_match)
		, "");
	NB_die_if(!(
		yaml_document_append_sequence_item(outdoc, outlist, reply)
		), "");
die:
	return err_cnt;
}
