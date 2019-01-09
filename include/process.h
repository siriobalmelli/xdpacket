#ifndef process_h_
#define process_h_

/*	process.h
 * A process is an input interface and a list of (node, output_interface)
 * tuples to be executed on it.
 * (c) 2018 Sirio Balmelli
 */

#include <Judy.h>
#include <yaml.h>
#include <iface.h>


/*	rout
 * Representation of user-supplied (rule, output) tuple, given to us as strings.
 */
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


/*	process
 */
struct process {
	struct iface	*in_iface;
	Pvoid_t		rout_JQ;	/* (uint64_t seq) -> (struct rout *rout) */
};


void	process_free		(void *arg);

struct process	*process_new	(const char *in_iface_name,
				Pvoid_t rout_JQ);


int		process_parse	(enum parse_mode mode,
				yaml_document_t *doc,
				yaml_node_t *mapping,
				yaml_document_t *outdoc,
				int outlist);

int		process_emit	(struct process *process,
				yaml_document_t *outdoc,
				int outlist);


#endif /* process_h_ */
