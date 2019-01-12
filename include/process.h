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
#include <rule.h>


/*	rout_set
 * Packed representation of the 'write' side of a rule.
 * @out_fd	: fd of socket where packets should be output after writing/mangling.
 * @counter	: number of matched and output packets.
 * @write_cnt	: number of (struct fval_bytes) in 'writes'.
 * @writes	: an array of (struct fval_bytes *) pointing into 'memory', because:
 *		  - each (struct fval_bytes) is variable-length (can't index an array directly)
 *		  - we want them packed in memory to avoid pointer chasing in the fast path
 */
struct rout_set {
	int			out_fd;
	size_t			counter;
	size_t			write_cnt;
	struct fval_bytes	**writes;
	uint8_t			memory[];
};


void		rout_set_free	(void *arg);

struct rout_set	*rout_set_new	(int out_fd, Pvoid_t writes_JQ);

int		rout_set_write	(struct rout_set *rst, void *pkt, size_t plen);


/*	rout
 * Representation of user-supplied (rule, output) tuple, given to us as strings.
 */
struct rout {
	struct rule		*rule;
	struct iface		*output;
	struct rout_set		*set;
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
