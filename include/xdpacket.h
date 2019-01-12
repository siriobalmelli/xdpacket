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

/* TCP port for xdpacket socket.
 * This is the 16bit fnv1a hash of "xdpacket"
 */
#define PORT "7044"

/* TODO: stopgap measure, find a way to pass this cleanly without it being global */
extern struct epoll_track *tk;

#endif /* xdpacket_h_ */
