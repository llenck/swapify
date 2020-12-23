CFLAGS ?= -Wall -Wextra
CC ?= gcc

LIB_OBJECTS := libparsemaps-lib.o
TEST_OBJECTS := main-test.o
HEADERS := libparsemaps.h

debug : CFLAGS += -Og -g -pg
release : CFLAGS += -O3

debug: libparsemaps.a
release: libparsemaps.a

libparsemaps.a: $(LIB_OBJECTS) $(HEADERS)
	$(RM) $@
	$(AR) -crs $@ $(LIB_OBJECTS)

test: $(TEST_OBJECTS) libparsemaps.a $(HEADERS)
	$(CC) $(TEST_OBJECTS) libparsemaps.a -o $@

%-lib.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -fpic $< -c -o $@

%-test.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $< -c -o $@

.PHONY: clean
clean:
	$(RM) test $(TEST_OBJECTS) libparsemaps.a $(LIB_OBJECTS)
