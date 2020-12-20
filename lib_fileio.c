#include "lib_fileio.h"

#include <fcntl.h>
#include <linux/limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lib_utils.h"

static int dirfd = -1;

#ifndef NDEBUG
static int logfd = -1;
#endif

void swapify_init_fileio() {
	char path[PATH_MAX];
	int ret = snprintf(path, PATH_MAX, "%s/.swapify", getenv("HOME"));
	if (ret >= PATH_MAX) {
		_exit(0);
	}

	mkdir(path, 0700);
	dirfd = open(path, O_DIRECTORY | O_RDONLY | O_CLOEXEC);
	if (dirfd < 0) {
		_exit(0);
	}

#ifndef NDEBUG
	// generate a almost securely unique path name for our log file
	long long unsigned int random = 0;
	getrandom(&random, sizeof(random), 0); // ignore error; probably unique anyway
	char log_name[64];
	sprintf(log_name, "log-%d-%d-%016llx", getpid(), swapify_parent_pid, random);

	// ignore if this fails, swapify_log checks for logfd < 0
	logfd = openat(dirfd, log_name, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0755);
#endif
}

int swapify_open_swap() {
	char name[64];
	sprintf(name, "%d.swap", swapify_parent_pid);
	int fd = openat(dirfd, name, O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC); // O_TMPFILE?

	return fd;
}

void swapify_unlink_swap() {
	char name[64];
	sprintf(name, "%d.swap", swapify_parent_pid);

	unlinkat(dirfd, name, 0);
}

void swapify_close_fileio() {
	if (dirfd >= 0) {
		char path[64];
		sprintf(path, "%d.swap", swapify_parent_pid);
		unlinkat(dirfd, path, 0);
		close(dirfd);
		dirfd = -1;
	}
}

#ifndef NDEBUG
void swapify_log(const char* s) {
	if (logfd < 0) {
		return;
	}

	size_t n = strlen(s);
	size_t done = 0;

	do {
		ssize_t x = write(logfd, s + done, n - done);

		if (x <= 0) {
			return;
		}

		done += x;
	} while (done < n);
}
#endif
