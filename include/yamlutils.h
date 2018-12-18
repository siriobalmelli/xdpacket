#ifndef yamlutils_h_
#define yamlutils_h_

/*	yamlutils.h
 *
 * Informal set of utilities for working with libyaml
 * (c) 2018 Sirio Balmelli
 */

#include <yaml.h>
#include <nonlibc.h>
#include <ndebug.h>
#include <stdarg.h> /* varargs to emulate printf() constructs */

#define YML_FIELD_MAX 128 /* don't allow va_args values (aka: stack params)
			   * larger than this.
			   */

/*	yml_insert_pair()
 * Insert 'key' and 'val' as a pair into 'mapping'.
 * Return 0 on success.
 */
NLC_INLINE int yml_insert_pair(yaml_document_t *doc, int mapping,
				const char *key, const char *val)
{
	int err_cnt = 0;
	int i_key, i_val;
	NB_die_if(!(
		i_key = yaml_document_add_scalar(doc, NULL,
			(yaml_char_t *)key, -1,
			YAML_PLAIN_SCALAR_STYLE)
		), "cannot insert scalar");
	NB_die_if(!(
		i_val = yaml_document_add_scalar(doc, NULL,
			(yaml_char_t *)val, -1,
			YAML_PLAIN_SCALAR_STYLE)
		), "cannot insert scalar");
	NB_die_if(!(
		yaml_document_append_mapping_pair(doc, mapping, i_key, i_val)
		), "cannot append to mapping:\n%s: %s", key, val);
die:
	return err_cnt;
}

/*	yml_insert_pairnf()
 * Do printf formatting on variadic 'val' and then call yml_insert_pair().
 */
int yml_insert_pair_nf(yaml_document_t *doc, int mapping,
				const char *key,
				size_t val_max, const char *val, ...);

#endif /* yamlutils_h_ */
