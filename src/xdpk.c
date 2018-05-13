#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if_ether.h>	/* ETH_P_ALL and friends */
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <zed_dbg.h>
#include <xdpk.h>


/*	xdpk_free()
 */
void xdpk_sock_free(struct xdpk_sock *sk)
{
	if (!sk)
		return;
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

	/* grok interface
	 * ordered by increasing field size: avoid zeroing the union between calls
	 */
	ret->fd = -1;	
	Z_die_if((
		ret->fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))
		) < 0, "");
	struct ifreq ifr = {{{0}}};
	snprintf (ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
	Z_die_if(
		ioctl(ret->fd, SIOCGIFINDEX, &ifr)
		, "");
	ret->ifindex = ifr.ifr_ifindex;
	memset(&ifr.ifr_ifru, 0, sizeof(ifr.ifr_ifru));

	Z_die_if(
		ioctl(ret->fd, SIOCGIFMTU, &ifr)
		, "");
	ret->mtu = ifr.ifr_mtu;
	memset(&ifr.ifr_ifru, 0, sizeof(ifr.ifr_ifru));

	Z_die_if(
		ioctl(ret->fd, SIOCGIFADDR, &ifr)
		, "");
	memcpy(&ret->addr, &ifr.ifr_addr, sizeof(ret->addr));
	Z_die_if(!(
		inet_ntop(ret->addr.sin_family, &ret->addr.sin_addr,
			ret->ip_prn, sizeof(ret->ip_prn))
		), "");
	memset(&ifr.ifr_ifru, 0, sizeof(ifr.ifr_ifru));

	Z_die_if(
		ioctl(ret->fd, SIOCGIFHWADDR, &ifr)
		, "");
	memcpy(&ret->hwaddr, &ifr.ifr_hwaddr, sizeof(ret->hwaddr));
	Z_log(Z_inf, XDPK_PRN(ret));

	return ret;
out:
	xdpk_sock_free(ret);
	return NULL;
}


/*	main()
 */
int main(int argc, char **argv)
{
	struct xdpk_sock *sk = NULL;

	Z_die_if(!(
		sk = xdpk_sock_new(argv[1])
		), "");

out:
	xdpk_sock_free(sk);
	return 0;
}
