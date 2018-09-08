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

struct xdpk_matcher {
    struct xdpk_field fields[XDPK_MATCH_FIELD_MAX];
}__attribute__((packed));

/*	xdpk_match()
 * Test 'pkt' against all fields in '*match'.
 * TODO: @alan please implement
 */
NLC_PUBLIC __attribute__((pure))
    bool xdpk_match(const struct xdpk_matcher *match,
		    const void *pkt, size_t plen);

#endif /* matcher_h_ */
