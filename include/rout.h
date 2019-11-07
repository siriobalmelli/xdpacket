#ifndef rout_h_
#define rout_h_

/*	rout
 * (rule, output) tuple, for use in the 'rules' list of a 'process' object.
 *
 * (c) 2018 Sirio Balmelli
 */

#include <judyutils.h>
#include <yaml.h>
#include <iface.h>


/*	rout_set
 * Packed representation of a rule (match -> write -> output) sequence.
 *
 * @if_out	: interface where packets should be output after writing/mangling.
 * @match_JQ	: queue (sequence) of match operatioons to be applied to packet.
 * @write_JQ	: write operations to be applied if all match operations pass.
 * @count_match	: number of packets matched and processed
 */
struct rout_set {
	struct iface		*if_out;

	Pvoid_t			match_JQ; /* (uint64_t seq) -> (struct (fval|fref)_set *action) */
	Pvoid_t			write_JQ; /* (uint64_t seq) -> (struct sref_set *state) */

	uint32_t		count_match;
};


void		rout_set_free	(void *arg);

struct rout_set	*rout_set_new	(struct rule *rule,
				struct iface *output);

bool		rout_set_match	(struct rout_set *set,
				const void *pkt,
				size_t plen);

bool		rout_set_exec	(struct rout_set *set,
				void *pkt,
				size_t plen);


/*	rout
 * Representation of user-supplied (rule, output) tuple, given to us as strings.
 * TODO: combine with rout_set?
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
