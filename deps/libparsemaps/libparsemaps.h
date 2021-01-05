#ifndef _LIBPARSEMAPS_H_INCLUDED
#define _LIBPARSEMAPS_H_INCLUDED

#include <stdint.h>

struct mapping_info {
	uintptr_t start, end;
	int perms;
	int private;
	int offs;
	int major, minor;
	uint64_t ino;
	const char* path;
	int path_len;
};

int parse_maps(long int pid, int (*cb)(struct mapping_info*));

#endif
