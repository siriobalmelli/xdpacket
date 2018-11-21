#include <xdpacket.h>
#include <yaml.h>
#include <zed_dbg.h>
#include <epoll_track.h>
#include <posigs.h>
#include <limits.h> /* PIPE_BUF */
#include <unistd.h>


struct parse {
	FILE		*fin;
	FILE		*fout;
	yaml_parser_t	parser;
	yaml_event_t	event;
};


/*	parse_free()
 */
void parse_free(struct parse *ps)
{
	if (!ps)
		return;
	yaml_parser_delete(&ps->parser);
	free(ps);
}

/*	parse_new()
 */
struct parse *parse_new(FILE *fin, FILE *fout)
{
	struct parse *ret = NULL;
	Z_die_if(!(
		ret = calloc(sizeof(struct parse), 1)
		), "alloc %zu", sizeof(struct parse));
	ret->fin = fin;
	ret->fout = fout;
	return ret;
out:
	free(ret);
	return NULL;
}

/*	parse_callback()
 */
void parse_callback(int fd, uint32_t events, epoll_data_t context)
{
	struct parse *ps = context.ptr;
	Z_die_if(!
		yaml_parser_initialize(&ps->parser)
		, "");
	yaml_parser_set_input_file(&ps->parser, ps->fin);

	/* a non-zero return indicates we did not error */
	if (yaml_parser_parse(&ps->parser, &ps->event)) {
		Z_log(Z_inf, "valid token");
		if (ps->event.type == YAML_STREAM_END_EVENT)
			Z_log(Z_inf, "end token");
		yaml_event_delete(&ps->event);
	}
	yaml_parser_delete(&ps->parser);
	return;
out:
	psg_kill(); /* the body cannot survive without the mind */
	return;
}


/*	main()
 */
int main()
{
	int err_cnt = 0;
	struct epoll_track *tk = NULL;
	struct parse *ps = NULL;

#if 0
	Z_die_if(
		psg_sigsetup(NULL)
		, "failed to set up signals");
	Z_die_if(!(
		tk = eptk_new()
		) || !(
		ps = parse_new(stdin, stdout)
		), "failed to allocate objects");

	/* all user input dealt with by parse_callback() */
	Z_die_if(
		eptk_register(tk, fileno(stdin), EPOLLIN,
			parse_callback, (epoll_data_t)(void*)ps)
		, "fd_in %d", fileno(stdin));

	/* epoll loop */
	int res;
	while(!psg_kill_check()) {
		Z_die_if((
			res = eptk_pwait_exec(tk, -1, NULL)
			) < 0, "");
	}
#else
	yaml_parser_t parser;
	yaml_event_t event;
	Z_die_if(!
		yaml_parser_initialize(&parser)
		, "");
	yaml_parser_set_input_file(&parser, stdin);

	/* a non-zero return indicates we did not error */
	while (yaml_parser_parse(&parser, &event)) {
		Z_log(Z_inf, "valid event");
		if (event.type == YAML_STREAM_END_EVENT) {
			Z_log(Z_inf, "end token");
			break;
		}
		yaml_event_delete(&event);
	}
	yaml_parser_delete(&parser);
#endif

out:
	parse_free(ps);
	eptk_free(tk, false);
	return err_cnt;
}
