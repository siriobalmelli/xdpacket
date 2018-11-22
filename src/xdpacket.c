#include <xdpacket.h>
#include <yaml.h>
#include <zed_dbg.h>
#include <epoll_track.h>
#include <posigs.h>
#include <limits.h> /* PIPE_BUF */
#include <unistd.h>
#include <fcntl.h> /* fcntl() for nonblocking I/O */


struct parse {
	int		fdin;
	int		fdout;
	unsigned char	*buf;
	size_t		buf_len;
};


/*	parse_free()
 */
void parse_free(struct parse *ps)
{
	if (!ps)
		return;
	free(ps->buf);
	free(ps);
}

/*	parse_new()
 */
struct parse *parse_new(int fdin, int fdout)
{
	struct parse *ret = NULL;
	Z_die_if(!(
		ret = calloc(sizeof(struct parse), 1)
		), "alloc %zu", sizeof(struct parse));
	/* input fd must be nonblocking */
	Z_die_if(
		fcntl(fdin, F_SETFL, fcntl(fdin, F_GETFL) | O_NONBLOCK)
		, "");
	ret->fdin = fdin;
	ret->fdout = fdout;
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

	/* read all available bytes into our contiguous buffer without blocking */
	size_t bcnt = 0;
	ssize_t res = 0;
	do {
		bcnt += res;
		Z_log(Z_inf, "loop read on %d @%zu", ps->fdin, bcnt);
		/* extend memory as needed */
		if (bcnt + PIPE_BUF > ps->buf_len) {
			ps->buf_len += PIPE_BUF;
			Z_die_if(!(
				ps->buf = realloc(ps->buf, ps->buf_len)
				), "size %zu", ps->buf_len);
		}
		res = read(ps->fdin, &ps->buf[bcnt], PIPE_BUF);
	} while (res == PIPE_BUF); /* a read of < PIPE_BUF implies no data left */

	yaml_parser_t parser;
	Z_die_if(!
		yaml_parser_initialize(&parser)
		, "");
	yaml_parser_set_input_string(&parser, ps->buf, bcnt);

	/* a non-zero return indicates we did not error */
	yaml_event_t event;
	while (yaml_parser_parse(&parser, &event)) {
		Z_log(Z_inf, "valid event");
		if (event.type == YAML_STREAM_END_EVENT) {
			Z_log(Z_inf, "end token");
			break;
		}
		yaml_event_delete(&event);
	}
	yaml_parser_delete(&parser);
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

	Z_die_if(
		psg_sigsetup(NULL)
		, "failed to set up signals");
	Z_die_if(!(
		tk = eptk_new()
		) || !(
		ps = parse_new(fileno(stdin), fileno(stdout))
		), "failed to allocate objects");

	/* all user input dealt with by parse_callback() */
	Z_die_if(
		eptk_register(tk, ps->fdin, EPOLLIN,
			parse_callback, (epoll_data_t)(void*)ps)
		, "fd_in %d", fileno(stdin));

	/* epoll loop */
	int res;
	while(!psg_kill_check()) {
		Z_die_if((
			res = eptk_pwait_exec(tk, -1, NULL)
			) < 0, "");
	}

out:
	parse_free(ps);
	eptk_free(tk, false);
	return err_cnt;
}
