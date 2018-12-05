#include <iface.h>
#include <arpa/inet.h>
#include <linux/if_arp.h>	/* struct sockaddr_ll.sll_hatype */
#include <linux/if_ether.h>	/* ETH_P_ALL and friends */
#include <linux/if_packet.h>	/* struct packet_mreq */
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>


#define XDPK_MAC_PROTO "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx"
#define XDPK_MAC_BYTES(ptr) ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]
#define XDPK_SOCK_PRN(sk_p) "%d: %s %s "XDPK_MAC_PROTO" mtu %d", \
		sk_p->ifindex, sk_p->name, sk_p->ip_prn, \
		sk_p->hwaddr.sa_data[0], sk_p->hwaddr.sa_data[1], sk_p->hwaddr.sa_data[2], \
		sk_p->hwaddr.sa_data[3], sk_p->hwaddr.sa_data[4], sk_p->hwaddr.sa_data[5], \
		sk_p->mtu


/*	iface_free()
 */
void iface_free(struct iface *sk)
{
	if (!sk)
		return;
	Z_log(Z_inf, "close "XDPK_SOCK_PRN(sk));

	hook_free(sk->in);
	hook_free(sk->out);

	if (sk->fd != -1)
		close(sk->fd);

	free(sk);
}

/*	iface_open()
 * open an interface
 */
struct iface *iface_new(const char *ifname)
{
	struct iface *ret = NULL;
	Z_die_if(!ifname, "cannot create interface without name");
	Z_die_if(!(
		ret = calloc(sizeof(struct iface), 1)
		), "alloc size %zu", sizeof(struct iface));
	snprintf(ret->name, IFNAMSIZ, "%s", ifname);

	/* hooks */
	Z_die_if(!(
		ret->in = hook_new()
		) || !(
		ret->out = hook_new()
		), "");

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
	struct hook *hk = sk->in;
	if (addr.sll_pkttype == PACKET_OUTGOING)
		hk = sk->out;
	hook_callback(hk, buf, res);
}



Pvoid_t	iface_parse_JS; /* (char *iface_name) -> (struct iface_parse *parse) */

/*	iface_parse()
 * Parse 'root' according to 'mode' (add | rem | prn).
 * Returns 0 on success.
 */
int iface_parse(enum parse_mode mode, yaml_document_t *doc, yaml_node_t *node)
{
	int err_cnt = 0;
	const char *name = NULL;
	struct iface *iface = NULL;

	/* parse node */
	for (yaml_node_pair_t *pair = node->data.mapping.pairs.start;
		pair < node->data.mapping.pairs.top;
		pair++)
	{
		yaml_node_t *key = yaml_document_get_node(doc, pair->key);
		const char *keyname = (const char *)key->data.scalar.value;
		yaml_node_t *val = yaml_document_get_node(doc, pair->value);
		const char *valtxt = (const char *)val->data.scalar.value;

		/* sanity */
		if (val->type != YAML_SCALAR_NODE) {
			Z_log_err("'%s' in iface not a scalar.\n"
				"Example iface node:\n"
				"```yaml\n"
				"iface: eth0\n"
				"```\n",
				keyname);
			continue;
		}

		/* Match field names and populate 'local' */
		if (!strcmp("iface", keyname)) {
			name = valtxt;
		} else {
			Z_log_err("'iface' does not implement '%s'", keyname);
		};
	}

	/* execute mode */
	switch (mode) {
	case PARSE_ADD:
	{
		Z_die_if(!(
			iface = iface_new(name)
			), "");
		/* TODO: add to JS array */
		break;
	}
	case PARSE_DEL:
		/* TODO: find and delete */
		break;
	case PARSE_PRN:
		/* TODO: treat 'name' as a regex for filtering interfaces to print */
		break;
	default:
		Z_log_err("unknown mode %s", parse_mode_prn(mode));
	};

out:
	return err_cnt;
}
