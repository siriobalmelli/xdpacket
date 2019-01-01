#include <iface.h>
#include <arpa/inet.h>
#include <linux/if_arp.h>	/* struct sockaddr_ll.sll_hatype */
#include <linux/if_ether.h>	/* ETH_P_ALL and friends */
#include <linux/if_packet.h>	/* struct packet_mreq */
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ndebug.h>
#include <judyutils.h>
#include <yamlutils.h>

#define XDPK_MAC_PROTO "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx"
#define XDPK_MAC_BYTES(ptr) ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]
#define XDPK_SOCK_PRN(sk_p) "%d: %s %s "XDPK_MAC_PROTO" mtu %d", \
		sk_p->ifindex, sk_p->name, sk_p->ip_prn, \
		sk_p->hwaddr.sa_data[0], sk_p->hwaddr.sa_data[1], sk_p->hwaddr.sa_data[2], \
		sk_p->hwaddr.sa_data[3], sk_p->hwaddr.sa_data[4], sk_p->hwaddr.sa_data[5], \
		sk_p->mtu


static Pvoid_t iface_JS = NULL; /* (char *iface_name) -> (struct iface *iface) */


/*	iface_free()
 */
void iface_free(void *arg)
{
	if (!arg)
		return;
	struct iface *sk = arg;
	js_delete(&iface_JS, sk->name);
	NB_wrn("close "XDPK_SOCK_PRN(sk));

	hook_free(sk->in);
	hook_free(sk->out);

	if (sk->fd != -1)
		close(sk->fd);

	free(sk);
}

/*	iface_free_all()
 */
static void __attribute__((destructor)) iface_free_all()
{
	JS_LOOP(&iface_JS,
		NB_wrn("iface not freed, freeing by destructor");
		iface_free(val);
		);
}

/*	iface_new()
 * Open a socket on 'ifname' or return an already open socket.
 */
struct iface *iface_new(const char *ifname)
{
	struct iface *ret = NULL;
	NB_die_if(!ifname, "no name given for iface");

	/* if already exists, return existing */
	if ((ret = js_get(&iface_JS, ifname)))
		return ret;

	NB_die_if(!(
		ret = calloc(sizeof(struct iface), 1)
		), "alloc size %zu", sizeof(struct iface));
	snprintf(ret->name, IFNAMSIZ, "%s", ifname);

	/* hooks */
	NB_die_if(!(
		ret->in = hook_new()
		) || !(
		ret->out = hook_new()
		), "");

	/* socket */
	ret->fd = -1;
	NB_die_if((
		ret->fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))
		) < 0, "unable to open socket on %s", ret->name);
	struct ifreq ifr = {{{0}}};
	snprintf (ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);

	/* grok interface
	 * ordered by increasing field size: avoid zeroing the union between calls
	 */
	NB_die_if(
		ioctl(ret->fd, SIOCGIFINDEX, &ifr)
		, "");
	ret->ifindex = ifr.ifr_ifindex;
	NB_die_if(
		ioctl(ret->fd, SIOCGIFMTU, &ifr)
		, "");
	ret->mtu = ifr.ifr_mtu;
	NB_die_if(
		ioctl(ret->fd, SIOCGIFADDR, &ifr)
		, "");
	memcpy(&ret->addr, &ifr.ifr_addr, sizeof(ret->addr));
	NB_die_if(!(
		inet_ntop(ret->addr.sin_family, &ret->addr.sin_addr,
			ret->ip_prn, sizeof(ret->ip_prn))
		), "");
	NB_die_if(
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
	NB_die_if(
		bind(ret->fd, (struct sockaddr *)&saddr, sizeof(saddr))
		, "");

	NB_die_if(
		js_insert(&iface_JS, ret->name, ret)
		, "");
	NB_inf("add "XDPK_SOCK_PRN(ret));

	return ret;
die:
	iface_free(ret);
	return NULL;
}


/*	iface_callback()
 */
int iface_callback(int fd, uint32_t events, epoll_data_t context)
{
	/* receive packet and discard outgoing packets */
	struct sockaddr_ll addr;
        socklen_t addr_len = sizeof(addr);
	char buf[16384];
	ssize_t res = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&addr, &addr_len);
	if (res < 1)
		return 1;

	/* handle packet */
	struct iface *sk = (struct iface *)context.ptr;
	struct hook *hk = sk->in;
	if (addr.sll_pkttype == PACKET_OUTGOING)
		hk = sk->out;
	hook_callback(hk, buf, res);

	return 0;
}



/*	iface_parse()
 * Parse 'root' according to 'mode' (add | rem | prn).
 * Returns 0 on success.
 */
int iface_parse(enum parse_mode	mode,
		yaml_document_t *doc, yaml_node_t *mapping,
		yaml_document_t *outdoc, int outlist)
{
	int err_cnt = 0;
	const char *name = NULL;
	struct iface *iface = NULL;

	/* parse mapping */
	for (yaml_node_pair_t *pair = mapping->data.mapping.pairs.start;
		pair < mapping->data.mapping.pairs.top;
		pair++)
	{
		/* loop boilerplate */
		yaml_node_t *key = yaml_document_get_node(doc, pair->key);
		const char *keyname = (const char *)key->data.scalar.value;

		yaml_node_t *val = yaml_document_get_node(doc, pair->value);
		NB_die_if(val->type != YAML_SCALAR_NODE,
			"'%s' in iface not a scalar", keyname);
		const char *valtxt = (const char *)val->data.scalar.value;

		/* Match field names and populate 'local' */
		if (!strcmp("iface", keyname)
			|| !strcmp("i", keyname))
		{
			name = valtxt;
		} else {
			NB_err("'iface' does not implement '%s'", keyname);
		}
	}

	/* process based on 'mode' */
	switch (mode) {
	case PARSE_ADD:
	{
		NB_die_if(!(
			iface = iface_new(name)
			), "");
		NB_die_if(
			eptk_register(tk, iface->fd, EPOLLIN, iface_callback, iface, iface_free)
			, "could not register epoll on '%s'", iface->name);
		NB_die_if(iface_emit(iface, outdoc, outlist), "");
		break;
	}

	case PARSE_DEL:
		NB_die_if(!(
			iface = js_get(&iface_JS, name)
			), "could not get iface '%s'", name);
		NB_die_if(
			iface_emit(iface, outdoc, outlist)
			, "");
		/* rely on eptk_remove() calling iface_free() (destructor),
		 * which will also remove it from the JS array.
		 */
		NB_die_if((
			eptk_remove(tk, iface->fd)
			) != 1, "could not remove '%s'", name);
		break;

	case PARSE_PRN:
		/* if nothing is given, print all */
		if (!strcmp("", name)) {
			JS_LOOP(&iface_JS,
				NB_die_if(
					iface_emit(val, outdoc, outlist)
					, "");
				);
		/* otherwise, search for a literal match */
		} else if ((iface = js_get(&iface_JS, name))) {
			NB_die_if(iface_emit(iface, outdoc, outlist), "");
		}
		break;

	default:
		NB_err("unknown mode %s", parse_mode_prn(mode));
	};

die:
	return err_cnt;
}

/*	iface_emit()
 * Emit an interface as a mapping under 'outlist' in 'outdoc'.
 */
int iface_emit(struct iface *iface, yaml_document_t *outdoc, int outlist)
{
	int err_cnt = 0;
	int reply = yaml_document_add_mapping(outdoc, NULL, YAML_BLOCK_MAPPING_STYLE);
	NB_die_if(
		y_insert_pair(outdoc, reply, "iface", iface->name)
		, "");
	NB_die_if(!(
		yaml_document_append_sequence_item(outdoc, outlist, reply)
		), "");
die:
	return err_cnt;
}
