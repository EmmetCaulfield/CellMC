/**
 * @file   univec.h
 * @brief  Provides 'universal' support for 128-bit vectors across 
 *         PC and Cell/BE.
 * @author Emmet Caulfield
 *
 * The 'vNpt' type-naming style from intel is adopted here. Although a
 * little cryptic, it gets extremely tedious writing 'vector float' as
 * for VMX/AltiVec, and one quickly becomes accustomed to the handful
 * of abbreviations. Explanation:
 *
 *    * v = 'vector'
 *    * N = number of elements: 2, 4, 8, or 16
 *    * x = precision: d=double (64-bit), s=single (32-bit), 
 *                     h=half   (16-bit), q=quarter (8-bit)
 *    * t = type: i=integer, f=floating-point, u=unsigned int (added).
 *
 * Note that because the vector size is fixed at 128 bits, the N and x
 * fields are mutually redundant: 2<=>d, 4<=>f, 8<=>h, 16<=>q.
 *
 * $Id: univec.h 85 2009-02-02 03:31:39Z emmet $
 */
#ifndef UNIVEC_H
#define UNIVEC_H
#include <stdint.h>


#define UV_4 (4)
#define UV_2 (2)


#if defined(__PPU__)
#   if !defined(__VEC__) || !defined(__ALTIVEC__)
#      error You must enable AltiVec in gcc with "-maltivec"
#   endif
#elif defined(__SSE2__)
#   include <emmintrin.h>
#   if defined(__ICC)
#      define v4sf __m128
#      define v2df __m128d
#      define v2sf __m64
#      define v4si __m128i
#      define v2di __m128i
#      define v2si __m64
#   elif defined(__GNUC__)
#      define v4sf __v4sf
#      define v2df __v2df
#      define v2sf __v2sf
#      define v4si __v4si
#      define v2di __v2di
#      define v2si __v2si
#   endif
#elif defined(__SPU__)
#   include <spu_internals.h>	/* qword is defined here */
#endif


#if defined(__SSE2__)
/* 
 * Convenience types, meaningless in the context of SSE2, which
 * doesn't have unsigned types.
 */
typedef uint64_t  v2du __attribute__(( vector_size(16), aligned(16) ));
typedef uint32_t  v4su __attribute__(( vector_size(16), aligned(16) ));
typedef uint32_t  v2su __attribute__(( vector_size(8), aligned(8) ));

/* 
 * substitute for __m128i whole vector 
 */
typedef long long v1wi __attribute__(( vector_size(16), aligned(16), may_alias ));



#elif defined(__SPU__) || defined(__PPU__)
typedef vector unsigned char   v16qu;// __attribute__ (( vector_size (16), aligned(16) ));
typedef vector signed char     v16qi;// __attribute__ (( vector_size (16), aligned(16) ));
typedef vector unsigned short  v8hu;//  __attribute__ (( vector_size (16), aligned(16) ));
typedef vector signed short    v8hi;//  __attribute__ (( vector_size (16), aligned(16) ));
typedef vector unsigned int    v4su;//  __attribute__ (( vector_size (16), aligned(16) ));
typedef vector signed int      v4si;//  __attribute__ (( vector_size (16), aligned(16) ));
typedef vector float           v4sf;//  __attribute__ (( vector_size (16), aligned(16) ));

#if defined(__PPU__)
/* 
 * We won't want to use these PPU-only data types:
 *
 *	vector bool char          16 8-bit bools – 0 (false) 255 (true)
 *	vector bool short         8 16-bit bools – 0 (false) 65535 (true)
 *	vector bool int           4 32-bit bools – 0 (false) 2     – 1 (true)
 *	vector pixel              8 16-bit unsigned halfword, 1/5/5/5 pixel
 *
 */
//#warning Type 'v2df' (2-vector of double) cannot be defined on the PPU
//#warning Type 'v2du' (2-vector of uint64_t) cannot be defined on the PPU
//#warning Type 'v2di' (2-vector of int64_t) cannot be defined on the PPU
#endif /* defined(__PPU__) */

#if defined(__SPU__)
//typedef vector signed char qword;// __attribute__ (( vector_size (16), aligned(16) ));
typedef vector double             v2df;//  __attribute__ (( vector_size (16), aligned(16) ));
typedef vector unsigned long long v2du;//  __attribute__ (( vector_size (16), aligned(16) ));
typedef vector signed long long   v2di;//  __attribute__ (( vector_size (16), aligned(16) ));
#endif /* defined(__SPU__) */
#endif /* defined(__SPU__) || defined(__PPU__) */

/*
 * A unified 64-bit vector union.
 */
#if defined(__SSE2__)
typedef union {
    double   df;
    long int di;

//    v2sf  sf;
    v2si  si;
    v2su  su;

    int8_t    i8[8];
    int16_t  i16[4];
    int32_t  i32[2];

    uint8_t   u8[8];
    uint16_t u16[4];
    uint32_t u32[2];

    float      f[ 2];
} uv64_t;
#endif


/*
 * A unified 128-bit vector union.
 */
typedef union {
#if defined(__SSE2__)
    v1wi    wi;
#elif defined(__SPU__)
    qword   wi;
#endif
#if defined(__SSE2__) || defined(__SPU__)
    v2df  df;
    v2di  di;
//    v2du  du;
//#else
//#   warning 2x64-bit fields will be missing from uv_t
#endif
    v4sf  sf;
    v4si  si;
    v4su  su;

#if defined(__SSE2__)
    uv64_t half[2];
#endif

    int8_t    i8[16];
    int16_t  i16[ 8];
    int32_t  i32[ 4];
    int64_t  i64[ 2];

    uint8_t   u8[16];
    uint16_t u16[ 8];
    uint32_t u32[ 4];
    uint64_t u64[ 2];

    float      f[ 4];
    double     d[ 2];
} uv_t;


static const v4sf UV_1_4sf={1.0f, 1.0f, 1.0f, 1.0f};
static const v4sf UV_0_4sf={0.0f, 0.0f, 0.0f, 0.0f};
#if !defined(__PPU__)
static const v2df UV_0_2df={0.0, 0.0};
static const v2df UV_1_2df={1.0, 1.0};
//const v2di UV_1_2di={0x0000000000000001,0x0000000000000001};
//const v2di UV_f_2di={0xffffffffffffffff,0xffffffffffffffff};
#endif

#if defined(__ICC)
    static const v4si UV_0_4si={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    static const v4si UV_1_4si={0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00};
    static const v2di UV_1_2di={0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    static const v2di UV_f_2di={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
#elif defined(__GNUC__)
static const v4si UV_0_4si={0x00000000,0x00000000,0x00000000,0x00000000};
static const v4si UV_1_4si={0x00000001,0x00000001,0x00000001,0x00000001};
#   if !defined(__PPU__)
        static const v2di UV_1_2di={0x0000000000000001LL,0x0000000000000001LL};
        static const v2di UV_f_2di={0xffffffffffffffffLL,0xffffffffffffffffLL};
#   endif
#endif
    

#if defined(__SSE2__)
/*
 * Amazing omission from intel intrinsics:
 */
static inline v4sf uv_cvtdq2ps(v4si x) 
{
    asm("cvtdq2ps %0, %0" : "=x"(x) : "0"(x));
    return (v4sf)x;
}
#endif

#define uv_print_sf(S,X) printf(S": %10.7f %10.7f %10.7f %10.7f\n", X.f[0], X.f[1], X.f[2], X.f[3])
#define uv_print_se(S,X) printf(S": %10.7e %10.7e %10.7e %10.7e\n", X.f[0], X.f[1], X.f[2], X.f[3])
#define uv_print_si(S,X) printf(S": %d %d %d %d\n", X.i32[0], X.i32[1], X.i32[2], X.i32[3])
#define uv_print_su(S,X) printf(S": %u %u %u %u\n", X.u32[0], X.u32[1], X.u32[2], X.u32[3])
#define uv_print_sx(S,X) printf(S": %08x %08x %08x %08x\n", X.u32[0], X.u32[1], X.u32[2], X.u32[3])
#define uv_print_df(S,X) printf(S": %lg %lg\n", X.d[0], X.d[1])
#define uv_print_di(S,X) printf(S": %ld %ld\n", X.u64[0], X.u64[1])
#define uv_print_dx(S,X) printf(S": %016llx %016llx\n", X.u64[0], X.u64[1]);

#ifdef UV_DEBUG
#   define uv_dprint_sf(V,...) {					\
	fprintf(stderr, __VA_ARGS__);					\
	fprintf(stderr, ": %10.7f %10.7f %10.7f %10.7f\n", V.f[0], V.f[1], V.f[2], V.f[3]); \
    }
#   define uv_dprint_se(V,...) {					\
	fprintf(stderr, __VA_ARGS__);					\
	fprintf(stderr, ": %10.7e %10.7e %10.7e %10.7e\n", V.f[0], V.f[1], V.f[2], V.f[3]); \
    }
#   define uv_dprint_si(V,...) {					\
	fprintf(stderr, __VA_ARGS__);					\
	fprintf(stderr, ": %d %d %d %d\n", V.i32[0], V.i32[1], V.i32[2], V.i32[3]); \
    }
#   define uv_dprint_su(V,...) {					\
	fprintf(stderr, __VA_ARGS__);					\
	fprintf(stderr, ": %u %u %u %u\n", V.u32[0], V.u32[1], V.u32[2], V.u32[3]); \
    }
#   define uv_dprint_sx(V,...) {					\
	fprintf(stderr, __VA_ARGS__);					\
	fprintf(stderr, ": %08x %08x %08x %08x\n", V.u32[0], V.u32[1], V.u32[2], V.u32[3]); \
    }
#   define uv_dprint_df(V,...) {					\
	fprintf(stderr, __VA_ARGS__);					\
	fprintf(stderr, ": %lg %lg\n", V.d[0], V.d[1]);			\
    }
#   define uv_dprint_di(V,...) {					\
	fprintf(stderr, __VA_ARGS__);					\
	fprintf(stderr, ": %ld %ld\n", V.u64[0], V.u64[1]);		\
    }
#   define uv_dprint_dx(V,...) {					\
	fprintf(stderr, __VA_ARGS__);					\
	fprintf(stderr, ": %016llx %016llx\n", V.u64[0], V.u64[1]);	\
    }
#else
#   define uv_dprint_sf(...) /* NOP */ 
#   define uv_dprint_se(...) /* NOP */ 
#   define uv_dprint_si(...) /* NOP */ 
#   define uv_dprint_su(...) /* NOP */ 
#   define uv_dprint_sx(...) /* NOP */ 
#   define uv_dprint_df(...) /* NOP */ 
#   define uv_dprint_di(...) /* NOP */ 
#   define uv_dprint_dx(...) /* NOP */ 
#endif



#endif /* UNIVEC_H */
