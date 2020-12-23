debug : BUILD_TYPE := debug
release : BUILD_TYPE := release
debug: libswapify.so swap
release: libswapify.so swap

.PHONY: clean libswapify.so swap

libswapify.so:
	$(MAKE) $(BUILD_TYPE) -C lib
	ln -f lib/libswapify.so $@

swap:
	$(MAKE) $(BUILD_TYPE) -C client
	ln -f client/swap $@

clean:
	$(MAKE) clean -C lib
	$(MAKE) clean -C client
	$(MAKE) clean -C deps/libparsemaps
	$(RM) libswapify.so swap

install: libswapify.so swap
	install -m755 libswapify.so /lib/libswapify.so
	install -m755 swap /bin/swap
	install -m755 swapify /bin/swapify

uninstall:
	$(RM) /lib/libswapify.so
	$(RM) /bin/swap
	$(RM) /bin/swapify
