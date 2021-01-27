#include "check_avmem.h"

#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "parse_opts.h"
#include "ipc_defs.h"

struct mem_stats {
	intptr_t available_mem;
	intptr_t total_mem;
};

const char metric_pref_pot[] = { 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 60, -1, 30, -1, -1, -1, 10, -1, 20, -1, -1, 50, -1, -1, -1, 40, -1, -1, -1, -1, 80, 70, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 60, -1, 30, -1, -1, -1, 10, -1, 20, -1, -1, 50, -1, -1, -1, 40, -1, -1, -1, -1, 80, 70, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

static int strstartsw(const char* s1, const char* s2) {
	return !strncmp(s1, s2, strlen(s2));
}

static int try_parse_field_into(const char* l, const char* fmt, intptr_t* out) {
	unsigned char c = 0;
	sscanf(l, fmt, out, &c);

	char pot = metric_pref_pot[c];

	if (pot < 0 || *out == 0) {
		return -1;
	}

	*out <<= pot;

	return 0;
}

static int get_avmem(struct mem_stats* s) {
	FILE* f = fopen("/proc/meminfo", "r");
	if (f == NULL) {
		return 0;
	}

	s->available_mem = 0;
	s->total_mem = 0;

	char* l = NULL;
	size_t n = 0;

	while (getline(&l, &n, f) > 0 && (!s->available_mem || !s->total_mem)) {
		// try parsing any of these fields, and break if parsing fails on matching line
		if (strstartsw(l, "MemAvailable:")) {
			if (try_parse_field_into(l, "MemAvailable: %" SCNiPTR " %cB",
						&s->available_mem) < 0)
			{
				break;
			}
		}
		else if (strstartsw(l, "MemTotal:")) {
			if (try_parse_field_into(l, "MemTotal: %" SCNiPTR " %cB",
						&s->total_mem) < 0)
			{
				break;
			}
		}
	}

	fclose(f);
	free(l);
	return -(!s->total_mem || !s->available_mem);
}

static uintptr_t pid_mem(int dfd, int pid) {
	char path[32];
	sprintf(path, "%d.swap", pid);

	struct stat stats;
	if (fstatat(dfd, path, &stats, 0) < 0) {
		return 0;
	}

	return stats.st_size;
}

// this function mostly fails softly, so caution is still advised
void check_avmem(struct opts* opt) {
	if (opt->force_overcommit
	 || opt->swap_path == NULL
	 || opt->action != SWAPIFY_MSG_UNSWAP)
	{
		return;
	}

	struct mem_stats st;
	if (get_avmem(&st) < 0) {
		puts("b");
		return;
	}

	intptr_t mem_limit = st.total_mem * 0.05;

	int dfd = open(opt->swap_path, O_DIRECTORY | O_RDONLY);
	if (dfd < 0) {
		puts("c");
		return;
	}

	intptr_t swapped_memory = 0;
	for (int i = 0; i < opt->num_pids; i++) {
		swapped_memory += pid_mem(dfd, opt->pids[i]);
	}

	close(dfd);

	if (st.available_mem - swapped_memory < mem_limit) {
		fprintf(stderr, "Error: memory that would be swapped close to or larger than"
				" available memory. Use -f to ignore.\n");
		exit(1);
	}
}
