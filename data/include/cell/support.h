#ifndef SUPPORT_H
#define SUPPORT_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <spu_intrinsics.h>
#include <spu_mfcio.h>
#include <libmisc.h>

#include <cellmc.h>
#include <ctrlblk.h>
#include <resblk.h>
#include <debug_utils.h>

#define CVT_VEC_ITOF(X) spu_convtf(X,0)

#if PREC == CMC_PREC_SINGLE
#   include <prec-single.h>
#elif PREC == CMC_PREC_DOUBLE
#   include <prec-double.h>
#else
#   error Precision not specified or invalid in support.h
#endif

#endif /* SUPPORT_H */
