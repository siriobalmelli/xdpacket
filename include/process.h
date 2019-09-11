#ifndef process_h_
#define process_h_

/*	process.h
 * A process is an input interface and a list of (node, output_interface)
 * tuples to be executed on it.
 * (c) 2018 Sirio Balmelli
 */

#include <judyutils.h>
#include <yaml.h>
#include <iface.h>
#include <rule.h>
#include <rout.h>


/*	process
 */
struct process {
	struct iface	*in_iface;
	Pvoid_t		rout_JQ;	/* (uint64_t seq) -> (struct rout *rout) */
	Pvoid_t		rout_set_JQ;	/* (uint64_t seq) -> (struct rout_set *rst) */
};


void		process_free	(void *arg);
void		process_free_all();
struct process	*process_new	(const char *in_iface_name,
				Pvoid_t rout_JQ);

void		process_exec	(void *context, void *pkt, size_t len);


int		process_parse	(enum parse_mode mode,
				yaml_document_t *doc,
				yaml_node_t *mapping,
				yaml_document_t *outdoc,
				int outlist);

int		process_emit	(struct process *process,
				yaml_document_t *outdoc,
				int outlist);

int		process_emit_all (yaml_document_t *outdoc,
				int outlist);


#endif /* process_h_ */
