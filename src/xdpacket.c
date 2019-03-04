#include <xdpacket.h>
#include <ndebug.h>
#include <epoll_track.h>
#include <posigs.h>
#include <iface.h>
#include <parse2.h>
#include <ndebug.h>
#include <process.h>
#include <getopt.h>

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
int netsock(struct epoll_track *tk, const char *ip_addr)
{
	int err_cnt = 0;
	char *port = PORT;

	struct addrinfo *servinfo = NULL;
	struct addrinfo hints = {
		.ai_family = AF_UNSPEC, /* don't care whether IPv4 or v6 */
		.ai_socktype = SOCK_STREAM /* TCP */
	};
	NB_die_if(
		getaddrinfo(ip_addr, port, &hints, &servinfo)
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


/* Use as a printf prototype.
 * Expects 'program_name' as a string variable.
 */
static const char *usage =
"usage: %s [[-i IP_ADDRESS], ...]\n"
"Options:\n"
"	-i, --ip IP_ADDRESS	: open a CLI socket on IP_ADDRESS:7044\n"
"	-h, --help		: print usage and exit\n";


/*	main()
 * TODO: reopen an fd to stdin if given a file fd? (aka: don't abandon CLI)
 */
int main(int argc, char **argv)
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

	/* Parse options after setting up epoll_track:
	 * create e.g. several listening sockets.
	 */
	{
		int opt;
		static struct option long_options[] = {
			{ "ip",		required_argument,	0,	'i'},
			{ "help",	no_argument,		0,	'h'},
			{0, 0, 0, 0}
		};
		while ((opt = getopt_long(argc, argv, "i:h", long_options, NULL)) != -1) {
			switch(opt) {
			case 'i':
				NB_die_if(
					netsock(tk, optarg)
					, "");
				break;
			case 'h':
				fprintf(stderr, usage, argv[0]);
				goto die;
				break;
			default:
				NB_die(""); /* libc will already complain about invalid option */
			}
		}
	}

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
