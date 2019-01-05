#ifndef rout_h_
#define rout_h_

/*	rout.h
 * Representation of user-supplied (rule, output) tuple, given to us as strings.
 *
 * (c) 2018 Sirio Balmelli
 */

#include <yaml.h>
#include <rule.h>
#include <iface.h>


struct rout {
	struct rule	*rule;
	struct iface	*output;
};


void		rout_free	(void *arg);

struct rout	*rout_new	(const char *rule_name,
				const char *out_name);

int		rout_emit	(struct rout *nout,
				yaml_document_t *outdoc,
				int outlist);

#endif /* rout_h_ */
