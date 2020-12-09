#define _GNU_SOURCE // for clone

#include <sched.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <unistd.h>

#include "libparsemaps.h"
#include "lib_ipc.h"
#include "lib_fileio.h"

#define STACK_SZ (64 * 1024)

static int child_pid = 0;
static int parent_pid = 0;

static void sigint_handler(int sig) {
	(void)sig;
	// exit directly; cleanup is done by the main process
	_exit(0);
}

// doing anything at all in the child isn't safe; we rely on memory allocated by the other
// process, which may exit at any time. but unless it exits abnormally, we will get
// SIGKILLed, and otherwise we should get a segfault and crash, which might leave an
// unused socket in /run/user/[uid]/swapify, which isn't too big of an issue
static int lib_main(void* arg) {
	(void)arg;

	prctl(PR_SET_PDEATHSIG, SIGINT);
	signal(SIGINT, sigint_handler);

	swapify_init_ipc(parent_pid);
	swapify_init_fileio();

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

	// CLONE_PARENT prevents programs like strace from waiting for the child to exit,
	// which it never would without the parent exiting
	child_pid = clone(lib_main, child_stack_last_qword,
			CLONE_VM | CLONE_FILES | CLONE_PARENT, child_stack);
}

static void __attribute__((destructor)) destroy() {
	if (child_pid) {
		kill(child_pid, SIGKILL);
	}

	swapify_close_ipc();
	swapify_close_fileio();
}
