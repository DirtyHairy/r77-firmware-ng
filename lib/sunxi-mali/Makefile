SUNXI_MALI_DIR=$(TOPDIR)/lib/sunxi-mali/sunxi-mali

lib/sunxi-mali/compile:
	make lib/sunxi-mali/install

lib/sunxi-mali/install:
	make -C$(SUNXI_MALI_DIR) install DESTDIR=$(BUILDDIR) prefix=/ VERSION=r3p0 ABI=armhf EGL_TYPE=framebuffer
	cp -raf $(BUILDDIR)/lib/libEGL* $(BUILDDIR)/lib/libGLES* $(BUILDDIR)/lib/libMali.so $(ROOTFSDIR)/lib

lib/sunxi-mali/clean:

lib/sunxi-mali/distclean:
