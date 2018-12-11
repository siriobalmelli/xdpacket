#ifndef offset_defs_h_
#define offset_defs_h_

/*	offset_defs.h
 * A data structure for representing a symbolic offset
 * of an xdpk_field
 */
#include <field.h>
#include <stdint.h>

struct xdpk_offt {
	const char * symbol;
	uint16_t offt;
};

static struct xdpk_offt xdpk_offt_sym[] =
{
	{"udp.sport", 34},
	{"tcp.flags", 46},
};

static int nsyms = sizeof(xdpk_offt_sym)/sizeof(struct xdpk_offt);

#endif /* offset_defs_h_ */
