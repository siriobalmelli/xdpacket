#include <xdpacket.h>
#include <yaml.h>
#include <zed_dbg.h>
#include <epoll_track.h>
#include <posigs.h>
#include <limits.h> /* PIPE_BUF */
#include <unistd.h>


struct parse {
	int		fd_in;
	int		fd_out;
	yaml_parser_t	parser;
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
struct parse *parse_new(int fd_in, int fd_out)
{
	struct parse *ret = NULL;
	Z_die_if(!(
		ret = calloc(sizeof(struct parse), 1)
		), "alloc %zu", sizeof(struct parse));
	ret->fd_in = fd_in;
	ret->fd_out = fd_out;
	yaml_parser_initialize(&ret->parser);
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
	char buf[PIPE_BUF];
	ssize_t res;
	while ((res = read(ps->fd_in, buf, PIPE_BUF)) > 0)
		Z_wrn_if(
			write(ps->fd_out, buf, res)
			!= res, "");
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
		eptk_register(tk, ps->fd_in, EPOLLIN,
			parse_callback, (epoll_data_t)(void*)ps)
		, "fd_in %d", ps->fd_in);

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
