#define _GNU_SOURCE // for memmem

#include "check_address.h"

#if defined(__i386__) || defined(__x86_64__)
#include <asm/prctl.h>
#endif

#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "fileio.h"

#if defined(__i386__) || defined(__x86_64__)
int arch_prctl(int code, unsigned long* ad) {
	return syscall(SYS_arch_prctl, code, ad);
}
#endif

static int chk_lib(struct mapping_info* info) {
	static int last_was_lib = 0;
	int is_lib = memmem(info->path, info->path_len, ".so", 3) != NULL;

	// I don't know exactly what that memory is used for, but ld.so creates a
	// mapping directly after shared libraries that doesn't play nice with swapping
	// (TODO is this important for libraries other than libc?)
	if (last_was_lib) {
		last_was_lib = is_lib;
		swapify_log("Skipping mapping directly after dynamic object...\n");
		return 1;
	}

	last_was_lib = is_lib;

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
	// suppress unused parameter warnings on non-x86 systems
	(void)info;

	// also don't swap regions that are pointed to by segment registers, which are often
	// used for tls (e.g. on my machine I segfaulted for unmapping *fs, which is used for
	// stack protection functions, which can be turned off for our code but are enabled
	// in glibc)

#if defined(__i386__) || defined(__x86_64__)
	// if arch_prctl doesn't work, just set fs and gs to 0
	unsigned long fs = 0, gs = 0;
	arch_prctl(ARCH_GET_FS, &fs);
	arch_prctl(ARCH_GET_GS, &gs);

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
