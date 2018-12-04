#ifndef parse2_h_
#define parse2_h_

/*	parse2.h
 * YAML I/O into the respective "parse" structures and tracking J arrays.
 * None of these are in the fast path: simply encode and spit back user text
 * and allow for lookup by name.
 * (c) 2018 Sirio Balmelli
 */

#include <yaml.h>
#include <stdint.h>
#include <nonlibc.h>


struct parse {
	int		fdin;
	int		fdout;
	unsigned char	*buf;
	size_t		buf_len;
	size_t		buf_pos;
};


void parse_free(struct parse *ps);
struct parse *parse_new(int fdin, int fdout);
void parse_exec(const unsigned char *doc, size_t doc_len);
void parse_callback(int fd, uint32_t events, void *context);


/* global "mode": how to treat nodes being parsed */
enum parse_mode {
	PARSE_INVALID,
	PARSE_ADD,
	PARSE_DEL,
	PARSE_PRN
};

extern const char *parse_modes[];

NLC_INLINE const char *parse_mode_prn(enum parse_mode mode)
{
	return parse_modes[mode];
}


#endif /* parse2_h_ */
