#ifndef checksums_h_
#define checksums_h_

/*	checksums.h
 * Network checksumming for packets being output.
 *
 * TODO:
 * - test all protocols
 * - make into a standalone library ?
 *
 * (c) 2018 Sirio Balmelli
 */

#include <ndebug.h>
#include <nonlibc.h>
#include <stdint.h>

#include <netinet/in.h>
#include <linux/if_ether.h>	/* struct ethhdr */
#include <linux/ip.h>		/* struct iphdr */
#include <linux/ipv6.h>		/* struct ipv6hdr */
#include <linux/icmp.h>		/* struct icmphdr */
#include <linux/icmpv6.h>	/* struct icmp6hdr */
#include <linux/udp.h>		/* struct udphdr */
#include <linux/tcp.h>		/* struct tcphdr */


/*	pseudo headers
 * Fake headers used when computing tcp/udp checksum.
 */
struct pseudo_ip4 {
	uint32_t	saddr;
	uint32_t	daddr;
	uint8_t		reserved;
	uint8_t		protocol;
	uint16_t	data_len;
}__attribute__((packed));
NLC_ASSERT(pseudo_ip4_size, sizeof(struct pseudo_ip4) == 12);

struct pseudo_ip6 {
	struct	in6_addr	saddr;
	struct	in6_addr	daddr;
	uint32_t		data_len;
	uint8_t			zero[3];
	uint8_t			nexthdr;
}__attribute__((packed));
NLC_ASSERT(pseudo_ip6_size, sizeof(struct pseudo_ip6) == 40);


/*	ones_sum()
 * Add 'byte_count' bytes to 'sum'.
 * 'sum' must be 0 if this is the first block being summed, otherwise it should
 * be the previous output of a call to ones_sum().
 */
NLC_INLINE uint32_t ones_sum(void *blocks, size_t byte_count, uint32_t sum)
{
	size_t block_cnt = byte_count >> 1;
	for (unsigned int i=0; i < block_cnt; i++)
		sum += ((uint16_t *)blocks)[i];

	/* Trailing byte.
	 * A simple addition would _also_ work, but only on little-endian machines
	 * (subtle).
	 */
	if (byte_count & 0x1) {
		uint8_t padded[2] = { ((uint8_t *)blocks)[byte_count -1], 0x0 };
		sum += *((uint16_t *)padded);
	}

	return sum;
}

/*	ones_final()
 * Produce the ones-complement of the ones-complement sum.
 * See RFC1071 for full implementation details;
 */
NLC_INLINE uint16_t ones_final(uint32_t sum)
{
	/* Speed up ones-complement sum by adding all into 32-bit and then
	 * "carrying" overflow twice.
	 */
        sum = (sum >> 16) + (sum & 0xFFFF);
        sum = (sum >> 16) + (sum & 0xFFFF);
        return ~sum; /* ones-complement (NOT) of the sum */
}


/*	checksum()
 * Calculate IP and protocol (tcp/udp) checksums on an ethernet frame.
 * Returns 0 on success (was able to calculate all checksums).
 * NOTE: all words are network (big-endian) byte order.
 */
NLC_INLINE int checksum(void *frame, size_t len)
{
	struct ethhdr *l2 = frame;

	struct iphdr *l3 = frame + sizeof(*l2);
	uint16_t head_len; /* length of ip(4|6) header */
	uint8_t *l3_proto; /* location of protocol/next_header */

	union l4 {
		void		*ptr;
		struct tcphdr	*tcp;
		struct udphdr	*udp;
		struct icmphdr	*icmp;
		struct icmp6hdr	*icmp6;
	};
	union l4 l4 = { NULL };
	uint16_t l4_len; /* length of tcp/udp header plus payload */
	uint32_t l4_sum; /* accumulator for 1s complement sum of:
			  * - pseudo-header
			  * - l4 header
			  * - payload
			  */


	/* sanity check provided pointer and length values */
	if (!l2 || len <= (sizeof(*l2) + sizeof(*l3)) || len > UINT16_MAX)
		return -1;

	/* verify protocol */
	if (be16toh(l2->h_proto) != ETH_P_IP)
		return -1;


	/* l3 == ipv4
	 */
	if (l3->version == 4) {
		/* verify length values supplied in ipv4 header.
		 * NOTE: ihl is header length is in 32-bit words,
		 * multiply by 4 to get header size in bytes.
		 */
		head_len = (uint16_t)l3->ihl << 2;
		if (head_len < 20 || head_len > 60)
			return -1;
		l4.ptr = (void *)l3 + head_len;
		l4_len = be16toh(l3->tot_len) - head_len;
		if (l4_len > (len - sizeof(*l2) - head_len))
			return -1;

		l3_proto = &l3->protocol;

		/* ipv4 has header checksum */
		l3->check = 0;
		l3->check = ones_final(ones_sum(l3, head_len, 0));

		/* sum pseudo-header */
		struct pseudo_ip4 pseudo = {
			.saddr = l3->saddr,
			.daddr = l3->daddr,
			.reserved = 0,
			.protocol = l3->protocol,
			.data_len = h16tobe(l4_len)
		};
		l4_sum = ones_sum(&pseudo, sizeof(pseudo), 0);


	/* l3 == ipv6
	 * ipv6 is a fixed-length header with no header checksum.
	 * NOTE: does NOT support _all_ ipv6 packets as there may _be_ a "next header",
	 * but it's good enough for now.
	 */
	} else if (l3->version == 6) {
		struct ipv6hdr *l3 = frame + sizeof(*l2); /* clobber for clarity */
		head_len = 40;
		l4.ptr = (void *)l3 + head_len;
		l4_len = be16toh(l3->payload_len);
		if (l4_len > (len - sizeof(*l2) - head_len))
			return -1;

		l3_proto = &l3->nexthdr;

		struct pseudo_ip6 pseudo = {
			.saddr = l3->saddr,
			.daddr = l3->daddr,
			.data_len = l3->payload_len,
			.nexthdr = l3->nexthdr,
			.zero = { 0x0 }
		};
		l4_sum = ones_sum(&pseudo, sizeof(pseudo), 0);


	/* malformed IP protocol */
	} else {
		return -1;
	}


	/* l4 checksums by protocol
	 */
	if (*l3_proto == IPPROTO_TCP) {
		l4.tcp->check = 0;
		l4.tcp->check = ones_final(ones_sum(l4.ptr, l4_len, l4_sum));

	} else if (*l3_proto == IPPROTO_UDP) {
		l4.udp->check = 0;
#if 1
		l4.udp->check = ones_final(ones_sum(l4.ptr, l4_len, l4_sum));
		/* '0x0000' checksum value not allowed by the standard,
		 * since it means "not implemented"
		 */
		if (!l4.udp->check)
			l4.udp->check = 0xffff;
#endif

	/* ICMPv4 checksum does not include pseudo-header */
	} else if (*l3_proto == IPPROTO_ICMP) {
		l4.icmp->checksum = 0;
		l4.icmp->checksum = ones_final(ones_sum(l4.ptr, l4_len, 0));

	/* ICMPv6, on the other hand, does */
	} else if (*l3_proto == IPPROTO_ICMPV6) {
		l4.icmp6->icmp6_cksum = 0;
		l4.icmp6->icmp6_cksum = ones_final(ones_sum(l4.ptr, l4_len, l4_sum));

	} else {
		return -1;
	}

	return 0;
}

#endif /* checksums_h_ */
