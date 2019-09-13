#ifndef sval_h_
#define sval_h_

/*	sval.h
 * Representation of a state-value tuple:
 * a reference to a global state variable and an expected value to match against.
 *
 * (c) 2019 Sirio Balmelli
 */

#include <xdpacket.h>
#include <state.h>
#include <yaml.h>
#include <string.h> /* memcmp */


/*	sval_set
 * Parsed/compiled sval, used in packet matching hot-path.
 * @state	:	ref to a state struct (which contains 'len')
 * @bytes	:	big-endian "parsed" value to match in 'state'
 */
struct sval_set {
	struct state	*state;
	uint8_t		bytes[];
};


void		sval_set_free	(void * arg);

struct sval_set	*sval_set_new	(struct state *state,
				const char *value);

/*	sval_test()
 * Test if the global state variable pointed to by 'set' corresponds to
 * the desired value encoded in 'set'.
 * Returns true if matching.
 */
NLC_INLINE bool sval_test	(struct sval_set *set)
{
	return !(memcmp(set->bytes, set->state->bytes, set->state->len));
}


/*	sval
 * @state	:	ref to a state struct (which contains 'len')
 * @val		:	user-supplied string of value to match
 * @set		:	compiled: ref to 'state' and big-endian bytes parsed from 'val'
 * @rendered	:	human-readable printed form of set->bytes
 */
struct sval {
	struct state	*state;
	char		*val;
	struct sval_set	*set;
	char		*rendered;
};


void		sval_free	(void *arg);

struct sval	*sval_new	(const char *state_name,
				const char *value);

int		sval_emit	(struct sval *sval,
				yaml_document_t *outdoc,
				int outlist);


#endif /* sval_h_ */
