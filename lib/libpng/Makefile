PNG_VERSION:=1.2.56
PNG_DIR:=lib/libpng/libpng-$(PNG_VERSION)
PNG_MAKEFILE:=$(PNG_DIR)/Makefile

png:
	@echo "png config"

lib/png/compile:
	@echo "png compile"
ifeq (,$(wildcard $(PNG_MAKEFILE)))
	cd $(PNG_DIR)/ && \
	LDFLAGS=-L$(BUILDDIR)/lib CFLAGS=-I$(BUILDDIR)/usr/include ./configure \
	--host=arm-linux --enable-shared
endif
	make -j$(CORES) -C $(PNG_DIR)/ CROSS_COMPILE=$(CROSS_COMPILE) V=$(VERBOSE) LDFLAGS=-L$(BUILDDIR)/lib CFLAGS=-I$(BUILDDIR)/usr/include CPPFLAGS=-I$(BUILDDIR)/usr/include
	make lib/png/install

lib/png/menuconfig:
	@echo "png menuconfig"

lib/png/clean:
	@echo "png clean"
ifeq (,$(wildcard $(PNG_MAKEFILE)))
else
	make -j$(CORES) -C $(PNG_DIR)/ CROSS_COMPILE=$(CROSS_COMPILE) V=$(VERBOSE) clean
endif
	@rm -f $(ROOTFSDIR)/lib/libpng.so
	@rm -f $(BUILDDIR)/lib/libpng.so

lib/png/install:
	@echo "png install"
	@mkdir -p  $(BUILDDIR)/usr/include/
	@cp -raf $(PNG_DIR)/png.h $(BUILDDIR)/usr/include/
	@cp -raf $(PNG_DIR)/pngconf.h $(BUILDDIR)/usr/include/
	@cp -raf $(PNG_DIR)/.libs/libpng*.so* $(BUILDDIR)/lib
	@cp -raf $(PNG_DIR)/.libs/libpng*.so* $(ROOTFSDIR)/lib
	@$(STRIP) $(ROOTFSDIR)/lib/libpng12.so.0.56.0
	@$(STRIP) $(ROOTFSDIR)/lib/libpng.so.3.56.0
	
lib/png/distclean:
	@echo "png distclean"
ifeq (,$(wildcard $(PNG_MAKEFILE)))
else
	make -j$(CORES) -C $(PNG_DIR)/ CROSS_COMPILE=$(CROSS_COMPILE) V=$(VERBOSE) distclean
endif
