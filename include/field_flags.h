#ifndef field_flags_h_
#define field_flags_h_

/*	field_flags.h
 * Central repository of flags used in 'struct field_set',
 * preclude accidental conflicting usage.
 */
enum field_flags {
	FIELD_NONE		= 0x0,
	FIELD_FVAL		= 0x1,
	FIELD_FREF_STORE	= 0x2,
	FIELD_FREF_COPY		= 0x4
};


#endif /* field_flags_h_ */
