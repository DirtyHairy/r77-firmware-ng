STELLA_CONFIG:=app/stella/stella/config.mak

stella:

app/stella/compile:
ifeq (,$(wildcard $(STELLA_CONFIG)))
	cp app/stella/config/config.mak $(STELLA_CONFIG)
endif
	make -j$(CORES) -C ./app/stella/stella/ $(STELLA_BINARY)
	make app/stella/install

app/stella/install:
	cp app/stella/stella/$(STELLA_BINARY) $(BUILDDIR)/usr/bin/stella
	cp app/stella/stella/$(STELLA_BINARY) $(ROOTFSDIR)/usr/bin/stella
	$(STRIP) $(ROOTFSDIR)/usr/bin/stella

app/stella/menuconfig:

app/stella/clean:
	make -C ./app/stella/stella/ clean

app/stella/distclean:
ifeq (,$(wildcard $(STELLA_CONFIG)))
	cp app/stella/config/config.mak $(STELLA_CONFIG)
endif
	make -C ./app/stella/stella/ distclean
