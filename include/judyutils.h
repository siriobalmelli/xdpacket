#ifndef judyutils_h_
#define judyutils_h_

/*	judyutils.h
 *
 * Informal set of utils for simplifying calls to Judy.
 * NOTE that we simplify the "Word_t" construct to just "void *"
 * which increases legibility everywhere by avoiding casts.
 *
 * (c) 2018 Sirio Balmelli
 *
 * TODO: handle "Conditional jump or move depends on uninitialised value(s)"
 * errors thrown by Valgrind when dealing with Judy
 * ... until then use '--undef-value-errors=no'
 */

#include <xdpacket.h>
#include <Judy.h>
#include <nonlibc.h>


/* Common internal logic for insert calls.
 * These aren't the droids you are looking for.
 * Move along.
 */
#define JXX_INSERT_COMMON			\
	if (pval) {				\
		if (!clobber && (*pval))	\
			return 1;		\
		*pval = datum;			\
		return 0;			\
	}					\
	return 1;				\


/*	j[type]_insert()
 * Insert 'datum' at 'index' of '*array'.
 * If 'clobber' is true, then then any existing datum will be overwritten.
 * Return 0 on success.
 * NOTE this relies on 'datum' never being NULL/0.
 */
NLC_INLINE int jl_insert(Pvoid_t *array, uint64_t index, void *datum, bool clobber)
{
	if (!datum)
		return 1;
	void **pval;
	JLI(pval, *array, index);
	JXX_INSERT_COMMON
}
NLC_INLINE int js_insert(Pvoid_t *array, const char *index, void *datum, bool clobber)
{
	if (!datum)
		return 1;
	void **pval;
	JSLI(pval, *array, (const uint8_t *)index);
	JXX_INSERT_COMMON
}


/*	j[type]_enqueue()
 * Enqueue 'datum' in the FIRST AVAILABLE SLOT of '*array'.
 * If '*array' contains no "holes", then the result, intuitively, "append at tail" 
 * Otherwise, the first available gap will be filled.
 * The reasons for this possibly weird approach are:
 * - avoid having to migrate/shuffle/defragment array elements
 * - most use cases are a SERIES of enqueue() operations followed by a series
 *   of pop()s, which performs exactly as expected.
 */
NLC_INLINE int jl_enqueue(Pvoid_t *array, void *datum)
{
	uint64_t index;
	int rc;
	JLFE(rc, *array, index);
	if (!rc)
		return 1;
	return jl_insert(array, index, datum, false);
}


/*	j[type]_pop()
 * Pop 'datum' from the FIRST FILLED SLOT of '*array', removing it from the array.
 * See enqueue() above for comments and caveats.
 * Return 'datum' or NULL if not found (NULL is not a valid array datum).
 */
NLC_INLINE void *jl_pop(Pvoid_t *array)
{
	/* TODO: implement */
	return NULL;
}


/*	j[type]_get()
 * Get 'index' in '*array'.
 * Return 'datum' or NULL if not found (NULL is not a valid array datum).
 */
NLC_INLINE void *jl_get(Pvoid_t *array, uint64_t index)
{
	void **pval;
	JLG(pval, *array, index);
	if (pval)
		return *pval;
	return NULL;
}
NLC_INLINE void *js_get(Pvoid_t *array, const char *index)
{
	void **pval;
	JSLG(pval, *array, (const uint8_t *)index);
	if (pval)
		return *pval;
	return NULL;
}


/*	j[type]_delete()
 * Delete 'index' in '*array'.
 */
NLC_INLINE void j_delete(Pvoid_t *array, uint64_t index)
{
	int rc;
	JLD(rc, *array, index);
}
NLC_INLINE void js_delete(Pvoid_t *array, const char *index)
{
	int rc;
	JSLD(rc, *array, (const uint8_t *)index);
}


/*	J[type]_LOOP()
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
#define JL_LOOP(array_ptr, statements)				\
do {								\
	uint64_t index = 0;					\
	void **pval;						\
	Pvoid_t __attribute__((unused)) *parr = array_ptr;	\
	JLF(pval, *array_ptr, index);				\
	while (pval) {						\
		void *val = *pval;				\
		/* user statements executed here */		\
		statements;					\
		JLN(pval, *array_ptr, index);			\
	}							\
} while(0)

#define JS_LOOP(array_ptr, statements)				\
do {								\
	uint8_t index[MAXLINELEN] = {0};			\
	void **pval;						\
	Pvoid_t __attribute__((unused)) *parr = array_ptr;	\
	JSLF(pval, *array_ptr, index);				\
	while (pval) {						\
		void *val = *pval;				\
		/* user statements executed here */		\
		statements;					\
		JSLN(pval, *array_ptr, index);			\
	}							\
} while(0)


#endif /* judyutils_h_ */
