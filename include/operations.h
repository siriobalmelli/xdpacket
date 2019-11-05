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
#include <yamlutils.h>
#include <memref.h>


/*	struct op_set
 * Encoded/compiled form of 'op', optimized for hot path
 */
struct op_set {
	struct field_set	set_to;
	void			*to;
	struct field_set	set_from;
	void			*from;
} __attribute__((packed));

int op_match(struct op_set *op, void *pkt, size_t plen);
int op_write(struct op_set *op, void *pkt, size_t plen);


/*	struct op
 * Encode operation and relevant parameters.
 */
struct op {
	struct field	*dst_field;
	struct memref	*dst;

	struct field	*src_field;
	struct memref	*src;

	struct op_set	set;
};
NLC_ASSERT(struct_op_size, sizeof(struct op) <= 64);

void		op_free		(void *arg);

struct op	*op_new		(struct field	*dst_field_name,
				struct state	*dst_state_name,
				struct field	*src_field_name,
				struct state	*src_state_name,
				char		*src_value);

int		op_emit		(struct op *op,
				yaml_document_t *outdoc,
				int outlist);


#endif /* operations_h_ */
