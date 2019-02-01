SDL_VERSION:=SDL-1.2.15
SDL_MAKEFILE:=lib/sdl/SDL-1.2.15/Makefile

sdl:
	@echo "SDL config"

lib/sdl/compile:
	@echo "lib SDL compile"
ifeq (,$(wildcard $(SDL_MAKEFILE)))
	cd lib/sdl/SDL-1.2.15/ && \
	./configure \
	--host=arm-linux \
	--enable-libc \
	--enable-video \
	--enable-events \
	--enable-joystick \
	--disable-cdrom \
	--enable-threads \
	--enable-timers \
	--enable-file \
	--enable-loadso \
	--enable-cpuinfo \
	--enable-assembly \
	--disable-oss \
	--disable-alsatest \
	--enable-alsa \
	--enable-alsa-shared \
	--disable-esd \
	--disable-esdtest \
	--enable-esd-shared \
	--disable-arts \
	--disable-arts-shared \
	--disable-nas \
	--disable-nas-shared \
	--disable-diskaudio \
	--enable-audio \
	--enable-dummyaudio \
	--disable-pulseaudio \
	--disable-mintaudio \
	--disable-nasm \
	--disable-altivec \
	--disable-ipod \
	--disable-nanox \
	--disable-video-x11 \
	--disable-x11-shared \
	--disable-dga \
	--disable-video-dga \
	--disable-video-x11-dgamouse \
	--disable-video-x11-vm \
	--disable-video-x11-xv \
	--disable-video-x11-xinerama \
	--disable-video-x11-xme \
	--disable-video-x11-xrandr \
	--disable-video-photon \
	--disable-video-carbon \
	--disable-video-cocoa \
	--enable-video-fbcon \
	--disable-video-directfb \
	--disable-video-ps2gs \
	--disable-video-ps3 \
	--disable-video-ggi \
	--disable-video-svga \
	--disable-video-vgl \
	--disable-video-wscons \
	--disable-video-aalib \
	--disable-video-caca \
	--disable-video-qtopia \
	--disable-video-picogui \
	--disable-video-xbios \
	--disable-video-gem \
	--disable-video-dummy \
	--disable-video-opengl \
	--disable-osmesa-shared \
	--disable-screensaver \
	--enable-input-events \
	--disable-input-tslib \
	--enable-pth \
	--enable-pthreads \
	--enable-pthread-sem \
	--disable-stdio-redirect \
	--disable-directx \
	--enable-sdl-dlopen \
	--disable-atari-ldg \
	--disable-clock_gettime \
	--without-x \
	--disable-rpath \
	--with-alsa-prefix=$(BUILDDIR)/lib \
	--with-alsa-inc-prefix=$(BUILDDIR)/usr/include
endif
	make -j$(CORES) -C ./lib/sdl/SDL-1.2.15/ CROSS_COMPILE=$(CROSS_COMPILE) V=$(VERBOSE)
	make lib/sdl/install

lib/sdl/menuconfig:
	@echo "lib SDL menuconfig"

lib/sdl/clean:
	@echo "lib sdl clean"
ifeq (,$(wildcard $(SDL_MAKEFILE)))
else
	make -j4 -C ./lib/sdl/SDL-1.2.15/ CROSS_COMPILE=$(CROSS_COMPILE) clean
endif

lib/sdl/install:
	@echo "lib SDL install"
	@mkdir -p $(BUILDDIR)/usr/include/SDL
	@cp -raf ./lib/sdl/SDL-1.2.15/include/* $(BUILDDIR)/usr/include/SDL
	@cp -raf ./lib/sdl/SDL-1.2.15/build/.libs/libSDL*.so* $(BUILDDIR)/lib
	@cp -raf ./lib/sdl/SDL-1.2.15/build/.libs/libSDL*.so* $(ROOTFSDIR)/lib
	@$(STRIP) $(ROOTFSDIR)/lib/libSDL-1.2.so.0.11.4

lib/sdl/distclean:
	@echo "lib SDL distclean"
ifeq (,$(wildcard $(SDL_MAKEFILE)))
else
	make -j4 -C ./lib/sdl/SDL-1.2.15/ CROSS_COMPILE=$(CROSS_COMPILE) distclean
endif
