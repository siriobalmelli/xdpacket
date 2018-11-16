#ifndef match2_h_
#define match2_h_

/*	match2.h
 * Describes use of fields to hash-match data inside a packet.
 * (c) 2018 Sirio Balmelli
 */

#include <field2.h>
#include <Judy.h>


/*	match_parse
 * Describe a (field, value) tuple to be matched.
 */
struct match_parse {
	char		*field_name;
	char		*value;
};

extern Pvoid_t	match_parse_J; /* (uint64_t match_id) -> (struct match_parse *parse) */


#endif /* match2_h_ */
