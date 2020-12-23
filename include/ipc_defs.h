#ifndef _IPC_DEFS_INCLUDED
#define _IPC_DEFS_INCLUDED

// used in both client and server
enum swapify_msg { SWAPIFY_MSG_EXIT, SWAPIFY_MSG_SWAP, SWAPIFY_MSG_UNSWAP };
enum swapify_reply { SWAPIFY_REPLY_SUCCESS, SWAPIFY_REPLY_ERROR, SWAPIFY_REPLY_NOOP };

#endif
