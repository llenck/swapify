#define _GNU_SOURCE // for clone

#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "libparsemaps.h"

#define STACK_SZ (64 * 1024)

static pid_t child_pid = 0;
static pid_t parent_pid = 0;
static int dirfd = -1;
static int sockfd = -1;

static void sigint_handler(int sig) {
	(void)sig;
	//cleanup();
	// exit without doing the standard libraries cleanup
	_exit(0);
}

static void open_run_dir() {
	char path[512];
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
			"/run/user/%d/swapify/%n%d.sock", geteuid(), &sock_name_offs, child_pid);

	if ((unsigned)ret >= sizeof(address.sun_path)) {
		_exit(1);
	}

	unlinkat(dirfd, address.sun_path + sock_name_offs, 0);

	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd < 0) {
		_exit(1);
	}

	if (bind(sockfd, &address, sizeof(address)) < 0) {
		_exit(1);
	}
}

// doing anything in this function isn't safe; we rely on memory allocated by the other
// process, which may exit at any time. however, this program isn't meant to be used for
// processes that are short-lived anyway, so idc, and if i ever do, the best thing is
// probably to pass the child pid to the main process where the destructor of the
// library SIGKILLS the child.
static int lib_main(void* arg) {
	(void)arg;

	prctl(PR_SET_PDEATHSIG, SIGINT);
	signal(SIGINT, sigint_handler);

	open_run_dir();
	open_pid_socket();

	// crashes
//	printf("PID of process to fuck up: %ld\nown PID: %ld\n",
//			(long int)pid, (long int)getpid());

	sleep(999999999);

	return 0;
}

static void __attribute__((constructor)) setup() {
	// could use MAP_GROWSDOWN to be more efficient; STACK_SZ wouldn't be needed then
	unsigned char* child_stack = mmap(NULL, STACK_SZ, PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE | MAP_STACK, -1, 0);
	unsigned char* child_stack_last_qword = child_stack + STACK_SZ - 8;

	// send our pid to our child
	parent_pid = getpid();

	child_pid = clone(lib_main, child_stack_last_qword,
			CLONE_VM | CLONE_FILES | CLONE_FS | CLONE_PARENT, child_stack);
}

static void __attribute__((destructor)) destroy() {
	if (child_pid) {
		kill(child_pid, SIGKILL);
	}

	if (dirfd >= 0 && sockfd >= 0) {
		char socket_name[64];
		sprintf(socket_name, "%d.sock", child_pid);
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
