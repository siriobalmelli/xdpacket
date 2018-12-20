#include <yamlutils.h>

/*	y_insert_pair_nf()
 */
int y_insert_pair_nf(yaml_document_t *doc, int mapping,
			const char *key, const char *val, ...)
{
	char buf[MAXLINELEN];
	va_list arglist;
	va_start(arglist, val);
	vsnprintf(buf, MAXLINELEN, val, arglist);
	va_end(arglist);

	return y_insert_pair(doc, mapping, key, buf);
}
