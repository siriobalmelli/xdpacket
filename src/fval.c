#include <fval.h>
#include <ndebug.h>
#include <yamlutils.h>
#include <judyutils.h>
#include <sys/types.h>
#include <regex.h>
#include <arpa/inet.h> /* inet_pton() */
#include <endian.h>
#include <binhex.h>


/* Use regex to match special-case parser inputs.
 * NOTES:
 * 1. Do NOT use regex capture groups: GNU ERE is impractical for a variable number
 *    of matches; it is enough for the regex to match.
 *    We will then separately tokenize and parse
 *    (or use a standard system parser, e.g. for IP addresses).
 * 2. Fractional values are NOT considered regex matches. Examples:
 *    - "192.168.1.0" with field length 3 to match 192.168.1.0/24
 *    - "192.168.1.128" with field length 4 and mask 0x80 to match 192.178.1.128/25
 * 3. We match in order of size (an IP address is <= 4B; MAC <= 6B; etc),
 *    so that "later" (larger-size) parsers can count on earlier parsers
 *    already having matched and not reaching them.
 */
const int regex_flags = REG_ICASE | REG_EXTENDED | REG_NOSUB;
static regex_t re_ipv4 = {0};
const char *txt_ipv4 = "^([0-9]{1,3}\\.){3}([0-9]+)$";
static regex_t re_mac = {0};
const char *txt_mac = "^([0-9a-fA-F]{2}:){5}([0-9a-fA-F]{2})$";
static regex_t re_int = {0};
const char *txt_int = "^[+-]?([0-9]+|b[01]+|(0x|x)[0-9a-f]+)$";
static regex_t re_ipv6 = {0};
const char *txt_ipv6 = "^((([0-9a-f]{1,4}(:|::)){1,7}([0-9a-f]{1,4}))|::|::1)$";
static regex_t re_hex = {0};
const char *txt_hex = "^(x|0x)[0-9a-f]+$";

/*	fval_init()
 * Compile regexes only once at program start.
 */
static void __attribute__((constructor)) fval_bytes_init()
{
	NB_err_if(
		regcomp(&re_ipv4, txt_ipv4, regex_flags)
		, "could not compile ipv4 regex '%s'", txt_ipv4);
	NB_err_if(
		regcomp(&re_mac, txt_mac, regex_flags)
		, "could not compile mac regex '%s'", txt_mac);
	NB_err_if(
		regcomp(&re_int, txt_int, regex_flags)
		, "could not compile int regex '%s'", txt_int);
	NB_err_if(
		regcomp(&re_ipv6, txt_ipv6, regex_flags)
		, "could not compile ipv6 regex '%s'", txt_ipv6);
	NB_err_if(
		regcomp(&re_hex, txt_hex, regex_flags)
		, "could not compile hex regex '%s'", txt_hex);
}

/*	fval_cleanup()
 */
static void __attribute__((destructor)) fval_bytes_cleanup()
{
	regfree(&re_ipv4);
	regfree(&re_mac);
	regfree(&re_int);
	regfree(&re_ipv6);
	regfree(&re_hex);
}


/*	fval_bytes_free()
 */
void fval_bytes_free(void * arg)
{
	free(arg);
}


/*	fval_bytes_new()
 * NOTE: 'value_len' is the length of the user value string without '\0' terminator.
 */
struct fval_bytes *fval_bytes_new(const char *value, size_t value_len, struct field_set set)
{
	struct fval_bytes *ret = NULL;
	size_t len = set.len; /* for legibility only */

	/* 'len' may be zero for zero-length fields */
	size_t alloc_size = sizeof(*ret) + len;
	NB_die_if(!(
		ret = malloc(alloc_size)
		), "fail malloc size %zu", alloc_size);
	ret->where = set;

	/* 0-length fields are valid: just ignore any user-supplied values */
	if (!len) {
		;

	/* parse IPv4 */
	} else if (len <= 4 && !regexec(&re_ipv4, value, 0, NULL, 0)) {
		struct in_addr	i4;
		NB_die_if(inet_pton(AF_INET, value, &i4) != 1,
			"could not parse ipv4 '%s'", value);
		memcpy(ret->bytes, &i4, len);

	/* parse MAC */
	} else if (len <= 6 && !regexec(&re_mac, value, 0, NULL, 0)) {
		uint8_t by[6];
		NB_die_if(sscanf(value, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
				&by[0], &by[1], &by[2], &by[3], &by[4], &by[5]) != 6,
			"could not parse MAC '%s'", value);
		memcpy(ret->bytes, by, len);

	/* parse integer */
	} else if (len <= 8 && !regexec(&re_int, value, 0, NULL, 0)) {
		uint8_t by[8];
		errno = 0;
		long long tt = strtoll(value, NULL, 0);
		NB_die_if(errno, "could not parse integer '%s'", value);

		/* overflow checking using builtins;
		 * see 'test/overflow_test.c' for a proof that this is kosher
		 */
		if (len == 1 && tt >= 0) {
			uint8_t *out = by;
			NB_die_if(__builtin_add_overflow(tt, 0, out),
				"uint8_t value '%lld' out of bounds", tt);

		} else if (len == 1) {
			int8_t *out = (int8_t *)by;
			NB_die_if(__builtin_add_overflow(tt, 0, out),
				"int8_t value '%lld' out of bounds", tt);

		} else if (len == 2 && tt >= 0) {
			uint16_t *out = (uint16_t *)by;
			NB_die_if(__builtin_add_overflow(tt, 0, out),
				"uint16_t value '%lld' out of bounds", tt);
			*out = htobe16(*out);

		} else if (len == 2) {
			int16_t *out = (int16_t *)by;
			NB_die_if(__builtin_add_overflow(tt, 0, out),
				"int16_t value '%lld' out of bounds", tt);
			*out = htobe16(*out);

		} else if (len == 4 && tt >= 0) {
			uint32_t *out = (uint32_t *)by;
			NB_die_if(__builtin_add_overflow(tt, 0, out),
				"uint32_t value '%lld' out of bounds", tt);
			*out = htobe32(*out);

		} else if (len == 4) {
			int32_t *out = (int32_t *)by;
			NB_die_if(__builtin_add_overflow(tt, 0, out),
				"int32_t value '%lld' out of bounds", tt);
			*out = htobe32(*out);

		} else if (len == 8 && tt >= 0) {
			int64_t *out = (int64_t *)by;
			NB_die_if(__builtin_add_overflow(tt, 0, out),
				"int64_t value '%lld' out of bounds", tt);
			*out = htobe64(*out);

		} else if (len == 8) {
			uint64_t *out = (uint64_t *)by;
			NB_die_if(__builtin_add_overflow(tt, 0, out),
				"uint64_t value '%lld' out of bounds", tt);
			*out = htobe64(*out);

		} else {
			NB_die("could not parse integer '%s' with nonstandard length '%zu'",
				value, len);
		}
		memcpy(ret->bytes, by, len);

	/* parse IPv6 */
	} else if (len <= 16 && !regexec(&re_ipv6, value, 0, NULL, 0)) {
		struct in6_addr	i6;
		NB_die_if(inet_pton(AF_INET6, value, &i6) != 1,
			"could not parse ipv6 address '%s'", value);
		memcpy(ret->bytes, &i6, len);

	/* parse arbitrary series of hex digits */
	} else if (!regexec(&re_hex, value, 0, NULL, 0)) {
		/* Don't care exactly how many bytes/characters parsed,
		 * as long as at least one nibble made it.
		 * missing characters will be zero-padded by hx2b_BE()..
		 */
		NB_die_if(!hx2b_BE(value, ret->bytes, len),
			"could not parse hex sequence '%s'", value);

	/* Parse literal series of characters.
	 * NOTEs:
	 * - 'bytes' shjould NOT be a valid C string and should NOT have a '\0' terminator;
	 *   this is what goes on the wire and values on the wire should ONLY
	 *   be regarded (and printed) as a series of bytes.
	 * - LibYAML requires "double quotes" around strings where special characters
	 *   such as "\r" should be escaped into their proper ASCII value e.g. 0x0d.
	 */
	} else {
		if (value_len > len) {
			NB_wrn("truncate character sequence value to %zu bytes", len);
			value_len = len;
		}
		NB_die_if(value_len != len,
			"character sequence not of expected length %zu", len);
		memcpy(ret->bytes, value, value_len); /* does not copy \0 terminator */
	}

	return ret;
die:
	fval_bytes_free(ret);
	return NULL;
};


/*	fval_bytes_print()
 * Allocates and returns a string containing the hexadecimal representation
 * of '*fvb'.
 * Caller is responsible for eventually calling free() on returned pointer.
 */
char *fval_bytes_print(struct fval_bytes *fvb)
{
	char *ret = NULL;
	size_t prnlen = (size_t)fvb->where.len * 2 +3; /* leading "0x" and trailing '\0' */

	NB_die_if(!(
		ret = malloc(prnlen)
		), "fail alloc size %zu", prnlen);

	if (fvb->where.len) {
		ret[0] = '0';
		ret[1] = 'x';
		b2hx_BE(fvb->bytes, &ret[2], fvb->where.len);
	} else {
		ret[0] = '\0';
	}

	return ret;
die:
	free(ret);
	return NULL;
}


/*	fval_bytes_write()
 * Write '*fvb' to '*pkt', observing the offset and mask in 'fvb->set'.
 * Returns 0 on success, non-zero on failure (e.g. packet not long enough).
 */
int fval_bytes_write(struct fval_bytes *fvb, void *pkt, size_t plen)
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
	NB_wrn_if(fv->field && fv->val,
		"erase fval %s: %s", fv->field->name, fv->val);

	field_release(fv->field);
	free(fv->val);
	free(fv->bytes_prn);
	fval_bytes_free(fv->bytes);
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

	/* do not allow a length longer than the maximum length value of a field-set */
	size_t max = ((size_t)1 << (sizeof(ret->field->set.len) * 8)) -1;
	size_t vlen = strnlen(value, max);
	NB_die_if(vlen == max, "value truncated:\n%.*s", (int)max, value);

	/* alloc and copy user-supplied value string as-is */
	NB_die_if(!(
		ret->val = malloc(vlen +1) /* add \0 terminator */
		), "fail alloc size %zu", vlen +1);
	snprintf(ret->val, vlen +1, "%s", value);

	NB_die_if(!(
		ret->bytes = fval_bytes_new(ret->val, vlen, ret->field->set)
		), "");
	NB_die_if(!(
		ret->bytes_prn = fval_bytes_print(ret->bytes)
		), "");

	ret->bytes_hash = fnv_hash64(NULL, NULL, 0); /* seed with initializer */
	NB_die_if(
		fval_bytes_hash(ret->bytes, &ret->bytes_hash)
		, "");

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
		|| y_pair_insert(outdoc, reply, "bytes", fval->bytes_prn)
#endif
		// || y_pair_insert_nf(outdoc, reply, "hash", "0x%"PRIx64, fval->bytes_hash)
		, "");
	NB_die_if(!(
		yaml_document_append_sequence_item(outdoc, outlist, reply)
		), "");
die:
	return err_cnt;
}
