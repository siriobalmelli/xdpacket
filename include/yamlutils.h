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



/*	Y_MAP_PAIRS_EXEC_COMMON()
 * Common code for Y_MAP_PAIRS_EXEC_[X] and nested loop in Y_SEQ_MAP_PAIRS_EXEC_[X]
 *
 * TODO: make sure all 'for (yaml_node_pair_t' loops in the code are moved over
 * to these Y_[X]_EXEC_[Y]() macros
 */
#define Y_MAP_PAIRS_EXEC_COMMON(doc_ptr, map_ptr)				\
	for (yaml_node_pair_t *pair = map_ptr->data.mapping.pairs.start;	\
		pair < map_ptr->data.mapping.pairs.top;				\
		pair++)								\
	{									\
		/* loop boilerplate */						\
		yaml_node_t *key = yaml_document_get_node(doc_ptr, pair->key);	\
		const char *keyname = (const char *)key->data.scalar.value;	\
										\
		yaml_node_t *val = yaml_document_get_node(doc_ptr, pair->value);

/*	Y_MAP_PAIRS_EXEC_OBJ()
 * Execute 'statements' on every pair of the mapping the pointed to by 'map_ptr'.
 *
 * The following variables are available to 'statements':
 * - (char *)keyname
 * - (yaml_node_t *)val
 */
#define Y_MAP_PAIRS_EXEC_OBJ(doc_ptr, map_ptr, statements)			\
	Y_MAP_PAIRS_EXEC_COMMON(doc_ptr, map_ptr)				\
		/* user-supplied statements here */				\
		statements;							\
	}

/*	Y_MAP_PAIRS_EXEC_STR()
 * Execute 'statements' on every pair of the mapping the pointed to by 'map_ptr'.
 *
 * The following variables are available to 'statements':
 * - (char *)keyname
 * - (char *)valtxt
 */
#define Y_MAP_PAIRS_EXEC_STR(doc_ptr, map_ptr, statements)			\
	Y_MAP_PAIRS_EXEC_COMMON(doc_ptr, map_ptr)				\
		if (val->type != YAML_SCALAR_NODE) {				\
			NB_err("'%s' in field not a scalar", keyname);		\
			continue;						\
		}								\
		const char *valtxt = (const char *)val->data.scalar.value;	\
										\
		/* user-supplied statements here */				\
		statements;							\
	}


/*	Y_SEQ_MAP_PAIRS_EXEC_COMMON()
 * Common code for Y_SEQ_MAP_PAIRS_[X] code
 */
#define Y_SEQ_MAP_PAIRS_EXEC_COMMON(doc_ptr, seq_ptr)					\
	/* process children list objects */						\
	for (yaml_node_item_t *child = seq_ptr->data.sequence.items.start;		\
		child < seq_ptr->data.sequence.items.top;				\
		child++)								\
	{										\
		yaml_node_t *mapping = yaml_document_get_node(doc_ptr, *child);		\
		if (mapping->type != YAML_MAPPING_NODE) {				\
			NB_err("node is not a map: %s @%zu:%zu",			\
				mapping->tag,						\
				mapping->start_mark.line, mapping->start_mark.column);	\
			continue;							\
		}

/*	Y_SEQ_MAP_PAIRS_EXEC_OBJ()
 * Execute 'statements' on every pair of every mapping in the sequence
 * pointed to by 'seq_ptr'.
 *
 * The following variables are available to 'statements':
 * - (char *)keyname
 * - (yaml_node_t *)val
 */
#define Y_SEQ_MAP_PAIRS_EXEC_OBJ(doc_ptr, seq_ptr, statements)				\
	Y_SEQ_MAP_PAIRS_EXEC_COMMON(doc_ptr, seq_ptr)					\
		Y_MAP_PAIRS_EXEC_COMMON(doc_ptr, mapping)				\
			/* user-supplied statements here */				\
			statements;							\
		}									\
	}

/*	Y_SEQ_MAP_PAIRS_EXEC_STR()
 * Execute 'statements' on every pair of every mapping in the sequence
 * pointed to by 'seq_ptr'.
 *
 * The following variables are available to 'statements':
 * - (char *)keyname
 * - (char *)valtxt
 */
#define Y_SEQ_MAP_PAIRS_EXEC_STR(doc_ptr, seq_ptr, statements)				\
	Y_SEQ_MAP_PAIRS_EXEC_COMMON(doc_ptr, seq_ptr)					\
		Y_MAP_PAIRS_EXEC_COMMON(doc_ptr, mapping)				\
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
