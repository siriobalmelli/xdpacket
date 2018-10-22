#include <matcher.h>
#include <field.h>
#include <zed_dbg.h>

/*	xdpk_match()
 * Test 'pkt' against all fields in '*match'.
 * Return true if any field matches, otherwise false.
 */
bool xdpk_match(const struct xdpk_matcher *match,
		const void *pkt, size_t plen, uint64_t hash)
{
	uint64_t h = 0;
	uint64_t *p = NULL;
	for (int i = 0; i < XDPK_MATCH_FIELD_MAX; i++) {
		struct xdpk_field fld = match->fields[i];
		if (!xdpk_field_valid(fld)) 
			break;
		h = xdpk_field_hash(fld, pkt, plen, p);
		p = &h;
	}
	fprintf(stderr, "Hash == 0x%lx\n", h);
	return h == hash;
}


//#include <matcher2.h>

/* matcher_free()
 */
void matcher_free(struct matcher *mch)
{
	free(mch);
}

/* matcher_new()
 * Allocates a new matcher.
 * Returns NULL on error
 */
struct matcher *matcher_new()
{
	return NULL;
}

/* matcher_do()
 * Matches 'buf' (of 'buf_len' bytes) against 'mch'.
 * If matching, executes actions and returns true,
 * else return false.
 */
bool matcher_do(struct matcher *mch, const void *buf, size_t buf_len)
{
	return false;
}
