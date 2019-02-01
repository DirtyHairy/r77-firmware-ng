#APP:=app/busybox
ALSA_VERSION:=alsa-lib-1.1.0
ALSA_MAKEFILE:=lib/alsa/alsa-lib-1.1.0/Makefile

alsa:
	@echo "alsa config"

lib/alsa/compile:
	@echo "lib alsa compile"
ifeq (,$(wildcard $(ALSA_MAKEFILE)))
	cd lib/alsa/alsa-lib-1.1.0/ && \
	./configure \
        --prefix=${OUTDIR} \
        --host=arm-linux \
        --disable-python \
        --without-debug \
        --disable-hwdep \
        --with-versioned=no \
        --enable-pcm \
        --enable-mixer \
        --enable-hwdep \
        --with-configdir=/usr/share/alsa \
        --with-plugindir=/usr/lib/alsa-lib \
        --with-debug=no \
        --with-tmpdir=/tmp \
        --with-pthread=yes \
        --with-librt=yes \
        --with-alsa-devdir=/dev/snd \
        --with-aload-devdir=/dev \
        --disable-alisp
endif
	make -j$(CORES) -C ./lib/alsa/alsa-lib-1.1.0/ CROSS_COMPILE=$(CROSS_COMPILE) V=$(VERBOSE)
	make lib/alsa/install

lib/alsa/menuconfig:
	@echo "lib alsa menuconfig"

lib/alsa/clean:
	@echo "lib alsa clean"
ifeq (,$(wildcard $(ALSA_MAKEFILE)))
else
	make -j4 -C ./lib/alsa/alsa-lib-1.1.0/ CROSS_COMPILE=$(CROSS_COMPILE) clean
endif

lib/alsa/install:
	@echo "lib alsa install"
	@cp -raf ./lib/alsa/alsa-lib-1.1.0/include/* $(BUILDDIR)/usr/include
	@cp -raf ./lib/alsa/alsa-lib-1.1.0/src/.libs/libasound.so* $(BUILDDIR)/lib
	@mkdir -p $(BUILDDIR)/usr/share/alsa
	@cp -raf ./lib/alsa/alsa-lib-1.1.0/src/conf/alsa.conf $(BUILDDIR)/usr/share/alsa
	@cp -raf ./lib/alsa/files/* $(BUILDDIR)/
	#rootfs
	@cp -raf $(BUILDDIR)/lib/libasound.so* $(ROOTFSDIR)/lib
	@$(STRIP) $(ROOTFSDIR)/lib/libasound.so.2.0.0
	@mkdir -p $(ROOTFSDIR)/usr/share/alsa
	@cp -raf ./lib/alsa/alsa-lib-1.1.0/src/conf/alsa.conf $(ROOTFSDIR)/usr/share/alsa
	@cp -raf ./lib/alsa/files/* $(ROOTFSDIR)/

lib/alsa/distclean:
	@echo "lib alsa distclean"
ifeq (,$(wildcard $(ALSA_MAKEFILE)))
else
	@make -j4 -C ./lib/alsa/alsa-lib-1.1.0/ CROSS_COMPILE=$(CROSS_COMPILE) distclean
endif
