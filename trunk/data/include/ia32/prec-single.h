#ifndef PREC_SINGLE_H
#define PREC_SINGLE_H

#include <cellmc.h>

#if defined(PREC_DOUBLE_H)
#   error Cannot include prec-single.h if prec-double.h already included.
#endif
#if defined(PREC) && PREC==CMC_PREC_DOUBLE
#   error PREC is double, but single-precision header included.
#endif


#include <univec.h>


#define CVT_VEC_ITOF uv_cvtdq2ps


#if RNG == CMC_RNG_RSMT
#   include "rsmt/rsmt.c"
#   define S_RNG_STATE_T rm_state_t
#   define _vutil_srand(STATE,SEED) rm_init(STATE,SEED)
#   define _vutil_rand(STATE) rm_rand_v4sf(STATE)

#elif RNG == CMC_RNG_STDLIB
#   define S_RNG_STATE_T unsigned short
#   define _vutil_srand(STATE,SEED) /* No initializer */
static inline v4sf _vutil_rand(S_RNG_STATE_T *state) {
    uv_t v;
    int i;
    for(i=0; i<UV_4; i++) {
        v.f[i]=(float)erand48( state );
    }
    return v.sf;
}

#elif RNG == CMC_RNG_MCRAND
#   error mc_rand() RNG not available on IA32

#endif


#if LOG == CMC_LOG_LIB
#   include "liblog.c"
inline static v4sf _vutil_tau(S_RNG_STATE_T *s, v4sf r) {
    return -_lib_logf4( _vutil_rand(s) )/r;
}
#elif LOG == CMC_LOG_ASM
#   include "asmlog.c"
inline static v4sf _vutil_tau(S_RNG_STATE_T *s, v4sf r) {
    return -_asm_logf4( _vutil_rand(s) )/r;
}
#elif LOG == CMC_LOG_FPU
inline static v4sf _vutil_tau(S_RNG_STATE_T *s, v4sf r) {
#   include "fpulog.c"
    return -_fpu_logf4( _vutil_rand(s) )/r;
}
#endif

#endif /* PREC_SINGLE_H */
