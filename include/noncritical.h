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
#include <xform2.h>
#include <match2.h>


/*	node_desc
 * Describe a node.
 */
struct node_desc {
	uint64_t		seq;	/* 'seq' is used to both uniquely identify nodes
					 * and establish parsing order.
					 */
	char			*iface_name; /* input interface */
	struct match_parse	matches[];

};

extern Pvoid_t	node_J;		/* (uint64_t seq) -> (struct node_desc node_desc) */


#endif /* noncritical_h_ */
