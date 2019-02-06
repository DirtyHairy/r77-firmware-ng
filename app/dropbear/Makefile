DROPBEAR_DIR=$(TOPDIR)/app/dropbear/dropbear
DROPBEAR_WORKDIR=$(WORKDIR_BASE)/dropbear
DROPBEAR_MAKEFILE=$(DROPBEAR_WORKDIR)/Makefile

app/dropbear/compile:
	-mkdir -p $(DROPBEAR_WORKDIR)
ifeq (,$(wildcard $(DROPBEAR_MAKEFILE)))
	cd $(DROPBEAR_DIR) && autoconf && autoheader
	cd $(DROPBEAR_WORKDIR) && $(DROPBEAR_DIR)/configure --host=arm-linux --with-zlib=$(BUILDDIR) --disable-syslog
endif
	make -j$(CORES) -C$(DROPBEAR_WORKDIR) PROGRAMS="dropbear dropbearkey scp" MULTI=1
	make app/dropbear/install

app/dropbear/install:
	cp -raf $(DROPBEAR_WORKDIR)/dropbearmulti $(ROOTFSDIR)/bin

	ln -fs /bin/dropbearmulti $(ROOTFSDIR)/sbin/dropbear
	ln -fs /bin/dropbearmulti $(ROOTFSDIR)/sbin/dropbearkey
	ln -fs /bin/dropbearmulti $(ROOTFSDIR)/bin/scp

	$(STRIP) $(ROOTFSDIR)/bin/dropbearmulti

app/dropbear/clean:
		make -C$(DROPBEAR_WORKDIR) clean

app/dropbear/distclean:
ifneq (,$(wildcard $(DROPBEAR_MAKEFILE)))
	make -C$(DROPBEAR_WORKDIR) distclean
endif
	rm -fr $(DROPBEAR_WORKDIR)
