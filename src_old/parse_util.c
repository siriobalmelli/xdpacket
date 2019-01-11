#include <parse_util.h>
#include <ctype.h>

/*	parse_data_type()
 * The starting characters of a value determine whether it's hexadecimal ('0x'),
 * binary ('b'), decimal ('[-]0-9'), or ASCII (all other).
 */
enum data_type parse_data_type(const char *begin, size_t len)
{
	switch (*begin) {
	case '0':
		if (len == 1)
			return DECIMAL_TYPE;
		else if ((len > 1) && *(begin+1) == 'x')
			return HEX_TYPE;
		break;
	case 'b':
		if ((len > 1) && (*(begin+1) == '0' || *(begin+1) == '1'))
			return BIN_TYPE;
		break;
	case '-':
		if ((len > 1) && isdigit(*(begin+1)))
			return DECIMAL_TYPE;
		break;
	default:
		if (isdigit(*begin))
			return DECIMAL_TYPE;
		break;
	}

	return ASCII_TYPE;
}


/*	parse_binary
 *
 */
uint64_t parse_binary(const char *begin, size_t len, uint64_t max, bool *err)
{
	uint64_t val = 0;
	if (len > 64)
		len = 64;
	const char *cp = begin;
	for (int i = 0; (i < len) && (val <= max); i++, cp++) {
		switch(*cp) {
		case '0':
			val = (val << 1);
			break;
		case '1':
			val = (val << 1) + 1;
			break;
		default:
			*err = true;
			goto error;
		}
	}

error:
	return val;
}

/*	parse_decimal()
 */
int64_t parse_decimal(const char *begin, size_t len, uint64_t max, bool *err)
{
	bool neg = false;
	if (*begin == '-') {
		neg = true;
		begin++;
	}

	int64_t val = 0;
	if (len > 19)
		len = 19;
	const char *cp = begin;
	for (int i = 0; (i < len) && (val <= max); i++, cp++) {
		switch(*cp) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			val = (val * 10) + (*cp - '0');
			break;
		default:
			*err = true;
			goto error;
		}
	}
	if (neg) val = -val;

error:
	return val;
}

/*	hex_value()
 */
uint8_t hex_value(const char c)
{
	return (c > '9') ? (c - 'A' + 10) : (c - '0');
}

/*	parse_hex()
 */
uint64_t parse_hex(const char *begin, size_t len, uint64_t max, bool *err)
{
	uint64_t val = 0;
	if (len > 8)
		len = 8;
	if ((len % 2) != 0) {
		*err = true;
		goto error;
	}
	const char *end = begin + len;
	for (const char *cp = begin; cp < end; cp += 2) {
		int c1 = *cp;
		int c2 = *(cp + 1);
		if (!isxdigit(c1) || !isxdigit(c2)) {
			*err = true;
			goto error;
		}
		val = (val << 8) | (hex_value(toupper(c1)) << 4)
			| hex_value(toupper(c2));
	}

error:
	return val;
}

/*	parse_long_hex()
 * Parse fields longer than will fit in a 64-bit integer.
 */
void parse_long_hex(const char *begin, size_t len, uint8_t *buf, size_t flen, bool *err)
{
	if ((len % 2) != 0) {
		*err = true;
		goto error;
	}
	int i = 0;
	for (const char *cp = begin; (i < len) && (i < flen); cp += 2, i++) {
		char c1 = *cp;
		char c2 = *(cp + 1);
		if (!isxdigit(c1) || !isxdigit(c2)) {
			*err = true;
			goto error;
		}
		buf[i] = (hex_value(toupper(c1)) << 4)
			| hex_value(toupper(c2));
	}

error:
	return;
}

/*	parse_ascii()
 */
void parse_ascii(const char *begin, size_t len, uint8_t *buf, size_t flen, bool *err)
{
	const char *cp = begin;
	for (int i = 0; (i < len) && (i < flen); cp++, i++) {
		buf[i] = *cp;
	}

	return;
}

/*	parse_16bit()
 * Parse a 16-bit integer from an ASCII string.  Numeric value may be
 * decimal, binary or hex.  This determines which and calls the appropriate
 * sub-parsing function.  64-bits is returned so that this doesn't have to
 * account for overflow.  Caller can truncate.
 */
int64_t parse_16bit(const char *begin, size_t len, bool *err)
{
	switch (parse_data_type(begin, len))
	{
	case DECIMAL_TYPE:
		return parse_decimal(begin, len, 0xFFFF, err);
		break;
	case HEX_TYPE:
		return parse_hex(begin+2, len-2, 0xFFFF, err);
		break;
	case BIN_TYPE:
		return parse_binary(begin+1, len-1, 0xFFFF, err);
		break;
	default:
		*err = true;
		return 0;
	}
}


/*	parse_32bit()
 * Parse a 32-bit integer from an ASCII string.  Numeric value may be
 * decimal, binary or hex.  This determines which and calls the appropriate
 * sub-parsing function.  64-bits is returned so that this doesn't have to
 * account for overflow.  Caller can truncate.
 */
int64_t parse_32bit(const char *begin, size_t len, bool *err)
{
	switch (parse_data_type(begin, len))
	{
	case DECIMAL_TYPE:
		return parse_decimal(begin, len, 0xFFFFFFFF, err);
		break;
	case HEX_TYPE:
		return parse_hex(begin+2, len-2, 0xFFFFFFFF, err);
		break;
	case BIN_TYPE:
		return parse_binary(begin + 1, len-1, 0xFFFFFFFF, err);
		break;
	default:
		*err = true;
		return 0;
	}
}

/*	parse_uint8()
 * Parse an unsigned 8-bit integer from an ASCII string.
 */
uint8_t parse_uint8(const char *begin, size_t len, bool *err)
{
	int64_t num = parse_16bit(begin, len, err);
	if ((num < 0) || (num > 0x00FF)) *err = true;

	return (uint8_t)num;
}

/*	parse_uint16()
 * Parse an unsigned 16-bit integer from an ASCII string.
 */
uint16_t parse_uint16(const char *begin, size_t len, bool *err) {
	int64_t num = parse_16bit(begin, len, err);
	if ((num < 0) || (num > 0xFFFF)) *err = true;

	return (uint16_t)num;
}

/*	parse_int16()
 * Parse a signed 16-bit integer from an ASCII string.  32-bits is returned
 * so that this doesn't have to account for overflow.  Caller can truncate.
 */
int32_t parse_int16(const char *begin, size_t len, bool *err)
{
	int64_t num = parse_16bit(begin, len, err);
	if ((num < -32768) || (num > 32767)) *err = true;

	return (int32_t)num;
}
