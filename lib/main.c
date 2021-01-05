#define _GNU_SOURCE // for clone

#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "ipc.h"
#include "fileio.h"
#include "utils.h"
#include "swap.h"

#define STACK_SZ (128 * 1024)

static int child_pid = 0;
int swapify_parent_pid = 0;

static void exit_handler(int sig) {
	(void)sig;
	swapify_log("Caught signal, exiting\n");
	swapify_exit(0);
}

// doing anything at all in the child isn't safe; we rely on memory allocated by the other
// process, which may exit at any time. but unless it exits abnormally, we will get
// SIGKILLed, and otherwise we should get a segfault and crash, which might leave an
// unused socket in /run/user/[uid]/swapify, which isn't too big of an issue
static int lib_main(void* arg) {
	(void)arg;

	signal(SIGINT, exit_handler);
	signal(SIGTERM, exit_handler);
	signal(SIGPIPE, SIG_IGN);

	swapify_init_fileio();
	swapify_init_ipc();
	swapify_log("Set up ipc socket...\n");

	int proc_swapped = 0;

	enum swapify_msg msg;
	while ((msg = swapify_get_message()) != SWAPIFY_MSG_EXIT) {
		swapify_log_fmt(32, "Got message: %d\n", msg);

		if (msg == SWAPIFY_MSG_SWAP) {
			if (proc_swapped) {
				swapify_log("[ignored]\n");
				swapify_reply_message(SWAPIFY_REPLY_NOOP);
				continue;
			}

			if (swapify_do_swap() == 0) {
				swapify_log("[success]\n");
				swapify_reply_message(SWAPIFY_REPLY_SUCCESS);
				proc_swapped = 1;
			}
			else {
				swapify_log("[error]\n");
				swapify_reply_message(SWAPIFY_REPLY_ERROR);
			}
		}
		else if (msg == SWAPIFY_MSG_UNSWAP) {
			if (!proc_swapped) {
				swapify_log("[ignored]\n");
				swapify_reply_message(SWAPIFY_REPLY_NOOP);
				continue;
			}

			if (swapify_do_unswap() == 0) {
				swapify_log("[success]\n");
				swapify_reply_message(SWAPIFY_REPLY_SUCCESS);
				proc_swapped = 0;
			}
			else {
				swapify_log("[error]\n");
				swapify_reply_message(SWAPIFY_REPLY_ERROR);
			}
		}
		else {
			// ??
			swapify_log("[unknown]\n");
			swapify_reply_message(SWAPIFY_REPLY_ERROR);
		}
	}

	swapify_reply_message(SWAPIFY_REPLY_SUCCESS);
	swapify_log("Exiting due to user request\n");

	if (proc_swapped) {
		// don't leave parent process swapped if get told to kys
		swapify_do_unswap();
	}

	swapify_exit(0);
}

static int double_clone(void* arg) {
	(void)arg;

	// could use MAP_GROWSDOWN to be more efficient; STACK_SZ wouldn't be needed then
	unsigned char* child_stack = mmap(NULL, STACK_SZ, PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE | MAP_STACK, -1, 0);

	if (child_stack == NULL) {
		// soft fail
		_exit(0);
	}

	unsigned char* child_stack_last_aligned_qword = child_stack + STACK_SZ - 16;

	// CLONE_PARENT prevents programs like strace from waiting for the child to exit,
	// which it never would without the parent exiting
	child_pid = clone(lib_main, child_stack_last_aligned_qword,
			CLONE_VM | CLONE_FILES, child_stack);

	_exit(0);
}

static void __attribute__((constructor)) setup() {
	// could use MAP_GROWSDOWN to be more efficient; STACK_SZ wouldn't be needed then
	unsigned char* child_stack = mmap(NULL, STACK_SZ, PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE | MAP_STACK, -1, 0);

	if (child_stack == NULL) {
		return;
	}

	unsigned char* child_stack_last_aligned_qword = child_stack + STACK_SZ - 16;

	// send our pid to our childs child
	swapify_parent_pid = getpid();

	// spawn a child process that exits directly after spawning another child that will
	// be the actualy swapify process
	int cloner_pid = clone(double_clone, child_stack_last_aligned_qword,
			CLONE_VM | CLONE_FILES, child_stack);

	if (cloner_pid < 0) {
		goto end;
	}

	// and wait for that child so that we don't create a zombie process, which might be
	// the cause of some deadlocks. this effectively reparents the swapify process to
	// pid 1 (or a parent that used prctl(PR_SET_CHILD_SUBREAPER, ...))
	int st;
	while (1) {
		int r = waitpid(cloner_pid, &st, __WCLONE);

		if (r == cloner_pid) {
			break;
		}
	}

end:
	munmap(child_stack, STACK_SZ);
}

void swapify_cleanup() {
	swapify_close_ipc();
	swapify_close_fileio();
}

static void __attribute__((destructor)) destroy() {
	if (child_pid) {
		kill(child_pid, SIGKILL);
	}

	swapify_cleanup();
}

void __attribute__((noreturn)) swapify_exit(int code) {
	swapify_cleanup();
	_exit(code);
}
