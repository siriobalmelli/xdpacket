#ifndef packet_h_
#define packet_h_

/*	packet.h
 * A data structure for representing a packet
 * (c) 2018 Alan Morrissett and Sirio Balmelli
 */
#include <field.h>
#include <stdint.h>

struct pkt {
	size_t len;
	uint8_t * data;
};

NLC_PUBLIC
	void init_pkts(char *cpkts[], int npkts, struct pkt **pkts);

NLC_PUBLIC
	void free_pkts(int npkts, struct pkt **pkts);

NLC_PUBLIC
	void dump_pkt(struct pkt *pkt);

#endif /* packet_h_ */
