#include <operations.h>
#include <nonlibc.h>
#include <ndebug.h>


/*	op_pkt_offset()
 * Validates 'set' for 'pkt' with length 'plen'.
 *
 * Returns pointer to start of data (applies offset in 'set') if valid,
 * otherwise NULL.
 */
NLC_INLINE
void *op_pkt_offset(void *pkt, size_t plen, struct field_set set)
{
	/* Offset sanity.
	 * 'offt' may be negative, in which case it denotes offset from
	 * the end of the packet.
	 */
	void *where = pkt + set.offt;
	if (set.offt < 0)
		where += plen;
	if (where < pkt)
		return NULL;

	/* Size sanity. */
	size_t flen = set.len;
	if ((where + flen) > (pkt + plen))
		return NULL;

	return where;
}


/*	op_common()
 * Common sanity and offset code for operations.
 */
NLC_INLINE
int op_common(struct op_set *op, void *pkt, size_t plen,
			size_t *out_len, uint8_t **out_to, uint8_t **out_from)
{
	*out_len = op->set_to.len > op->set_from.len ? op->set_to.len : op->set_from.len;
	*out_len -= 1; /* IMPORTANT: last byte is copied/matched through a mask! */
	*out_to = op->to;
	*out_from = op->from;

	/* If not provided with addresses (aka pointers to state),
	 * point into local packet.
	 * This may fail if packet e.g. is not big enough.
	 */
	if (!*out_to && !(*out_to = op_pkt_offset(pkt, plen, op->set_to)))
		return 1;
	if (!*out_from && !(*out_from = op_pkt_offset(pkt, plen, op->set_from)))
		return 1;
	return 0;
}


/*	op_match()
 */
int __attribute__((hot)) op_match(struct op_set *op, void *pkt, size_t plen)
{
	size_t len;
	uint8_t *to;
	uint8_t *from;
	if (op_common(op, pkt, plen, &len, &to, &from))
		return 1;

	if (memcmp(to, from, len))
		return 1;
	if ((to[len] & op->set_to.mask) != (from[len] & op->set_from.mask))
		return 1;
	return 0;
}


/*	op_write()
 */
int __attribute__((hot)) op_write(struct op_set *op, void *pkt, size_t plen)
{
	size_t len;
	uint8_t *to;
	uint8_t *from;
	if (op_common(op, pkt, plen, &len, &to, &from))
		return 1;

	memcpy(to, from, len);
	to[len] = (to[len] & ~op->set_to.mask) | /* respect existing bits untouched by dst mask */
		((from[len] & op->set_to.mask) & op->set_from.mask);

	return 0;
}

/*	op_free()
 */
void op_free (void *arg)
{

}

/*	op_new()
 */
struct op *op_new (struct field *dst_field_name, struct state *dst_state_name,
		struct field *src_field_name, struct state *src_state_name,
		char *src_value)
{
	/* TODO: implement */
	return NULL;
}

/*	op_emit()
 */
int op_emit (struct op *op, yaml_document_t *outdoc, int outlist)
{
	int err_cnt = 0;
	int reply = yaml_document_add_mapping(outdoc, NULL, YAML_BLOCK_MAPPING_STYLE);
	int dst = yaml_document_add_mapping(outdoc, NULL, YAML_BLOCK_MAPPING_STYLE);
	int src = yaml_document_add_mapping(outdoc, NULL, YAML_BLOCK_MAPPING_STYLE);

	if (op->dst_field) {
		NB_die_if(
			y_pair_insert(outdoc, dst, "field", op->dst_field->name)
			, "failed to emit dst->field");
	}
	NB_die_if(
		memref_emit(op->dst, outdoc, dst)
		, "");
	NB_die_if(
		y_pair_insert_obj(outdoc, reply, "dst", dst)
		, "failed to emit 'dst'");

	if (op->src_field) {
		NB_die_if(
			y_pair_insert(outdoc, src, "field", op->src_field->name)
			, "failed to emit src->field");
	}
	NB_die_if(
		memref_emit(op->src, outdoc, src)
		, "");
	NB_die_if(
		y_pair_insert_obj(outdoc, reply, "src", src)
		, "failed to emit 'src'");

	NB_die_if(!(
		yaml_document_append_sequence_item(outdoc, outlist, reply)
		), "");
die:
	return err_cnt;
}
