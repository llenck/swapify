#include "lib_ipc.h"
#include "lib_fileio.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

static int dirfd = -1;
static int sockfd = -1;

static void open_run_dir() {
	char path[128];
	sprintf(path, "/run/user/%d/swapify", geteuid());

	mkdir(path, 0700);
	dirfd = open(path, O_DIRECTORY | O_RDONLY);
	if (dirfd < 0) {
		// abort without doing any cleanup
		_exit(1);
	}
}

static void open_pid_socket(int parent_pid) {
	struct sockaddr_un address;
	address.sun_family = AF_UNIX;
	int sock_name_offs;
	int ret = snprintf(address.sun_path, sizeof(address.sun_path),
			"/run/user/%d/swapify/%n%d.sock", geteuid(), &sock_name_offs, parent_pid);

	if ((unsigned)ret >= sizeof(address.sun_path)) {
		_exit(1);
	}

	unlinkat(dirfd, address.sun_path + sock_name_offs, 0);

	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd < 0) {
		_exit(1);
	}

	if (bind(sockfd, (const struct sockaddr*)&address, sizeof(address)) < 0) {
		_exit(1);
	}
}

void swapify_init_ipc(int parent_pid) {
	open_run_dir();
	open_pid_socket(parent_pid);
}

void swapify_close_ipc() {
	if (dirfd >= 0 && sockfd >= 0) {
		char socket_name[64];
		sprintf(socket_name, "%d.sock", getpid());
		unlinkat(dirfd, socket_name, 0);

		char dir_path[128];
		sprintf(dir_path, "/run/user/%d/swapify", geteuid());
		rmdir(dir_path);
	}

	if (dirfd >= 0) {
		close(dirfd);
	}
	if (sockfd >= 0) {
		close(sockfd);
	}
}
