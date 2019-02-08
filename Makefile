include build/main.mk
include ./app/busybox/Makefile
include ./app/uboot/Makefile
include ./lib/alsa/Makefile
include ./lib/sdl/Makefile
include ./lib/libpng/Makefile
include ./lib/zlib/Makefile
include ./app/stella/Makefile
include ./kernel/armbian-linux/Makefile
include ./app/dumper/Makefile
include ./app/dropbear/Makefile
include ./app/sftpserver/Makefile
include ./lib/libump/Makefile

all:
	@echo "TOP Makefile"
	make app/busybox/compile
	make app/uboot/compile
	make lib/alsa/compile
	make lib/sdl/compile
	make lib/zlib/compile
	make lib/png/compile
	make lib/libump/compile
	make app/dropbear/compile
	make app/sftpserver/compile
	make app/stella/compile
	make app/dumper/compile
	make cpio
	make kernel/armbian-linux/compile
	make sdcard
clean:
	@echo "TOP Makefile clean"
	make app/busybox/clean
	make app/uboot/clean
	make lib/alsa/clean
	make lib/sdl/clean
	make lib/zlib/clean
	make lib/png/clean
	make lib/libump/clean
	make app/dropbear/clean
	make app/sftpserver/distclean
	make app/stella/clean
	make app/dumper/clean
	make kernel/armbian-linux/clean

distclean:
	@echo "TOP Makefile distclean"
	make app/busybox/distclean
	make app/uboot/distclean
	make lib/alsa/distclean
	make lib/sdl/distclean
	make lib/zlib/distclean
	make lib/png/distclean
	make lib/libump/distclean
	make app/stella/distclean
	make app/dumper/distclean
	make app/dropbear/distclean
	make app/sftpserver/distclean
	make kernel/armbian-linux/distclean
	rm -rf out

install:
	make app/busybox/install
	make lib/alsa/install
	make lib/sdl/install
	make lib/png/install
	make lib/zlib/install
	make lib/libump/install
	make app/dropbear/install
	make app/sftpserver/install
	make app/stella/install
	make app/dumper/install
	make kernel/armbian-linux/install

cpio:
	rm -rf $(OUTDIR)/rootfs.cpio.gz
	cd $(ROOTFSDIR) && \
	find . | cpio -H newc -o --owner root:root -F ../rootfs.cpio && \
	cd .. && \
	gzip rootfs.cpio

sdcard:
	mkdir -p $(OUTDIR)/tmp
	rm -rf $(OUTDIR)/sdcard.img
	dd if=/dev/zero of=$(OUTDIR)/sdcard.img bs=1M count=80
	sudo echo -e "o\nn\np\n1\n\n\nw" | fdisk $(OUTDIR)/sdcard.img
	sudo echo -e "t\nc\nw" | fdisk $(OUTDIR)/sdcard.img
	sync
	sudo dd if=$(OUTDIR)/u-boot-sunxi-with-spl.bin of=$(OUTDIR)/sdcard.img bs=1k seek=8 conv=notrunc
	sudo losetup --offset 1048576 /dev/loop7 $(OUTDIR)/sdcard.img
	sudo mkfs.msdos /dev/loop7
	sudo mount /dev/loop7 $(OUTDIR)/tmp/
	sudo cp -rf $(TOPDIR)/rom/* $(OUTDIR)/tmp/
	sudo cp $(OUTDIR)/boot.scr $(OUTDIR)/tmp/
	sudo cp $(OUTDIR)/script.bin $(OUTDIR)/tmp/
	sudo cp $(OUTDIR)/uImage $(OUTDIR)/tmp/
	sudo sync
	sudo umount -f $(OUTDIR)/tmp/
	sudo losetup -d /dev/loop7
	@echo "\033[1;31m[SDCard Image Created: $(OUTDIR)/sdcard.img]\033[0m"
