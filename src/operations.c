#include <operations.h>
#include <nonlibc.h>


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
}


/*	op_match()
 */
int op_match(struct op_set *op, void *pkt, size_t plen)
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
int op_write(struct op_set *op, void *pkt, size_t plen)
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


#if 0  /* TODO: delete this */
/*	op_execute()
 *
 * NOTES:
 * - If a pointer (where || from) is NULL, it will be replaced
 *   with 'pkt + set.offt' at runtime.
 *   Otherwise, we will use the given pointer and _ignore_ 'set.offt'.
 * - An OP_MOVE will mask the trailing byte with the mask
 *   of _both_ 'set' and 'set_from'.
 */
int op_execute(struct op_set *op, void *pkt, size_t plen)
{
	uint8_t *where;
	uint8_t *from;
	enum op_type type = op->set.flags & 0xF0;
	size_t len = op->set.len -1; /* last byte is masked */
	uint8_t mask = op->set.mask;

	/* addressing */
	where = op->where;
	if (!where && !(where = op_pkt_offset(pkt, plen, op->set)))
		return 1;
	switch (type) {
	case OP_MATCH:
	case OP_COPY:
		from = op->from;
		if (!from && !(from = op_pkt_offset(pkt, plen, op->set)))
			return 1;
		break;
	case OP_MOVE:
		if (!(from = op_pkt_offset(pkt, plen, op->set_from)))
			return 1;
		break;
	}

	/* operation */
	switch (type) {
	case OP_MATCH:
		if (memcmp(where, from, len))
			return 1;
		if (where[len] & mask != from[len] & mask)
			return 1;
		break;

	case OP_COPY:
		memcpy(where, from, len);
		/* mask source; preserve masked bits in destination */
		where[len] = (where[len] & ~mask) | (from[len] & mask);
		break;

	case OP_MOVE:
		memcpy(where, from, len);
		where[len] = (where[len] & ~mask) |
			((from[len] & mask) & op->set_from.mask);
		break;

	return 0;
}
#endif
