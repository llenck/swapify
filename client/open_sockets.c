#include "open_sockets.h"

#include <errno.h>
#include <linux/limits.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "parse_opts.h"

void close_fds(int* fds, int n) {
	for (int i = 0; i < n; i++) {
		close(fds[i]);
	}
}

// returns fd, or -errno
static int open_unix(const char* path) {
	struct sockaddr_un address;
	address.sun_family = AF_UNIX;
	if (strlen(path) + 1 > sizeof(address.sun_path)) {
		return -errno;
	}

	strcpy(address.sun_path, path);

	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		return -errno;
	}

	if (connect(fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		int ret = -errno;
		close(fd);
		return ret;
	}

	return fd;
}

static int open_pid_fds(struct opts* opt, int* fds) {
	for (int i = 0; i < opt->num_pids; i++) {
		char sock_name[PATH_MAX];
		sprintf(sock_name, "%s/%d.sock", opt->sock_path, opt->pids[i]);
		fds[i] = open_unix(sock_name);

		if (fds[i] < 0) {
			close_fds(fds, i);
			fprintf(stderr, "Couldn't open socket for pid %d: %s\n",
					opt->pids[i], strerror(-fds[i]));

			return -1;
		}
	}

	return 0;
}

static int open_sockname_fds(struct opts* opt, int* fds) {
	for (int i = 0; i < opt->num_sockets; i++) {
		char sock_name[PATH_MAX];
		int r = snprintf(sock_name, PATH_MAX, "%s/%s", opt->sock_path, opt->sockets[i]);

		if (r >= PATH_MAX) {
			close_fds(fds, i);
			fprintf(stderr, "Path for socket with name %s exceeds max path length\n",
					opt->sockets[i]);

			return -1;
		}

		fds[i] = open_unix(sock_name);

		if (fds[i] < 0) {
			close_fds(fds, i);
			fprintf(stderr, "Couldn't open socket for name %s: %s\n",
					opt->sockets[i], strerror(-fds[i]));

			return -1;
		}
	}

	return 0;
}

int open_sockets(struct opts* opt, int* fds) {
	if (open_pid_fds(opt, fds) < 0) {
		return -1;
	}

	if (open_sockname_fds(opt, fds + opt->num_pids) < 0) {
		close_fds(fds, opt->num_pids);
		return -1;
	}

	return 0;
}
