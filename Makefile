debug : BUILD_TYPE := debug
release : BUILD_TYPE := release
debug: libswapify.so swap
release: libswapify.so swap

.PHONY: clean libswapify.so swap

libswapify.so:
	$(MAKE) $(BUILD_TYPE) -C lib
	cp --reflink=auto lib/libswapify.so $@

swap:
	$(MAKE) $(BUILD_TYPE) -C client
	cp --reflink=auto client/swap $@

clean:
	$(MAKE) clean -C lib
	$(MAKE) clean -C client
	$(MAKE) clean -C deps/libparsemaps
	$(RM) libswapify.so swap

INSTALL_PREFIX ?= # empty -> root

install: libswapify.so swap
	mkdir -p $(INSTALL_PREFIX)/lib
	mkdir -p $(INSTALL_PREFIX)/bin
	install -m755 libswapify.so $(INSTALL_PREFIX)/lib/libswapify.so
	install -m755 swap $(INSTALL_PREFIX)/bin/swap
	install -m755 swapify $(INSTALL_PREFIX)/bin/swapify
	mkdir -p $(INSTALL_PREFIX)/etc/bash_completion.d
	install -m755 swap_completion.bash $(INSTALL_PREFIX)/etc/bash_completion.d/

uninstall:
	$(RM) $(INSTALL_PREFIX)/lib/libswapify.so
	$(RM) $(INSTALL_PREFIX)/bin/swap
	$(RM) $(INSTALL_PREFIX)/bin/swapify
	$(RM) $(INSTALL_PREFIX)/etc/bash_completion.d/swap_completion.bash
