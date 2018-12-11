#include <xdpacket.h>
#include <ndebug.h>
#include <epoll_track.h>
#include <posigs.h>
#include <iface.h>
#include <parse2.h>


/*	main()
 */
int main()
{
	int err_cnt = 0;
	struct epoll_track *tk = NULL;
	struct parse *ps = NULL;

	NB_die_if(
		psg_sigsetup(NULL)
		, "failed to set up signals");
	NB_die_if(!(
		tk = eptk_new()
		) || !(
		ps = parse_new(fileno(stdin), fileno(stdout))
		), "failed to allocate objects");

	/* all user input dealt with by parse_callback() */
	NB_die_if(
		eptk_register(tk, ps->fdin, EPOLLIN, parse_callback, ps)
		, "fd_in %d", fileno(stdin));

	/* epoll loop */
	int res;
	while(!psg_kill_check()) {
		NB_die_if((
			res = eptk_pwait_exec(tk, -1, NULL)
			) < 0, "");
	}

die:
	parse_free(ps);
	eptk_free(tk, false);
	return err_cnt;
}
