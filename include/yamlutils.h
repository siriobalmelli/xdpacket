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



/* boilerplate typing code used by Y_FOR_X() functions */
#define Y_FOR_TYPE_								\
		yaml_node_type_t type = val_->type;				\
		/* assign proper value variable based on type */		\
		const char *txt = NULL; yaml_node_t *map = NULL, *seq = NULL;	\
		if (type == YAML_SCALAR_NODE) {					\
			txt = (const char *)val_->data.scalar.value;		\
		} else if (type == YAML_MAPPING_NODE) {				\
			map = val_;						\
		} else if (type == YAML_SEQUENCE_NODE) {			\
			seq = val_;						\
		} else {							\
			NB_err("'%s' is not of a known type", keyname);		\
			continue;						\
		}


/*	Y_FOR_MAP()
 * Iterate through every key-value pair of the mapping 'map', execute 'statements' on it.
 *
 * The following variables are available to 'statements':
 * - (const char *)keyname
 * - (yaml_node_type_t)type
 * - (const char *)txt		// optional: if value is a scalar
 * - (yaml_node_t *)map		// optional: if value is a mapping
 * - (yaml_node_t *)seq		// optional: if value is a sequence
 */
#define Y_FOR_MAP(doc, map, statements)							\
	if (doc && map) {								\
		for (yaml_node_pair_t *pair = map->data.mapping.pairs.start;		\
			pair < map->data.mapping.pairs.top;				\
			pair++)								\
		{									\
			yaml_node_t *key = yaml_document_get_node(doc, pair->key);	\
			const char *keyname = (const char *)key->data.scalar.value;	\
											\
			yaml_node_t *val_ = yaml_document_get_node(doc, pair->value);	\
			Y_FOR_TYPE_							\
											\
			/* user-supplied statements here */				\
			statements;							\
		}									\
	} else {									\
		NB_err("%s%s", doc ? "" : "doc == NULL ", map ? "" : "map == NULL");	\
	}

/*	Y_FOR_SEQ()
 * Iterate through every item/value in the sequence 'seq', execute 'statements' on it.
 *
 * The following variables are available to 'statements':
 * - (yaml_node_type_t)type
 * - (const char *)txt		// optional: if item is a scalar
 * - (yaml_node_t *)map		// optional: if item is a mapping
 * - (yaml_node_t *)seq		// optional: if item is a sequence
 */
#define Y_FOR_SEQ(doc, seq, statements)							\
	if (doc && seq) {								\
		for (yaml_node_item_t *child = seq->data.sequence.items.start;		\
			child < seq->data.sequence.items.top;				\
			child++)							\
		{									\
			yaml_node_t *val_ = yaml_document_get_node(doc, *child);	\
			Y_FOR_TYPE_							\
											\
			/* user-supplied statements here */				\
			statements;							\
		}									\
	} else {									\
		NB_err("%s%s", doc ? "" : "doc == NULL ", seq ? "" : "seq == NULL");	\
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
