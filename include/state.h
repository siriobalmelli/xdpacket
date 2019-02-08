#ifndef state_h_
#define state_h_

/*	state.h
 * Global named variable which can be referenced by "store" and "copy" rules.
 * These are not directly manipulated by the user, so no "new/free" functionality
 * is provided but rather "get/release" semantics which hide alloc/free.
 *
 * (c) 2018 Sirio Balmelli
 */

#include <stdint.h>
#include <stdlib.h>


/*	state
 * Memory buffer storing actual state.
 */
struct state {
	char		*name;
	size_t		len;
	uint32_t	refcnt;
	uint8_t		bytes[];
};


void		state_release(struct state *state);
struct state	*state_get(const char *ref_name, size_t len);


#endif /* state_h_ */
