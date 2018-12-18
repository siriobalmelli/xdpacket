#ifndef judyutils_h_
#define judyutils_h_

/*	judyutils.h
 *
 * Informal set of utils for simplifying calls to Judy
 * (c) 2018 Sirio Balmelli
 */

#include <xdpacket.h>
#include <Judy.h>
#include <nonlibc.h>


/*	j_word
 * Transparent union to avoid pesky compiler warnings.
 */
typedef union {
	Word_t	word;
	void	*ptr;
} j_word __attribute__((__transparent_union__));

/*	j_callback
 * A callback function that can be loop-executed on all members of a list.
 * Callback may delete list members without ill effects.
 */
typedef void(*j_callback)(void *datum, void *context);


/*	js_insert()
 * Insert (or update and clobber!) 'datum' at 'index' of 'array'.
 * Return 0 on success.
 */
NLC_INLINE int js_insert(Pvoid_t *array, const char *index, j_word datum)
{
	j_word *pval;
	JSLI(pval, *array, (const uint8_t *)index);
	if (pval) {
		*pval = datum;
		return 0;
	}
	return 1;
}

/*	js_get()
 * Get 'index' in array.
 * Return 'datum' or NULL if not found
 * (or NULL if 'datum' is NULL but please don't be a muppet and add NULL datums
 * to the array).
 */
NLC_INLINE void *js_get(Pvoid_t *array, const char *index)
{
	j_word *pval;
	JSLG(pval, *array, (const uint8_t *)index);
	if (pval)
		return (*pval).ptr;
	return NULL;
}

/*	js_delete()
 * Delete 'index' in array.
 */
NLC_INLINE void js_delete(Pvoid_t *array, const char *index)
{
	int rc;
	JSLD(rc, *array, (const uint8_t *)index);
}


/*	JS_LOOP()
 * Execute 'statements' inside a loop iterating through all elements of '*array_ptr'.
 *
 * The following variables are available to 'statements':
 * - 'parr' :: pointer to the array
 * - 'index' :: current element index
 * - 'val' :: current datum
 *
 * NOTE: indirection with 'array_ptr' and 'parr' allows 'statements'
 * to validly call _other_ array manipulation functions.
 *
 * EXAMPLE:
 * ```
 *	JS_LOOP(&iface_parse_JS,
 *		iface_free(val);
 *		);
 * ```
 */
#define JS_LOOP(array_ptr, statements) \
do { \
	uint8_t index[MAXLINELEN] = {0}; \
	j_word *pval; \
	Pvoid_t __attribute__((unused)) *parr = array_ptr; \
	JSLF(pval, *array_ptr, index); \
	while (pval) { \
		void *val = (*pval).ptr; \
		statements; \
		JSLN(pval, *array_ptr, index); \
	} \
} while(0)


#endif /* judyutils_h_ */
