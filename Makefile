CFLAGS ?= -Wall -Wextra
LDFLAGS ?=
CC ?= gcc

TEST_OBJECTS := main-exe.o libparsemaps-exe.o
LIB_OBJECTS := lib_main-lib.o lib_ipc-lib.o lib_fileio-lib.o lib_procstate-lib.o lib_swap-lib.o libparsemaps-lib.o
HEADERS := lib_ipc.h ipc_defs.h lib_fileio.h lib_utils.h lib_procstate.h lib_swap.h libparsemaps.h

debug : CFLAGS += -Og -g -pg
release : CFLAGS += -O3 -DNDEBUG
debug : LDFLAGS +=
release : LDFLAGS += -O

debug: test libswapify.so
release: test libswapify.so

test: $(TEST_OBJECTS) $(HEADERS)
	$(CC) $(TEST_OBJECTS) $(LDFLAGS) -o $@

libswapify.so: $(LIB_OBJECTS) $(HEADERS)
	$(CC) $(LIB_OBJECTS) $(LDFLAGS) -shared -o $@

%-exe.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $< -c -o $@

%-lib.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -fpic $< -c -o $@

.PHONY: clean
clean:
	$(RM) test $(TEST_OBJECTS) libswapify.so $(LIB_OBJECTS)
