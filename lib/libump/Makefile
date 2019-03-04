LIBUMP_DIR=$(TOPDIR)/lib/libump/libump
LIBUMP_WORKDIR=$(WORKDIR_BASE)/libump
LIBUMP_MAKEFILE=$(LIBUMP_WORKDIR)/Makefile

lib/libump/compile:
	-mkdir -p $(LIBUMP_WORKDIR)
ifeq (,$(wildcard $(LIBUMP_MAKEFILE)))
	cd $(LIBUMP_DIR) && autoreconf -i
	cd $(LIBUMP_WORKDIR) && $(LIBUMP_DIR)/configure --host=arm-linux --prefix=$(BUILDDIR)
endif
	make -j$(CORES) -C$(LIBUMP_WORKDIR)
	make lib/libump/install

lib/libump/install:
	make -C$(LIBUMP_WORKDIR) install
	cp -raf $(BUILDDIR)/lib/libUMP*so* $(ROOTFSDIR)/lib
	$(STRIP) $(ROOTFSDIR)/lib/libUMP.so

lib/libump/clean:
		make -C$(LIBUMP_WORKDIR) clean

lib/libump/distclean:
ifneq (,$(wildcard $(LIBUMP_MAKEFILE)))
	make -C$(LIBUMP_WORKDIR) distclean
endif
	-rm -fr $(LIBUMP_WORKDIR)
