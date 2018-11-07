#ifndef iface_h_
#define iface_h_

/*	iface.h
 * A data structure describing a physical interface on the machine,
 * and the matcher chains associated with it.
 */
#include <xdpk.h> /* must always be first */
#include <unistd.h>
#include <stdint.h>
#include <netinet/in.h> /* sockaddr_in */
#include <Judy.h>
#include <epoll_track.h>
#include <hook.h>


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
 * @JS_mch_out	: output matchers in alphabetical order
 * @JS_mch_in	: input matchers in alphabetical order
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

	struct hook	*in;
	struct hook	*out;
};


void		iface_free	(struct iface *sk);

struct iface	*iface_new	(const char *ifname);

void		iface_callback	(int fd,
				uint32_t events,
				epoll_data_t context);

#endif /* iface_h_ */
