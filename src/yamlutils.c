#include <yamlutils.h>

/*	yml_insert_pair_nf()
 */
int yml_insert_pair_nf(yaml_document_t *doc, int mapping,
				const char *key,
				size_t val_max, const char *val, ...)
{
	int err_cnt = 0;
	char *buf = NULL;
	NB_die_if(!val_max || val_max > YML_FIELD_MAX,
		"variadic call length %zu out of bounds", val_max);
	NB_die_if(!(
		buf = malloc(val_max)
		), "alloc size %zu fail", val_max);

	va_list arglist;
	va_start(arglist, val);
	vsnprintf(buf, val_max, val, arglist);
	va_end(arglist);

	err_cnt = yml_insert_pair(doc, mapping, key, buf);
die:
	free(buf);
	return err_cnt;
}
