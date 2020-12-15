#ifndef _LIB_IPC_H_INCLUDED
#define _LIB_IPC_H_INCLUDED

#include "ipc_defs.h"

// most of these use swapify_parent_pid

void swapify_init_ipc();

// depends on state from swapify_init_ipc
void swapify_close_ipc();

// both of these block, and swapify_get_message calls _exit(0) if
// swapify_parent_pid stops existing
enum swapify_msg swapify_get_message();
void swapify_reply_message(enum swapify_reply reply);

#endif
