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

/*	y_pair_insert()
 * Insert 'key' and 'val' as a pair into 'mapping'.
 * Return 0 on success.
 */
NLC_INLINE int y_pair_insert(yaml_document_t *doc, int mapping,
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

/*	y_pair_insert_nf()
 * Do printf formatting on variadic 'val' and then call y_pair_insert().
 */
int y_pair_insert_nf(yaml_document_t *doc, int mapping,
			const char *key, const char *val, ...);

/*	y_pair_insert_obj()
 * Same as above, except that 'val' is the index of an object
 * already present in 'doc'.
 */
NLC_INLINE int y_pair_insert_obj(yaml_document_t *doc, int mapping,
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


/*	Y_SEQ_MAP_PAIRS_EXEC()
 * Execute 'statements' on every pair of every mapping in the sequence
 * pointed to by 'seq_ptr'.
 *
 * The following variables are available to 'statements':
 * - (char *)keyname
 * - (char *)valtxt
 */
#define Y_SEQ_MAP_PAIRS_EXEC(doc_ptr, seq_ptr, statements)				\
	/* process children list objects */						\
	for (yaml_node_item_t *child = seq_ptr->data.sequence.items.start;		\
		child < seq_ptr->data.sequence.items.top;				\
		child++)								\
	{										\
		yaml_node_t *mapping = yaml_document_get_node(doc_ptr, *child);		\
		if (mapping->type != YAML_MAPPING_NODE) {				\
			NB_err("node not a map");					\
			continue;							\
		}									\
											\
		for (yaml_node_pair_t *pair = mapping->data.mapping.pairs.start;	\
			pair < mapping->data.mapping.pairs.top;				\
			pair++)								\
		{									\
			/* loop boilerplate */						\
			yaml_node_t *key = yaml_document_get_node(doc_ptr, pair->key);	\
			const char *keyname = (const char *)key->data.scalar.value;	\
											\
			yaml_node_t *val = yaml_document_get_node(doc_ptr, pair->value);\
			if (val->type != YAML_SCALAR_NODE) {				\
				NB_err("'%s' in field not a scalar", keyname);		\
				continue;						\
			}								\
			const char *valtxt = (const char *)val->data.scalar.value;	\
											\
			/* user-supplied statements here */				\
			statements;							\
		}									\
	}



/*	y_map_count()
 */
NLC_INLINE int y_map_count(const yaml_node_t *node)
{
	int cnt = 0;
	NB_die_if(node->type != YAML_MAPPING_NODE, "Not a mapping node");
	yaml_node_pair_t *i_node_p;
	for (i_node_p = node->data.mapping.pairs.start; i_node_p < node->data.mapping.pairs.top; i_node_p++)
		cnt++;
die:
	return cnt;
}


/*	y_seq_empty()
 * Return true if 'seq' is, in fact, an empty list.
 */
NLC_INLINE bool y_seq_empty(yaml_node_t *seq)
{
	return (seq->data.sequence.items.start == seq->data.sequence.items.top);
}


#endif /* yamlutils_h_ */
