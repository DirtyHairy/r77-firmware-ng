# -------- Generated by configure -----------

CXX := arm-linux-gnueabihf-c++
CXXFLAGS := $(CFLAGS_HP)
LD :=  arm-linux-gnueabihf-c++
LIBS +=  -L$(BUILDDIR)/lib -lSDL2 -lpng -lz -lsqlite3
RANLIB := arm-linux-gnueabihf-ranlib
INSTALL := install
AR := arm-linux-gnueabihf-ar cru
MKDIR := mkdir -p
ECHO := printf
CAT := cat
RM := rm -f
RM_REC := rm -f -r
ZIP := zip -q
CP := cp
WINDOWSPATH=
STRIP := arm-linux-gnueabihfstrip
BINARY_LOADER := qemu-arm -cpu cortex-a7 -L $(ROOTFSDIR)

MODULES +=  src/unix src/unix/r77 src/gui src/common/repository/sqlite
MODULE_DIRS +=
EXEEXT :=

PREFIX := /usr/local
BINDIR := /usr/local/bin
DOCDIR := /usr/local/share/doc/stella
DATADIR := /usr/local/share
PROFILE :=

HAVE_GCC = 1

INCLUDES += -I$(BUILDDIR)/include -I/$(BUILDDIR)/include/SDL2 -Isrc/emucore -Isrc/common -Isrc/common/tv_filters -Isrc/gui -Isrc/emucore/tia -Isrc/emucore/tia/frame-manager -Isrc/unix -Isrc/unix/r77 -Isrc/common/repository/sqlite -I/usr/local/include/SDL2 -D_THREAD_SAFE
OBJS +=
DEFINES +=  -DUNIX -DBSPF_UNIX -DRETRON77 -DSOUND_SUPPORT -DJOYSTICK_SUPPORT -DSDL_SUPPORT -DGUI_SUPPORT -DPNG_SUPPORT -DZIP_SUPPORT -DSQLITE_SUPPORT
LDFLAGS +=

CXX_UPDATE_DEP_FLAG = -MMD -MF "$(*D)/$(DEPDIR)/$(*F).d" -MQ "$@" -MP
