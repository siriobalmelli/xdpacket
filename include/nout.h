#ifndef nout_h_
#define nout_h_

/*	nout.h
 * Representation of user-supplied (node_name, out_iface_name) tuple,
 * given to us as strings.
 *
 * (c) 2018 Sirio Balmelli
 */

#include <yaml.h>
#include <node2.h>
#include <iface.h>


struct nout {
	struct iface	*out_if;
	struct node	*node;
};


void		nout_free	(void *arg);

struct nout	*nout_new	(const char *field_name,
				const char *value);

int		nout_emit	(struct nout *nout,
				yaml_document_t *outdoc,
				int outlist);

#endif /* nout_h_ */
