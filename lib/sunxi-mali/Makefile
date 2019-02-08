SUNXI_MALI_DIR=$(TOPDIR)/lib/sunxi-mali/sunxi-mali

lib/sunxi-mali/compile:
	make lib/sunxi-mali/install

lib/sunxi-mali/install:
	make -C$(SUNXI_MALI_DIR) install DESTDIR=$(BUILD_ROOT) prefix=/ VERSION=r3p0 ABI=armhf EGL_TYPE=framebuffer
	cp -raf $(BUILDROOT)/lib/libEGL* $(BUILDROOT)/lib/libGLES* $(BUILDROOT)/lib/libMali.so $(ROOTFSDIR)/lib

lib/sunxi-mali/clean:

lib/sunxi-mali/distclean:
