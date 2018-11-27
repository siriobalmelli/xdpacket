#include <parse_util.h>
#include <zed_dbg.h>

/*	yaml_map_count()
 */
 int yaml_map_count(const yaml_node_t *node) 
 {
	int cnt = 0;
	Z_die_if(node->type != YAML_MAPPING_NODE, "Not a mapping node");
	yaml_node_pair_t *i_node_p;
	for (i_node_p = node->data.mapping.pairs.start; i_node_p < node->data.mapping.pairs.top; i_node_p++)
		cnt++;
out:
	return cnt;
 }

