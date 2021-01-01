#define _GNU_SOURCE // for memmem

#include "check_address.h"

#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

#include "fileio.h"

static int chk_lib(struct mapping_info* info) {
	static int last_was_lib = 0;
	last_was_lib = memmem(info->path, info->path_len, ".so", 3) != NULL;

	// we also need a heuristic to not swap out stuff that will segfault us. I don't
	// know exactly what that memory is used for, but ld.so creates a mapping directly
	// after shared libraries that doesn't play nice with swapping
	if (last_was_lib) {
		last_was_lib = 0;
		swapify_log("Skipping mapping directly after dynamic object...\n");
		return 1;
	}

	return 0;
}


static int chk_perms(struct mapping_info* info) {
	if (
			info->perms != (PROT_READ | PROT_WRITE) ||
			!info->private ||
			info->offs != 0 ||
			info->major != 0 || info->minor != 0 ||
			info->ino != 0 ||
			info->path_len > 0)
	{
		swapify_log("Skipping mapping of file or with wrong permissions...\n");
		return 1;
	}

	return 0;
}

static int chk_stack(struct mapping_info* info) {
	// also don't swap our own stack
	char x;
	if (info->start < (uint64_t)&x && info->end > (uint64_t)&x) {
		swapify_log("Skipping our own stack...\n");
		return 1;
	}

	return 0;
}

static int chk_tls(struct mapping_info* info) {
	// also don't swap regions that are pointed to by segment registers, which are often
	// used for tls (e.g. on my machine I segfaulted for unmapping *fs, which is used for
	// stack protection functions, which can be turned off for our code but are enabled
	// in glibc)
#if defined(__i386__)
	uint32_t fs, gs;
	asm (
		"movl %%fs:0x0, %0\n"
		"movl %%gs:0x0, %1\n"
		: "=r"(fs), "=r"(gs)
		:
		:);
#elif defined(__x86_64__)
	uint64_t fs, gs;

	asm (
		"movq %%fs:0x0, %0\n"
		"movq %%gs:0x0, %1\n"
		: "=r"(fs), "=r"(gs)
		:
		:);
#endif

#if defined(__i386__) || defined(__x86_64__)
	if (info->start < fs && info->end > fs) {
		swapify_log("Skipping *$fs...\n");
		return 1;
	}
	if (info->start < gs && info->end > gs) {
		swapify_log("Skipping *$gs...\n");
		return 1;
	}
#endif

	// TODO does tls use similar mechanism on other architectures?

	return 0;
}

int swapify_is_indispensible(struct mapping_info* info) {
	// chk_lib needs to be called everytime, so call it first. After that, call the
	// checks that are most likely to return true earlier for performance

	int (*checks[])(struct mapping_info*) = { chk_lib, chk_perms, chk_tls, chk_stack };

	for (unsigned long long i = 0; i < sizeof(checks) / sizeof(*checks); i++) {
		if (checks[i](info)) {
			return 1;
		}
	}

	return 0;
}
