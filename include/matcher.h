#ifndef matcher_h_
#define matcher_h_

/*	matcher.h
 * A data structure describing one or more 'fields' to be matched (see field.h)
 * and associated manipulation functions.
 * (c) 2018 Alan Morrissett and Sirio Balmelli
 */

/* TODO: @alan: merge matcher2.h into this one, clean it up, delete matcher2.h */

#include <xdpk.h> /* must always be first */
#include <Judy.h>
#include <stdbool.h>

struct selector {
	/* TODO: implement */
};

struct matcher {
	/* TODO: implement */	
	size_t sel_cnt;
	struct selector sel[];
};

void		matcher_free(struct matcher *mch);
struct matcher	*matcher_new();
bool		matcher_do(struct matcher *mch, const void *buf, size_t buf_len);


/* The following are obsolete, but are temporarily renamed as an incremental step
 * to maintain compilation until full switchover
 */
#include <field.h>
#include <stdint.h>

/* allow for compile-time adjustment (memory footprint) */
#ifndef XDPK_MATCH_FIELD_MAX
#define XDPK_MATCH_FIELD_MAX 4
#endif
#ifndef XDPK_MATCHER_SET_MAX
#define XDPK_MATCHER_SET_MAX 4
#endif

/* Set of xdpk_fields, all of which must match (logical AND) */
struct xdpk_matcher {
	struct xdpk_field fields[XDPK_MATCH_FIELD_MAX];
}__attribute__((packed));

/*	xdpk_match()
 * Test 'pkt' against all fields in '*match'.
 */
NLC_PUBLIC __attribute__((pure))
    bool xdpk_match(const struct xdpk_matcher *match,
		const void *pkt, size_t plen, uint64_t hash);

#endif /* matcher_h_ */
