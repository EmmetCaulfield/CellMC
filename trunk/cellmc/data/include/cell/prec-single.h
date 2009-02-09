#ifndef PREC_SINGLE_H
#define PREC_SINGLE_H

#include <cellmc.h>
#include <univec.h>

#if defined(PREC_DOUBLE_H)
#   error Cannot include prec-single.h if prec-double.h already included.
#endif
#if defined(PREC) && PREC==CMC_PREC_DOUBLE
#   error PREC is double, but single-precision header included.
#endif


#if RNG == CMC_RNG_RSMT
#   error RSMT not ported to Cell/BE SPU
#elif RNG == CMC_RNG_STDLIB
#   error stdlib RNG not available on SPU
#elif RNG == CMC_RNG_MCRAND
#   include <mc_rand.h>
#   include <mc_rand_mt.h>
#   define _vutil_srand mc_rand_mt_init
#   define _vutil_rand mc_rand_mt_0_to_1_f4
#endif


#if LOG == CMC_LOG_LIB
#   include <simdmath.h>
#   include <simdmath/logf4.h>
inline static v4sf _vutil_tau(v4sf r) {
    return -_logf4( _vutil_rand() )/r;
}
#elif LOG == CMC_LOG_ASM
#   error ASM log function not ported to Cell/BE SPU
#elif LOG == CMC_LOG_FPU
#   error No FPU log function on Cell/BE SPU
#endif

#endif /* PREC_SINGLE_H */
