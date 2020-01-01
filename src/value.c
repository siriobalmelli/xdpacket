#include <value.h>
#include <sys/types.h>
#include <regex.h>
#include <arpa/inet.h> /* inet_pton() */
#include <binhex.h>
#include <nstring.h>
#include <ndebug.h>
#include <nlc_endian.h> /* htobeXX macros */


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

/*	value_init()
 * Compile regexes only once at program start.
 */
static void __attribute__((constructor)) value_set_init()
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

/*	value_cleanup()
 */
static void __attribute__((destructor)) value_set_cleanup()
{
	regfree(&re_ipv4);
	regfree(&re_mac);
	regfree(&re_int);
	regfree(&re_ipv6);
	regfree(&re_hex);
}


/*	value_parse()
 * Parse 'value' user input text into 'parsed' big-endian representation,
 * which is always 'len' bytes long (or padded thereto).
 * Length is important since we use memcmp() in sval_test().
 *
 * Returns 0 on success.
 */
int value_parse(const char *value, uint8_t *parsed, size_t len)
{
	int err_cnt = 0;
	struct in6_addr	i6;

	/* 0-length fields are valid: just ignore any user-supplied values */
	if (!len) {
		;

	/* parse IPv4 */
	} else if (len <= 4 && !regexec(&re_ipv4, value, 0, NULL, 0)) {
		struct in_addr	i4;
		NB_die_if(inet_pton(AF_INET, value, &i4) != 1,
			"could not parse ipv4 '%s'", value);
		memcpy(parsed, &i4, len);

	/* parse MAC */
	} else if (len <= 6 && !regexec(&re_mac, value, 0, NULL, 0)) {
		uint8_t by[6];
		NB_die_if(sscanf(value, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
				&by[0], &by[1], &by[2], &by[3], &by[4], &by[5]) != 6,
			"could not parse MAC '%s'", value);
		memcpy(parsed, by, len);

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
			*out = h16tobe(*out);

		} else if (len == 2) {
			int16_t *out = (int16_t *)by;
			NB_die_if(__builtin_add_overflow(tt, 0, out),
				"int16_t value '%lld' out of bounds", tt);
			*out = h16tobe(*out);

		} else if (len == 4 && tt >= 0) {
			uint32_t *out = (uint32_t *)by;
			NB_die_if(__builtin_add_overflow(tt, 0, out),
				"uint32_t value '%lld' out of bounds", tt);
			*out = h32tobe(*out);

		} else if (len == 4) {
			int32_t *out = (int32_t *)by;
			NB_die_if(__builtin_add_overflow(tt, 0, out),
				"int32_t value '%lld' out of bounds", tt);
			*out = h32tobe(*out);

		} else if (len == 8 && tt >= 0) {
			int64_t *out = (int64_t *)by;
			NB_die_if(__builtin_add_overflow(tt, 0, out),
				"int64_t value '%lld' out of bounds", tt);
			*out = h64tobe(*out);

		} else if (len == 8) {
			uint64_t *out = (uint64_t *)by;
			NB_die_if(__builtin_add_overflow(tt, 0, out),
				"uint64_t value '%lld' out of bounds", tt);
			*out = h64tobe(*out);

		} else {
			NB_die("could not parse integer '%s' with nonstandard length '%zu'",
				value, len);
		}
		memcpy(parsed, by, len);

	/* parse IPv6 */
	} else if (len <= 16 && !regexec(&re_ipv6, value, 0, NULL, 0)
			&& inet_pton(AF_INET6, value, &i6) == 1)
	{
		memcpy(parsed, &i6, len);

	/* parse arbitrary series of hex digits */
	} else if (!regexec(&re_hex, value, 0, NULL, 0)) {
		/* Don't care exactly how many bytes/characters parsed,
		 * as long as at least one nibble made it.
		 * missing characters will be zero-padded by hx2b_BE()..
		 */
		NB_die_if(!hx2b_BE(value, parsed, len),
			"could not parse hex sequence '%s'", value);

	/* Parse literal series of characters.
	 * NOTEs:
	 * - 'bytes' should NOT be a valid C string and should NOT have a '\0' terminator;
	 *   this is what goes on the wire and values on the wire should ONLY
	 *   be regarded (and printed) as a series of bytes.
	 * - LibYAML requires "double quotes" around strings where special characters
	 *   such as "\r" should be escaped into their proper ASCII value e.g. 0x0d.
	 */
	} else {
		size_t value_len = strnlen(value, len + 1);
		if (value_len > len) {
			NB_wrn("truncate character sequence value to %zu bytes", len);
			value_len = len;
		}
		NB_die_if(value_len != len,
			"character sequence not of expected length %zu", len);
		memcpy(parsed, value, value_len); /* does not copy \0 terminator */
	}

die:
	return err_cnt;
}


/*	value_render()
 * Allocates and returns a string containing the hexadecimal representation
 * of '*parsed' with length 'len'.
 * Caller is responsible for eventually calling free() on returned pointer.
 */
char *value_render(const uint8_t *parsed, size_t len)
{
	char *ret = NULL;
	size_t prnlen = len * 2 +3; /* leading "0x" and trailing '\0' */

	NB_die_if(!(
		ret = malloc(prnlen)
		), "fail alloc size %zu", prnlen);

	if (len) {
		ret[0] = '0';
		ret[1] = 'x';
		b2hx_BE(parsed, &ret[2], len);
	} else {
		ret[0] = '\0';
	}

	return ret;
die:
	free(ret);
	return NULL;
}
