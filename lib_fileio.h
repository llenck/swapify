#ifndef _LIB_FILEIO_INCLUDED
#define _LIB_FILEIO_INCLUDED

// most of these use swapify_parent_pid
void swapify_init_fileio();
void swapify_close_fileio();
int swapify_open_swap();
void swapify_unlink_swap();

#endif
