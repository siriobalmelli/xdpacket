#include <stdlib.h>
#include <unistd.h>

#include <epoll_track.h>
#include <posigs.h>
#include <ndebug.h>
#include <iface.h>

/*	main()
 */
int main(int argc, char **argv)
{
	struct iface *sk = NULL;

	NB_die_if(
		psg_sigsetup(NULL)
		, "failed to set up signals");
	NB_die_if(!(
		tk = eptk_new()
		), "failed to set up epoll");
	NB_die_if(!(
		sk = iface_new(argv[1])
		), "failed to open socket on '%s'", argv[1]);
	NB_die_if(
		eptk_register(tk, sk->fd, EPOLLIN, iface_callback, sk, iface_free),
		"failed to register epoll on %d", sk->fd);

	int res;
	while(!psg_kill_check()) {
		NB_die_if((
			res = eptk_pwait_exec(tk, -1, NULL)
			) < 0, "");
	}

die:
	eptk_free(tk);
	return 0;
}
