#ifndef xform2_h_
#define xform2_h_

/*	xform2.h
 *
 * Fast-path structures for manipulating packets.
 * Relies on fields for *all* specification of data location+size
 * (c) 2018 Sirio Balmelli
 */

#include <stdint.h>
#include <field2.h>


enum xform_type {
	XFORM_INVALID = 0,
	XFORM_SET = 1,	/* value -> field */
	XFORM_COPY = 2,	/* field -> field */
	XFORM_OUT = 3	/* out_interface */
};

struct xform_parse {
	enum xform_type type;
	char		*dest; /* e.g.: field name, interface name */
	char		*source; /* e.g.: value, field name */
};


/*	xform_set
 * Describes a block of bytes to be written to the 'dest' field of a packet.
 * NOTE that 'len' for 'bytes' is already contained in 'dest'.
 */
struct xform_set {
	struct field	dest;
	uint8_t		bytes[];
};

/*	xform_copy
 * Copy bytes at 'source' to 'dest'.
 */
struct xform_copy {
	struct field	dest;
	struct field	source;
};

/*	xform_out
 * Describes the comm_fd of an interface where packet should be routed.
 */
struct xform_out {
	int		comm_fd;
};


struct xform {
enum xform_type	type;
union {
	struct xform_set	set;
	struct xform_copy	copy;
	struct xform_out	out;
};
}__attribute__((packed));

struct xform_array {
	size_t		len;
	struct xform	arr[];
};


#endif /* xform2_h_ */
