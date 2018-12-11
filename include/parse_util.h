#ifndef parse_util_h_
#define parse_util_h_

/*	parse_util.h
 * Utilities used by parse, broken out for simplicity only
 */

#include <yaml.h>
#include <stdbool.h>
#include <string.h>
#include <nonlibc.h>
#include <ndebug.h>


/*	yaml_map_count()
 */
NLC_INLINE
int yaml_map_count(const yaml_node_t *node)
{
	int cnt = 0;
	NB_die_if(node->type != YAML_MAPPING_NODE, "Not a mapping node");
	yaml_node_pair_t *i_node_p;
	for (i_node_p = node->data.mapping.pairs.start; i_node_p < node->data.mapping.pairs.top; i_node_p++)
		cnt++;
die:
	return cnt;
}


#endif /* parse_util_h_ */
