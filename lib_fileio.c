#include "lib_fileio.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/limits.h>

static int dirfd = -1;

void swapify_init_fileio() {
	char path[PATH_MAX];
	int ret = snprintf(path, PATH_MAX, "%s/.cache", getenv("HOME"));
	if (ret >= PATH_MAX) {
		_exit(0);
	}

	int cachefd = open(path, O_DIRECTORY | O_RDONLY);
	if (cachefd < 0) {
		_exit(0);
	}

	mkdirat(cachefd, "swapify", 0700);
	dirfd = openat(cachefd, "swapify", O_DIRECTORY | O_RDONLY);
	if (dirfd < 0) {
		_exit(0);
	}
}

int swapify_open_swap(int parent_pid) {
	char name[64];
	sprintf(name, "%d.swap", parent_pid);
	int fd = openat(dirfd, name, O_RDWR | O_CREAT | O_EXCL); // O_TMPFILE?

	return fd;
}

void swapify_unlink_swap(int parent_pid) {
	char name[64];
	sprintf(name, "%d.swap", parent_pid);

	unlinkat(dirfd, name, 0);
}

void swapify_close_fileio() {
	if (dirfd >= 0) {
		char path[64];
		sprintf(path, "%d.swap", getpid());
		unlinkat(dirfd, path, 0);
		close(dirfd);

		// unlike in the ipc exit function, we don't try to delete ~/.cache/swapify,
		// because there might be swapify instances running that have a file descriptor for
		// the directory but don't have any files open in the directory
	}
}
