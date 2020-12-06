#include "libparsemaps.h"

#include <stdio.h>

#include <unistd.h>

int map_cb(struct mapping_info* info) {
	printf("from %lx to %lx with perms %d priv %d; offs major:minor = %x %x:%x with"
			" inode %ld and path %.*s\n",
			info->start, info->end, info->perms, info->private, info->offs, info->major,
			info->minor, info->ino, info->path_len, info->path);

	return 0;
}

int main() {
	parse_maps(getpid(), map_cb);

	return 0;
}
