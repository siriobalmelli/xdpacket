#ifndef xdpacket_h_
#define xdpacket_h_

/*	xdpacket.h
 * Program-wide defines: include before all source files.
 * (c) 2018 Sirio Balmelli and Alan Morrissett
 */

#include <epoll_track.h>

/* names for user-supplied structs (iface, field, fval, rule) are _short_.
 * A max of 48B leaves 16B (2 * sizeof(uint64_t)) in one cache line.
 */
#define MAXLINELEN 48

/* User-supplied value strings must be checked for overflow, and should:
 * - never result in a binary value larger than (struct field_set).len can represent.
 * - not impose arbitrarily low limits to users trying to match enormous fields.
 */
#define MAXVALUELEN (((size_t)1 << (sizeof(uint16_t) * 8)) -1)

/* TCP port for xdpacket socket.
 * This is the 16bit fnv1a hash of "xdpacket"
 */
#define PORT "7044"

/* Placeholder so that we can mark all areas of code affected.
 * The basic question is how to behave if a caller wants to "create" a new
 * resource (field, rule, iface).
 * This is because the user may be describing:
 * - an identical resource (in which case creation should be idempotent)
 * - a different resource with the same name, in which case it's unclear whether
 *   it is _really_ desired that the existing resource be clobbered/replaced.
 * If we _do_ clobber, there is also the question of how to best handle an
 * existing resource with a non-zero refcount (we don't necessarily know _who_
 * refers to it, only that refcount is non-zero).
 *
 * Obviously, comparing a requested resource and an existing resource to device
 * whether they are "identical" can introduce subtle errors.
 * The simplest solution is to simply error _any_ time we are asked to create
 * an identically named resource, and therefore ask the caller to explicitly
 * delete a resource before creating a new, identically-named one.
 * This can definitely be reviewed in future, but that's the call right now.
 */
#define XDPACKET_DISALLOW_CLOBBER

/* TODO: stopgap measure, find a way to pass this cleanly without it being global */
extern struct epoll_track *tk;

#endif /* xdpacket_h_ */
