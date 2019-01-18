/*	checksums.c
 * (c) 2018 Sirio Balmelli
 */

#include <checksums.h>


/*	l4_checksum()
 * Returns pointer to l4 header, for convenience.
 */
void *l4_checksum(struct iphdr *l3)
{
	if (!l3)
		return NULL;

	void *l4;
	uint8_t *protocol;
	uint32_t sum;
	size_t data_len;

	/* ipv4 pseudo-header
	 */
	if (l3->version == 4) {
		protocol = &l3->protocol;
		size_t head_len = ((uint16_t)l3->ihl << 2); /* ihl is in 32-bit words; `<<2` == `*4` */
		data_len = be16toh(l3->tot_len) - head_len;
		l4 = (void *)l3 + head_len; 
		struct pseudo_ip4 pseudo = {
			.saddr = l3->saddr,
			.daddr = l3->daddr,
			.reserved = 0,
			.protocol = l3->protocol,
			.data_len = h16tobe(data_len)
		};
		sum = ones_sum(&pseudo, sizeof(pseudo), 0);

	/* ipv6 pseudo-header
	 * NOTE: does NOT support _all_ ipv6 packets as there may _be_ a "next header",
	 * but it's good enough for now.
	 */
	} else if (l3->version == 6) {
		struct ipv6hdr *l3v6 = (struct ipv6hdr *)l3;
		protocol = &l3v6->nexthdr;
		data_len = be16toh(l3v6->payload_len);
		l4 = l3 + sizeof(*l3v6); /* ipv6 header is fixed-length */
		struct pseudo_ip6 pseudo = {
			.saddr = l3v6->saddr,
			.daddr = l3v6->daddr,
			.data_len = h16tobe(data_len),
			.nexthdr = l3v6->nexthdr,
			.zero = { 0x0 }
		};
		sum = ones_sum(&pseudo, sizeof(pseudo), 0);

	} else {
		return NULL;
	}


	/* checksum is placed differently for TCP vs UDP */
	if (*protocol == IPPROTO_TCP) {
		struct tcphdr *header = l4;
		header->check = 0;
		header->check = ones_final(ones_sum(l4, data_len, sum));

	/* UDP checksum cannot be zero ... handle corner case */
	} else if (*protocol == IPPROTO_UDP) {
		struct udphdr *header = l4;
		header->check = 0;
		header->check = ones_final(ones_sum(l4, data_len, sum));
		if (!header->check)
			header->check = 0xffff;

	}
	
	return l4;
}



/*	l3_checksum()
 * Compute l3 (usually IP) checksum of a packet.
 * Returns pointer to l3 header, which can e.g. passed to l4_checksum().
 * NULL if no checksum could be computed.
 */
void *l3_checksum(struct ethhdr *l2)
{
	/* only IP supported right now */
	if (!l2 || be16toh(l2->h_proto) != ETH_P_IP)
		return NULL;
	struct iphdr *l3 = (void *)l2 + sizeof(*l2);

	/* ipv6 does not have a checksum */
	if (l3->version != 4)
		return l3;

	/* NOTE: ihl is header length is in 32-bit words,
	 * multiply by 4 to get header size in bytes.
	 */
	l3->check = 0;
	uint32_t sum = ones_sum(l3, (uint16_t)l3->ihl << 2, 0);
	l3->check = ones_final(sum);

	return l3;
}
