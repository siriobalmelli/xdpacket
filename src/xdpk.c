#include <stdlib.h>
#include <unistd.h>

#include <linux/if_packet.h>	/* struct packet_mreq */
#include <linux/if_arp.h>	/* struct sockaddr_ll.sll_hatype */
#include <linux/if_ether.h>	/* ETH_P_ALL and friends */
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

#include <zed_dbg.h>
#include <epoll_track.h>
#include <posigs.h>

#include <xdpk.h>


/*	xdpk_free()
 */
void xdpk_sock_free(struct xdpk_sock *sk)
{
	if (!sk)
		return;
	Z_log(Z_inf, "close "XDPK_PRN(sk));
	if (sk->fd != -1)
		close(sk->fd);
	free(sk);
}

/*	xdpk_open()
 * open an interface
 */
struct xdpk_sock *xdpk_sock_new(const char *ifname)
{
	struct xdpk_sock *ret = NULL;
	Z_die_if(!(
		ret = calloc(sizeof(struct xdpk_sock), 1)
		), "alloc size %zu", sizeof(struct xdpk_sock));
	snprintf(ret->name, IFNAMSIZ, "%s", ifname);

	/* socket */
	ret->fd = -1;	
	Z_die_if((
		ret->fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))
		) < 0, "");
	struct ifreq ifr = {{{0}}};
	snprintf (ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);

	/* grok interface
	 * ordered by increasing field size: avoid zeroing the union between calls
	 */
	Z_die_if(
		ioctl(ret->fd, SIOCGIFINDEX, &ifr)
		, "");
	ret->ifindex = ifr.ifr_ifindex;
	Z_die_if(
		ioctl(ret->fd, SIOCGIFMTU, &ifr)
		, "");
	ret->mtu = ifr.ifr_mtu;
	Z_die_if(
		ioctl(ret->fd, SIOCGIFADDR, &ifr)
		, "");
	memcpy(&ret->addr, &ifr.ifr_addr, sizeof(ret->addr));
	Z_die_if(!(
		inet_ntop(ret->addr.sin_family, &ret->addr.sin_addr,
			ret->ip_prn, sizeof(ret->ip_prn))
		), "");
	Z_die_if(
		ioctl(ret->fd, SIOCGIFHWADDR, &ifr)
		, "");
	memcpy(&ret->hwaddr, &ifr.ifr_hwaddr, sizeof(ret->hwaddr));

	/* bind to interface */
	struct sockaddr_ll saddr = {
		.sll_family = AF_PACKET,
		.sll_protocol = htons(ETH_P_ALL),
		.sll_ifindex = ret->ifindex,
		.sll_hatype = ARPHRD_ETHER, /* no clue what an ARP type is */
		.sll_pkttype = PACKET_HOST | PACKET_BROADCAST | PACKET_MULTICAST,
		.sll_halen = 6,
		.sll_addr = { ret->hwaddr.sa_data[0], ret->hwaddr.sa_data[1], ret->hwaddr.sa_data[2], 
			ret->hwaddr.sa_data[3], ret->hwaddr.sa_data[4], ret->hwaddr.sa_data[5]}
	};
	Z_die_if(
		bind(ret->fd, (struct sockaddr *)&saddr, sizeof(saddr))
		, "");

#if 1
	/* promiscuous */
	struct packet_mreq mr = {
		.mr_ifindex = ret->ifindex,
		.mr_type = PACKET_MR_PROMISC
	};
	Z_die_if(
		setsockopt(ret->fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr))
		, "");
	Z_die_if(
		ioctl(ret->fd, SIOCGIFFLAGS, &ifr)
		, "");
	ifr.ifr_flags |= IFF_PROMISC;
	Z_die_if(
		ioctl(ret->fd, SIOCSIFFLAGS, &ifr)
		, "");
#endif

	Z_log(Z_inf, "add "XDPK_PRN(ret));

	return ret;
out:
	xdpk_sock_free(ret);
	return NULL;
}

/*	xdpk_sock_callback()
 */
void xdpk_sock_callback(int fd, uint32_t events, epoll_data_t context)
{
	Z_log(Z_inf, "callback");
	char buf[16384];
	ssize_t res = recv(fd, buf, sizeof(buf), 0);
	if (res < 1)
		return;
	struct ethhdr *eth = (struct ethhdr *)buf;
	//struct iphdr *ip = (struct iphdr*)(buf + sizeof(struct ethhdr));
	Z_log(Z_inf, "recv len %zd from %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
		res, eth->h_source[0],eth->h_source[1],eth->h_source[2],
		eth->h_source[3],eth->h_source[4],eth->h_source[5]);

	return;
}


/*	main()
 */
int main(int argc, char **argv)
{
	struct epoll_track *tk = NULL;
	struct xdpk_sock *sk = NULL;

	Z_die_if(
		psg_sigsetup(NULL)
		, "failed to set up signals");
	Z_die_if(!(
		tk = eptk_new()
		), "failed to set up epoll");

	Z_die_if(!(
		sk = xdpk_sock_new(argv[1])
		), "failed to open socket on '%s'", argv[1]);
	struct epoll_track_cb cb = {
		.fd = sk->fd,
		.events = EPOLLIN,
		.context.ptr = sk,
		.callback = xdpk_sock_callback
	};
	Z_die_if(eptk_register(tk, &cb), "");

	int res;
	while(!psg_kill_check()) {
		Z_die_if((
			res = eptk_pwait_exec(tk, -1, NULL)
			) < 0, "");
	}

out:
	eptk_free(tk, false);
	xdpk_sock_free(sk);
	return 0;
}
