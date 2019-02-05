PNG_VERSION:=1.2.56
PNG_DIR:=$(TOPDIR)/lib/libpng/libpng-$(PNG_VERSION)
PNG_WORKDIR:=$(WORKDIR_BASE)/libpng
PNG_MAKEFILE:=$(PNG_WORKDIR)/Makefile

png:
	@echo "png config"

lib/png/compile:
	@echo "png compile"
	-mkdir -p $(PNG_WORKDIR)
ifeq (,$(wildcard $(PNG_MAKEFILE)))
	cd $(PNG_WORKDIR) && $(PNG_DIR)/configure \
		CFLAGS="$(CFLAGS) -I$(BUILDDIR)/include" \
		CPPFLAGS="$(CPPFLAGS) -I$(BUILDDIR)/include" \
		LDFLAGS="$(LDFLAGS) -L$(BUILDDIR)/lib" \
		--prefix=$(BUILDDIR) \
		--host=arm-linux \
		--enable-shared
endif
	make -j$(CORES) -C$(PNG_WORKDIR) CROSS_COMPILE=$(CROSS_COMPILE) V=$(VERBOSE)
	make lib/png/install

lib/png/menuconfig:
	@echo "png menuconfig"

lib/png/clean:
	@echo "png clean"
ifeq (,$(wildcard $(PNG_MAKEFILE)))
else
	make -C$(PNG_WORKDIR) clean
endif
	@rm -f $(ROOTFSDIR)/lib/libpng.so
	@rm -f $(BUILDDIR)/lib/libpng.so

lib/png/install:
	@echo "png install"
	make -C$(PNG_WORKDIR) install
	cp -raf $(BUILDDIR)/lib/libpng*so* $(ROOTFSDIR)/lib
	$(STRIP) $(ROOTFSDIR)/lib/libpng12.so
	$(STRIP) $(ROOTFSDIR)/lib/libpng.so
	
lib/png/distclean:
	@echo "png distclean"
ifeq (,$(wildcard $(PNG_MAKEFILE)))
else
	make -C$(PNG_WORKDIR) distclean
endif
	-rm -fr $(PNG_WORKDIR)
