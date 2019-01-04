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
#include <fval.h>


/*	node
 * The basic atom of xdpacket; describes user intent in terms of
 * input (seq) -> match -> write (aka: mangle bytes) -> output
 */
struct node {
	char		name[MAXLINELEN];
	Pvoid_t		matches_JQ; /* queue of (struct fval *mch) */
	Pvoid_t		writes_JQ; /* queue of (struct fval *wrt) */
};


void		node_free	(void *arg);

struct node	*node_new	(const char *name,
				Pvoid_t matches_JQ,
				Pvoid_t writes_JQ);

struct node	*node_get	(const char *name);


int		node_parse	(enum parse_mode mode,
				yaml_document_t *doc,
				yaml_node_t *mapping,
				yaml_document_t *outdoc,
				int outlist);

int		node_emit	(struct node *node,
				yaml_document_t *outdoc,
				int outlist);


#endif /* node2_h_ */
