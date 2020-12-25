#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ipc_defs.h"

#define streq(a, b) (!strcmp(a, b))

static struct option opts[] = {
	{ "help" , no_argument, 0, 'h' },
	{ "verbose" , no_argument, 0, 'v' },
	{ "action", required_argument, 0, 'a' },
	{ "pid", required_argument, 0, 'p' },
	{ "socket", required_argument, 0, 's' },
	{ "socket-path", required_argument, 0, 'P' }
};


void __attribute__((noreturn)) usage(int code) {
	fprintf(stderr, "Usage: TODO\n");
	exit(code);
}

void __attribute__((noreturn)) err(const char* name, const char* msg) {
	fprintf(stderr, "%s: %s\n", name, msg);
	usage(1);
}

int main(int argc, char** argv) {
	int action = -1, verbose = 0;
	char* sock_path = NULL;

	int* pids = NULL;
	int num_pids = 0;
	const char** sockets = NULL;
	int num_sockets = 0;

	while (1) {
		int opt = getopt_long(argc, argv, "hva:p:s:P:", opts, NULL);

		if (opt == -1) {
			break;
		}

		switch (opt) {
		case '?':
			usage(1);
			break;

		case 'h':
			usage(0);
			break;

		case 'v':
			verbose = 1;
			break;

		case 'a':
			if (action != -1) {
				err(argv[0], "You should specify only one action");
			}

			if (streq(optarg, "swap")) {
				action = SWAPIFY_MSG_SWAP;
			}
			else if (streq(optarg, "unswap")) {
				action = SWAPIFY_MSG_UNSWAP;
			}
			else if (streq(optarg, "exit")) {
				action = SWAPIFY_MSG_EXIT;
			}
			else {
				err(argv[0], "argument to --action must be swap, unswap or exit");
				return 1;
			}
			break;

		case 'p':
		{
			int new_pid = atoi(optarg);
			if (new_pid <= 0) {
				err(argv[0], "Invalid pid");
				return 1;
			}

			pids = realloc(pids, ++num_pids * sizeof(*pids));
			if (pids == NULL) {
				err(argv[0], "Out of memory");
			}

			pids[num_pids - 1] = new_pid;
		}
			break;

		case 's':
		{
			sockets = realloc(sockets, ++num_sockets * sizeof(*sockets));
			if (sockets == NULL) {
				err(argv[0], "Out of memory");
			}

			sockets[num_sockets - 1] = optarg;
		}
			break;

		case 'P':
			sock_path = optarg;
			break;
		}
	}

	// try to use extra arguments as pids
	for (int i = optind; i < argc; i++) {
		int new_pid = atoi(argv[i]);
		if (new_pid <= 0) {
			err(argv[0], "Invalid pid");
			return 1;
		}

		pids = realloc(pids, ++num_pids * sizeof(*pids));
		if (pids == NULL) {
			err(argv[0], "Out of memory");
		}

		pids[num_pids - 1] = new_pid;
	}

	if (action < 0) {
		err(argv[0], "--action is required");
	}

	if (num_pids == 0 && num_sockets == 0) {
		err(argv[0], "at least one pid or socket is required");
	}

	printf("%d, %d, %s\n", action, verbose, sock_path);

	printf("pids:\n");
	for (int i = 0; i < num_pids; i++) {
		printf("  %d\n", pids[i]);
	}
	printf("\n");

	printf("sockets:\n");
	for (int i = 0; i < num_sockets; i++) {
		printf("  %s\n", sockets[i]);
	}

	return 0;
}
