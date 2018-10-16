#ifndef matcher_h_
#define matcher_h_

/*	matcher.h
 * A data structure describing one or more 'fields' to be matched (see field.h)
 * and associated manipulation functions.
 * (c) 2018 Alan Morrissett and Sirio Balmelli
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

/* Set of xdpk_matcher structs, any one of which can match (logical OR) */
struct xdpk_matcher_set {
	struct xdpk_matcher matchers[XDPK_MATCHER_SET_MAX];	
}__attribute__((packed));

/*	xdpk_match()
 * Test 'pkt' against all fields in '*match'.
 */
NLC_PUBLIC __attribute__((pure))
    bool xdpk_match(const struct xdpk_matcher *match,
		const void *pkt, size_t plen, uint64_t hash);

#endif /* matcher_h_ */
