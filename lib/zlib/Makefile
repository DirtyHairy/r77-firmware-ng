ZLIB_VERSION:=zlib-1.2.11
ZLIB_WORKDIR:=$(WORKDIR_BASE)/zlib
ZLIB_DIR:=$(TOPDIR)/lib/zlib/$(ZLIB_VERSION)
ZLIB_MAKEFILE:=$(ZLIB_WORKDIR)/Makefile

zlib:
	@echo "zlib config"

lib/zlib/compile:
	@echo "zlib compile"
	-mkdir -p $(ZLIB_WORKDIR)
ifeq (,$(wildcard $(ZLIB_MAKEFILE)))
	cd $(ZLIB_WORKDIR) && $(ZLIB_DIR)/configure --prefix=$(BUILDDIR)
endif
	make -j$(CORES) -C$(ZLIB_WORKDIR) CROSS_COMPILE=$(CROSS_COMPILE) V=$(VERBOSE)
	make lib/zlib/install

lib/zlib/menuconfig:
	@echo "zlib menuconfig"

lib/zlib/clean:
	@echo "zlib clean"
	make $(ZLIB_WORKDIR) clean

lib/zlib/install:
	@echo "zlib install"
	make -C$(ZLIB_WORKDIR) install
	cp -raf $(BUILDDIR)/lib/libz*so* $(ROOTFSDIR)/lib
	$(STRIP) $(ROOTFSDIR)/lib/libz.so

lib/zlib/distclean:
	@echo "zlib distclean"
	-rm -fr $(ZLIB_WORKDIR)
