#ifndef refcnt_h_
#define refcnt_h_

#include <ndebug.h>


/*	refcnt_take()
 * Take a refcount.
 */
#define refcnt_take(structure)							\
	structure->refcnt++;


/*	refcnt_release()
 * Release a refcount, where we are _expecting_ there to be one.
 */
#define refcnt_release(structure)						\
	if (!structure->refcnt)							\
		NB_err("attempt to release 0-refcnt on " #structure);		\
	else									\
		structure->refcnt--;


#if 0
/*	refcnt_release_cond()
 * Release a refcount if it exists, return whether refcount is non-zero
 * _after_ decrement.
 */
#define refcnt_release_cond(structure)						\
	(structure->refcnt && --structure->refcnt)
#endif

#endif /* refcnt_h_ */
