#VERSION:=OrangePiH3_uboot
KERNEL_CONFIG:=$(OUTDIR)/kernel/.config

kernel:
	@echo "kernel config"

kernel/armbian-linux/compile:
	@echo "kernel compile"
ifeq (,$(wildcard $(OUTDIR)/kernel))
	@mkdir -p $(OUTDIR)/kernel
ifeq (,$(wildcard $(KERNEL_CONFIG)))
	@echo "create kernel .config"
	@cp kernel/armbian-linux/config/config $(KERNEL_CONFIG)
	@ln -fs ../rootfs.cpio.lzma $(OUTDIR)/kernel
	@mkdir -p $(OUTDIR)/kernel/arch/arm/mach-sunxi/power/brom/
	@ln -fs $(TOPDIR)/kernel/armbian-linux/linux-3.4.113/arch/arm/mach-sunxi/power/brom/sun8i_brom_scatter.scat $(OUTDIR)/kernel/arch/arm/mach-sunxi/power/brom/sun8i_brom_scatter.scat
	@ln -fs $(TOPDIR)/kernel/armbian-linux/linux-3.4.113/arch/arm/mach-sunxi/power/brom/gen_check_code $(OUTDIR)/kernel/arch/arm/mach-sunxi/power/brom/gen_check_code
	@mkdir -p $(OUTDIR)/kernel/drivers/arisc/binary/
	@ln -fs $(TOPDIR)/kernel/armbian-linux/linux-3.4.113/drivers/arisc/binary/arisc_sun8iw7p1.bin $(OUTDIR)/kernel/drivers/arisc/binary/arisc_sun8iw7p1.bin
	@ln -fs $(OUTDIR)/kernel/arch/arm/boot/uImage $(OUTDIR)/uImage
endif
endif
	@echo "Compiling kernel"
	cd kernel/armbian-linux/linux-3.4.113/ && \
	make -j$(CORES) O=$(OUTDIR)/kernel/ ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} KCFLAGS="-Wno-attributes" uImage V=$(VERBOSE)

kernel/armbian-linux/menuconfig:
	@echo "kernel menuconfig"
	cd kernel/armbian-linux/linux-3.4.113/ && \
	make O=$(OUTDIR)/kernel/ ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} menuconfig

kernel/armbian-linux/modules:
	@echo "kernel modules"
	cd kernel/armbian-linux/linux-3.4.113/ && \
	make -j$(CORES) O=$(OUTDIR)/kernel/ ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} modules
	cd kernel/armbian-linux/linux-3.4.113/ && \
	make -j$(CORES) O=$(OUTDIR)/kernel/ ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) -C modules/mali LICHEE_MODE_DIR=$(OUTDIR)/kernel/out LICHEE_KDIR=$(TOPDIR)/kernel/armbian-linux/linux-3.4.113/ LICHEE_PLATFORM=linux

kernel/armbian-linux/clean:
	@echo "kernel clean"
	cd kernel/armbian-linux/linux-3.4.113/ && \
	make O=$(OUTDIR)/kernel/ ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} clean

kernel/armbian-linux/install:
	@echo "install kernel"
#	@cp $(OUTDIR)/kernel/arch/arm/boot/uImage $(OUTDIR)

kernel/armbian-linux/distclean:
	@echo "kernel distclean"
	cd kernel/armbian-linux/linux-3.4.113/ && \
	make O=$(OUTDIR)/kernel/ ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} mrproper
