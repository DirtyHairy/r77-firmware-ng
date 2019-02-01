ZLIB_VERSION:=zlib-1.2.11
ZLIB_MAKEFILE:=lib/zlib/$(ZLIB_VERSION)/Makefile

zlib:
	@echo "zlib config"

lib/zlib/compile:
	@echo "zlib compile"
ifeq (,$(wildcard $(ZLIB_MAKEFILE)))
	cd lib/zlib/$(ZLIB_VERSION)/ && \
	./configure
endif
	make -j$(CORES) -C lib/zlib/$(ZLIB_VERSION)/ CROSS_COMPILE=$(CROSS_COMPILE) V=$(VERBOSE)
	make lib/zlib/install

lib/zlib/menuconfig:
	@echo "zlib menuconfig"

lib/zlib/clean:
	@echo "zlib clean"
	make -j$(CORES) -C lib/zlib/$(ZLIB_VERSION)/ CROSS_COMPILE=$(CROSS_COMPILE) clean

lib/zlib/install:
	@echo "zlib install"
	@cp -raf lib/zlib/$(ZLIB_VERSION)/zconf.h $(BUILDDIR)/usr/include
	@cp -raf lib/zlib/$(ZLIB_VERSION)/zlib.h $(BUILDDIR)/usr/include
	@cp -raf lib/zlib/$(ZLIB_VERSION)/libz*.so* $(BUILDDIR)/lib
	@cp -raf lib/zlib/$(ZLIB_VERSION)/libz*.so* $(ROOTFSDIR)/lib
	@$(STRIP) $(ROOTFSDIR)/lib/libz.so.1.2.11

lib/zlib/distclean:
	@echo "zlib distclean"
ifneq (,$(wildcard $(ZLIB_MAKEFILE)))
	make -j4 -C lib/zlib/$(ZLIB_VERSION)/ CROSS_COMPILE=$(CROSS_COMPILE) distclean
	rm -rf lib/zlib/$(ZLIB_VERSION)/Makefile
endif
