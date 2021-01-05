#include "procstate.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int swapify_process_state(int pid) {
	char path[128];
	sprintf(path, "/proc/%d/stat", pid);
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		return -1;
	}

	const int buf_len = 4096;
	char buf[buf_len]; // should be larger than max size
	ssize_t r;
	size_t off = 0;

	// read the whole file
	do {
		r = read(fd, buf + off, buf_len - off);
		if (r < 0) {
			if (errno == EINTR) {
				continue;
			}

			close(fd);
			return -1;
		}

		off += r;
	} while (r != 0);

	close(fd);

	// to prevent sqli-like problems with the second field, look for the last
	// closing brace (which is how most good libraries handle it)
	char* last_closing_brace = memchr(buf, ')', off);

	// if, for whatever reason, there was none, or it is too close to the end of the
	// file, return an error intead of crashing or returning garbage
	int idx = last_closing_brace - buf;
	if (last_closing_brace == NULL || idx + 2 > (ssize_t)off) {
		return -1;
	}

	// return the character of the next field
	return buf[idx + 2];
}
