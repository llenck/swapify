#ifndef _LIB_UTILS_INCLUDED
#define _LIB_UTILS_INCLUDED

extern int swapify_parent_pid;

void sawpify_init();
void __attribute__((noreturn)) swapify_exit(int code);
void swapify_cleanup(); // like swapify_exit, but doesn't exit

#endif
