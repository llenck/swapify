#include "parse_opts.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

static void __attribute__((noreturn)) usage(int code) {
	fprintf(stderr, "Usage: TODO\n");
	exit(code);
}

static void __attribute__((noreturn)) err(const char* name, const char* msg) {
	fprintf(stderr, "%s: %s\n", name, msg);
	usage(1);
}

void parse_opts(int argc, char** argv, struct opts* out) {
	out->action = -1;
	out->verbose = 0;

	out->pids = NULL;
	out->num_pids = 0;
	out->sockets = NULL;
	out->num_sockets = 0;

	out->sock_path = NULL;

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
			out->verbose = 1;
			break;

		case 'a':
			if (out->action != -1) {
				err(argv[0], "You should specify only one action");
			}

			if (streq(optarg, "swap")) {
				out->action = SWAPIFY_MSG_SWAP;
			}
			else if (streq(optarg, "unswap")) {
				out->action = SWAPIFY_MSG_UNSWAP;
			}
			else if (streq(optarg, "exit")) {
				out->action = SWAPIFY_MSG_EXIT;
			}
			else {
				err(argv[0], "argument to --action must be swap, unswap or exit");
			}
			break;

		case 'p':
		{
			int new_pid = atoi(optarg);
			if (new_pid <= 0) {
				err(argv[0], "Invalid pid");
			}

			out->pids = realloc(out->pids, ++out->num_pids * sizeof(*out->pids));
			if (out->pids == NULL) {
				err(argv[0], "Out of memory");
			}

			out->pids[out->num_pids - 1] = new_pid;
		}
			break;

		case 's':
		{
			out->sockets = realloc(out->sockets,
					++out->num_sockets * sizeof(*out->sockets));
			if (out->sockets == NULL) {
				err(argv[0], "Out of memory");
			}

			out->sockets[out->num_sockets - 1] = optarg;
		}
			break;

		case 'P':
			out->sock_path = optarg;
			break;
		}
	}

	// try to use extra arguments as pids
	for (int i = optind; i < argc; i++) {
		int new_pid = atoi(argv[i]);
		if (new_pid <= 0) {
			err(argv[0], "Invalid pid");
		}

		out->pids = realloc(out->pids, ++out->num_pids * sizeof(*out->pids));
		if (out->pids == NULL) {
			err(argv[0], "Out of memory");
		}

		out->pids[out->num_pids - 1] = new_pid;
	}

	if (out->action < 0) {
		err(argv[0], "--action is required");
	}

	if (out->num_pids == 0 && out->num_sockets == 0) {
		err(argv[0], "at least one pid or socket is required");
	}

	if (out->sock_path == NULL) {
		out->sock_path = malloc(64);
		if (out->sock_path == NULL) {
			err(argv[0], "Out of memory");
		}

		sprintf(out->sock_path, "/run/user/%d/swapify", getuid());
	}
}
