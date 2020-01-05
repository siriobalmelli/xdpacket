#ifndef refcnt_h_
#define refcnt_h_

#include <ndebug.h>

//#define REFCNT_TRACE


/*	refcnt_take()
 * Take a refcount.
 */
#ifdef REFCNT_TRACE
#define refcnt_take(structure)							\
	NB_inf("take refcnt on " #structure);					\
	structure->refcnt++;

#else
#define refcnt_take(structure)							\
	structure->refcnt++;
#endif


/*	refcnt_release()
 * Release a refcount, where we are _expecting_ there to be one.
 */
#ifdef REFCNT_TRACE
#define refcnt_release(structure)						\
	if (!structure->refcnt) {						\
		NB_err("attempt to release 0-refcnt on " #structure);		\
	} else {								\
		NB_inf("release refcnt on " #structure);			\
		structure->refcnt--;						\
	}

#else
#define refcnt_release(structure)						\
	if (!structure->refcnt)							\
		NB_err("attempt to release 0-refcnt on " #structure);		\
	else									\
		structure->refcnt--;
#endif


#endif /* refcnt_h_ */
