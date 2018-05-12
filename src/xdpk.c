#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>

#include <zed_dbg.h>
#include <xdpk.h>


/*	xdpk_free()
 */
void xdpk_sock_free(struct xdpk_sock *sk)
{
	if (!sk)
		return;
	if (sk->fd != -1)
		close(sk->fd);
	free(sk);
}

/*	xdpk_open()
 * open an interface
 */
struct xdpk_sock *xdpk_sock_new(const char *ifname)
{
	struct xdpk_sock *ret = NULL;
	size_t alloc = sizeof(struct xdpk_sock) + (XDPK_FRAME_SZ * XDPK_FRAME_CNT);
	Z_die_if(!(
		ret = calloc(alloc, 1)
		), "alloc size %zu", alloc);
	ret->fd = -1;	
	Z_die_if((
		ret->fd = socket(PF_XDP, SOCK_RAW, 0)
		) == -1, "");

	return ret;
out:
	xdpk_sock_free(ret);
	return NULL;
}


/*	main()
 */
int main(int argc, char **argv)
{
	struct xdpk_sock *sk = NULL;

	Z_die_if(!(
		sk = xdpk_sock_new(argv[1])
		), "");

out:
	xdpk_sock_free(sk);
	return 0;
}
