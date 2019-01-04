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
#include <nout.h>


struct process {
	struct iface	*in_iface;
	Pvoid_t		nout_JQ;	/* (uint64_t seq) -> (struct nout *nout) */
};


void	process_free		(void *arg);

struct process	*process_new	(const char *in_iface_name,
				Pvoid_t nout_JQ);


int		process_parse	(enum parse_mode mode,
				yaml_document_t *doc,
				yaml_node_t *mapping,
				yaml_document_t *outdoc,
				int outlist);

int		process_emit	(struct process *process,
				yaml_document_t *outdoc,
				int outlist);

#endif /* process_h_ */
