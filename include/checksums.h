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


enum checksum_err {
	CHK_OK = 0,
	CHK_INVAL,	/* provided values are not sane */
	CHK_UNSUPPORTED,/* protocol or extension not supported */
	CHK_MALFORMED	/* malformed (invalid) header data */
};

const char *checksum_strerr(enum checksum_err error);

enum checksum_err checksum(void *frame, size_t len);


#endif /* checksums_h_ */
