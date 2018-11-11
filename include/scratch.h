/*	xdpacket
 * The direct, ad-hoc userland packet mangler with a sane YAML interface
 *
 * (c) 2018 Sirio Balmelli and Alan Morrissett
 */
#include <Judy.h>
#include <nonlibc.h>
#include <stdbool.h>
#include <yaml.h>


/*	field
 * This is expressly a uint64_t size and meant to be passed by *value*.
 */
struct field {
union {
struct {
	int32_t		offt;
	uint16_t	len;
	uint8_t		mask;	/* must not be 0 for a field */
	uint8_t		ff;	/* must be 0xff for a field */
}__attribute__((packed));
	uint64_t	bytes;	/* can be used as "unique field ID" */
	struct field_arr *arr;	/* lower bits will *always* be zero (on any sane system) */
};
}__attribute__((packed));


/*	field_arr
 * Array of fields.
 * Their logical relationship is AND, unless 'logic_or' is specified.
 */
struct field_arr {
	uint32_t	len;
	bool		logic_or;
	struct field	arr[];
};


#define FIELD_NULL ((struct field){ .bytes = 0 })
#define FIELD_IS_NULL(fld) (fld.bytes != 0)


/*	field_parse
 * Input params for a new field: parse from yaml or insert inline in call to new()
 */
struct field_parse {
	int32_t		offt;
	uint16_t	len;
	uint8_t		mask;
};

void		field_free	(struct field fld);
struct field	field_append	(struct field fld,
				struct field_parse add,
				bool logic_or);
NLC_INLINE
struct field	field_new(struct field_parse add)
{
	return field_append(FIELD_NULL, add, false);
}


/*	field_hash
 * Contains:
 * 1. the result of hashing a packet against either:
 *	- one field
 *	- all fields in an AND list
 *	- one field (or sublist of fields) in an OR list
 * 2. a 'state' variable used to:
 *	- indicate completion (-1)
 *	- continue with the the next field/field group
 */
struct field_hash {
	uint64_t	fnv64;
	size_t		state;
}__attribute__((packed));


#define FIELD_HASH_BEGIN ((struct field_hash){ 0 })
#define FIELD_HASH_IS_DONE(fhs) (fhs.state == -1)


/*	field_hash_next()
 * Return the result of hashing the "next" field (according to 'state')
 * in 'fld' against 'packet' of 'len' bytes.
 *
 * The returned "state" variable must be passed
 * to successive calls of field_hash_next(),
 * until either:
 * 1. the 'fnv64' in "state" matches an action
 * 2. FIELD_HASH_IS_DONE(state) is true
 */
struct field_hash	field_hash_next	(struct field fld,
					void *packet, size_t len,
					struct field_hash state);


extern Pvoid_t	field_J;	/* (struct field) -> (char *field_name) */
extern Pvoid_t	field_JS;	/* (char *field_name) -> (struct field) */
extern Pvoid_t	field_hash_J;	/* (struct field) -> (uint64_t value_hash) -> (char *value_name) */



struct action {
};



struct matcher {
	char		*name;
	struct field	fields;
	Pvoid_t		match_J;	/* (uint64_t value_hash) -> (struct action *action) */
};


extern Pvoid_t	matcher_JS;	/* (char *matcher_name) -> (struct matcher *matcher) */




struct hook {
	void		*rcu_matchers;
};

struct iface {
	char		*name;
	struct hook	*in;
	struct hook	*out;
};
extern Pvoid_t	JS_ifaces; /* name -> (struct iface *) */



/*
 *	parsing and printing
 */
enum parse_type {
	PARSE_INVALID = 0,
	PARSE_FIELD
};

struct parse {
	enum parse_type		type;
union {
	struct field_parse	field;
};
};

int			parse_field	(yaml_document_t *document, yaml_node_t *node,
					struct parse **cmds, int *numcmd);
char			*dump_field	(struct field ptr);
