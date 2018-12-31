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

/*	node
 * The basic atom of xdpacket; describes user intent in terms of
 * input (seq) -> match -> write (aka: mangle) -> output
 */
struct node {
	char		name[MAXLINELEN];
	struct iface	*in;
	uint64_t	seq;
	struct iface	*out;
	Pvoid_t		matches_JQ; /* queue of (struct field_value *mch) */
	Pvoid_t		writes_JQ; /* queue of (struct field_value *wrt) */
};


void node_free(void *arg);

struct node *node_new(const char *name,
			uint64_t seq,
			struct iface *in,
			struct iface *out,
			Pvoid_t matches_JQ,
			Pvoid_t writes_JQ);

struct node *node_add_match(struct node *ne,
			const char *value,
			struct field *field);

struct node *node_add_write(struct node *ne,
			const char *value,
			struct field *field);


#endif /* node2_h_ */
