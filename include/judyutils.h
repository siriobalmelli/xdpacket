#ifndef judyutils_h_
#define judyutils_h_

/*	judyutils.h
 *
 * Informal set of utils for simplifying calls to Judy.
 * NOTE that we simplify the "Word_t" construct to just "void *"
 * which increases legibility everywhere by avoiding casts.
 *
 * (c) 2018 Sirio Balmelli
 */

#include <xdpacket.h>
#include <Judy.h>
#include <nonlibc.h>


/*	js_insert()
 * Insert (or update and clobber!) 'datum' at 'index' of 'array'.
 * Return 0 on success.
 */
NLC_INLINE int js_insert(Pvoid_t *array, const char *index, void *datum)
{
	void **pval;
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
 * (or NULL if 'datum' is NULL but please just don't put NULL datums in array).
 */
NLC_INLINE void *js_get(Pvoid_t *array, const char *index)
{
	void **pval;
	JSLG(pval, *array, (const uint8_t *)index);
	if (pval)
		return *pval;
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
 * - 'val' :: datum associated with 'index'
 *
 * NOTE: indirection with 'array_ptr' (== 'parr') allows 'statements'
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
	void **pval; \
	Pvoid_t __attribute__((unused)) *parr = array_ptr; \
	JSLF(pval, *array_ptr, index); \
	while (pval) { \
		void *val = *pval; \
		statements; \
		JSLN(pval, *array_ptr, index); \
	} \
} while(0)


#endif /* judyutils_h_ */
