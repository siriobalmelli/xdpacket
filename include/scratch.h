/* multiple possible interfaces */
#include <Judy.h>

struct field {
};

struct matcher {
};

struct action {
};

struct hook {
};

struct iface {
	char *name;
};

extern Pvoid_t JS_ifaces; /* name -> (struct iface *) */

 
