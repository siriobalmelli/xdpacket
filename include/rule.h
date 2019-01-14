#ifndef rule_h_
#define rule_h_

/*	rule.h
 * Rule: a set of matchers and a set of resulting actions.
 * (c) 2018 Sirio Balmelli
 */

#include <xdpacket.h>
#include <fval.h>


/*	rule
 * The basic atom of xdpacket; describes user intent in terms of
 * input (seq) -> match -> write (aka: mangle bytes) -> output
 */
struct rule {
	char		name[MAXLINELEN];
	Pvoid_t		matches_JQ; /* queue of (struct fval *mch) */
	Pvoid_t		writes_JQ; /* queue of (struct fval *wrt) */
};


void		rule_free	(void *arg);

struct rule	*rule_new	(const char *name,
				Pvoid_t matches_JQ,
				Pvoid_t writes_JQ);

struct rule	*rule_get	(const char *name);


int		rule_parse	(enum parse_mode mode,
				yaml_document_t *doc,
				yaml_node_t *mapping,
				yaml_document_t *outdoc,
				int outlist);

int		rule_emit	(struct rule *node,
				yaml_document_t *outdoc,
				int outlist);


#endif /* rule_h_ */
