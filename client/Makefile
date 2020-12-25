CFLAGS ?= -Wall -Wextra
LDFLAGS ?=
CC ?= gcc

CLIENT_OBJECTS := main.o
CLIENT_INCLUDEDIRS := -I ../include
HEADERS := ../include/ipc_defs.h

debug : CFLAGS += -Og -g -pg
release : CFLAGS += -O3
debug : LDFLAGS +=
release : LDFLAGS += -O

debug: swap
release: swap

swap: $(CLIENT_OBJECTS) $(HEADERS)
	$(CC) $(LDFLAGS) $(CLIENT_OBJECTS) -o $@

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(CLIENT_INCLUDEDIRS) $< -c -o $@

.PHONY: clean
clean:
	$(RM) swap $(CLIENT_OBJECTS)
