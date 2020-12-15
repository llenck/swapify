#include "lib_ipc.h"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "lib_utils.h"

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

static void open_pid_socket() {
	struct sockaddr_un address;
	address.sun_family = AF_UNIX;
	int sock_name_offs;
	int ret = snprintf(address.sun_path, sizeof(address.sun_path),
			"/run/user/%d/swapify/%n%d.sock",
			geteuid(), &sock_name_offs, swapify_parent_pid);

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

	if (listen(sockfd, 4) < 0) {
		_exit(1);
	}
}

void swapify_init_ipc() {
	open_run_dir();
	open_pid_socket();
}

void swapify_close_ipc() {
	if (dirfd >= 0 && sockfd >= 0) {
		char socket_name[64];
		sprintf(socket_name, "%d.sock", swapify_parent_pid);
		unlinkat(dirfd, socket_name, 0);

		char dir_path[128];
		sprintf(dir_path, "/run/user/%d/swapify", geteuid());
		rmdir(dir_path);
	}

	if (dirfd >= 0) {
		close(dirfd);
		dirfd = -1;
	}
	if (sockfd >= 0) {
		close(sockfd);
		sockfd = -1;
	}
}

static int connfd = -1;

static void loop_for_pollin(int fd) {
	struct pollfd ev = { fd, POLLIN, 0 };

	do {
		poll(&ev, 1, 1000);

		if (kill(swapify_parent_pid, 0) < 0) {
			swapify_exit(0);
		}
	} while (!ev.revents);
}

static void accept_conn() {
	loop_for_pollin(sockfd);

	if ((connfd = accept(sockfd, NULL, NULL)) < 0) {
		swapify_exit(1);
	}
}

static signed char read_cmd() {
	loop_for_pollin(connfd);

	signed char cmd;
	if (recv(connfd, &cmd, 1, 0) < 0) {
		close(connfd);
		connfd = -1;
		return -1;
	}

	return cmd;
}

enum swapify_msg swapify_get_message() {
	while (1) {
		accept_conn();
		signed char cmd = read_cmd();

		if (cmd != -1) {
			return cmd;
		}
	}
}

void swapify_reply_message(enum swapify_reply reply) {
	if (connfd != -1) {
		char msg = reply;
		send(connfd, &msg, 1, 0);
		close(connfd);
		connfd = -1;
	}
}
