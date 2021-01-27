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
	{ "action", required_argument, 0, 'a' },
	{ "pid", required_argument, 0, 'p' },
	{ "socket", required_argument, 0, 's' },
	{ "socket-path", required_argument, 0, 'P' },
	{ "swap-path", required_argument, 0, 'S' },
	{ "force", no_argument, 0, 'f' },
	{ NULL, 0, 0, 0 }
};

static void __attribute__((noreturn)) usage(int code) {
	fprintf(stderr,
"Usage: swap [options] [pids]\n"
"Where options are a combination of:\n"
"    -h, --help                print this help and exit\n"
"    -a, --action <ACTION>     action to send to specified pids and sockets (required)\n"
"    -p, --pid <PID>           act on PID (can be used multiple times)\n"
"    -s, --socket <NAME>       act on swapify instance listening at NAME\n"
"    -P, --socket-path <PATH>  path that sockets are resolved relative to\n"
"    -S, --swap-path <PATH>    directory to search for swap files in\n"
"    -f, --force               force unswapping even if it requires overcommitting\n"
"\n"
"At least one PID or NAME is required. ACTION has to be one of swap, unswap\n"
"or exit. Only one ACTION may be specified. Additional arguments are\n"
"interpreted as additional pids to act on.\n"
"\n"
"The default path that swap looks for sockets in is /run/user/[UID]/swapify,\n"
"which is also the default path used by swapify.\n"
"This program can only act on pids that have a swapify instance attached, and\n"
"it interacts with that instance by looking for sockets at PATH/PID.sock\n"
"\n"
"Available memory is checked before unswapping, and a unswap command won't be sent\n"
"if over 95%% of memory would need to be used after unswapping, unless --force\n"
"was specified.\n"
"\n"
"This software is developed at https://github.com/llenck/swapify\n");

	exit(code);
}

static void __attribute__((noreturn)) err(const char* name, const char* msg) {
	fprintf(stderr, "%s: %s\n", name, msg);
	usage(1);
}

void parse_opts(int argc, char** argv, struct opts* out) {
	out->action = -1;

	out->pids = NULL;
	out->num_pids = 0;
	out->sockets = NULL;
	out->num_sockets = 0;

	out->sock_path = NULL;
	out->sock_path_allocated = 0;
	out->swap_path = NULL;
	out->swap_path_allocated = 0;

	out->force_overcommit = 0;

	while (1) {
		int opt = getopt_long(argc, argv, "hva:p:s:P:S:f", opts, NULL);

		if (opt == -1) {
			break;
		}

		switch (opt) {
		case '?':
			usage(1);
			break;

		case 'h':
			free_opts(out);
			usage(0);
			break;

		case 'a':
			if (out->action != -1) {
				free_opts(out);
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
				free_opts(out);
				err(argv[0], "argument to --action must be swap, unswap or exit");
			}
			break;

		case 'p':
		{
			int new_pid = atoi(optarg);
			if (new_pid <= 0) {
				free_opts(out);
				err(argv[0], "Invalid pid");
			}

			out->pids = realloc(out->pids, ++out->num_pids * sizeof(*out->pids));
			if (out->pids == NULL) {
				free_opts(out);
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
				free_opts(out);
				err(argv[0], "Out of memory");
			}

			out->sockets[out->num_sockets - 1] = optarg;
		}
			break;

		case 'P':
			out->sock_path = optarg;
			break;

		case 'S':
			out->swap_path = optarg;
			break;

		case 'f':
			out->force_overcommit = 1;
		}
	}

	// try to use extra arguments as pids
	for (int i = optind; i < argc; i++) {
		int new_pid = atoi(argv[i]);
		if (new_pid <= 0) {
			free_opts(out);
			err(argv[0], "Invalid pid");
		}

		out->pids = realloc(out->pids, ++out->num_pids * sizeof(*out->pids));
		if (out->pids == NULL) {
			free_opts(out);
			err(argv[0], "Out of memory");
		}

		out->pids[out->num_pids - 1] = new_pid;
	}

	if (out->action < 0) {
		free_opts(out);
		err(argv[0], "--action is required");
	}

	if (out->num_pids == 0 && out->num_sockets == 0) {
		free_opts(out);
		err(argv[0], "at least one pid or socket is required");
	}

	if (out->sock_path == NULL) {
		out->sock_path = malloc(64);
		if (out->sock_path == NULL) {
			free_opts(out);
			err(argv[0], "Out of memory");
		}

		sprintf(out->sock_path, "/run/user/%d/swapify", getuid());
		out->sock_path_allocated = 1;
	}

	if (out->swap_path == NULL) {
		const char* home = getenv("HOME");
		if (home == NULL) {
			return; // soft fail
		}

		out->swap_path = malloc(strlen(home) + 16);
		if (out->swap_path == NULL) {
			if (out->force_overcommit) {
				return; // soft fail if we're forcing overcommitting anyways
			}
			else {
				// otherwise, hard fail
				free_opts(out);
				err(argv[0], "Out of memory");
			}
		}

		sprintf(out->swap_path, "%s/.swapify", home);
		out->swap_path_allocated = 1;
	}
}

void free_opts(struct opts* opt) {
	free(opt->pids);
	free(opt->sockets);
	if (opt->sock_path_allocated) {
		free(opt->sock_path);
	}
	if (opt->swap_path_allocated) {
		free(opt->swap_path);
	}
}
