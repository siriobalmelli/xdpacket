#ifndef node2_h_
#define node2_h_

/*	node2.h
 * Node: an input interface, a set of matchers and a set of resulting actions.
 * A node is the atomic unit of program control.
 * (c) 2018 Sirio Balmelli
 */

#include <field2.h>
#include <xform2.h>
#include <match2.h>
#include <iface.h>

/*	field_value
 * Representation of user-supplied (fieldname, value) tuple.
 * NOTE that the user gave 'fieldname' as a string, but we store a pointer so that:
 * - we clearly show that 'fieldname' _must_ refer to a valid field
 * - we save a string lookup to get the field's 'set'
 * - this struct stays the size of a cacheline
 * - we are forced to clean up dependant field_value structs when fields are freed
 *   or else have dangling pointers
 */
struct field_value {
	char		value[MAXLINELEN];
	struct field	*field;
};

/*	node
 * The basic atom of xdpacket; describes user intent in terms of
 * input (seq) -> match -> write (aka: mangle) -> output
 */
struct node {
	char		name[MAXLINELEN];
	struct iface	*in;
	uint64_t	seq;
	struct iface	*out;

	struct field_value	*match_list;
	size_t			match_count;

	struct field_value	*write_list;
	size_t			write_count;

};


void node_free(void *arg);

struct node *node_new	(const char *name,
			struct iface *in,
			uint64_t seq,
			struct iface *out);

struct node *node_add_match(struct node *ne,
			const char *value,
			struct field *field);

struct node *node_add_write(struct node *ne,
			const char *value,
			struct field *field);


#endif /* node2_h_ */
