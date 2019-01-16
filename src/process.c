/*	process.c
 * (c) 2018 Sirio Balmelli
 */

#include <process.h>
#include <ndebug.h>
#include <judyutils.h>
#include <yamlutils.h>


static Pvoid_t	process_JS = NULL; /* (char *in_iface_name) -> (struct process *process) */


/*	process_free()
 */
void process_free(void *arg)
{
	if (!arg)
		return;
	struct process *pc = arg;

	if (pc->in_iface) {
		iface_handler_clear(pc->in_iface, process_exec, pc->rout_set_JQ);
		iface_release(pc->in_iface);
		js_delete(&process_JS, pc->in_iface->name);
	}

	JL_LOOP(&pc->rout_JQ,
		rout_free(val);
	);
	int rc;
	JLFA(rc, pc->rout_JQ);
	JLFA(rc, pc->rout_set_JQ);

	free(pc);
}

/*	process_free_all()
 */
static void __attribute__((destructor(2))) process_free_all()
{
	JS_LOOP(&process_JS,
		process_free(val);
	);
}


/*	process_new()
 * Create a new process.
 */
struct process *process_new(const char *in_iface_name, Pvoid_t rout_JQ)
{
	struct process *ret = NULL;
	NB_die_if(!in_iface_name, "process requires in_iface_name");

	/* no easy way of knowing if dups are identical, kill them */
	if ((ret = js_get(&process_JS, in_iface_name))) {
		NB_wrn("process '%s' already exists: deleting", in_iface_name);
		process_free(ret);
	}

	NB_die_if(!(
		ret = calloc(1, sizeof(*ret))
		), "fail alloc size %zu", sizeof(*ret));

	NB_die_if(!(
		ret->in_iface = iface_get(in_iface_name)
		), "could not get interface '%s'", in_iface_name);
	ret->rout_JQ = rout_JQ;

	JL_LOOP(&ret->rout_JQ,
		struct rout *rt = val;
		jl_enqueue(&ret->rout_set_JQ, rt->set);
	);
	NB_die_if(
		iface_handler_register(ret->in_iface, process_exec, ret->rout_set_JQ)
		, "");

	js_insert(&process_JS, ret->in_iface->name, ret, true);

	return ret;
die:
	process_free(ret);
	return NULL;
}


/*	process_get()
 */
struct process *process_get(const char *in_iface_name)
{
	return js_get(&process_JS, in_iface_name);
}


/*	process_exec()
 */
void process_exec(void *context, void *pkt, size_t len)
{
	Pvoid_t rout_set_JQ = context;
	JL_LOOP(&rout_set_JQ,
		struct rout_set *rst = val;
		if (rout_set_match(rst, pkt, len) && !rout_set_write(rst, pkt, len)) {
			iface_output(rst->if_out, pkt, len);
			break;
		}
	);
}


/*	process_parse()
 */
int process_parse(enum parse_mode mode,
		yaml_document_t *doc, yaml_node_t *mapping,
		yaml_document_t *outdoc, int outlist)
{
	int err_cnt = 0;
	struct process *process = NULL;

	/* possible params for a new field.
	 * All 'long' because we use strtol() to parse.
	 */
	const char *name = NULL;
	Pvoid_t routs_JQ = NULL;

	/* parse mapping */
	for (yaml_node_pair_t *pair = mapping->data.mapping.pairs.start;
		pair < mapping->data.mapping.pairs.top;
		pair++)
	{
		/* loop boilerplate */
		yaml_node_t *key = yaml_document_get_node(doc, pair->key);
		const char *keyname = (const char *)key->data.scalar.value;
		yaml_node_t *val = yaml_document_get_node(doc, pair->value);

		if (val->type == YAML_SCALAR_NODE) {
			const char *valtxt = (const char *)val->data.scalar.value;

			if (!strcmp("process", keyname) || !strcmp("p", keyname))
				name = valtxt;
			else
				NB_err("'process' does not implement '%s'", keyname);

		} else if (val->type == YAML_SEQUENCE_NODE) {
			if (!strcmp("rules", keyname) || !strcmp("r", keyname)) {
				Y_SEQ_MAP_PAIRS_EXEC(doc, val,
					/* rely on enqueue() to test 'fv' (a NULL datum is invalid) */
					NB_err_if(
						jl_enqueue(&routs_JQ, rout_new(keyname, valtxt))
						, "");
					);

			} else {
				NB_err("'process' does not implement '%s'", keyname);
			}

		} else {
			NB_die("'%s' in field not a scalar or sequence", keyname);
		}
	}

	/* process based on 'mode' */
	switch (mode) {
	case PARSE_ADD:
	{
		NB_die_if(!(
			process = process_new(name, routs_JQ)
			), "could not create process on interface '%s'", name);
		NB_die_if(process_emit(process, outdoc, outlist), "");
		break;
	}

	case PARSE_DEL:
		NB_die_if(!(
			process = process_get(name)
			), "could not get interfaces '%s'", name);
		NB_die_if(
			process_emit(process, outdoc, outlist)
			, "");
		process_free(process);
		break;

	case PARSE_PRN:
		/* if nothing is given, print all */
		if (!strcmp("", name)) {
			JS_LOOP(&process_JS,
				NB_die_if(
					process_emit(val, outdoc, outlist)
					, "");
			);
		/* otherwise, search for a literal match */
		} else if ((process = process_get(name))) {
			NB_die_if(process_emit(process, outdoc, outlist), "");
		}
		break;

	default:
		NB_err("unknown mode %s", parse_mode_prn(mode));
	};

die:
	return err_cnt;
}


/*	process_emit()
 * Emit a process as a mapping under 'outlist' in 'outdoc'.
 */
int process_emit(struct process *process, yaml_document_t *outdoc, int outlist)
{
	int err_cnt = 0;
	int reply = yaml_document_add_mapping(outdoc, NULL, YAML_BLOCK_MAPPING_STYLE);

	int nodes = yaml_document_add_sequence(outdoc, NULL, YAML_BLOCK_SEQUENCE_STYLE);
	JL_LOOP(&process->rout_JQ,
		NB_die_if(
			rout_emit(val, outdoc, nodes)
			, "fail to emit rout");
	);

	NB_die_if(
		y_pair_insert(outdoc, reply, "process", process->in_iface->name)
		|| y_pair_insert_obj(outdoc, reply, "nodes", nodes)
		, "");
	NB_die_if(!(
		yaml_document_append_sequence_item(outdoc, outlist, reply)
		), "");
die:
	return err_cnt;
}
