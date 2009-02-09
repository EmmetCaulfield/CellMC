#ifndef VUTIL_H
#define VUTIL_H

#include <stdint.h>
#include <spu_intrinsics.h>
#include <mc_rand.h>
#include <mc_rand_mt.h>
// #include <rand_0_to_1_v.h>
#include <debug_utils.h>

// Number of "floats per vector" (i.e. the number of 32-bit
// floating-point numbers in a 128-bit AltiVec register). It's nice to
// have this defined to make it clear when we're dealing with the
// individual elements of a vector register rather than having the 
// magic number 4 all over the place.
#define FPV (4)


// Handy ones and zeros:
#define ONESf4  ((vec_float4){1.0f, 1.0f, 1.0f, 1.0f})
#define ZEROSf4 ((vec_float4){0.0f, 0.0f, 0.0f, 0.0f})

#define ONESu4  ((vec_uint4){1,1,1,1})
#define ZEROSu4 ((vec_uint4){0,0,0,0})

#define ONESi4     ((vec_int4){1,1,1,1})
#define ZEROSi4    ((vec_int4){0,0,0,0})


// Define unions so we have easy access to the elements of vectors:
typedef union {
    vec_float4 v;
    float      f[FPV];
} quadfloat_t;

typedef union {
    vec_uint4 v;
    uint32_t  u[FPV];
} quaduint_t;

typedef union {
    vec_int4 v;
    int32_t  i[FPV];
} quadint_t;


// Initialise the RNG
static inline void _vutil_srand( uint32_t seed ) {
    mc_rand_mt_init( seed );
//    srand_v(spu_splats(seed));
}

// Get a value from the RNG.
static inline vec_float4 _vutil_rand(void) {
#ifdef DU_DEBUG
    vec_float4 f=
    DU_DUMPf4(f, "SPU| (rand) = ");
    return f;
#else
    return mc_rand_mt_0_to_1_f4();
//    return _rand_0_to_1_v();
#endif
}

// Provide analogue to vec_any_gt
static inline uint32_t _vutil_any_gt(vec_float4 a, vec_float4 b) {
    return (uint32_t)(spu_extract(spu_gather(spu_cmpgt(a, b)), 0) != 0);
}

static inline uint32_t _vutil_any_ge0(vec_float4 a) {
//    return (uint32_t)(spu_gather(a, 0)!=0x000f);
// spu_gather() grabs the LSB (a lotta fuckin' use that is)
// so we do a logical rotate left 1 bit to manoeuvre the sign bit
// into position, gather, and compare to ...00001111b = 0x000f
    return spu_extract(spu_gather(spu_rl((vec_uint4)a,1)),0)==0x000f;
}

static inline vec_int4 _vutil_binpow(vec_float4 a) {
    return ((vec_int4)spu_rl( spu_and((vec_uint4)a, 
			   spu_splats((uint32_t)0x7f800000)),9)
	    -(vec_int4){127,127,127,127});
}

static inline vec_int4 _vutil_delta_binpow(vec_float4 a, vec_float4 b) {
    return (_vutil_binpow(a)-_vutil_binpow(b));
}


// Provide analogue to vec_any_gt
static inline uint32_t _vutil_elt_gt(vec_float4 a, vec_float4 b, uint8_t elt) {
    return (uint32_t)(spu_extract(spu_gather(spu_cmpgt(a, b)), 0) & (1 << elt) );
}




#endif /* VUTIL_H */
