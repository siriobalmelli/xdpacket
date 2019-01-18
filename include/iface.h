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
#include <parse2.h>
#include <rule.h>


/*	iface_handler_t
 * Function pointer for a packet handler to be registered.
 * @context	: opaque value given at registration
 * @pkt		: pointer to packet being handled
 * @len		: length of 'pkt' in Bytes
 *
 * NOTES:
 * - caller may modify '*pkt' but must NOT call free() on 'pkt'
 */
typedef void (*iface_handler_t)(void *context, void *pkt, size_t len);


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
	/* first cache line: runtime (packet-handling-time) parameters */
	int		fd;
	int		pad;

	int		ifindex;
	int		mtu;

	iface_handler_t	handler;
	void		*context;

	size_t		count_in;
	size_t		count_out;
	size_t		count_checkfail;
	size_t		count_sockdrop;

	struct sockaddr		*hwaddr;
	struct sockaddr_in	*addr;

	/* second cache line: metadata nonessentials */
	char		*name;
	char		*ip_prn;
	size_t		refcnt;
};


void		iface_free	(void *arg);
void		iface_free_all	();
struct iface	*iface_new	(const char *name);

void		iface_release	(struct iface *iface);
struct iface	*iface_get	(const char *name);

int		iface_callback	(int fd,
				uint32_t events,
				void *context);

int	iface_handler_register	(struct iface *iface,
				iface_handler_t handler,
				void *context);

int	iface_handler_clear	(struct iface *iface,
				iface_handler_t handler,
				void *context);

int	iface_output		(struct iface *iface,
				void *pkt,
				size_t plen);


/* integrate into parse2.h
 */
int iface_parse(enum parse_mode	mode,
		yaml_document_t	*doc,
		yaml_node_t	*mapping,
		yaml_document_t	*outdoc,
		int		outlist);

int iface_emit(struct iface	*iface,
		yaml_document_t	*outdoc,
		int		outlist);

#endif /* iface_h_ */
