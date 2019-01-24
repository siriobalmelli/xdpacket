#ifndef checksums_h_
#define checksums_h_

/*	checksums.h
 * Network checksumming for packets being output.
 *
 * TODO:
 * - test ipv6 protocols
 * - make into a standalone library ?
 *
 * (c) 2018 Sirio Balmelli
 */

#include <ndebug.h>
#include <nonlibc.h>


int checksum(void *frame, size_t len);


#endif /* checksums_h_ */
