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
 * The notion of a "field" is simply a range of bytes, with an optional mask,
 * which can be used to:
 * - specify which parts of a packet to hash, when matching incoming packets.
 * - specify which bytes to write to (and possibly also read from),
 *	when mangling packet contents.
 *
 * This is expressly a uint64_t size and meant to be passed by *value*.
 */
struct field {
union {
struct {
	int32_t		offt;
	uint16_t	len;
	uint8_t		mask;	/* default 0xff, must no be 0 for a field */
	uint8_t		ff;	/* must be 0xff for a field */
}__attribute__((packed));
	uint64_t	bytes;
	struct field_arr *arr;	/* lower bits will *always* be zero (on any sane system) */
};
}__attribute__((packed));


/*	field_arr
 * An opaque 'struct field' (above) may actually be a pointer to a packed
 * array of fields; meaning that *multiple* fields are to be hashed
 * in sequence.
 */
struct field_arr {
	size_t		len;
	struct field	arr[];
};


#define FIELD_NULL ((struct field){ .bytes = 0 })
#define FIELD_IS_NULL(fld) (fld.bytes != 0)
#define FIELD_IS_ARRAY(fld) (fld.ff == 0)


/*	field_parse
 * Input params for a new field: parse from yaml or insert inline in call to new()
 */
struct field_parse {
	int32_t		offt;
	uint16_t	len;
	uint8_t		mask;
};


/*	field_free()
 * A field may be a *pointer* to an allocated field array;
 * always dispose of fields by calling field_free.
 */
NLC_INLINE
void		field_free	(struct field fld)
{
	if (!FIELD_IS_NULL(fld) && FIELD_IS_ARRAY(fld))
		free(fld.arr);
}

/*	field_id()
 * Returns the unique ID of a field or field array,
 * which is the fnv1a-64 hash of field parameters.
 * This ID is the *starting* seed used matching packets (see field_hash()),
 * so that the resulting hash represents a *unique* field+content fingerprint.
 */
uint64_t	field_id	(struct field fld);

/*	field_append()
 * Appends 'add' to 'fld' (which may be a field or array),
 * Writes the fnv1a-64 hash of field contents to '*field_id_out'.
 */
struct field	field_append	(struct field fld,
				struct field_parse add,
				uint64_t *field_id_out);

/*	field_new()
 */
NLC_INLINE
struct field	field_new	(struct field_parse add,
				uint64_t *field_id_out)
{
	return field_append(FIELD_NULL, add, field_id_out);
}

/*	field_hash()
 * Hashes 'packet' of 'len' bytes against 'fld'.
 * NOTE: 'fld' may be a single field or a field list.
 * Returns non-zero on failure to hash
 * (e.g. packet too short to allow for the offset specified in 'fld').
 * On success, sets '*out_hash' to the hashed value,
 * otherwise its value is undefined (possibly mangled).
 */
int	field_hash		(struct field fld,
				void *packet, size_t len,
				uint64_t *out_hash);


extern Pvoid_t	field_J;	/* (struct field field) -> (char *field_name) */
extern Pvoid_t	field_JS;	/* (char *field_name) -> (struct field field) */



enum action_type {
	ACTION_INVALID = 0,
	ACTION_MANGLE = 1,	/* value -> field */
	ACTION_COPY = 2,	/* field -> field */
	ACTION_OUTPUT = 3	/* out_interface */
};

struct action_parse {
	char	*source;
};

struct action {
	enum action_type	type;
	union {
		struct field		field;
		uint64_t		u64_be;
		uint32_t		u32_be;
		uint16_t		u16_be;
		uint8_t			u8;
		uint8_t			bytes[4];
		char			*mem;
	}			source;
	union {
		/* TODO: interface */
		struct field		field;
	}			dest;
};



struct node {
	struct field	match;
};
