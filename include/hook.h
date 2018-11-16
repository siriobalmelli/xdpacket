#ifndef hook_h_
#define hook_h_
/*	hook.h
 * A 'hook' is the top-level of the packet matching stack.
 *  (c) 2018 Sirio Balmelli
 */
#include <xdpacket.h> /* must always be first */
#include <matcher.h>


/*	hook
 * one direction (in/out) of an interface, with relevant counters
 * and matchers to be run.
 */
struct hook {
	size_t	cnt_pkt;
	size_t	cnt_drop;
	Pvoid_t JS_match;
};


void		hook_free	(struct hook *hk);
struct hook	*hook_new	();

void		hook_callback	(struct hook *hk,
				void *buf,
				size_t len);

Word_t		hook_insert	(struct hook *hk,
				struct matcher *mch,
				const char *name);

struct matcher	*hook_delete	(struct hook *hk,
				const char *name);

Word_t		hook_iter	(struct hook *hk,
				void(*exec)(struct matcher *mch));



#endif /* hook_h_ */
