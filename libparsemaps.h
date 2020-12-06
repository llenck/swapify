#ifndef _LIBPARSEMAPS_H_INCLUDED
#define _LIBPARSEMAPS_H_INCLUDED

#include <stdint.h>

struct mapping_info {
	uint64_t start, end;
	int perms;
	int private;
	int offs;
	int major, minor;
	const char* path;
	int path_len;
};

int parse_maps(long int pid);

#endif
