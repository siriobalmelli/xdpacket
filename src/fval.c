#include <fval.h>

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
	return ret;
die:
	fval_free(ret);
	return NULL;
}
