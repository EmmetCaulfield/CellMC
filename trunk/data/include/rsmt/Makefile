# @file  Makefile
# @brief Makefile
#
# @author Mutsuo Saito (Hiroshima University)
# @author Makoto Matsumoto (Hiroshima University)
# @author Emmet Caulfield
#
# Copyright (C) 2007 Mutsuo Saito, Makoto Matsumoto and Hiroshima
# University. All rights reserved.
#
# Copyright (C) 2008 Emmet Caulfield. All rights reserved.
#
# The new BSD License is applied to this software. See LICENSE.txt
#
# EC2008: Subtantially shortened this Makefile by using automatic
# variables in pattern rules more.

# Where the Mersenne prime parameter tables live:
PARAMS=params

# Mersenne primes for which we have parameter tables:
MERPS=607 1279 2281 4253 11213 19937 44497 86243 132049 216091

# Compiler
CC = gcc

# Set CFLAGS
WARN = -Wmissing-prototypes -Wall #-Winline 
#WARN = -Wmissing-prototypes -Wall -W
OPTI = -O3 -finline-functions -fomit-frame-pointer -DNDEBUG -fno-strict-aliasing
#OPTI = -pg -msse2 -DHAVE_SSE2 -m64
#--param inline-unit-growth=500 --param large-function-growth=900 #for gcc 4
#STD =
#STD = -std=c89 -pedantic
#STD = -std=c99 -pedantic
#STD          = -std=c99 -fms-extensions
CFLAGS       = $(OPTI) $(WARN) $(STD)
ALTIFLAGS    = -mabi=altivec -maltivec -DHAVE_ALTIVEC
OSXALTIFLAGS = -faltivec -maltivec -DHAVE_ALTIVEC
SSE2FLAGS    = -msse2 -DHAVE_SSE2

# Standard targets:
STD_TARGET  := test-std-M19937
BIG_TARGET  := test-big64-M19937
ALTI_TARGET := test-alti-M19937
SSE2_TARGET := test-sse2-M19937

# Exhaustive multi-file targets:
ALL_STD_TARGET     := $(MERPS:%=test-std-M%)
ALL_BIG_TARGET     := $(MERPS:%=test-big64-M%)
ALL_ALTI_TARGET    := $(MERPS:%=test-alti-M%)
ALL_ALTIBIG_TARGET := $(MERPS:%=test-alti64-M%)
ALL_SSE2_TARGET    := $(MERPS:%=test-sse2-M%)

# ==========================================================
# comment out or EDIT following lines to get max performance
# ==========================================================
# --------------------
# for gcc 4
# --------------------
# CFLAGS += --param inline-unit-growth=500 --param large-function-growth=900
# --------------------
# for icl
# --------------------
#CC = icl /Wcheck /O3 /QxB /Qprefetch
# -----------------
# for PowerPC
# -----------------
#CFLAGS += -arch ppc
# -----------------
# for Pentium M
# -----------------
#CFLAGS += -march=prescott
# -----------------
# for Athlon 64
# -----------------
# CFLAGS += -march=athlon64

default: std
#	echo $(ALL_STD_TARGET)

std: ${STD_TARGET}

sse2:${SSE2_TARGET}

alti:${ALTI_TARGET}

osx-alti:
	make "ALTIFLAGS=${OSXALTIFLAGS}" alti

big:${BIG_TARGET}

std-check: ${ALL_STD_TARGET}
	./check.sh 32 test-std

sse2-check: ${ALL_SSE2_TARGET}
	./check.sh 32 test-sse2

alti-check: ${ALL_ALTI_TARGET}
	./check.sh 32 test-alti

osx-alti-check:
	make "ALTIFLAGS=${OSXALTIFLAGS}" alti-check

big-check: ${ALL_BIG_TARGET} ${ALL_STD_TARGET}
	./check.sh 64 test-big64

altibig-check: ${ALL_ALTIBIG_TARGET} ${ALL_STD_TARGET}
	./check.sh 64 test-alti64

osx-altibig-check:
	make "ALTIFLAGS=${OSXALTIFLAGS}" altibig-check

test-std-M%: test.c rsmt.c rsmt.h $(PARAMS)/%.h Makefile
	${CC} ${CFLAGS} -DMEXP=$* -o $@ test.c

test-alti-M%: test.c rsmt.c rsmt.h rsmt-alti.h $(PARAMS)/%.h
	${CC} ${CFLAGS} ${ALTIFLAGS} -DMEXP=$* -o $@ test.c

test-uv-M%: test.c rsmt.c rsmt.h rsmt-uv.h $(PARAMS)/%.h Makefile
	${CC} ${CFLAGS} ${SSE2FLAGS} -DMEXP=$* -o $@ test.c

test-sse2-M%: test.c rsmt.c rsmt.h rsmt-sse2.h $(PARAMS)/%.h Makefile
	${CC} ${CFLAGS} ${SSE2FLAGS} -DMEXP=$* -o $@ test.c

test-big64-M%: test.c rsmt.c rsmt.h $(PARAMS)/%.h
	${CC} ${CFLAGS} -DONLY64 -DMEXP=$* -o $@ test.c

test-alti64-M%: test.c rsmt.c rsmt.h rsmt-alti.h $(PARAMS)/607.h
	${CC} ${CFLAGS} ${ALTIFLAGS} -DONLY64 -DMEXP=$* -o $@ test.c

clean:
	rm -f *.o *~
	rm -f $(ALL_STD_TARGET) 
	rm -f $(ALL_BIG_TARGET)
	rm -f $(ALL_ALTI_TARGET)
	rm -f $(ALL_ALTIBIG_TARGET)
	rm -f $(ALL_SSE2_TARGET)

%.b32: %-M19937
	./$< -b32 > $@

test-%: test-%.b32 exemplar-%.b32
	diff -aqs $^
	./$@-M19937 -s


.PHONY: clean std-check sse2-check alti-check
