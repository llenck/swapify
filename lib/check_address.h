#ifndef _CHECK_ADDRESS_H_INCLUDED
#define _CHECK_ADDRESS_H_INCLUDED

#include "libparsemaps.h"

// returns 0 if the memory is expected to be OK to swap, and 1 if it should stay
// loaded at all times
int swapify_is_indispensible(struct mapping_info* info);

#endif
