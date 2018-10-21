#ifndef MATCHER2_H
#define MATCHER2_H

#include <xdpk.h> /* must always be first */
#include <Judy.h>
#include <stdbool.h>

struct selector {
	/* TODO: implement */
};

struct matcher {
	/* TODO: implement */	
	size_t sel_cnt;
	struct selector sel[];
};

void		matcher_free(struct matcher *mch);
struct matcher	*matcher_new();
bool		matcher_do(struct matcher *mch, const void *buf, size_t buf_len);

#endif /* MATCHER2_H */
