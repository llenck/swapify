CFLAGS ?= -Wall -Wextra
LDFLAGS ?=
CC ?= gcc

TEST_OBJECTS := main-exe.o libparsemaps-exe.o
LIB_OBJECTS := lib_main-lib.o lib_ipc-lib.o lib_fileio-lib.o lib_procstate-lib.o libparsemaps-lib.o
HEADERS := lib_ipc.h ipc_defs.h lib_fileio.h lib_utils.h lib_procstate.h libparsemaps.h

debug : CFLAGS += -Og -g -pg
release : CFLAGS += -O3
debug : LDFLAGS +=
release : LDFLAGS += -O

debug: test lib.so
release: test lib.so

test: $(TEST_OBJECTS) $(HEADERS)
	$(CC) $(TEST_OBJECTS) $(LDFLAGS) -o $@

lib.so: $(LIB_OBJECTS) $(HEADERS)
	$(CC) $(LIB_OBJECTS) $(LDFLAGS) -shared -o $@

%-exe.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $< -c -o $@

%-lib.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -fpic $< -c -o $@

.PHONY: clean
clean:
	$(RM) test $(TEST_OBJECTS) lib.so $(LIB_OBJECTS)
