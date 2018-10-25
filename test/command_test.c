/*	command_test.c
 * (c) 2018 Alan Morrissett
 * (c) 2018 Sirio Balmelli
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <getopt.h>
#include <assert.h>
#include <yaml.h>
#include <parse.h>

/*	bulk_command_check()
 * Validate that all commands parse correctly.
 */
int bulk_command_check()
{
	int err_cnt = 0;

	return err_cnt;
}

#ifdef BUF_SIZE
#undef BUF_SIZE
#endif
#define BUF_SIZE 4096

/*	interactive_parse_check()
 */
void interactive_command_check()
{
	char buf[BUF_SIZE];
	int numcmds;
	xdpk_command_t *cmds;

	while (true) {
		int len = 0;
		memset(buf, 0, BUF_SIZE);
		printf("Enter XDPK command: ");
		if (fgets(buf, sizeof(buf), stdin) == NULL)
			break;
		len = strlen(buf);
		if (buf[len-1] == '\n')
			buf[--len] = '\0';
		if (len == 0) 
			continue;
		printf("You entered: '%s'\n", buf);
		if (parse_commands(buf, len, &cmds, &numcmds)) {
			printf("error on parsing\n");
		} else {
			printf("Commands parsed: %d\n", numcmds);
			if (cmds != NULL)
				delete_commands(&cmds, numcmds);
		}

	}
}

/*	main()
 * Run all test routines in this file.
 * Expects test routines to return err_cnt (0 on success).
 */
int main(int argc, char *argv[])
{
	int err_cnt = 0;
	int opt = 0;
	bool interactive = false;

	if (isatty(0)) {
		while ((opt = getopt(argc, argv, "i")) != -1) {
			switch (opt) {
			case 'i':
				interactive = true;
				break;
			default:
				fprintf(stderr, "Usage: %s [-i]\n", argv[0]);
				exit(EXIT_FAILURE);
			}
		}
		if (interactive)
			interactive_command_check();
	}

	if (!interactive)
		err_cnt += bulk_command_check();

	return err_cnt;
}
