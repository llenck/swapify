#include <stdio.h>
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
		return 1;
	}

	close_fds(sfds, num_fds);

	return 0;
}
