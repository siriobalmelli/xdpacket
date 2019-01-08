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
 *    We will then separately tokenize and parse.
 * 2. Fractional values are NOT considered regex matches. Examples:
 *    - "192.168.1.0" with field length 3 to match 192.168.1.0/24
 *    - "192.168.1.128" with field length 4 and mask 0x80 to match 192.178.1.128/25
 * 3. We match in order of size (an IP address is <= 4B; MAC <= 6B; etc)
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
static void __attribute__((constructor)) fval_init()
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
static void __attribute__((destructor)) fval_cleanup()
{
	regfree(&re_ipv4);
	regfree(&re_mac);
	regfree(&re_int);
	regfree(&re_ipv6);
	regfree(&re_hex);
}


/*	fval_free()
 */
void fval_free(void *arg)
{
	free(arg);
}


/*	fval_parse_value()
 * Parse 'val' into 'bytes'.
 * This could be part of fval_new() as-is, only broken out for clarity.
 */
static int fval_parse_value(struct fval *ret)
{
	int err_cnt = 0;
	size_t blen = ret->field->set.len;

	/* parse IPv4 */
	if (blen <= 4 && !regexec(&re_ipv4, ret->val, 0, NULL, 0)) {
		struct in_addr	i4;
		NB_die_if(inet_pton(AF_INET, ret->val, &i4) != 1,
			"could not parse ipv4 '%s'", ret->val);
		memcpy(ret->bytes, &i4, blen);

	/* parse MAC */
	} else if (blen <= 6 && !regexec(&re_mac, ret->val, 0, NULL, 0)) {
		uint8_t by[6];
		NB_die_if(sscanf(ret->val, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
				&by[0], &by[1], &by[2], &by[3], &by[4], &by[5]) != 6,
			"could not parse MAC '%s'", ret->val);
		memcpy(ret->bytes, by, blen);

	/* parse integer */
	} else if (blen <= 8 && !regexec(&re_int, ret->val, 0, NULL, 0)) {
		uint8_t by[8];
		errno = 0;
		long long tt = strtoll(ret->val, NULL, 0);
		NB_die_if(errno, "could not parse integer '%s'", ret->val);

		/* overflow checking using builtins;
		 * see 'test/overflow_test.c' for a proof that this is kosher
		 */
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
		if (blen == 1 && tt >= 0) {
			uint8_t *out = by;
			NB_die_if(__builtin_add_overflow(tt, 0, out),
				"uint8_t value '%lld' out of bounds", tt);

		} else if (blen == 1) {
			int8_t *out = (int8_t *)by;
			NB_die_if(__builtin_add_overflow(tt, 0, out),
				"int8_t value '%lld' out of bounds", tt);

		} else if (blen == 2 && tt >= 0) {
			uint16_t *out = (uint16_t *)by;
			NB_die_if(__builtin_add_overflow(tt, 0, out),
				"uint16_t value '%lld' out of bounds", tt);
			*out = htobe16(*out);

		} else if (blen == 2) {
			int16_t *out = (int16_t *)by;
			NB_die_if(__builtin_add_overflow(tt, 0, out),
				"int16_t value '%lld' out of bounds", tt);
			*out = htobe16(*out);

		} else if (blen == 4 && tt >= 0) {
			uint32_t *out = (uint32_t *)by;
			NB_die_if(__builtin_add_overflow(tt, 0, out),
				"uint32_t value '%lld' out of bounds", tt);
			*out = htobe32(*out);

		} else if (blen == 4) {
			int32_t *out = (int32_t *)by;
			NB_die_if(__builtin_add_overflow(tt, 0, out),
				"int32_t value '%lld' out of bounds", tt);
			*out = htobe32(*out);

		} else if (blen == 8 && tt >= 0) {
			int64_t *out = (int64_t *)by;
			NB_die_if(__builtin_add_overflow(tt, 0, out),
				"int64_t value '%lld' out of bounds", tt);
			*out = htobe64(*out);

		} else if (blen == 8) {
			uint64_t *out = (uint64_t *)by;
			NB_die_if(__builtin_add_overflow(tt, 0, out),
				"uint64_t value '%lld' out of bounds", tt);
			*out = htobe64(*out);

		} else {
			NB_die("could not parse integer '%s' with nonstandard blength '%zu'",
				ret->val, blen);
		}
		#pragma GCC diagnostic pop
		memcpy(ret->bytes, by, blen);

	/* parse IPv6 */
	} else if (blen <= 16 && !regexec(&re_ipv6, ret->val, 0, NULL, 0)) {
		struct in6_addr	i6;
		NB_die_if(inet_pton(AF_INET6, ret->val, &i6) != 1,
			"could not parse ipv6 address '%s'", ret->val);
		memcpy(ret->bytes, &i6, blen);

	/* parse arbitrary series of hex digits */
	} else if (!regexec(&re_hex, ret->val, 0, NULL, 0)) {
		/* Don't care exactly how many bytes/characters parsed,
		 * as long as at least one nibble made it.
		 * missing characters will be zero-padded by hx2b_BE()..
		 */
		NB_die_if(!hx2b_BE(ret->val, ret->bytes, blen),
			"could not parse hex sequence '%s'", ret->val);

	/* parse literal series of characters */
	} else {
		NB_die_if(ret->vlen > blen,
			"character sequence '%s' shorter than field blength %zu",
			ret->val, blen);
		memcpy(ret->bytes, ret->val, blen);
	}

die:
	return err_cnt;
};


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
	size_t max = ((size_t)1 << (sizeof(field->set.len) * 8)) -1;
	size_t vlen = strnlen(value, max);
	NB_die_if(!vlen, "zero-length 'value' invalid");
	NB_die_if(vlen == max, "value truncated:\n%.*s", (int)max, value);
	vlen++; /* \0 terminator */

	size_t blen = field->set.len;
	size_t prnlen = blen * 2 +1;

	NB_die_if(!(
		ret = malloc(sizeof(*ret) + vlen + blen + prnlen)
		), "fail alloc size %zu", sizeof(*ret) + vlen + blen + prnlen);
	ret->field = field;
	ret->vlen = vlen;
	ret->val = (char *)ret->memory;
	ret->bytes = &ret->memory[vlen];
	ret->bytes_prn = (char *)&ret->memory[vlen + blen];

	/* copy user-supplied value string as-is */
	memcpy(ret->val, value, ret->vlen-1);
	ret->val[ret->vlen-1] = '\0';

	/* parse bytes, and print hex representation for user verification */
	NB_die_if(fval_parse_value(ret), "");
	b2hx_BE(ret->bytes, ret->bytes_prn, blen);

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
		, "");
	NB_die_if(
		y_pair_insert(outdoc, reply, "bytes", fval->bytes_prn)
		, "");
	NB_die_if(!(
		yaml_document_append_sequence_item(outdoc, outlist, reply)
		), "");
die:
	return err_cnt;
}
