#include "libparsemaps.h"

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

int map_cb(struct mapping_info* info) {
	printf("from %lx to %lx with perms %d priv %d; offs major:minor = %x %x:%x with"
			" inode %ld and path %.*s\n",
			info->start, info->end, info->perms, info->private, info->offs, info->major,
			info->minor, info->ino, info->path_len, info->path);

	return 0;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		parse_maps(getpid(), map_cb);
	}
	else {
		parse_maps(atoll(argv[1]), map_cb);
	}

	return 0;
}
