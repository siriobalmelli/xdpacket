#ifndef checksums_h_
#define checksums_h_

/*	checksums.h
 * Network checksumming for packets being output.
 * (c) 2018 Sirio Balmelli
 */

#include <ndebug.h>
#include <nonlibc.h>
#include <stdint.h>
#include <linux/if_ether.h>	/* struct ethhdr */
#include <linux/ip.h>		/* struct iphdr */
#include <linux/ipv6.h>		/* struct iphdr */
#include <linux/udp.h>
#include <linux/tcp.h>
#include <netinet/in.h>

/*	l4_checksum()
 * Returns pointer to l4 header, for convenience.
 */
NLC_INLINE void	*l4_checksum(struct iphdr *l3)
{
	if (!l3)
		return NULL;

	uint8_t *protocol = 0;
	void *l4 = NULL;

	if (l3->version == 4) {
		protocol = &l3->protocol;
		l4 = (void *)l3 + (l3->ihl << 2); /* ihl is in 32-bit words; `<<2` == `*4` */

	/* This does NOT support _all_ ipv6 packets as there may _be_ a
	 * "next header" but it's good enough for now.
	 */
	} else if (l3->version == 6) {
		struct ipv6hdr *l3v6 = (struct ipv6hdr *)l3;
		protocol = &l3v6->nexthdr;
		l4 = l3 + sizeof(*l3v6); /* ipv6 header is fixed-length */
	}

	if (*protocol == IPPROTO_TCP) {
		NB_wrn("TCP checksum not (yet) implemented");

	} else if (*protocol == IPPROTO_UDP) {
		struct udphdr *udp = l4;
		NB_wrn("UDP checksum not (yet) implemented");
	}

	return l4;
}

/*	l3_checksum()
 * Compute l3 (usually IP) checksum a packet.
 * Returns pointer to l3 header (for convenience),
 * NULL if no checksum could be computed.
 */
NLC_INLINE void *l3_checksum(struct ethhdr *l2)
{
	/* only IP supported right now */
	if (!l2 || be16toh(l2->h_proto) != ETH_P_IP)
		return NULL;
	struct iphdr *l3 = (void *)l2 + sizeof(*l2);

	/* ipv6 does not have a checksum */
	if (l3->version != 4)
		return l3;

	l3->check = 0;
	uint32_t sum = 0;
	/* Speed up ones-complement sum by adding all into 32-bit and then
	 * "carrying" overflow twice.
	 * NOTE: ihl is header length is in 32-bit words,
	 * multiply by 2 to get header size as 16-bit words.
	 */
	for (unsigned int i=0; i < (l3->ihl << 1); i++)
		sum += ((uint16_t *)l3)[i];
        sum = (sum >> 16) + (sum & 0xFFFF);
        sum += (sum >> 16);
        l3->check = ~sum; /* ones-complement (NOT) of the sum */

	/* NOTE: I don't conceptually understand why this doesn't seem to
	 * require byteswapping at the end.
	 */
	return l3;
}


#endif /* checksums_h_ */
