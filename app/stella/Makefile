STELLA_CONFIG:=app/stella/stella/config.mak


stella:
	@echo "stella config"

app/stella/compile:
	@echo "stella compile"
ifeq (,$(wildcard $(STELLA_CONFIG)))
	@echo "creating stella config => "$(STELLA_CONFIG)
	@cp app/stella/config/config.mak $(STELLA_CONFIG)
#	cd app/stella/stella-3.7.5/ && \
        ./configure \
	--disable-gl \
	--disable-windowed \
	--disable-debugger \
	--enable-joystick \
	--disable-cheats \
	--disable-static \
	--enable-sound \
	--force-builtin-libpng \
	--force-builtin-zlib
endif
	@echo "stella all CROSS_COMPILE="$(CROSS_COMPILE)
	make -j$(CORES) -C ./app/stella/stella/ V=$(VERBOSE)
	make app/stella/install

app/stella/install:
	@echo "stella install"
	@cp app/stella/stella/stella $(BUILDDIR)/usr/bin
	@cp app/stella/stella/stella $(ROOTFSDIR)/usr/bin
	@$(STRIP) $(ROOTFSDIR)/usr/bin/stella

app/stella/menuconfig:
	@echo "stella menuconfig"

app/stella/clean:
	@echo "stella clean"
	make -C ./app/stella/stella/ clean

app/stella/distclean:
	@echo "stella distclean"
	make -C ./app/stella/stella/ distclean
