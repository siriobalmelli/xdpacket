#ifndef IFACE_H
#define IFACE_H

#include <unistd.h>
#include <stdint.h>
#include <netinet/in.h> /* sockaddr_in */
#include <Judy.h>
#include <epoll_track.h>

/*	fixed-size strings FTW
The largest address is '[0-9]{12}' + '\.{3}' + '\0'
*/
#define XDPK_ADDR_MAX 16


/*	iface
 * all parameters for an 'if' or 'iface' element in the grammar
 *
 * @pkin : queue of packets to be output
 * @msg : pipe containing control messages
 * @fd : raw socket
 * @ip_prn : IP address as a string
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

	Word_t	match_out;
};

void iface_free(struct iface *sk);
struct iface *iface_new(const char *ifname);
void iface_callback(int fd, uint32_t events, epoll_data_t context);

#endif /* IFACE_H */
