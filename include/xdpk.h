#ifndef xdpk_h_
#define xdpk_h_

#include <unistd.h>
#include <stdint.h>
#include <netinet/in.h> /* sockaddr_in */

/*	fixed-size strings FTW
The largest address is '[0-9]{12}' + '\.{3}' + '\0'
*/
#define XDPK_ADDR_MAX 16


/*	xdpk_sock
 */
struct xdpk_sock {
	int		fd;
	char		ip_prn[XDPK_ADDR_MAX];
	/* taken directly from struct ifreq: */
	int		ifindex;
	int		mtu;
	struct sockaddr	hwaddr;
	struct sockaddr_in addr;
	char		name[XDPK_ADDR_MAX];
};

#define XDPK_MAC_PROTO "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx"
#define XDPK_MAC_BYTES(ptr) ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]
#define XDPK_SOCK_PRN(sk_p) "%d: %s %s "XDPK_MAC_PROTO" mtu %d", \
			sk_p->ifindex, sk_p->name, sk_p->ip_prn, \
			sk_p->hwaddr.sa_data[0], sk_p->hwaddr.sa_data[1], sk_p->hwaddr.sa_data[2], \
			sk_p->hwaddr.sa_data[3], sk_p->hwaddr.sa_data[4], sk_p->hwaddr.sa_data[5], \
			sk_p->mtu

#endif /* xdpk_h_ */
