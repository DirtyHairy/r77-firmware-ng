SDL2_DIR=$(TOPDIR)/lib/sdl2/sdl2
SDL2_WORKDIR=$(WORKDIR_BASE)/sdl2
SDL2_MAKEFILE=$(SDL2_WORKDIR)/Makefile

lib/sdl2/compile:
	-mkdir -p $(SDL2_WORKDIR)
ifeq (,$(wildcard $(SDL2_MAKEFILE)))
	cd $(SDL2_WORKDIR) && $(SDL2_DIR)/configure \
		CFLAGS="$(CFLAGS) -I$(BUILDDIR)/include -I$(BUILDDIR)/usr/include" \
		LDFLAGS="$(LDFLAGS) -L$(BUILDDIR)/lib" \
		--host=arm-linux \
		--prefix=$(BUILDDIR) \
		--enable-video-mali \
		--disable-video-opengles1 \
		--disable-video-vulkan \
		--disable-video-wayland \
		--disable-arts \
		--disable-nas \
		--disable-pulseaudio
endif
	make -j$(CORES) -C$(SDL2_WORKDIR) V=$(VERBOSE)
	make lib/sdl2/install

lib/sdl2/install:
	make -C$(SDL2_WORKDIR) install
	cp -raf $(BUILDDIR)/lib/libSDL2*so* $(ROOTFSDIR)/lib
	$(STRIP) $(ROOTFSDIR)/lib/libSDL2.so

lib/sdl2/clean:
	make -C$(SDL2_WORKDIR) clean

lib/sdl2/distclean:
ifneq (,$(wildcard $(SDL2_MAKEFILE)))
	make -C$(SDL2_WORKDIR) distclean
endif
	-rm -fr $(SDL2_WORKDIR)
