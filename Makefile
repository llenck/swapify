CFLAGS ?= -Wall -Wextra
LDFLAGS ?=
CC ?= gcc

EXE_OBJECTS := main-exe.o
LIB_OBJECTS := lib-lib.o
HEADERS :=

debug : CFLAGS += -Og -g -pg
release : CFLAGS += -O3
debug : LDFLAGS +=
release : LDFLAGS += -O

debug: main lib.so
release: main lib.so

main: $(EXE_OBJECTS) $(HEADERS)
	$(CC) $(EXE_OBJECTS) $(LDFLAGS) -o $@

lib.so: $(LIB_OBJECTS) $(HEADERS)
	$(CC) $(LIB_OBJECTS) $(LDFLAGS) -shared -o $@

%-exe.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $< -c -o $@

%-lib.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -fpic $< -c -o $@

.PHONY: clean
clean:
	$(RM) main $(EXE_OBJECTS) lib.so $(LIB_OBJECTS)
