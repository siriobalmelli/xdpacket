#ifndef memref_h_
#define memref_h_

/* memref = MEMory REFerence
 *
 * A chunk of allocated memory with relevant metadata.
 * This may be mutable globaly referenced memory (aka "state")
 * or may be a literal value for use in comparing or writing into packets.
 */

#include <stdint.h>
#include <field.h>
#include <yaml.h>


#define MEMREF_FLAG_STATIC	0x1  /* use memory->set.flags to track memory type */


struct memref {
union {
struct {	/* variable global state */
	char			*name;
	uint32_t		refcnt;
};
struct {	/* static value */
	char			*input;		/* literal user input value */
	char			*rendered;	/* human-readable representation of 'bytes' */
};
};
		/* referenced memory */
	struct field_set	set;
	uint8_t			bytes[];
};


void memref_free(void *ref);

struct memref *memref_value_new(const char *field_name, const char *value);
struct memref *memref_state_get(const char *state_name, size_t len);

int memref_emit (struct memref *ref, yaml_document_t *outdoc, int outmapping);


#endif /* memref_h_ */
