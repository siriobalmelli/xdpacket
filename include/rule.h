#ifndef rule_h_
#define rule_h_

/*	rule.h
 * Rule: a set of matchers and a set of resulting actions.
 * (c) 2018 Sirio Balmelli
 */

#include <xdpacket.h>
#include <fval.h>
#include <fref.h>


/*	rule
 * The basic atom of xdpacket; describes user intent in terms of
 * input (seq) -> match -> write (aka: mangle bytes) -> output
 */
struct rule {
	char		*name;
	Pvoid_t		matches_JQ; /* (uint64_t seq) -> (struct fval *mch) */
	Pvoid_t		stores_JQ; /* (uint64_t seq) -> (struct fref *store) */
	Pvoid_t		copies_JQ; /* (uint64_t seq) -> (struct fref *copy) */
	Pvoid_t		writes_JQ; /* (uint64_t seq) -> (struct fval *wrt) */
	uint32_t	refcnt;
};


void		rule_free	(void *arg);
void		rule_free_all	();
struct rule	*rule_new	(const char *name,
				Pvoid_t matches_JQ,
				Pvoid_t stores_JQ,
				Pvoid_t copies_JQ,
				Pvoid_t writes_JQ);

void		rule_release	(struct rule *rule);
struct rule	*rule_get	(const char *name);


int		rule_parse	(enum parse_mode mode,
				yaml_document_t *doc,
				yaml_node_t *mapping,
				yaml_document_t *outdoc,
				int outlist);

int		rule_emit	(struct rule *node,
				yaml_document_t *outdoc,
				int outlist);

int		rule_emit_all	(yaml_document_t *outdoc,
				int outlist);


#endif /* rule_h_ */
