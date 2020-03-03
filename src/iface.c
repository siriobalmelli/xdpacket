#include <iface.h>
#include <arpa/inet.h>
#include <linux/if_arp.h>	/* struct sockaddr_ll.sll_hatype */
#include <linux/if_ether.h>	/* ETH_P_ALL and friends */
#include <linux/if_packet.h>	/* struct packet_mreq */
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <ndebug.h>
#include <nstring.h>
#include <judyutils.h>
#include <yamlutils.h>

#include <checksums.h>
#include <refcnt.h>


#define XDPK_MAC_PROTO "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx"
#define XDPK_MAC_BYTES(ptr) ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]
#define XDPK_SOCK_PRN(sk_p) "%d: %s %s "XDPK_MAC_PROTO" mtu %d", \
		sk_p->ifindex, sk_p->name, sk_p->ip_prn, \
		sk_p->hwaddr->sa_data[0], sk_p->hwaddr->sa_data[1], sk_p->hwaddr->sa_data[2], \
		sk_p->hwaddr->sa_data[3], sk_p->hwaddr->sa_data[4], sk_p->hwaddr->sa_data[5], \
		sk_p->mtu


static Pvoid_t iface_JS = NULL; /* (char *iface_name) -> (struct iface *iface) */


/*	iface_free()
 */
void iface_free(void *arg)
{
	if (!arg)
		return;
	struct iface *iface = arg;
	NB_die_if(iface->refcnt,
		"iface '%s' free with non-zero refcnt == leak.", iface->name);

	/* we may be a dup: only delete from iface_JS if it points to us */
	if (js_get(&iface_JS, iface->name) == iface)
		js_delete(&iface_JS, iface->name);

	NB_wrn("close "XDPK_SOCK_PRN(iface));

	if (iface->fd != -1)
		close(iface->fd);

	free(iface->addr);
	free(iface->hwaddr);
	free(iface->ip_prn);
	free(iface->name);
	free(iface);
die:
	return;
}

/*	iface_free_all()
 */
void __attribute__((destructor(101))) iface_free_all()
{
	JS_LOOP(&iface_JS,
		NB_wrn("iface not freed, freeing by destructor");
		iface_free(val);
	);
}

/*	iface_new()
 * Open a socket on 'ifname' or return an already open socket.
 */
struct iface *iface_new(const char *name)
{
	struct iface *ret = NULL;
	NB_die_if(!name, "no name given for iface");

#ifdef XDPACKET_DISALLOW_CLOBBER
	NB_die_if(js_get(&iface_JS, name) != NULL,
		"iface '%s' already exists", name);
#else
	/* if already exists, return existing */
	if ((ret = js_get(&iface_JS, name)))
		return ret;
#endif

	NB_die_if(!(
		ret = calloc(sizeof(struct iface), 1)
		), "fail alloc size %zu", sizeof(struct iface));

	errno = 0;
	NB_die_if(!(
		ret->name = nstralloc(name, MAXLINELEN, NULL)
		), "string alloc fail");
	NB_die_if(errno == E2BIG, "value truncated:\n%s", ret->name);

	NB_die_if(!(
		ret->ip_prn = calloc(1, INET_ADDRSTRLEN)
		), "fail alloc size %d", INET_ADDRSTRLEN);
	NB_die_if(!(
		ret->addr = calloc(1, sizeof(*ret->addr))
		), "fail alloc size %zu", sizeof(*ret->addr));
	NB_die_if(!(
		ret->hwaddr = calloc(1, sizeof(*ret->hwaddr))
		), "fail alloc size %zu", sizeof(*ret->hwaddr));

	/* socket */
	ret->fd = -1;
	NB_die_if((
		ret->fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))
		) < 0, "unable to open socket on %s", ret->name);
	struct ifreq ifr = {{{0}}};
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", name);

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
	memcpy(ret->addr, &ifr.ifr_addr, sizeof(*ret->addr));
	NB_die_if(!(
		inet_ntop(ret->addr->sin_family, &ret->addr->sin_addr,
			ret->ip_prn, INET_ADDRSTRLEN)
		), "");
	NB_die_if(
		ioctl(ret->fd, SIOCGIFHWADDR, &ifr)
		, "");
	memcpy(ret->hwaddr, &ifr.ifr_hwaddr, sizeof(*ret->hwaddr));

	/* set promiscuous flag */
	memset(&ifr, 0x0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", name);
	ifr.ifr_ifindex = ret->ifindex;
	NB_die_if(
		ioctl(ret->fd, SIOCGIFFLAGS, &ifr)
		, "");
	ifr.ifr_flags |= IFF_PROMISC;
	NB_die_if(
		ioctl(ret->fd, SIOCSIFFLAGS, &ifr)
		, "");

	/* bind to interface */
	struct sockaddr_ll saddr = {
		.sll_family = AF_PACKET,
		.sll_protocol = htons(ETH_P_ALL),
		.sll_ifindex = ret->ifindex,
		.sll_halen = 6,
		.sll_addr = {
			ret->hwaddr->sa_data[0], ret->hwaddr->sa_data[1], ret->hwaddr->sa_data[2],
			ret->hwaddr->sa_data[3], ret->hwaddr->sa_data[4], ret->hwaddr->sa_data[5]},
		/* ignored by bind call */
		.sll_hatype = 0,
		.sll_pkttype = 0
	};
	NB_die_if(
		bind(ret->fd, (struct sockaddr *)&saddr, sizeof(saddr))
		, "");

	NB_die_if(
		js_insert(&iface_JS, ret->name, ret, true)
		, "");
	NB_inf(XDPK_SOCK_PRN(ret));

	return ret;
die:
	iface_free(ret);
	return NULL;
}


/*	iface_release()
 */
void iface_release(struct iface *iface)
{
	if (!iface)
		return;
	refcnt_release(iface);
}

/*	iface_get()
 * Increments refcount, don't call from inside this file.
 */
struct iface *iface_get(const char *name)
{
	struct iface *ret = js_get(&iface_JS, name);
	if (ret)
		refcnt_take(ret);
	return ret;
}


/*	iface_callback()
 */
int iface_callback(int fd, uint32_t events, void *context)
{
	/* receive packet and discard outgoing packets */
	struct sockaddr_ll addr;
        socklen_t addr_len = sizeof(addr);
	char buf[16384];
	ssize_t res = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&addr, &addr_len);
	if (res < 1)
		return 1;

	/* handle packet */
	struct iface *sk = (struct iface *)context;
	if (addr.sll_pkttype == PACKET_OUTGOING) {
		sk->count_out++;
	} else {
		sk->count_in++;
		if (sk->handler)
			sk->handler(sk->context, buf, res);
	}

	return 0;
}


/*	iface_handler_register()
 */
int iface_handler_register (struct iface *iface, iface_handler_t handler, void *context)
{
	int err_cnt = 0;
	NB_die_if(iface->handler && (iface->handler != handler || iface->context != context)
		, "iface '%s' has existing non-identical handler", iface->name);
	iface->handler = handler;
	iface->context = context;
die:
	return err_cnt;
}


/*	iface_handler_clear()
 */
int iface_handler_clear (struct iface *iface, iface_handler_t handler, void *context)
{
	int err_cnt = 0;
	NB_die_if(!iface
		|| !iface->handler
		|| (iface->handler != handler || iface->context != context)
		, "");
	iface->handler = iface->context = NULL;
die:
	return err_cnt;
}

/*	iface_output()
 */
int iface_output(struct iface *iface, void *pkt, size_t plen)
{
	enum checksum_err ret = checksum(pkt, plen);
	if (ret) {
		NB_wrn("checksum fail of packet size %zu: %s",
			plen, checksum_strerr(ret));
		iface->count_checkfail++;
		/* Dump the contents of an entire packet,
		 * every 128 packets so as not to overload output.
		 */
		if (!(iface->count_checkfail & 0x7f) && plen < 128) {
			NB_dump(pkt, plen, "failed packet:");
		}
		return 1;
	}

	/* We expect hardware to compute FCS (CRC32) for Ethernet.
	 * TODO: test errno values and close iface if socket died (aka: ifdown)
	 */
	if (send(iface->fd, pkt, plen, 0) != plen) {
		NB_wrn("sockdrop (truncation) of packet size %zu", plen);
		iface->count_sockdrop++;
		return 1;
	}

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
	const char *name = "";
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

		if (val->type == YAML_SCALAR_NODE) {
			const char *valtxt = (const char *)val->data.scalar.value;

			if (!strcmp("iface", keyname) || !strcmp("i", keyname))
				name = valtxt;

			else
				NB_err("'iface' does not implement '%s'", keyname);

		} else {
			NB_die("'%s' in iface not a scalar", keyname);
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
		NB_die_if(
			iface_emit(iface, outdoc, outlist)
			, "");
		break;
	}

	case PARSE_DEL:
		NB_die_if(!(
			iface = js_get(&iface_JS, name)
			), "could not get iface '%s'", name);
		NB_die_if(iface->refcnt, "iface '%s' still in use", name);
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
			NB_die_if(
				iface_emit_all(outdoc, outlist)
				, "");
		/* otherwise, search for a literal match */
		} else if ((iface = js_get(&iface_JS, name))) {
			NB_die_if(
				iface_emit(iface, outdoc, outlist)
				, "");
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
		y_pair_insert(outdoc, reply, "iface", iface->name)
		|| y_pair_insert_nf(outdoc, reply, "address", "%s", iface->ip_prn)
		|| y_pair_insert_nf(outdoc, reply, "pkt in", "%zu", iface->count_in)
		|| y_pair_insert_nf(outdoc, reply, "pkt out", "%zu", iface->count_out)
		|| y_pair_insert_nf(outdoc, reply, "pkt drop/truncate", "%zu", iface->count_sockdrop)
		|| y_pair_insert_nf(outdoc, reply, "pkt fail checksum", "%zu", iface->count_checkfail)
		, "");
	NB_die_if(!(
		yaml_document_append_sequence_item(outdoc, outlist, reply)
		), "");

die:
	return err_cnt;
}

/*	iface_emit_all()
 */
int iface_emit_all(yaml_document_t *outdoc, int outlist)
{
	int err_cnt = 0;

	JS_LOOP(&iface_JS,
		NB_die_if(
			iface_emit(val, outdoc, outlist)
			, "");
	);

die:
	return err_cnt;
}
