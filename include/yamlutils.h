#ifndef yamlutils_h_
#define yamlutils_h_

/*	yamlutils.h
 *
 * Informal set of utilities for working with libyaml
 * (c) 2018 Sirio Balmelli
 */

#include <xdpacket.h>
#include <yaml.h>
#include <nonlibc.h>
#include <ndebug.h>
#include <stdarg.h> /* varargs to emulate printf() constructs */

/*	y_insert_pair()
 * Insert 'key' and 'val' as a pair into 'mapping'.
 * Return 0 on success.
 */
NLC_INLINE int y_insert_pair(yaml_document_t *doc, int mapping,
			const char *key, const char *val)
{
	int err_cnt = 0;
	int key_idx, val_idx;
	NB_die_if(!(
		key_idx = yaml_document_add_scalar(doc, NULL,
			(yaml_char_t *)key, -1,
			YAML_PLAIN_SCALAR_STYLE)
		), "cannot insert scalar");
	NB_die_if(!(
		val_idx = yaml_document_add_scalar(doc, NULL,
			(yaml_char_t *)val, -1,
			YAML_PLAIN_SCALAR_STYLE)
		), "cannot insert scalar");
	NB_die_if(!(
		yaml_document_append_mapping_pair(doc, mapping, key_idx, val_idx)
		), "cannot append to mapping:\n%s: %s", key, val);
die:
	return err_cnt;
}

/*	y_insert_pair_nf()
 * Do printf formatting on variadic 'val' and then call y_insert_pair().
 */
int y_insert_pair_nf(yaml_document_t *doc, int mapping,
			const char *key, const char *val, ...);

/*	y_insert_pair_obj()
 * Same as above, except that 'val' is the index of an object
 * already present in 'doc'.
 */
NLC_INLINE int y_insert_pair_obj(yaml_document_t *doc, int mapping,
			const char *key, int val_idx)
{
	int err_cnt = 0;
	int key_idx;
	NB_die_if(!(
		key_idx = yaml_document_add_scalar(doc, NULL,
			(yaml_char_t *)key, -1,
			YAML_PLAIN_SCALAR_STYLE)
		), "cannot insert scalar");
	NB_die_if(!(
		yaml_document_append_mapping_pair(doc, mapping, key_idx, val_idx)
		), "cannot append to mapping:\n%s: [object]", key);
die:
	return err_cnt;
}


/*	y_map_count()
 */
NLC_INLINE
int y_map_count(const yaml_node_t *node)
{
	int cnt = 0;
	NB_die_if(node->type != YAML_MAPPING_NODE, "Not a mapping node");
	yaml_node_pair_t *i_node_p;
	for (i_node_p = node->data.mapping.pairs.start; i_node_p < node->data.mapping.pairs.top; i_node_p++)
		cnt++;
die:
	return cnt;
}

#endif /* yamlutils_h_ */
