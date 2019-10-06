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


#if 0
/* TODO: delete this */
#if 1
typedef uint64_t size_t;
#define NULL 0ULL
#define	NLC_INLINE static inline __attribute__((always_inline))
#endif
#endif


struct op_set {
	struct field_set	set_to;
	void			*to;
	struct field_set	set_from;
	void			*from;
} __attribute__((packed));

int op_execute(struct op_set *op, void *pkt, size_t plen);


/*	struct op
 * Encode operation and relevant parameters.
 */
struct op {
	struct field	*field_to;
	struct state	*to;
	struct field	*field_from;
union {
	struct state	*from_state;
	struct field	*from_field;
	char		*from_value;
};
};


#endif /* operations_h_ */
