/*	rout.c
 *
 * (c) 2018 Sirio Balmelli
 */

#include <rout.h>
#include <yamlutils.h>
#include <judyutils.h>


/*	rout_free()
 */
void rout_free(void *arg)
{
	free(arg);
}

/*	rout_new()
 */
struct rout *rout_new(const char *rule_name, const char *out_name)
{
	struct rout *ret = NULL;
	NB_die_if(!rule_name || !out_name, "rout requires 'rule_name' and 'out_name'");

	struct rule *rule = NULL;
	NB_die_if(!(
		rule = rule_get(rule_name)
		), "could not get rule '%s'", rule_name);

	struct iface *output = NULL;
	NB_die_if(!(
		output = iface_get(out_name)
		), "could not get iface '%s'", out_name);

	NB_die_if(!(
		ret = malloc(sizeof(*ret))
		), "fail alloc size %zu", sizeof(*ret));

	ret->rule = rule;
	ret->output = output;

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
		, "");
	NB_die_if(!(
		yaml_document_append_sequence_item(outdoc, outlist, reply)
		), "");
die:
	return err_cnt;
}
