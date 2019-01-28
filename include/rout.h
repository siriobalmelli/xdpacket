#ifndef rout_h_
#define rout_h_

/*	rout
 * (rule, output) tuple, for use in the 'rules' list of a 'process' object.
 *
 * (c) 2018 Sirio Balmelli
 */

#include <judyutils.h>
#include <fval.h>
#include <yaml.h>
#include <iface.h>


/*	rout_set
 * Packed representation of a rule (match -> write -> output) sequence.
 *
 * @count_out	: number of packets successfully output after writing.
 * @out_fd	: fd of socket where packets should be output after writing/mangling.
 * @writes_JQ	: queue (sequence) of (struct fval_set) to be applied to packet.
 * @count_match	: number of packets matched.
 * @hash	: fnv1a hash for a matching packet.
 * @match_cnt	: number of (struct field_set) in 'matches'.
 * @matches	: hash all bytes in the packet described 'matches'.
 *
 * NOTE: this is laid out in reverse sequence (because matches[] must be last);
 * the logic makes use of these struct params from bottom to top
 * (compute matches, check hash, increment counter, etc).
 */
struct rout_set {
	size_t			count_out;
	struct iface		*if_out;

	Pvoid_t			writes_JQ; /* (uint64_t seq) -> (struct fval_set *write) */

	size_t			count_match;
	uint64_t		hash;
	size_t			match_cnt;
	struct field_set	matches[];
};


void		rout_set_free	(void *arg);

struct rout_set	*rout_set_new	(struct rule *rule,
				struct iface *output);

bool		rout_set_match	(struct rout_set *set,
				const void *pkt,
				size_t plen);

bool		rout_set_write	(struct rout_set *set,
				void *pkt,
				size_t plen);



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


#endif /* rout_h_ */
