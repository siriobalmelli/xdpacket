#ifndef match2_h_
#define match2_h_

/*	match2.h
 * Actual packet matching infrastructure.
 * (c) 2018 Sirio Balmelli
 */

#include <field2.h>
#include <fval.h>

struct matcher_write {
	size_t			write_cnt;
	struct fval_bytes	writers[];
};

struct matcher {
	struct matcher_write	*writer;
	uint64_t		hash;
	size_t			set_cnt;
	struct field_set	sets[];
};


void	matcher_free	(void *arg);



#endif /* match2_h_ */
