TOPDIR:=${CURDIR}

OUTDIR ?= $(TOPDIR)/out
TOOLCHAIN ?= $(TOPDIR)/toolchain

BUILDDIR:=$(OUTDIR)/build_root
ROOTFSDIR:=$(OUTDIR)/rootfs
OUT_SDK_DIR:=$(BUILDDIR)/usr

WORKDIR_BASE := $(OUTDIR)/build

ARCH:=arm
CROSS_COMPILE:=arm-linux-gnueabihf-
CC:=${CROSS_COMPILE}gcc
GCC:=${CROSS_COMPILE}gcc
CPP:=${CROSS_COMPILE}cpp
CXX:=${CROSS_COMPILE}g++
LD:=${CROSS_COMPILE}ld
AR:=${CROSS_COMPILE}ar
RANLIB:=${CROSS_COMPILE}ranlib
STRIP:=${CROSS_COMPILE}strip
CFLAGS:=-Os -Wall -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4
CFLAGS_HP:=-O3 -Wall -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4 -funsafe-math-optimizations -flto

#LDFLAGS:="-L$(BUILDDIR)/lib"
LDFLAGS:=
DFLAGS:=
IFLAGS:=-I$(BUILDDIR)/usr/include/SDL -I$(BUILDDIR)/usr/include
LIBPATHS:=-L$(BUILDDIR)/lib
LIBS:=
VERBOSE:=$(V)
VERBOSE?=0
CORES:=$(shell grep -c ^processor /proc/cpuinfo)

path:=$(TOOLCHAIN)/toolchain/bin:$(TOPDIR)/tools/sunxi-tools:${PATH}
PATH:=${path}

LICHEE_PLATFORM:=linux
LICHEE_KDIR:=$(TOPDIR)/kernel/armbian-linux/linux-3.4.113/

STELLA_BINARY := stella
ifdef RELEASE
    STELLA_BINARY := stella-pgo
endif

export TOPDIR OUTDIR BUILDDIR ROOTFSDIR OUT_SDK_DIR ARCH CROSS_COMPILE CC GCC CXX LD AR RANLIB CFLAGS LDFLAGS PATH VERBOSE STELLA_BINARY CFLAGS_HP

main: setup
	make all

setup:
	@echo "main start"
	@echo "CURDIR="${CURDIR}
	@echo "TOPDIR="${TOPDIR}
	@echo "CROSS_COMPILE="${CROSS_COMPILE}
	@echo "PATH="${PATH}
	@echo "CPU CORES="$(CORES)
ifeq (,$(wildcard $(TOPDIR)/out))
	@echo "making out folder"
	@mkdir -p ${OUTDIR}
	@mkdir -p $(OUTDIR)/test
endif
ifeq (,$(wildcard $(BUILDDIR)))
	@echo "making out/build_root/ folder"
	@mkdir -p $(BUILDDIR)
	@echo "copying sysroot-glibc"
	@cp -raf $(TOOLCHAIN)/sysroot-glibc/* ${BUILDDIR}
endif
ifeq (,$(wildcard $(ROOTFSDIR)))
	@mkdir -p $(ROOTFSDIR)
endif
