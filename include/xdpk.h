#ifndef xdpk_h_
#define xdpk_h_

#include <unistd.h>
#include <stdint.h>

#define XDPK_FRAME_SZ (4096 * 3) /* larger than 9k jumbo frame, multiple of 4k pagesize */
#define XDPK_FRAME_CNT 1024 /* default number of frames in buffer */

struct xdpk_sock {
	int fd;
	int ifidx;
	char *ifname;
	size_t umem_cnt;
	uint8_t umem[];
};

#endif /* xdpk_h_ */
