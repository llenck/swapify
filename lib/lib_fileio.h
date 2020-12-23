#ifndef _LIB_FILEIO_INCLUDED
#define _LIB_FILEIO_INCLUDED

// most of these use swapify_parent_pid
void swapify_init_fileio();
void swapify_close_fileio();
int swapify_open_swap();
void swapify_unlink_swap();

#ifdef NDEBUG
#define swapify_log(x) ((void)0)
#define swapify_log_fmt(n, fmt, ...) ((void)0)
#else
void swapify_log(const char* s);
#define swapify_log_fmt(n, fmt, ...) { \
	__auto_type _n = n; \
	char _buf[_n]; \
	int _r = snprintf(_buf, _n, fmt, __VA_ARGS__); \
	if (_r <= _n) swapify_log(_buf); \
}
#endif

#endif
