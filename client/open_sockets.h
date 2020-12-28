#ifndef _OPEN_SOCKETS_H_INCLUDED
#define _OPEN_SOCKETS_H_INCLUDED

#include "parse_opts.h"

void close_fds(int* fds, int n);
int open_sockets(struct opts* opt, int* fds);

#endif
