#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "parse_opts.h"
#include "open_sockets.h"
#include "ipc_defs.h"

int main(int argc, char** argv) {
	struct opts opt;
	parse_opts(argc, argv, &opt);

	int num_fds = opt.num_pids + opt.num_sockets;
	int sfds[num_fds];
	if (open_sockets(&opt, sfds)) {
		free_opts(&opt);
		return 1;
	}

	char c = opt.action;
	for (int i = 0; i < num_fds; i++) {
		ssize_t r;
		do {
			r = send(sfds[i], &c, 1, 0);
		} while (r < 0 && errno == EINTR);

		if (r < 0) {
			close(sfds[i]);
			sfds[i] = -1;
		}
	}

	int s_cnt = 0, e_cnt = 0, n_cnt = 0;
	for (int i = 0; i < num_fds; i++) {
		if (sfds[i] < 0) {
			e_cnt++;
			continue;
		}

		ssize_t r;
		char ans;
		do {
			r = recv(sfds[i], &ans, 1, 0);
		} while (r < 0 && errno == EINTR);

		close(sfds[i]);

		if (r <= 0) {
			e_cnt++;
		}
		else if (ans == SWAPIFY_REPLY_NOOP) {
			n_cnt++;
		}
		else if (ans == SWAPIFY_REPLY_SUCCESS) {
			s_cnt++;
		}
		else {
			// don't check for error, just interpret anything else as error
			e_cnt++;
		}
	}

	if (num_fds == 1) {
		if (e_cnt == 1) {
			// exit always succeeds, so don't check for that
			fprintf(stderr, "Couldn't %s process.\n",
					opt.action == SWAPIFY_MSG_SWAP? "swap" : "unswap");
		}
		else if (n_cnt == 1) {
			// exit can't no-op, so don't check for that
			printf("Already %s.\n",
					opt.action == SWAPIFY_MSG_SWAP? "swapped" : "unswapped");
		}
		else {
			printf("Successfully %s.\n",
					opt.action == SWAPIFY_MSG_SWAP? "swapped" :
					opt.action == SWAPIFY_MSG_UNSWAP? "unswapped" : "exited");
		}
	}
	else {
		fprintf(e_cnt? stderr : stdout, "Got %d successes, %d no-ops and %d errors.\n",
				s_cnt, n_cnt, e_cnt);
	}

	free_opts(&opt);

	return e_cnt != 0;
}
