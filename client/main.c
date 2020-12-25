#include <stdio.h>

#include "parse_opts.h"
#include "ipc_defs.h"

int main(int argc, char** argv) {
	struct opts opt;
	parse_opts(argc, argv, &opt);

	printf("%d, %d, %s\n", opt.action, opt.verbose, opt.sock_path);

	printf("pids:\n");
	for (int i = 0; i < opt.num_pids; i++) {
		printf("  %d\n", opt.pids[i]);
	}
	printf("\n");

	printf("sockets:\n");
	for (int i = 0; i < opt.num_sockets; i++) {
		printf("  %s\n", opt.sockets[i]);
	}

	return 0;
}
