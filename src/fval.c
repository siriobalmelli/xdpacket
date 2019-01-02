#include <fval.h>
#include <ndebug.h>


/* (void *field) -> (Pvoid_t fval_J1) -> (struct fval *fv) */
//static Pvoid_t field_J_fval_J1 = NULL;
/* TODO: how to track fvals holding references to fields when the fields are deleted? */


/*	fval_free()
 */
void fval_free(void *arg)
{
	free(arg);
}

/*	fval_new()
 */
struct fval *fval_new(const char *field_name, const char *value)
{
	struct fval *ret = NULL;
	NB_die_if(!field_name || !value, "fval requires 'field_name' and 'value'");

	struct field *field = NULL;
	NB_die_if(!(
		field = field_get(field_name)
		), "could not get field '%s'", field_name);

	/* do not allow a length longer than the maximum length value of a field-set */
	size_t vlen = strnlen(value, ((size_t)1 << sizeof(field->set.len)) - 1);
	NB_die_if(!vlen, "zero-length 'value' invalid");

	NB_die_if(!(
		ret = malloc(sizeof(*ret) + vlen)
		), "fail alloc size %zu", sizeof(*ret) + vlen);
	ret->field = field;
	ret->vlen = vlen;
	memcpy(ret->val, value, ret->vlen);
	ret->val[ret->vlen] = '\0';

	/* TODO: verify it will parse? */

	return ret;
die:
	fval_free(ret);
	return NULL;
}
