BUSYBOX_VERSION:=busybox-1.22.1
BUSYBOX_DIR:=$(TOPDIR)/app/busybox/$(BUSYBOX_VERSION)
BUSYBOX_WORKDIR:=$(WORKDIR_BASE)/busybox
BUSYBOX_CONFIG:=$(BUSYBOX_WORKDIR)/.config
SHELL=bash

ROOTFS_LIBS = \
    ld-linux-armhf \
    libc \
    libcrypt \
    libdl \
    libgcc_s \
    libm \
    libnsl \
    libnss_compat \
    libnss_dns \
    libnss_files \
    libpthread \
    librt \
    libstdc++ \
    libutil \
    libresolv

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

	shopt -s nullglob; for libname in $(ROOTFS_LIBS); do \
	    lib=`echo $(BUILDDIR)/lib/$${libname}.so.[0-9]` \
	    lib="$${lib##*/}"; \
	    cp -L $(BUILDDIR)/lib/$$lib $(ROOTFSDIR)/lib; \
	    $(STRIP) $(ROOTFSDIR)/lib/$$lib; \
	    for link in `echo $(BUILDDIR)/lib/$${libname}*.so`; do \
	        ln -fs $$lib "$(ROOTFSDIR)/lib/$${link##*/}"; \
	    done; \
        done	    

	ln -fs libc.so.6 $(ROOTFSDIR)/lib/libc.s

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
