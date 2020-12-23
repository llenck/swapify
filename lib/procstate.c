#include "procstate.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

int swapify_process_state(int pid) {
	char path[128];
	sprintf(path, "/proc/%d/status", pid);
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		return -1;
	}

	char buf[128];
	int offs = 0, valid_bytes = 0;
	int line_index = 0, after_colon = 0;
	while (1) {
		if (offs <= valid_bytes) {
			int new_bytes = read(fd, buf + offs, 128 - offs);
			if (new_bytes < 0) {
				if (errno == EINTR) {
					continue;
				}
				else {
					return -1;
				}
			}
			if (new_bytes == 0) {
				return -1;
			}
		}

		if (after_colon && buf[offs] != '\t') {
			return buf[offs];
		}
		else if (line_index == 2 && buf[offs] == ':') {
			after_colon = 1;
		}
		else if (line_index < 2 && buf[offs] == '\n') {
			line_index++;
		}

		offs++;
	}
}
