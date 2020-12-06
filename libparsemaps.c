#include "libparsemaps.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static const char h2i[] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

static void parse_hex_u64(const char** str, off_t len, uint64_t* val) {
	const char* og_str = *str;
	*val = 0;
	while (*str - og_str < len) {
		char n = h2i[(unsigned char)*(*str)++];
		if (n == -1) {
			return;
		}

		*val <<= 4;
		*val |= n;
	}
}

static void parse_hex_i(const char** str, off_t len, int* val) {
	const char* og_str = *str;
	*val = 0;
	while (*str - og_str < len) {
		char n = h2i[(unsigned char)*(*str)++];
		if (n == -1) {
			return;
		}

		*val <<= 4;
		*val |= n;
	}
}

static int line_kek(const char* str, size_t len) {
	const char* const og_str = str;

	struct mapping_info inf = { 0 };

	parse_hex_u64(&str, len, &inf.start);
	if ((ssize_t)len <= str - og_str) {
		return -1;
	}

	parse_hex_u64(&str, len - (str - og_str), &inf.end);
	// also check length for the inf.permsissions, since those are constant-length
	if ((ssize_t)len <= str - og_str + 4) {
		return -1;
	}

	if (*str++ == 'r') {
		inf.perms |= PROT_READ;
	}
	if (*str++ == 'w') {
		inf.perms |= PROT_WRITE;
	}
	if (*str++ == 'x') {
		inf.perms |= PROT_EXEC;
	}
	inf.private = *str++ == 'p';

	str++;

	parse_hex_i(&str, len - (str - og_str), &inf.offs);
	if ((ssize_t)len <= str - og_str) {
		return -1;
	}
	parse_hex_i(&str, len - (str - og_str), &inf.major);
	if ((ssize_t)len <= str - og_str) {
		return -1;
	}
	parse_hex_i(&str, len - (str - og_str), &inf.minor);
	if ((ssize_t)len <= str - og_str) {
		return -1;
	}

	printf("from %x to %x with perms %d priv %d; offs major:minor = %x %x:%x\n",
			inf.start, inf.end, inf.perms, inf.private, inf.offs, inf.major, inf.minor);

	return 0;
}

static int open_smaps(long int pid) {
	char path[48]; // longer than the biggest path for the biggest possible pid
	sprintf(path, "/proc/%ld/maps", pid);
	int fd = open(path, O_RDONLY);
	return fd;
}

const int buf_len = 4096;

int parse_maps(long int pid) {
	int fd = open_smaps(pid);
	if (fd < 0) {
		perror("Failed to open smaps");
		return -1;
	}

	char buf[buf_len];
	int offs = 0;
	int valid_chars = 0;
	int first_of_line = 0;
	while (1) {
		if (offs == valid_chars) {
			// refill the buffer since we're at the end
			if (valid_chars > buf_len * 3 / 4) {
				// move the buffer back
				if (first_of_line == 0) {
					// this line is becoming kinda long, error out
					// (this shouldn't happen anyway because of max path len)
					return -1;
				}

				memmove(buf, buf + first_of_line, valid_chars - first_of_line);
				offs -= first_of_line;
				first_of_line = 0;
				valid_chars -= first_of_line;
			}

			int ret = read(fd, buf + offs, buf_len - offs);
			if (ret < 0) {
				// :/
				return -1;
			}

			if (ret == 0) {
				// :)
				break;
			}

			valid_chars += ret;
		}

		if (buf[offs] == '\n') {
			int ret = line_kek(buf + first_of_line, offs - first_of_line);
			if (ret < 0) {
				// exit and pass error
				return ret;
			}

			first_of_line = offs + 1;
		}
		offs++;
	}

	close(fd);

	return 0;
}
