#ifndef operations_h_
#define operations_h_

/*	operations.h
 *
 * The set of primitives that can be applied to packets or global state.
 *
 * (c) 2019 Sirio Balmelli
 */

#include <field.h>
#include <state.h>
#include <stddef.h>  /* size_t */
#include <nonlibc.h>


/* encoded in 'flags' byte of op->set */
enum op_type {
	OP_MATCH	= 0x10,
	OP_COPY		= 0x20,
	OP_MOVE		= 0x40
};

struct op_set {
	struct field_set	set;
	void			*where;
union {
	void			*from;
	struct field_set	set_from;
};
} __attribute__((packed));

int op_execute(struct op_set *op, void *pkt, size_t plen);


/*	struct op
 * Encode operation and relevant parameters.
 */
struct op {
	enum op_type	type;
	struct field	*field;
	struct state	*where;
union {
	struct state	*from_state;
	struct field	*from_field;
	char		*literal;
};
};


#endif /* operations_h_ */
