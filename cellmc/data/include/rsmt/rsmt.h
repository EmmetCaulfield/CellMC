/** 
 * @file rsmt.h 
 *
 * @brief Reentrant SIMDized Mersenne Twister pseudorandom
 *        number generator, based on SFMT.
 *
 * @author Mutsuo Saito (Hiroshima University)
 * @author Makoto Matsumoto (Hiroshima University)
 * @author Emmet Caulfield
 *
 * Copyright (C) 2006, 2007 Mutsuo Saito, Makoto Matsumoto and Hiroshima
 * University. All rights reserved.
 *
 * Copyright (C) 2008, Emmet Caulfield. All rights reserved.
 *
 * The new BSD License is applied to this software. See LICENSE.txt
 *
 * EC2008:
 *
 * The intention of RSMT is to turn SFMT into a clean thread-safe
 * library that can easily be integrated with other software without
 * horrid namespace pollution and observing usual conventions of 'C'
 * programming.
 *
 * Support for oddball compilers and systems has been unceremoniously
 * stripped, since I don't use these platforms and would rather hack
 * out such support than attempt to work around it and leave it badly
 * broken. This support can always be added back in later if
 * possible. Therefore, this version *WILL NOT* compile on all
 * platforms supported by SFMT.
 *
 * This version does not define any external symbols other than those
 * in the public interface. All symbols in the public interface begin
 * with 'rm_' (or 'RM_' for preprocessor macros) as a pseudo-namespace.
 *
 * Note that all of the functions returning double-precision floating
 * point numbers use 32-bit unsigned integers as a starting
 * point. Therefore, the resulting DPFP number has (at most) 32 bits
 * of precision, even though the IEEE-754 significand of a DPFP number
 * allows for 53 bits. A DPFP number with the full 52 bits of
 * significand can be generated trivially by converting a 64-bit
 * integer in the same manner as is done for 32-bit numbers below.
 *
 *
 * Although irrelevant to this version, I want to keep a note of the
 * following:
 *
 * Bizarrely, this typedef made a significant difference to speed.
 * Choosing 'int64_t' or 'uint64_t' instead of 'long long' incurs a
 * runtime penalty, even though these are themselves typedef-ed to be
 * the same. Very weird. (GCC 4.2.3, Athlon64) 
 *   typedef long long v2di * __attribute__ ((vector_size (16), may_alias));
 */

#ifndef RSMT_H
#define RSMT_H

#include <stdint.h>
#include <stdio.h>
#include "params/rsmt-params.h"
#include "../univec.h"


/*--------------------------------------------------*\
 * State structure, formerly global in SFMT.c, but
 * now needed here (for function prototypes in the
 * public interface).
 *
 * RM_N32 and RM_N64 are #defined to be the correct
 * multiples of RM_N in params/rsmt-params.h
 *--------------------------------------------------*/
typedef struct rm_state_t {
    /* 
     * Internal state array, accessible via 3 different arrays. We
     * eschew the anonymous union, since it is nonstandard (though
     * supported by gcc and xlc at least) and some versions of gcc
     * seem to have problems with it.
     */
    union {
	uv_t     sfmt[RM_N  ];
	uint32_t  p32[RM_N32];
	uint64_t  p64[RM_N64];
    };
    /* Index counter into 32-bit internal state array */

    int idx;

    /* True if this struct been initialized: */
    int initialized;
} rm_state_t;


uint32_t rm_rand32(rm_state_t *s);
uint64_t rm_rand64(rm_state_t *s);
static inline v4si rm_rand_v4si(rm_state_t *s);
void rm_rand32_array(rm_state_t *s, uint32_t *array, int size);
void rm_rand64_array(rm_state_t *s, uint64_t *array, int size);
void rm_init(rm_state_t *s, uint32_t seed);
void rm_init_array(rm_state_t *s, uint32_t *init_key, int key_length);
const char *rm_idstring(void);
int rm_min_array_size32(void);
int rm_min_array_size64(void);

/**
 * 2^32 and 2^64 as double-precision floating constants 
 */
#define RM_2R32D (4294967296.0)
#define RM_2R64D (18446744073709551616.0LL)
#define RM_MANT (0x007fffff)
#define RM_EXP  (0x3f800000)
#define RM_SGN  (0x7fffffff)
#define RM_AND_MASK ((v4si){RM_MANT, RM_MANT, RM_MANT, RM_MANT})
#define RM_XOR_MASK ((v4si){RM_EXP, RM_EXP, RM_EXP, RM_EXP})
#define RM_SGN_MASK ((v4si){RM_SGN, RM_SGN, RM_SGN, RM_SGN})
#define RM_2R31_SINV   (4.65661287307739e-10f)
#define RM_2R31_INV_V4 ((v4sf){RM_2R31_SINV,RM_2R31_SINV,RM_2R31_SINV,RM_2R31_SINV})

#define RM_DBL_SGN      (0x7fffffffffffffffLL)
#define RM_DBL_SGN_MASK ((v2di){RM_DBL_SGN, RM_DBL_SGN})
#define RM_2R31_DINV    (0x3e00000000000000LL)
#define RM_2R31_INV_V2  ((v2di){RM_2R31_DINV, RM_2R31_DINV})

/* Originals due to Isaku Wada */
/**
 * Converts a vector of unsigned 32-bit integers to a floating-point
 * approximation on [0,1) with single precision
 */
static inline v4sf rm_rand_v4sf(rm_state_t *s) {
    uv_t u;
    u.si = rm_rand_v4si(s);
    __asm__ (
	"cvtdq2ps   %[u],   %[u]   \n"
	"pand      %[sm],   %[u]   \n"
	"mulps     %[cf],   %[u]   \n"
	: [u]"+&x"(u.si)
	: [sm]"x"(RM_SGN_MASK), [cf]"x"(RM_2R31_INV_V4)
	);
    return u.sf;
}

/**
 * Converts a vector of unsigned 64-bit integers to a floating-point
 * approximation on [0,1) with double precision
 */
static inline v2df rm_rand_v2df(rm_state_t *s) {
    uv_t u;
    u.u64[0] = rm_rand64(s);
//    uv_print_di("before", u);
//    uv_print_di("before", u);
//    uv_print_si("before", u);
    __asm__ (
	"cvtdq2pd   %[u],   %[u]   \n"
	"pand      %[sm],   %[u]   \n"
	"mulpd     %[cf],   %[u]   \n"
	: [u]"+&x"(u.df)
	: [sm]"x"(RM_DBL_SGN_MASK), [cf]"x"(RM_2R31_INV_V2)
	);
//    uv_print_df("after ", u);
    return u.df;
}

#endif /* RSMT_H */
