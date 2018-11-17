#ifndef action2_h_
#define action2_h_

/*	action2.h
 * 
 * Fast-path structures for fields.
 * (c) 2018 Sirio Balmelli
 */
#include <stdint.h>
#include <field2.h>


enum action_type {
	ACTION_INVALID = 0,
	ACTION_SET = 1,	/* value -> field */
	ACTION_COPY = 2,/* field -> field */
	ACTION_OUT = 3	/* out_interface */
};

struct action_parse {
	enum action_type type;
	char		*dest; /* e.g.: field name, interface name */
	char		*source; /* e.g.: value, field name */
};


/*	action_set
 * Describes a block of bytes to be written to the 'dest' field of a packet.
 * NOTE that 'len' for 'bytes' is already contained in 'dest'.
 */
struct action_set {
	struct field	dest;
	uint8_t		bytes[];
};

/*	action_copy
 * Copy bytes at 'source' to 'dest'.
 */
struct action_copy {
	struct field	dest;
	struct field	source;
};

/*	action_out
 * Describes the comm_fd of an interface where packet should be routed.
 */
struct action_out {
	int		comm_fd;
};


struct action {
enum action_type	type;
union {
	struct action_set	set;
	struct action_copy	copy;
	struct action_out	out;
};
}__attribute__((packed));

struct action_array {
	size_t		len;
	struct action	arr[];
};

#endif /* action2_h_ */
