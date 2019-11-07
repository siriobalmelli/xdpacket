/*	rout.c
 * (c) 2018 Sirio Balmelli
 */

#include <rout.h>
#include <yamlutils.h>
#include <operations.h>


/*	rout_set_free()
 */
void rout_set_free (void *arg)
{
	if (!arg)
		return;
	free(arg);
}


/*	rout_set_new()
 * @writes_JQ	: (uint64_t seq) -> (struct fval *wrt)
 */
struct rout_set *rout_set_new(struct rule *rule, struct iface *output)
{
	struct rout_set *ret = NULL;

	NB_die_if(!(
		ret = calloc(1, sizeof(*ret))
		), "fail malloc size %zu", sizeof(*ret));
	ret->if_out = output;
	ret->match_JQ = rule->match_JQ;
	ret->write_JQ = rule->write_JQ;

	return ret;
die:
	rout_set_free(ret);
	return NULL;
}


/*	rule_set_match()
 * Attempt to match 'pkt' of 'plen' Bytes against all matches in 'set'.
 * Return 'true' if matching (and increment matches counter),
 * otherwise return 'false'.
 */
bool __attribute__((hot)) rout_set_match(struct rout_set *set, const void *pkt, size_t plen)
{
	JL_LOOP(&set->match_JQ,
		if (op_match(val, pkt, plen))
			return false;
	);

	set->count_match++;
	return true;
}


/*	rout_set_exec()
 * Execute all writes on 'pkt'.
 * Returns true on success;
 * on failure returns false and '*pkt' will be in an inconsistent state.
 */
bool rout_set_exec(struct rout_set *rst, void *pkt, size_t plen)
{
	JL_LOOP(&rst->write_JQ,
		if (op_write(val, pkt, plen))
			return false;
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
