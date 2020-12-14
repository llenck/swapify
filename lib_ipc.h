#ifndef _LIB_IPC_H_INCLUDED
#define _LIB_IPC_H_INCLUDED

#include "ipc_defs.h"

// uses pid of calling process for naming purposes
void swapify_init_ipc(int parent_pid);

// depends on state from swapify_init_ipc, and uses pid of calling process
void swapify_close_ipc();

// both of these block, and swapify_get_message calls _exit(0) if watch_pid
// stops existing
enum swapify_msg swapify_get_message(int watch_pid);
void swapify_reply_message(enum swapify_reply reply);

#endif
