#include "swap.h"

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "libparsemaps.h"
#include "procstate.h"
#include "fileio.h"
#include "utils.h"

// format of file: struct mapping_data, the actual data, repeat
struct mapping_data {
	uint64_t start, end;
};

static int swap_fd = -1;

static int do_io_op(char* buf, size_t n, ssize_t (*op)(int, void*, size_t)) {
	size_t done = 0;

	do {
		ssize_t x = op(swap_fd, buf + done, n - done);

		if (x <= 0) {
			return -1;
		}

		done += x;
	} while (done < n);

	return 0;
}

static int write_all_to_swap(void* buf, size_t n) {
	return do_io_op(buf, n, (ssize_t (*)(int, void*, size_t))write);
}

static int read_all_from_swap(void* to, size_t n) {
	return do_io_op(to, n, read);
}

static void __attribute__((noreturn)) process_is_fucked() {
	swapify_log("Process seems to be fucked, killing it and ourselves\n");
	kill(swapify_parent_pid, SIGKILL);
	close(swap_fd);
	swapify_exit(1);
}

static int _swapify_do_unswap(int nonfatal_fail) {
	lseek(swap_fd, 0, SEEK_SET);

	struct mapping_data next;

	while (1) {
		if (read_all_from_swap(&next, sizeof(next)) < 0) {
			// don't fail if we're just recovering from a failed swap, where there
			// wasn't enough disk space, and the file couldn't be conformant anyway
			if (nonfatal_fail) {
				break;
			}

			// otherwise, fail catastrophically, as the process is 99% fucked
			process_is_fucked();
		}

		if (next.start == 0 && next.end == 0) {
			// eof
			break;
		}

		// TODO re-mmap region

		if (read_all_from_swap((void*)next.start, next.end - next.start) < 0) {
			// don't fail if we're just recovering from a failed swap, where there
			// wasn't enough disk space, and the file couldn't be conformant anyway
			if (nonfatal_fail) {
				break;
			}

			// this time, the process is (100 - epsilon)% fucked
			process_is_fucked();
		}
	}

	swapify_log("Finished unswapping\n");

	kill(swapify_parent_pid, SIGCONT);
	close(swap_fd);
	swapify_unlink_swap();

	return 0;
}

int swapify_do_unswap() {
	return _swapify_do_unswap(0);
}

static int swap_cb(struct mapping_info* info) {
	if (
			info->perms != (PROT_READ | PROT_WRITE) ||
			!info->private ||
			info->offs != 0 ||
			info->major != 0 || info->minor != 0 ||
			info->ino != 0 ||
			info->path_len > 0)
	{
		swapify_log("Skipping mapping...\n");
		return 0;
	}


	swapify_log_fmt(256, "Would swap:\nFrom %lx to %lx with perms %d priv %d;"
			" offs major:minor = %x %x:%x with inode %ld and path %.*s\n",
			info->start, info->end, info->perms, info->private, info->offs, info->major,
			info->minor, info->ino, info->path_len, info->path);

	return 0;
}

int swapify_do_swap() {
	swap_fd = swapify_open_swap();
	if (swap_fd < 0) {
		swapify_log_fmt(128, "Couldn't open swap file: %s\n", strerror(errno));
		return -1;
	}

	if (kill(swapify_parent_pid, SIGSTOP) < 0) {
		swapify_log("Couldn't stop parent\n");
		close(swap_fd);
		swapify_unlink_swap();
		return -1;
	}

	int i = 0;
	for (char c; (c = swapify_process_state(swapify_parent_pid)) != 'T' && i < 100; i++) 
	{
		struct timespec slep = { 0, 100 * 1000 };
		nanosleep(&slep, NULL);
	}

	if (i >= 100) {
		// parent didn't stop after 10ms
		swapify_log("Parent didn't start sleeping within >10ms\n");
		close(swap_fd);
		swapify_unlink_swap();
		kill(swapify_parent_pid, SIGCONT);
		return -1;
	}

	swapify_log("Doing swap\n");
	if (parse_maps(swapify_parent_pid, swap_cb) < 0) {
		swapify_log("Error from parse_maps, doing emergency unswap\n");
		goto err_unswap;
	}

	struct mapping_data end = { 0, 0 };

	if (write_all_to_swap(&end, sizeof(end)) < 0) {
		swapify_log("Error writing last chunk, doing emergency unswap\n");
		goto err_unswap;
	}

	return 0;

err_unswap:
	_swapify_do_unswap(1);
	return -1;
}
