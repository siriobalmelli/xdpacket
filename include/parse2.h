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


#endif /* parse2_h_ */
