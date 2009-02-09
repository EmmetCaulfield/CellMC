# Note that, in this directory, we're building a PPU library that's
# needed by 'cellmc' to link against at runtime, so we have to use the
# 'ppu-*' compiler/linker suite, not the regular gcc/ld/ar suite
# that's used to build the 'cellmc' application itself.
CC=ppu-gcc
AR=ppu-ar
RANLIB=ppu-ranlib
lib_LIBRARIES = libcellmc.a
libcellmc_a_SOURCES = main.c cb_utils.c propstat.c
libcellmc_a_CPPFLAGS = -I/opt/cell/sdk/usr/include -I$(top_srcdir)/data/include -I$(top_srcdir)/data/include/cell -I$(top_srcdir)/data/include/cell/ppu -m32 -maltivec -mabi=altivec
