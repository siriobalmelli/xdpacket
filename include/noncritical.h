#ifndef noncritical_h_
#define noncritical_h_

/*	noncritical.h
 * User-facing parsing and global tracking; not inserted in the fast-path.
 * Parameters should be exactly as given by user;
 * each element links to the fastpath structures using a single 'id' parameter,
 * which is then used to look up in the appropriate Judy array.
 * (c) 2018 Sirio Balmelli
 */
#include <field2.h>
#include <action2.h>
#include <Judy.h>


/*	field_desc
 * Describe a field.
 */
struct field_desc {
	uint64_t	id;	/* fnv64 hash of field: use for lookup */
	struct field	field_desc;
	char		*name;
};

extern Pvoid_t	field_id_J;	/* (uint64_t id) -> (struct field field) */
extern Pvoid_t	field_name_JS;	/* (char *field_name) -> (struct field_desc field_desc) */


/*	match_desc
 * Describe a (field, value) tuple to be matched.
 */
struct match_desc {
	char		*field_name;
	char		*value;
};


/*	xform_desc
 * Describe a series of transformations (actions) to be performed.
 */



/*	node_desc
 * Describe a node.
 */
struct node_desc {
	uint64_t	seq;	/* 'seq' is used to both uniquely identify nodes
				 * and establish parsing order.
				 */
	char		*iface_name; /* input interface */
};

extern Pvoid_t	node_J;		/* (uint64_t seq) -> (struct node_desc node_desc) */

#endif /* noncritical_h_ */
