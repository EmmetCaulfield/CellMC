#ifndef PREC_DOUBLE_H
#define PREC_DOUBLE_H

#include <cellmc.h>
#include <univec.h>

#if defined(PREC_SINGLE_H)
#   error Cannot include prec-double.h if prec-single.h already included.
#endif
#if PREC == CMC_PREC_SINGLE
#   error PREC is single, but double-precision header included.
#endif


#define CVT_VEC_ITOF _mm_cvtpi32_pd


#if RNG == CMC_RNG_RSMT
#   include "rsmt/rsmt.c"
#   define S_RNG_STATE_T rm_state_t
#   define _vutil_srand(STATE,SEED) rm_init(STATE,SEED)
#   define _vutil_rand(STATE) rm_rand_v2df(STATE)

#elif RNG == CMC_RNG_STDLIB
#   define S_RNG_STATE_T unsigned short
#   define _vutil_srand(STATE,SEED) /* No initializer */
static inline v2df _vutil_rand(S_RNG_STATE_T *s) {
    uv_t v;

    v.d[0]=(double)erand48(s);
    v.d[1]=(double)erand48(s);

    return v.df;
}

#elif RNG == CMC_RNG_MCRAND
#   error mc_rand() RNG not available on IA32

#endif


#if LOG == CMC_LOG_LIB
#   include "liblog.c"
inline static v2df _vutil_tau(S_RNG_STATE_T *s, v2df r) {
    return -_lib_logd2( _vutil_rand(s) )/r;
}
#elif LOG == CMC_LOG_ASM
#   error No 'asm' log option in double-precision
#elif LOG == CMC_LOG_FPU
inline static v2df _vutil_tau(S_RNG_STATE_T *s, v2df r) {
#   include "fpulog.c"
    return -_fpu_logd2( _vutil_rand(s) )/r;
}
#endif

#endif /* PREC_DOUBLE_H */
