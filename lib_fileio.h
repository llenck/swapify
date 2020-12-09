#ifndef _LIB_FILEIO_INCLUDED
#define _LIB_FILEIO_INCLUDED

void swapify_init_fileio();
// like swapify_close_ipc, uses the pid of the calling process
void swapify_close_fileio();
int swapify_open_swap(int parent_pid);
void swapify_unlink_swap(int parent_pid);

#endif
