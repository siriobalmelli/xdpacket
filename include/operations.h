#ifndef operations_h_
#define operations_h_

/*	operations.h
 *
 * The set of primitives that can be applied to packets or global state.
 *
 * (c) 2019 Sirio Balmelli
 */

#include <field.h>
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

/**
 * return 0 if 'pkt' matches 'op'
 */
int op_match(struct op_set *op, const void *pkt, size_t plen);
/**
 * return 0 if 'op' could be applied (written) to 'pkt'
 */
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
NLC_ASSERT(struct_op_size, sizeof(struct op) <= NLC_CACHE_LINE);

void		op_free		(void *arg);

struct op	*op_new		(const char	*dst_field_name,
				const char	*dst_state_name,
				const char	*src_field_name,
				const char	*src_state_name,
				const char	*src_value);

struct op	*op_parse_new	(yaml_document_t *doc,
				yaml_node_t *mapping);

int		op_emit		(struct op *op,
				yaml_document_t *outdoc,
				int outlist);


#endif /* operations_h_ */
