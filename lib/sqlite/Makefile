SQLITE_DIR=$(TOPDIR)/lib/sqlite/sqlite
SQLITE_WORKDIR=$(WORKDIR_BASE)/sqlite
SQLITE_MAKEFILE=$(SQLITE_WORKDIR)/Makefile

lib/sqlite/compile:
	-mkdir -p $(SQLITE_WORKDIR)
ifeq (,$(wildcard $(SQLITE_MAKEFILE)))
	cd $(SQLITE_WORKDIR) && $(SQLITE_DIR)/configure \
		CFLAGS="$(CFLAGS)" \
		LDFLAGS="$(LDFLAGS)" \
		--host=arm-none-linux-gnueabihf \
		--build=amd64-linux-gnu \
		--prefix=$(BUILDDIR)
endif
	make -j$(CORES) -C$(SQLITE_WORKDIR)
	make lib/sqlite/install

lib/sqlite/install:
	make -C$(SQLITE_WORKDIR) install
	cp -raf $(BUILDDIR)/lib/libsqlite3*so* $(ROOTFSDIR)/lib
	cp -raf $(BUILDDIR)/bin/sqlite3 $(ROOTFSDIR)/bin
	$(STRIP) $(ROOTFSDIR)/lib/libsqlite3.so
	$(STRIP) $(ROOTFSDIR)/bin/sqlite3

lib/sqlite/clean:
	make -C$(SQLITE_WORKDIR) clean

lib/sqlite/distclean:
ifneq (,$(wildcard $(SQLITE_MAKEFILE)))
	make -C$(SQLITE_WORKDIR) distclean
endif
	-rm -fr $(SQLITE_WORKDIR)
