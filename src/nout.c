/*	nout.c
 *
 * (c) 2018 Sirio Balmelli
 */

#include <nout.h>
#include <yamlutils.h>
#include <judyutils.h>


/*	nout_free()
 */
void nout_free(void *arg)
{
	free(arg);
}

/*	nout_new()
 */
struct nout *nout_new(const char *node_name, const char *out_if_name)
{
	struct nout *ret = NULL;
	NB_die_if(!node_name || !out_if_name, "nout requires 'node_name' and 'out_if_name'");

	struct node *node = NULL;
	NB_die_if(!(
		node = node_get(node_name)
		), "could not get node '%s'", node_name);

	struct iface *out_if = NULL;
	NB_die_if(!(
		out_if = iface_get(out_if_name)
		), "could not get iface '%s'", out_if_name);

	NB_die_if(!(
		ret = malloc(sizeof(*ret))
		), "fail alloc size %zu", sizeof(*ret));

	ret->node = node;
	ret->out_if = out_if;

	return ret;
die:
	nout_free(ret);
	return NULL;
}


/*	nout_emit()
 * Emit an interface as a mapping under 'outlist' in 'outdoc'.
 */
int nout_emit(struct nout *nout, yaml_document_t *outdoc, int outlist)
{
	int err_cnt = 0;
	int reply = yaml_document_add_mapping(outdoc, NULL, YAML_BLOCK_MAPPING_STYLE);
	NB_die_if(
		y_pair_insert(outdoc, reply, nout->node->name, nout->out_if->name)
		, "");
	NB_die_if(!(
		yaml_document_append_sequence_item(outdoc, outlist, reply)
		), "");
die:
	return err_cnt;
}
