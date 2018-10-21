#ifndef IFACE_H
#define IFACE_H

#include <xdpk.h> /* must always be first */

#include <unistd.h>
#include <stdint.h>
#include <netinet/in.h> /* sockaddr_in */
#include <Judy.h>
#include <epoll_track.h>

#include <matcher2.h>

/*	fixed-size strings FTW
The largest address is '[0-9]{12}' + '\.{3}' + '\0'
*/
#define XDPK_ADDR_MAX 16

#define XDPK_MAC_PROTO "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx"
#define XDPK_MAC_BYTES(ptr) ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]
#define XDPK_SOCK_PRN(sk_p) "%d: %s %s "XDPK_MAC_PROTO" mtu %d", \
               sk_p->ifindex, sk_p->name, sk_p->ip_prn, \
               sk_p->hwaddr.sa_data[0], sk_p->hwaddr.sa_data[1], sk_p->hwaddr.sa_data[2], \
               sk_p->hwaddr.sa_data[3], sk_p->hwaddr.sa_data[4], sk_p->hwaddr.sa_data[5], \
               sk_p->mtu


/*	iface
 * all parameters for an 'if' or 'iface' element in the grammar
 *
 * @pkin	: queue of packets to be output
 * @msg		: pipe containing control messages
 * @fd		: raw socket
 * @ip_prn	: IP address as a string
 * @JS_mch_out	: output mathers in alphabetical order
 * @JS_mch_in	: input mathers in alphabetical order
 */
struct iface {
	int		fd;
	char		ip_prn[XDPK_ADDR_MAX];
	/* taken directly from struct ifreq: */
	int		ifindex;
	int		mtu;
	struct sockaddr	hwaddr;
	struct sockaddr_in addr;
	char		name[XDPK_ADDR_MAX];

	size_t		out_cnt;
	Pvoid_t		JS_mch_out;  /* (char *)name -> matcher */

	size_t		in_cnt;
	Pvoid_t		JS_mch_in;
};

void	iface_free(struct iface *sk);
struct	iface *iface_new(const char *ifname);
void	iface_callback(int fd, uint32_t events, epoll_data_t context);

Word_t		iface_mch_ins(struct iface *sk, bool in,
				struct matcher *mch, const char *name);
struct matcher	*iface_mch_del(struct iface *sk, bool in, const char *name);
Word_t		iface_mch_iter(struct iface *sk, bool in,
				void(*exec)(struct matcher *mch));

#endif /* IFACE_H */
