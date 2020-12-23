debug : BUILD_TYPE := debug
release : BUILD_TYPE := release
debug: libswapify.so swap
release: libswapify.so swap

.PHONY: clean libswapify.so swap

libswapify.so:
	$(MAKE) $(BUILD_TYPE) -C lib
	ln lib/libswapify.so $@

swap:
	$(MAKE) $(BUILD_TYPE) -C client
	ln client/swap $@

clean:
	$(MAKE) clean -C lib
	$(MAKE) clean -C client
	$(MAKE) clean -C deps/libparsemaps
	$(RM) libswapify.so swap
