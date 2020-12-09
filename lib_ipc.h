#ifndef _LIB_IPC_H_INCLUDED
#define _LIB_IPC_H_INCLUDED

// uses pid of calling process for naming purposes
void swapify_init_ipc(int parent_pid);

// depends on state from swapify_init_ipc, and uses pid of calling process
void swapify_close_ipc();

#endif
