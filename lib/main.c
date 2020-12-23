#define _GNU_SOURCE // for clone

#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <unistd.h>

#include "ipc.h"
#include "fileio.h"
#include "utils.h"
#include "swap.h"

#define STACK_SZ (64 * 1024)

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

	prctl(PR_SET_PDEATHSIG, SIGINT);
	signal(SIGINT, exit_handler);
	signal(SIGTERM, exit_handler);

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

	swapify_log("Exiting due to user request\n");

	if (proc_swapped) {
		// don't leave parent process swapped if get told to kys
		swapify_do_unswap();
	}

	swapify_exit(0);
}

static void __attribute__((constructor)) setup() {
	// could use MAP_GROWSDOWN to be more efficient; STACK_SZ wouldn't be needed then
	unsigned char* child_stack = mmap(NULL, STACK_SZ, PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE | MAP_STACK, -1, 0);
	unsigned char* child_stack_last_qword = child_stack + STACK_SZ - 8;

	// send our pid to our child
	swapify_parent_pid = getpid();

	// open the log from the parent, so that we log something even if the parent process
	// immediately does something that segfaults the child process
	swapify_init_fileio();
	swapify_log("Set up file io...\n");

	// CLONE_PARENT prevents programs like strace from waiting for the child to exit,
	// which it never would without the parent exiting
	child_pid = clone(lib_main, child_stack_last_qword,
			CLONE_VM | CLONE_FILES | CLONE_PARENT, child_stack);
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
