BUSYBOX_VERSION:=busybox-1.22.1
BUSYBOX_DIR:=$(TOPDIR)/app/busybox/$(BUSYBOX_VERSION)
BUSYBOX_WORKDIR:=$(WORKDIR_BASE)/busybox
BUSYBOX_CONFIG:=$(BUSYBOX_WORKDIR)/.config


busybox:
	@echo "busybox config"

app/busybox/compile:
	@echo "busybox compile"
	mkdir -p $(BUSYBOX_WORKDIR)
ifeq (,$(wildcard $(BUSYBOX_CONFIG)))
	@echo "copy busybox config $(BUSYBOX_CONFIG)"
	cp ./app/busybox/config/config $(BUSYBOX_CONFIG)
	make -j$(CORES) -C$(BUSYBOX_DIR) O=$(BUSYBOX_WORKDIR) CROSS_COMPILE=$(CROSS_COMPILE) V=$(VERBOSE) oldconfig
endif
	@echo "busybox all CROSS_COMPILE="$(CROSS_COMPILE)
	make -j$(CORES) -C$(BUSYBOX_WORKDIR) CROSS_COMPILE=$(CROSS_COMPILE) V=$(VERBOSE)
	make app/busybox/install

app/busybox/install:
	make -j4 -C$(BUSYBOX_WORKDIR) CROSS_COMPILE=$(CROSS_COMPILE) install
	cp -raf $(BUSYBOX_WORKDIR)/_install/* $(ROOTFSDIR)
	sudo tar -xjf ./app/busybox/files.tar.bz2 -C $(ROOTFSDIR)
	sudo cp -raf ./app/busybox/files/* $(ROOTFSDIR)
	cp $(BUILDDIR)/lib/ld-linux-armhf.so.3 $(ROOTFSDIR)/lib
	$(STRIP) $(ROOTFSDIR)/lib/ld-linux-armhf.so.3
	cp $(BUILDDIR)/lib/libdl-2.25.so $(ROOTFSDIR)/lib
	$(STRIP) $(ROOTFSDIR)/lib/libdl-2.25.so
	ln -f -s libdl-2.25.so $(ROOTFSDIR)/lib/libdl.so.2
	cp $(BUILDDIR)/lib/libpthread-2.25.so $(ROOTFSDIR)/lib
	$(STRIP) $(ROOTFSDIR)/lib/libpthread-2.25.so
	ln -f -s libpthread-2.25.so $(ROOTFSDIR)/lib/libpthread.so.0
	cp $(BUILDDIR)/lib/librt-2.25.so $(ROOTFSDIR)/lib
	$(STRIP) $(ROOTFSDIR)/lib/librt-2.25.so
	ln -f -s librt-2.25.so $(ROOTFSDIR)/lib/librt.so.1
	cp $(BUILDDIR)/lib/libgcc* $(ROOTFSDIR)/lib
	$(STRIP) $(ROOTFSDIR)/lib/libgcc_s.so.1
	cp $(BUILDDIR)/lib/libstdc++.so.6.0.24 $(ROOTFSDIR)/lib
	$(STRIP) $(ROOTFSDIR)/lib/libstdc++.so.6.0.24
	ln -f -s libstdc++.so.6.0.24 $(ROOTFSDIR)/lib/libstdc++.so.6
	cp $(BUILDDIR)/lib/libm-2.25.so $(ROOTFSDIR)/lib
	$(STRIP) $(ROOTFSDIR)/lib/libm-2.25.so
	ln -f -s libm-2.25.so $(ROOTFSDIR)/lib/libm.so.6
	cp $(BUILDDIR)/lib/libc.so.6 $(ROOTFSDIR)/lib
	$(STRIP) $(ROOTFSDIR)/lib/libc.so.6
	ln -f -s libc.so.6 $(ROOTFSDIR)/lib/libc.so

app/busybox/menuconfig:
	@echo "busybox menuconfig"
	make -j4 -C ./app/busybox/busybox-1.22.1/ CROSS_COMPILE=$(CROSS_COMPILE) menuconfig

app/busybox/clean:
	@echo "busybox clean"
	make -C$(BUSYBOX_WORKDIR) CROSS_COMPILE=$(CROSS_COMPILE) clean

app/busybox/distclean:
	@echo "busybox distclean"
ifneq (,$(wildcard $(BUSYBOX_CONFIG)))
	make -C$(BUSYBOX_WORKDIR) CROSS_COMPILE=$(CROSS_COMPILE) distclean
endif
	-rm -fr $(BUSYBOX_WORKDIR)
