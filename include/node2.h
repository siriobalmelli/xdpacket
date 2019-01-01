#ifndef node2_h_
#define node2_h_

/*	node2.h
 * Node: an input interface, a set of matchers and a set of resulting actions.
 * A node is the atomic unit of program control.
 * (c) 2018 Sirio Balmelli
 */

#include <field2.h>
#include <xform2.h>
#include <match2.h>
#include <iface.h>

/*	field_value
 * Representation of user-supplied (fieldname, value) tuple.
 * NOTE that the user gave 'fieldname' as a string, but we store a pointer so that:
 * - we clearly show that 'fieldname' _must_ refer to a valid field
 * - we save a string lookup to get the field's 'set'
 * - this struct stays the size of a cacheline
 */
struct field_value {
	char		value[MAXLINELEN];
	struct field	*field;
};


/*	node_parse
 */
struct node_parse {
	char			*iface_name; /* input interface */
	unsigned int		seq; /* unique sequence _and_ id */
	size_t			match_cnt;
	struct match_parse	*matches;
	size_t			xform_cnt;
	struct xform_parse	*xforms;
};

extern Pvoid_t	node_parse_J; /* (char *node_name) -> (struct node_parse *parse) */

#endif /* node2_h_ */
