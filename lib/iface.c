#include <iface.h>
#include <arpa/inet.h>
#include <linux/if_arp.h>	/* struct sockaddr_ll.sll_hatype */
#include <linux/if_ether.h>	/* ETH_P_ALL and friends */
#include <linux/if_packet.h>	/* struct packet_mreq */
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

/*	xdpk_free()
 */
void iface_free(struct iface *sk)
{
	if (!sk)
		return;
	Z_log(Z_inf, "close "XDPK_SOCK_PRN(sk));

	/* clean up matchers */
	iface_mch_iter(sk, true, matcher_free);
	iface_mch_iter(sk, false, matcher_free);
	int Rc_word;
	JSLFA(Rc_word, sk->JS_mch_in);
	JSLFA(Rc_word, sk->JS_mch_out);

	if (sk->fd != -1)
		close(sk->fd);

	free(sk);
}

/*	xdpk_open()
 * open an interface
 */
struct iface *iface_new(const char *ifname)
{
	struct iface *ret = NULL;
	Z_die_if(!(
		ret = calloc(sizeof(struct iface), 1)
		), "alloc size %zu", sizeof(struct iface));
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
		.sll_halen = 6,
		.sll_addr = { ret->hwaddr.sa_data[0], ret->hwaddr.sa_data[1], ret->hwaddr.sa_data[2],
			ret->hwaddr.sa_data[3], ret->hwaddr.sa_data[4], ret->hwaddr.sa_data[5]},
		/* ignored by bind call */
		.sll_hatype = 0,
		.sll_pkttype = 0
	};
	Z_die_if(
		bind(ret->fd, (struct sockaddr *)&saddr, sizeof(saddr))
		, "");

	Z_log(Z_inf, "add "XDPK_SOCK_PRN(ret));

	return ret;
out:
	iface_free(ret);
	return NULL;
}

/*	iface_callback()
 */
void iface_callback(int fd, uint32_t events, epoll_data_t context)
{
	/* receive packet and discard outgoing packets */
	struct sockaddr_ll addr;
        socklen_t addr_len = sizeof(addr);
	char buf[16384];
	ssize_t res = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&addr, &addr_len);
	if (res < 1)
		return;

	/* handle packet */
	struct iface *sk = (struct iface *)context.ptr;

	/* get proper matcher list, increment counters */
	Pvoid_t *array;
	if (addr.sll_pkttype == PACKET_OUTGOING) {
		array = &sk->JS_mch_out;
		sk->out_cnt++;
	} else {
		array = &sk->JS_mch_in;
		sk->in_cnt++;
	}

	Word_t *PValue;
	uint8_t index[MAXLINELEN] = { '\0' };
	JSLF(PValue, *array, index);
	while (PValue) {
		if (matcher_do((struct matcher *)(*PValue), buf, res))
			break;
		JSLN(PValue, *array, index);
	}
}

/* iface_mch_ins()
 * Insert 'mch' into 'in' or 'out' list as 'name'.
 *
 * Returns 0 on success, -1 on error;
 * attempting to insert a duplicate item is an error.
 */
Word_t iface_mch_ins(struct iface *sk, bool in, struct matcher *mch, const char *name)
{
	Pvoid_t *array;
	if (in)
		array = &sk->JS_mch_in;
	else
		array = &sk->JS_mch_out;

	Word_t *PValue;
	JSLI(PValue, *array, (uint8_t*)name);
	if (*PValue)
		return -1;
	*PValue = (Word_t)mch;
	return 0;
}

/* iface_mch_del()
 * Delete 'name' from 'in' or 'out' list.
 *
 * Returns the matcher at 'name' or NULL if nothing found to delete.
 * Caller is responsible for use or free() of returned matcher.
 */
struct matcher	*iface_mch_del(struct iface *sk, bool in, const char *name)
{
	Pvoid_t *array;
	if (in)
		array = &sk->JS_mch_in;
	else
		array = &sk->JS_mch_out;

	Word_t *PValue;
	JSLG(PValue, *array, (uint8_t *)name);
	if (!PValue)
		return NULL;

	struct matcher *ret = (void *)(*PValue);
	int Rc_int; /* we already know it exists, ignore this */
	JSLD(Rc_int, *array, (uint8_t*)name);
	return ret;
}

/* iface_mch_iter()
 * Iterate through all matchers of 'in' or 'out' list and call 'exec'
 * on each.
 * Returns number of matchers processed.
 */
Word_t iface_mch_iter(struct iface *sk, bool in, void(*exec)(struct matcher *mch))
{
	Pvoid_t *array;
	if (in)
		array = &sk->JS_mch_in;
	else
		array = &sk->JS_mch_out;

	Word_t *PValue;
	uint8_t index[MAXLINELEN] = { '\0' };
	JSLF(PValue, *array, index);
	Word_t ret = 0;
	while (PValue) {
		exec((struct matcher *)(*PValue));
		JSLN(PValue, *array, index);
		ret++;
	}
	return ret;
}
