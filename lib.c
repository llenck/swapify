#define _GNU_SOURCE // for clone

#include <stdio.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#define STACK_SZ (64 * 1024)

// doing anything in this function isn't safe; we rely on memory allocated by the other
// process, which may exit at any time. however, this program isn't meant to be used for
// processes that are short-lived anyway, so idc, and if i ever do, the best thing is
// probably to pass the child pid to the main process where the destructor of the
// library SIGKILLS the child.
int lib_main(void* arg) {
	pid_t pid = *(pid_t*)arg;

	printf("PID of process to fuck up: %ld\nown PID: %ld\n",
			(long int)pid, (long int)getpid());

	sleep(99999999);

	return 0;
}

void __attribute__((constructor)) setup() {
	// could use MAP_GROWSDOWN to be more efficient; STACK_SZ wouldn't be needed then
	unsigned char* child_stack = mmap(NULL, STACK_SZ, PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE | MAP_STACK, -1, 0);
	unsigned char* child_stack_last_qword = child_stack + STACK_SZ - 8;

	// send our pid to our child by storing it at *child_stack
	pid_t main_pid = getpid();
	*(pid_t*)child_stack = main_pid;

	clone(lib_main, child_stack_last_qword, CLONE_VM, child_stack);
}
