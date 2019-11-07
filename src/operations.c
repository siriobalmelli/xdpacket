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
const void *op_pkt_offset(const void *pkt, size_t plen, struct field_set set)
{
	/* Offset sanity.
	 * 'offt' may be negative, in which case it denotes offset from
	 * the end of the packet.
	 */
	const void *where = pkt + set.offt;
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
int op_common(struct op_set *op, const void *pkt, size_t plen,
			size_t *out_len, const uint8_t **out_to, const uint8_t **out_from)
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
int __attribute__((hot)) op_match(struct op_set *op, const void *pkt, size_t plen)
{
	size_t len;
	const uint8_t *to;
	const uint8_t *from;
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
	const uint8_t *from;
	if (op_common(op, pkt, plen, &len, (const uint8_t **)&to, &from))
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
	if (!arg)
		return;
	struct op *op = arg;
	
	field_release(op->dst_field);
	field_release(op->src_field);
	memref_release(op->dst);
	memref_release(op->src);

	free(op);
}

/*	op_new()
 */
struct op *op_new (const char *dst_field_name, const char *dst_state_name,
		const char *src_field_name, const char *src_state_name, const char *src_value)
{
	struct op *ret = NULL;
	NB_die_if(!dst_field_name && !src_field_name, "must provide at least one field");
	NB_die_if(!dst_field_name && !dst_state_name, "no destination specified");
	NB_die_if(!src_field_name && !src_state_name && !src_value, "no source specified");
	NB_die_if(src_state_name && src_value, "source cannot be both a state and a value");

	NB_die_if(!(
		ret = calloc(sizeof(*ret), 1)
		), "fail alloc size %zu", sizeof(*ret));

	if (dst_field_name) {
		NB_die_if(!(
			ret->dst_field = field_get(dst_field_name)
			), "cannot get field '%s'", dst_field_name);
	}
	if (src_field_name) {
		NB_die_if(!(
			ret->src_field = field_get(src_field_name)
			), "cannot get field '%s'", src_field_name);
	}

	if (dst_state_name) {
		NB_die_if(!(
			ret->dst = memref_state_get(
					dst_field_name ? ret->dst_field : ret->src_field,
					dst_state_name)
			), "cannot get destination state ref '%s'", dst_state_name);
	}

	if (src_state_name) {
		NB_die_if(!(
			ret->src = memref_state_get(
					src_field_name ? ret->src_field : ret->dst_field,
					src_state_name)
			), "cannot get source state ref '%s'", src_state_name);
	} else if (src_value) {
		NB_die_if(!(
			ret->src = memref_value_new(
					src_field_name ? ret->src_field : ret->dst_field,
					src_value)
			), "cannot parse source value '%s'", src_value);
	}

	ret->set.set_to = ret->dst_field ? ret->dst_field->set : ret->src_field->set;
	ret->set.set_from = ret->src_field ? ret->src_field->set : ret->dst_field->set;
	ret->set.to = ret->dst ? ret->dst->bytes : NULL;
	ret->set.from = ret->src ? ret->src->bytes : NULL;

	return ret;
die:
	op_free(ret);
	return NULL;
}

/*	op_parse_new()
 */
struct op *op_parse_new (yaml_document_t *doc, yaml_node_t *mapping)
{
	const char	*dst_field_name = NULL;
	const char	*dst_state_name = NULL;
	const char	*src_field_name = NULL;
	const char	*src_state_name = NULL;
	const char	*src_value = NULL;

	Y_SEQ_MAP_PAIRS_EXEC_OBJ(doc, mapping,
		if (!strcmp("dst", keyname) || !strcmp("d", keyname)) {
			Y_SEQ_MAP_PAIRS_EXEC_STR(doc, val,
				if (!strcmp("field", keyname) || !strcmp("f", keyname)) {
					dst_field_name = valtxt;
				} else if (!strcmp("state", keyname) || !strcmp("s", keyname)) {
					dst_state_name = valtxt;
				} else {
					NB_die("op destination does not implement '%s'", keyname);
				}
			);

		} else if (!strcmp("src", keyname) || !strcmp("s", keyname)) {
			Y_SEQ_MAP_PAIRS_EXEC_STR(doc, val,
				if (!strcmp("field", keyname) || !strcmp("f", keyname)) {
					src_field_name = valtxt;
				} else if (!strcmp("state", keyname) || !strcmp("s", keyname)) {
					src_state_name = valtxt;
				} else if (!strcmp("value", keyname) || !strcmp("v", keyname)) {
					src_value = valtxt;
				} else {
					NB_die("op source does not implement '%s'", keyname);
				}
			);

		} else {
			NB_die("'op' does not implement '%s'", keyname);
		}
	);

	return op_new(dst_field_name, dst_state_name, src_field_name, src_state_name, src_value);
die:
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
