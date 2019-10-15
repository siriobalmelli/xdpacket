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
#include <yamlutils.h>


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
	char		*from_value;
};
};


void		op_free		(void *arg);

struct op	*op_new		(const char *field_to_name,
				const char *state_to_name,
				const char *field_from_name,
				const char *from);

int		op_emit		(struct op *op,
				yaml_document_t *outdoc,
				int outlist);


#endif /* operations_h_ */
