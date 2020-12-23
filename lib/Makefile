CFLAGS ?= -Wall -Wextra
LDFLAGS ?=
CC ?= gcc

LIB_OBJECTS := main.o ipc.o fileio.o procstate.o swap.o
LIB_LIBS := ../deps/libparsemaps/libparsemaps.a
LIB_INCLUDEDIRS += -I ../include -I ../deps/libparsemaps
HEADERS := ipc.h ../include/ipc_defs.h fileio.h utils.h procstate.h swap.h ../deps/libparsemaps/libparsemaps.h

debug : CFLAGS += -Og -g -pg
release : CFLAGS += -O3 -DNDEBUG
debug : LDFLAGS +=
release : LDFLAGS += -O
debug : BUILD_TYPE := debug
release : BUILD_TYPE := release

debug: libswapify.so
release: libswapify.so

libswapify.so: $(LIB_OBJECTS) $(HEADERS) $(LIB_LIBS)
	$(CC) $(LIB_OBJECTS) $(LDFLAGS) -shared -o $@

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(LIB_INCLUDEDIRS) -fpic $< -c -o $@

../deps/libparsemaps/libparsemaps.a:
	$(MAKE) $(BUILD_TYPE) -C ../deps/libparsemaps

.PHONY: clean
clean:
	$(RM) libswapify.so $(LIB_OBJECTS)
