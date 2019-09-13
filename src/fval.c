#include <fval.h>
#include <ndebug.h>
#include <yamlutils.h>
#include <judyutils.h>
#include <value.h>
#include <nstring.h>


/*	fval_set_free()
 */
void fval_set_free(void * arg)
{
	free(arg);
}

/*	fval_set_new()
 * NOTE: 'value_len' is the length of the user value string without '\0' terminator.
 */
struct fval_set *fval_set_new(const char *value, struct field_set set)
{
	struct fval_set *ret = NULL;

	/* 'len' may be zero for zero-length fields */
	size_t alloc_size = sizeof(*ret) + set.len;
	NB_die_if(!(
		ret = malloc(alloc_size)
		), "fail malloc size %zu", alloc_size);
	ret->where = set;
	ret->where.flags |= FIELD_FVAL;

	NB_die_if(
		value_parse(value, ret->bytes, set.len)
		, "");

	return ret;
die:
	fval_set_free(ret);
	return NULL;
};


/*	fval_set_write()
 * Write '*fvb' to '*pkt', observing the offset and mask in 'fvb->set'.
 * Returns 0 on success, non-zero on failure (e.g. packet not long enough).
 */
int fval_set_write(struct fval_set *fvb, void *pkt, size_t plen)
{
	struct field_set set = fvb->where;

	/* see field.h */
	FIELD_PACKET_INDEXING

	memcpy(start, fvb->bytes, flen);
	return 0;
}



/*	fval_free()
 */
void fval_free(void *arg)
{
	if (!arg)
		return;
	struct fval *fv = arg;

	/* Counterintuitively, use this to only print info when:
	 * - we are compiled with debug flags
	 * - 'field' and 'val' are non-NULL
	 */
	NB_wrn_if(fv->field && fv->val, "%s: %s", fv->field->name, fv->val);

	field_release(fv->field);
	free(fv->val);
	free(fv->rendered);
	fval_set_free(fv->set);
	free(fv);
}

/*	fval_new()
 */
struct fval *fval_new(const char *field_name, const char *value)
{
	struct fval *ret = NULL;
	NB_die_if(!field_name || !value, "fval requires 'field_name' and 'value'");

	NB_die_if(!(
		ret = calloc(1, sizeof(*ret))
		), "failed malloc size %zu", sizeof(*ret));

	NB_die_if(!(
		ret->field = field_get(field_name)
		), "could not get field '%s'", field_name);

	/* If a field-set can store the length of the user value string,
	 * it can definitely store the resulting parsed set of bytes.
	 */
	errno = 0;
	ret->val = nstralloc(value, MAXVALUELEN, NULL); /* a zero-length value is valid */
	NB_die_if(errno == EINVAL, "");
	NB_die_if(errno == E2BIG, "value truncated:\n%s", ret->val);

	NB_die_if(!(
		ret->set = fval_set_new(ret->val, ret->field->set)
		), "");
	NB_die_if(!(
		ret->rendered = value_render(ret->set->bytes, ret->set->where.len)
		), "");

	ret->set_hash = fnv_hash64(NULL, NULL, 0); /* seed with initializer */
	NB_die_if(
		fval_set_hash(ret->set, &ret->set_hash)
		, "");

	NB_inf("%s: %s", ret->field->name, ret->val);
	return ret;
die:
	fval_free(ret);
	return NULL;
}


/*	fval_emit()
 * Emit an interface as a mapping under 'outlist' in 'outdoc'.
 */
int fval_emit(struct fval *fval, yaml_document_t *outdoc, int outlist)
{
	int err_cnt = 0;
	int reply = yaml_document_add_mapping(outdoc, NULL, YAML_BLOCK_MAPPING_STYLE);
	NB_die_if(
		y_pair_insert(outdoc, reply, fval->field->name, fval->val)
#ifndef NDEBUG
		|| y_pair_insert(outdoc, reply, "bytes", fval->rendered)
#endif
		// || y_pair_insert_nf(outdoc, reply, "hash", "0x%"PRIx64, fval->set_hash)
		, "");
	NB_die_if(!(
		yaml_document_append_sequence_item(outdoc, outlist, reply)
		), "");
die:
	return err_cnt;
}
