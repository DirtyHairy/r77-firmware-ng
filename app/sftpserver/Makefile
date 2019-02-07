SFTPSERVER_DIR=$(TOPDIR)/app/sftpserver/sftpserver
SFTPSERVER_WORKDIR=$(WORKDIR_BASE)/sftpserver
SFTPSERVER_MAKEFILE=$(SFTPSERVER_WORKDIR)/Makefile

app/sftpserver/compile:
	-mkdir -p $(SFTPSERVER_WORKDIR)
ifeq (,$(wildcard $(SFTPSERVER_MAKEFILE)))
	cd $(SFTPSERVER_DIR) && autoconf && automake -a
	cd $(SFTPSERVER_WORKDIR) && $(SFTPSERVER_DIR)/configure LDFLAGS="$(LDFLAGS) -lpthread" --host=arm-linux
endif
	make -j$(CORES) -C$(SFTPSERVER_WORKDIR)
	make app/sftpserver/install

app/sftpserver/install:
	cp -raf $(SFTPSERVER_WORKDIR)/gesftpserver $(ROOTFSDIR)/usr/libexec/sftp-server
	$(STRIP) $(ROOTFSDIR)/usr/libexec/sftp-server

app/sftpserver/clean:
		make -C$(SFTPSERVER_WORKDIR) clean

app/sftpserver/distclean:
ifneq (,$(wildcard $(SFTPSERVER_MAKEFILE)))
	make -C$(SFTPSERVER_WORKDIR) distclean
endif
	rm -fr $(SFTPSERVER_WORKDIR)
