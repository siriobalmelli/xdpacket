#ifndef iface_h_
#define iface_h_

/*	iface2.h
 * An iface is a promiscuous socket on an actual system network interface.
 * The entry point for packet handling is iface_callback() invoked by epoll()
 * on data ready to read/write.
 * (c) 2018 Sirio Balmelli
 */

#include <xdpacket.h>
#include <unistd.h>
#include <stdint.h>
#include <netinet/in.h> /* sockaddr_in */
#include <epoll_track.h>
#include <hook.h>
#include <parse2.h>


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
	char		ip_prn[MAXLINELEN];
	/* taken directly from struct ifreq: */
	int		ifindex;
	int		mtu;
	struct sockaddr	hwaddr;
	struct sockaddr_in addr;
	char		name[MAXLINELEN];

	struct hook	*in;
	struct hook	*out;
};


void		iface_free	(struct iface *sk);

struct iface	*iface_new	(const char *ifname);

void		iface_callback	(int fd,
				uint32_t events,
				epoll_data_t context);


int iface_parse(enum parse_mode mode, yaml_document_t *doc, yaml_node_t *node);


#endif /* iface_h_ */
