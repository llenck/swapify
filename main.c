#include "libparsemaps.h"

#include <unistd.h>

int main() {
	parse_maps(getpid());

	return 0;
}
