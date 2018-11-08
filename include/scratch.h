/* multiple possible interfaces */
#include <Judy.h>
#include <nonlibc.h>
#include <stdbool.h>


/*	field
 * This is expressly a uint64_t size and meant to be passed by *value*.
 * Passing by value *eliminates* memory management (refcounts)
 * and pointer chasing.
 */
struct field {
	int32_t		offt;
	uint16_t	len;
	uint8_t		mask; /* must not be 0 */
	uint8_t		ff;  /* must be 0xff */
}__attribute__((packed));

uint64_t	field_hash(struct field field);


/*	field_meta
 * The "master description" of a field, so the user can refer to it by name.
 */
struct field_meta {
	char		*name;
	unsigned int	refcount;
	struct field	field;
};

extern Pvoid_t	J_fields; /* field_hash -> (struct field_meta *) */
extern Pvoid_t	JS_fields; /* field_name -> (struct field_meta *) */


/*	field_arr
 * Array of fields
 */
struct field_arr {
	uint32_t	len;
	uint32_t	flags:31;
	uint32_t	logic_or:1;
	struct field	arr[];
}__attribute__((packed));


struct field_ptr {
union {
	struct field	field;
	struct field_arr *arr; /* lower byte will *always* be zero on any sane system */
};
}__attribute__((packed));

void		field_ptr_free		(struct field_ptr ptr);
struct field_ptr field_ptr_append	(struct field_ptr ptr,
					struct field field);
bool		field_ptr_match		(struct field_ptr ptr,
					void *packet, size_t len);
char		*field_ptr_prn		(struct field_ptr ptr);



struct matcher {
	size_t		field_cnt;
	struct field_ptr fields;
};

struct action {
	struct iface	*ifc;
};

struct hook {
	void		*rcu_matchers;
};

struct iface {
	char		*name;
	struct hook	*in;
	struct hook	*out;
};
extern Pvoid_t	JS_ifaces; /* name -> (struct iface *) */
