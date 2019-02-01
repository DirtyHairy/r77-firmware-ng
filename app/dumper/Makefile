STELLA_VERSION:=dumper

dumper:
	@echo "dumper config"

app/dumper/compile: app/dumper/dumper.c
	@echo "dumper compile"
	cd app/dumper && \
	$(GCC) -o dumper dumper.c $(CFLAGS)
	make app/dumper/install

app/dumper/install:
	@echo "dumper install"
	@cp app/dumper/dumper $(ROOTFSDIR)/usr/bin
	@$(STRIP) $(ROOTFSDIR)/usr/bin/dumper

app/dumper/clean:
	@echo "dumper clean"
	@rm -rf app/dumper/*.o app/dumper/dumper

app/dumper/distclean:
	@echo "dumper distclean"
	@rm -rf app/dumper/*.o app/dumper/dumper
