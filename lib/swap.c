#include "swap.h"

#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "libparsemaps.h"
#include "procstate.h"
#include "fileio.h"
#include "utils.h"
#include "check_address.h"

// format of file: struct mapping_data, the actual data, repeat
struct mapping_data {
	uintptr_t start, end;
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
			swapify_log("Couldn't read mapping info from swap\n");

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

		void* r = mmap((void*)next.start, next.end - next.start,
				PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS,
				-1, 0);

		if (r == NULL) {
			swapify_log_fmt(64, "Couldn't unswap: %s\n", strerror(errno));

			// in theory we could have recovering mechanisms here, but in practice I've
			// found that mmap only refuses mappings that are "obvious overcommits",
			// which doesn't happen if we didn't swap something like memtester.
			// Instead of mmap returning NULL, my systems just seems to freeze (and even
			// sysrq+f doesn't fix it). So, instead of having recovering mechanisms here,
			// we should have the swap utility check whether we have enough available
			// memory, and will have it exit if there's not enough memory and the user
			// doesn't tell it to force unswap.
			// the only thing that might change this is the user setting
			// vm.overcommit_memory = 2 ("never overcommit").

			process_is_fucked();
		}

		if (read_all_from_swap((void*)next.start, next.end - next.start) < 0) {
			swapify_log("Couldn't read mapping contents from swap\n");

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
	if (swapify_is_indispensible(info)) {
		return 0;
	}

	swapify_log_fmt(256,
			"Swapping:\nFrom %" PRIuPTR " to %" PRIuPTR " with perms %d priv %d;"
			" offs major:minor = %x %x:%x with inode %" PRIu64 " and path %.*s\n",
			info->start, info->end, info->perms, info->private, info->offs, info->major,
			info->minor, info->ino, info->path_len, info->path);

	struct mapping_data d = { info->start, info->end };

	if (write_all_to_swap(&d, sizeof(d)) < 0) {
		swapify_log("Error writing to swap file\n");
		return -1;
	}

	if (write_all_to_swap((void*)info->start, info->end - info->start) < 0) {
		swapify_log("Error writing to swap file\n");
		return -1;
	}

	if (munmap((void*)info->start, info->end - info->start) < 0) {
		swapify_log("Failed to unmap region\n");
		return -1;
	}

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

	swapify_log("Checking whether parent is sleeping...\n");
	char c;
	for (int i = 0; i < 100; i++) {
		c = swapify_process_state(swapify_parent_pid);
		if (c == 'T' || c == 't') {
			break;
		}
		swapify_log_fmt(64, "Waiting for parent to sleeping (state: %c)\n", c);
		struct timespec slep = { 0, 100 * 1000 };
		struct timespec rem;
		nanosleep(&slep, &rem);
	}

	swapify_log_fmt(32, "Final parent state: %c\n", c);

	if (c != 'T' && c != 't') {
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

	if (lseek(swap_fd, 0, SEEK_SET) < 0) {
		swapify_log("Couldn't seek on swap_fd after swapping\n");
		process_is_fucked();
	}
	return 0;

err_unswap:
	if (lseek(swap_fd, 0, SEEK_SET) < 0) {
		swapify_log("Couldn't seek on swap_fd during emergency unswap\n");
		process_is_fucked();
	}

	int r = _swapify_do_unswap(1);
	if (r < 0) {
		// this is pretty much the worst case; we're stuck halfway between being swapped
		// and being unswapped.
		process_is_fucked();
	}

	return -1;
}
