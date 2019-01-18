#ifndef checksums_h_
#define checksums_h_

/*	checksums.h
 * Network checksumming for packets being output.
 *
 * TODO:
 * - write tests for every protocol combination
 * - make into a standalone library
 *
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


/*	pseudo_ip4
 * Fake header used when computing tcp/udp checksum of ipv4 packets.
 */
struct pseudo_ip4 {
	uint32_t	saddr;
	uint32_t	daddr;
	uint8_t		reserved;
	uint8_t		protocol;
	uint16_t	data_len;
}__attribute__((packed));

NLC_ASSERT(pseudo_ip4_size, sizeof(struct pseudo_ip4) == 12);


/*	pseudo_ip6
 * Fake header used when computing tcp/udp checksum of ipv6 packets.
 */
struct pseudo_ip6 {
	struct	in6_addr	saddr;
	struct	in6_addr	daddr;
	uint32_t		data_len;
	uint8_t			zero[3];
	uint8_t			nexthdr;
}__attribute__((packed));

NLC_ASSERT(pseudo_ip6_size, sizeof(struct pseudo_ip6) == 40);


void *l4_checksum(struct iphdr *l3);

void *l3_checksum(struct ethhdr *l2);


#endif /* checksums_h_ */
