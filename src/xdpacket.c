#include <xdpacket.h>
#include <ndebug.h>
#include <epoll_track.h>
#include <posigs.h>
#include <iface.h>
#include <parse2.h>
#include <ndebug.h>
#include <process.h>

/* TCP socket
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>

/*	netsock_callback()
 * Executed when listening sockets trigger EPOLLIN; calls accept()
 * and sets up parser.
 */
int netsock_callback(int fd, uint32_t events, void *context)
{
	struct epoll_track *tk = context;
	struct parse *ps = NULL;
	struct sockaddr client = { 0 };
	socklen_t client_sz = sizeof(client);
	int newfd = accept(fd, &client, &client_sz);
	NB_die_if(newfd < 1, "cannot accept connection");

	NB_die_if(!(
		ps = parse_new(newfd, newfd)
		), "");
	NB_die_if(
		eptk_register(tk, ps->fdin, EPOLLIN | EPOLLMSG, parse_callback, ps, parse_free)
		, "");

	char buf[INET_ADDRSTRLEN];
	inet_ntop(client.sa_family, &client, buf, sizeof(buf));
	NB_wrn("connected to %s, fd %d", buf, ps->fdin);

	return 0;
die:
	if (newfd > 0) {
		close(newfd);
		if (ps)
			ps->fdin = ps->fdout = -1;
	}
	parse_free(ps);
	return 1;
}

/*	netsock()
 * Get all localhost sockets and listen() on every one;
 * then add them to 'tk'
 */
int netsock(struct epoll_track *tk)
{
	int err_cnt = 0;
	char *port = PORT;

	struct addrinfo *servinfo = NULL;
	struct addrinfo hints = {
		.ai_family = AF_UNSPEC, /* don't care whether IPv4 or v6 */
		.ai_socktype = SOCK_STREAM /* TCP */
	};
	NB_die_if(
		getaddrinfo("localhost", port, &hints, &servinfo)
		, "");

	void *addr;
	char buf[INET_ADDRSTRLEN];
	for (struct addrinfo *info = servinfo; info; info = info->ai_next) {
		/* hand-waving to print a legible address */
		if (info->ai_family == AF_INET)
			addr = &((struct sockaddr_in *)info->ai_addr)->sin_addr;
		else
			addr = &((struct sockaddr_in6 *)info->ai_addr)->sin6_addr;
		inet_ntop(info->ai_family, addr, buf, sizeof(buf));

		/* make socket, for fun and profit */
		int sockfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
		NB_die_if(sockfd < 1, "could not open socket on %s", buf);
		NB_die_if(
			fcntl(sockfd, F_SETFL, O_NONBLOCK)
			, "could not set socket nonblocking");
		int yes = 1;
		NB_die_if(
			setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))
			, "could not set socket reusable");
		NB_die_if(
			bind(sockfd, info->ai_addr, info->ai_addrlen)
			, "could not bind to %s :%s", buf, port);
		NB_die_if(
			listen(sockfd, 2)
			, "could not listen on %s: %s", buf, port);
		NB_prn("listening on: %s :%s", buf, port);

		/* No destructor: epoll_track will just close() sockfd when
		 * program exist or when netsock_callback() returns non-0.
		 */
		NB_die_if(
			eptk_register(tk, sockfd, EPOLLIN, netsock_callback, tk, NULL)
			, "could not register with epoll");
	}

die:
	freeaddrinfo(servinfo);
	return err_cnt;
}


/*	main()
 * TODO: reopen an fd to stdin if given a file fd? (aka: don't abandon CLI)
 */
int main()
{
	int err_cnt = 0;
	struct parse *ps = NULL;

	NB_die_if(
		psg_sigsetup(NULL)
		, "failed to set up signals");
	NB_die_if(!(
		tk = eptk_new()
		) || !(
		ps = parse_new(dup(fileno(stdin)), dup(fileno(stdout)))
		), "failed to allocate objects");

	/* all user input dealt with by parse_callback() */
	NB_die_if(
		eptk_register(tk, ps->fdin, EPOLLIN, parse_callback, ps, parse_free)
		, "fd_in %d", fileno(stdin));

	netsock(tk);

	/* epoll loop */
	int res;
	while(!psg_kill_check()) {
		NB_die_if((
			res = eptk_pwait_exec(tk, -1, NULL)
			) < 0, "");
	}

die:
	process_free_all();
	rule_free_all();
	field_free_all();

	/* no: eptk_free() will call iface_free() on all registered fd's
	 * iface_free_all();
	 */
	eptk_free(tk);
	return err_cnt;
}
