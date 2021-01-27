#ifndef _PARSE_OPTS_H_INCLUDED
#define _PARSE_OPTS_H_INCLUDED

struct opts {
	int* pids;
	const char** sockets;
	int num_pids;
	int num_sockets;

	char* sock_path;
	int sock_path_allocated;

	char* swap_path;
	int swap_path_allocated;
	int force_overcommit;

	int action;
};

void parse_opts(int argc, char** argv, struct opts* out);
void free_opts(struct opts* opt);

#endif
