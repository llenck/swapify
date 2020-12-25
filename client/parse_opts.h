#ifndef _PARSE_OPTS_H_INCLUDED
#define _PARSE_OPTS_H_INCLUDED

struct opts {
	int action, verbose;

	int* pids;
	const char** sockets;
	int num_pids;
	int num_sockets;

	char* sock_path;
	int sock_path_allocated;
};

void parse_opts(int argc, char** argv, struct opts* out);

#endif
