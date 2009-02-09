/** 
 * @file  rsmt.c
 * @brief Reentrant SIMDized Mersenne Twister
 *
 * @author Mutsuo Saito (Hiroshima University)
 * @author Makoto Matsumoto (Hiroshima University)
 * @author Emmet Caulfield
 *
 * Copyright (C) 2006,2007 Mutsuo Saito, Makoto Matsumoto and Hiroshima
 * University. All rights reserved.
 *
 * Copyright (C) 2008, Emmet Caulfield. All rights reserved.
 *
 * The new BSD License is applied to this software, see LICENSE.txt
 * 
 * $Id: rsmt.c 53 2009-01-26 03:13:46Z emmet $
 */
#ifndef RSMT_C
#define RSMT_C

#include <string.h>
#include <assert.h>
// #include <malloc.h>
#include "rsmt.h"

#if defined(__BIG_ENDIAN__) && !defined(__amd64) && !defined(BIG_ENDIAN64)
#define BIG_ENDIAN64 1
#endif
#if defined(HAVE_ALTIVEC) && !defined(BIG_ENDIAN64)
#define BIG_ENDIAN64 1
#endif
#if defined(ONLY64) && !defined(BIG_ENDIAN64)
  #if defined(__GNUC__)
    #error "-DONLY64 must be specified with -DBIG_ENDIAN64"
  #endif
#undef ONLY64
#endif


/*
 * The parity check vector is only ever read from, never written to,
 * so it can be made a global constant. The RM_PARITY macros are
 * defined in the respective params/nnnnn.h
 */
static const uint32_t const parity[4]={RM_PARITY1, RM_PARITY2, RM_PARITY3, RM_PARITY4};



/*==================================================*\
 * Static inline helper function prototypes
 *
 * These functions are not part of the public
 * interface and should not be called externally.
 *--------------------------------------------------*/
inline static void     _gen_rand_all(rm_state_t *s);
inline static void     _gen_rand_array(rm_state_t *s, uv_t *array, int size);
static void            _period_certification(rm_state_t *s);
inline static int      _idxof(int i);
inline static void     _rshift128(uv_t *out,  uv_t const *in, int shift);
inline static void     _lshift128(uv_t *out,  uv_t const *in, int shift);
inline static uint32_t _func1(uint32_t x);
inline static uint32_t _func2(uint32_t x);
#if (!defined(HAVE_ALTIVEC)) && (!defined(HAVE_SSE2))
inline static void     _do_recursion(uv_t *r, uv_t *a, uv_t *b, uv_t *c, uv_t *d);
#endif

#if defined(BIG_ENDIAN64) && !defined(ONLY64)
inline static void _swap(uv_t *array, int size);
#endif

#if defined(HAVE_ALTIVEC)
  #include "rsmt-alti.h"
#elif defined(HAVE_SSE2)
  #include <emmintrin.h>
  #include "rsmt-sse2.h"
#endif

/**
 * simulates a 64-bit index of LITTLE ENDIAN in BIG ENDIAN machine.
 */
#ifdef ONLY64
inline static int _idxof(int i) {
    return i ^ 1;
}
#else
inline static int _idxof(int i) {
    return i;
}
#endif


/**
 * This function simulates SIMD 128-bit right shift by the standard C.
 * The 128-bit integer given in in is shifted by (shift * 8) bits.
 * This function simulates the LITTLE ENDIAN SIMD.
 * @param out the output of this function
 * @param in the 128-bit data to be shifted
 * @param shift the shift value
 */
#ifdef ONLY64
inline static void _rshift128(uv_t *out, uv_t const *in, int shift) {
    uint64_t th, tl, oh, ol;

    th = ((uint64_t)in->u32[2] << 32) | ((uint64_t)in->u32[3]);
    tl = ((uint64_t)in->u32[0] << 32) | ((uint64_t)in->u32[1]);

    oh = th >> (shift * 8);
    ol = tl >> (shift * 8);
    ol |= th << (64 - shift * 8);
    out->u32[0] = (uint32_t)(ol >> 32);
    out->u32[1] = (uint32_t)ol;
    out->u32[2] = (uint32_t)(oh >> 32);
    out->u32[3] = (uint32_t)oh;
}
#else
inline static void _rshift128(uv_t *out, uv_t const *in, int shift) {
    uint64_t th, tl, oh, ol;

    th = ((uint64_t)in->u32[3] << 32) | ((uint64_t)in->u32[2]);
    tl = ((uint64_t)in->u32[1] << 32) | ((uint64_t)in->u32[0]);

    oh = th >> (shift * 8);
    ol = tl >> (shift * 8);
    ol |= th << (64 - shift * 8);
    out->u32[1] = (uint32_t)(ol >> 32);
    out->u32[0] = (uint32_t)ol;
    out->u32[3] = (uint32_t)(oh >> 32);
    out->u32[2] = (uint32_t)oh;
}
#endif
/**
 * This function simulates SIMD 128-bit left shift by the standard C.
 * The 128-bit integer given in in is shifted by (shift * 8) bits.
 * This function simulates the LITTLE ENDIAN SIMD.
 * @param out the output of this function
 * @param in the 128-bit data to be shifted
 * @param shift the shift value
 */
#ifdef ONLY64
inline static void _lshift128(uv_t *out, uv_t const *in, int shift) {
    uint64_t th, tl, oh, ol;

    th = ((uint64_t)in->u32[2] << 32) | ((uint64_t)in->u32[3]);
    tl = ((uint64_t)in->u32[0] << 32) | ((uint64_t)in->u32[1]);

    oh = th << (shift * 8);
    ol = tl << (shift * 8);
    oh |= tl >> (64 - shift * 8);
    out->u32[0] = (uint32_t)(ol >> 32);
    out->u32[1] = (uint32_t)ol;
    out->u32[2] = (uint32_t)(oh >> 32);
    out->u32[3] = (uint32_t)oh;
}
#else
inline static void _lshift128(uv_t *out, uv_t const *in, int shift) {
    uint64_t th, tl, oh, ol;

    th = ((uint64_t)in->u32[3] << 32) | ((uint64_t)in->u32[2]);
    tl = ((uint64_t)in->u32[1] << 32) | ((uint64_t)in->u32[0]);

    oh = th << (shift * 8);
    ol = tl << (shift * 8);
    oh |= tl >> (64 - shift * 8);
    out->u32[1] = (uint32_t)(ol >> 32);
    out->u32[0] = (uint32_t)ol;
    out->u32[3] = (uint32_t)(oh >> 32);
    out->u32[2] = (uint32_t)oh;
}
#endif

/**
 * Recursion formula.
 *
 * @param r output
 * @param a a 128-bit part of the internal state array
 * @param b a 128-bit part of the internal state array
 * @param c a 128-bit part of the internal state array
 * @param d a 128-bit part of the internal state array
 */
#if (!defined(HAVE_ALTIVEC)) && (!defined(HAVE_SSE2))
#ifdef ONLY64
inline static void _do_recursion(uv_t *r, uv_t *a, uv_t *b, uv_t *c,
				uv_t *d) {
    uv_t x;
    uv_t y;

    _lshift128(&x, a, RM_SL2);
    _rshift128(&y, c, RM_SR2);
    r->u32[0] = a->u32[0] ^ x.u32[0] ^ ((b->u32[0] >> RM_SR1) & RM_MSK2) ^ y.u32[0] 
	^ (d->u32[0] << RM_SL1);
    r->u32[1] = a->u32[1] ^ x.u32[1] ^ ((b->u32[1] >> RM_SR1) & RM_MSK1) ^ y.u32[1] 
	^ (d->u32[1] << RM_SL1);
    r->u32[2] = a->u32[2] ^ x.u32[2] ^ ((b->u32[2] >> RM_SR1) & RM_MSK4) ^ y.u32[2] 
	^ (d->u32[2] << RM_SL1);
    r->u32[3] = a->u32[3] ^ x.u32[3] ^ ((b->u32[3] >> RM_SR1) & RM_MSK3) ^ y.u32[3] 
	^ (d->u32[3] << RM_SL1);
}
#else
inline static void _do_recursion(uv_t *r, uv_t *a, uv_t *b, uv_t *c,
				uv_t *d) {
    uv_t x;
    uv_t y;

    _lshift128(&x, a, RM_SL2);
    _rshift128(&y, c, RM_SR2);
    r->u32[0] = a->u32[0] ^ x.u32[0] ^ ((b->u32[0] >> RM_SR1) & RM_MSK1) ^ y.u32[0] 
	^ (d->u32[0] << RM_SL1);
    r->u32[1] = a->u32[1] ^ x.u32[1] ^ ((b->u32[1] >> RM_SR1) & RM_MSK2) ^ y.u32[1] 
	^ (d->u32[1] << RM_SL1);
    r->u32[2] = a->u32[2] ^ x.u32[2] ^ ((b->u32[2] >> RM_SR1) & RM_MSK3) ^ y.u32[2] 
	^ (d->u32[2] << RM_SL1);
    r->u32[3] = a->u32[3] ^ x.u32[3] ^ ((b->u32[3] >> RM_SR1) & RM_MSK4) ^ y.u32[3] 
	^ (d->u32[3] << RM_SL1);
}
#endif
#endif

#if (!defined(HAVE_ALTIVEC)) && (!defined(HAVE_SSE2))
/**
 * This function fills the internal state array with pseudorandom
 * integers.
 */
inline static void _gen_rand_all(rm_state_t *s) {
    int i;
    uv_t *r1, *r2;

    assert(NULL!=s);

    r1 = &(s->sfmt[RM_N - 2]);
    r2 = &(s->sfmt[RM_N - 1]);
    for (i = 0; i < RM_N - RM_POS1; i++) {
	_do_recursion(&(s->sfmt[i]), &(s->sfmt[i]), &(s->sfmt[i + RM_POS1]), r1, r2);
	r1 = r2;
	r2 = &(s->sfmt[i]);
    }
    for (; i < RM_N; i++) {
	_do_recursion(&(s->sfmt[i]), &(s->sfmt[i]), &(s->sfmt[i + RM_POS1 - RM_N]), r1, r2);
	r1 = r2;
	r2 = &(s->sfmt[i]);
    }
}

/**
 * This function fills the user-specified array with pseudorandom
 * integers.
 *
 * @param array an 128-bit array to be filled by pseudorandom numbers.  
 * @param size number of 128-bit pseudorandom numbers to be generated.
 */
inline static void _gen_rand_array(rm_state_t *s, uv_t *array, int size) {
    int i, j;
    uv_t *r1, *r2;

    assert(NULL!=s);

    r1 = &(s->sfmt[RM_N - 2]);
    r2 = &(s->sfmt[RM_N - 1]);
    for (i = 0; i < RM_N - RM_POS1; i++) {
	_do_recursion(&array[i], &(s->sfmt[i]), &(s->sfmt[i + RM_POS1]), r1, r2);
	r1 = r2;
	r2 = &array[i];
    }
    for (; i < RM_N; i++) {
	_do_recursion(&array[i], &(s->sfmt[i]), &array[i + RM_POS1 - RM_N], r1, r2);
	r1 = r2;
	r2 = &array[i];
    }
    for (; i < size - RM_N; i++) {
	_do_recursion(&array[i], &array[i - RM_N], &array[i + RM_POS1 - RM_N], r1, r2);
	r1 = r2;
	r2 = &array[i];
    }
    for (j = 0; j < 2 * RM_N - size; j++) {
	s->sfmt[j] = array[j + size - RM_N];
    }
    for (; i < size; i++, j++) {
	_do_recursion(&array[i], &array[i - RM_N], &array[i + RM_POS1 - RM_N], r1, r2);
	r1 = r2;
	r2 = &array[i];
	s->sfmt[j] = array[i];
    }
}
#endif



#if defined(BIG_ENDIAN64) && !defined(ONLY64) && !defined(HAVE_ALTIVEC)
inline static void _swap(uv_t *array, const int size) {
    int i;
    uint32_t x, y;

    for (i = 0; i < size; i++) {
	x = array[i].u32[0];
	y = array[i].u32[2];
	array[i].u32[0] = array[i].u32[1];
	array[i].u32[2] = array[i].u32[3];
	array[i].u32[1] = x;
	array[i].u32[3] = y;
    }
}
#endif
/**
 * used in the initialization by rm_init_array
 *
 * @param x 32-bit integer
 * @return 32-bit integer
 */
static uint32_t _func1(const uint32_t x) {
    return (x ^ (x >> 27)) * (uint32_t)1664525UL;
}

/**
 * used in the initialization by rm_init_array
 *
 * @param x 32-bit integer
 * @return 32-bit integer
 */
static uint32_t _func2(const uint32_t x) {
    return (x ^ (x >> 27)) * (uint32_t)1566083941UL;
}

/**
 * This function certificate the period of 2^{MEXP}
 */
static void _period_certification(rm_state_t *s) {
    int inner = 0;
    int i, j;
    uint32_t work;

    assert(NULL!=s);

    for (i = 0; i < 4; i++)
	inner ^= s->p32[_idxof(i)] & parity[i];
    for (i = 16; i > 0; i >>= 1)
	inner ^= inner >> i;
    inner &= 1;
    /* check OK */
    if (inner == 1) {
	return;
    }
    /* check NG, and modification */
    for (i = 0; i < 4; i++) {
	work = 1;
	for (j = 0; j < 32; j++) {
	    if ((work & parity[i]) != 0) {
		s->p32[_idxof(i)] ^= work;
		return;
	    }
	    work = work << 1;
	}
    }
}

/*----------------
  PUBLIC FUNCTIONS
  ----------------*/
/**
 * This function returns the identification string.
 * The string shows the word size, the Mersenne exponent,
 * and all parameters of this generator.
 */
const char *rm_idstring(void) {
    return RM_IDSTR;
}

/**
 * This function returns the minimum size of array used for \b
 * fill_array32() function.
 * @return minimum size of array used for fill_array32() function.
 */
int rm_min_array_size32(void) {
    return RM_N32;
}

/**
 * This function returns the minimum size of array used for \b
 * fill_array64() function.
 * @return minimum size of array used for fill_array64() function.
 */
int rm_min_array_size64(void) {
    return RM_N64;
}

#ifndef ONLY64
/**
 * generates and returns 32-bit pseudorandom number.
 *
 * rm_init or rm_init_array must be called before this function.
 *
 * @return 32-bit pseudorandom number
 */
uint32_t rm_rand32(rm_state_t *s) {
    uint32_t r;

    assert(NULL!=s);
    assert(s->initialized);

    if (s->idx >= RM_N32) {
	_gen_rand_all(s);
	s->idx = 0;
    }
//    s->idx++;
    r = s->p32[s->idx];
    s->idx++;
    return r;
}
#endif


static inline v4si rm_rand_v4si(rm_state_t *s) {
    v4si r;

    assert(NULL!=s);
    assert(s->initialized);

    if (s->idx >= RM_N32) {
	_gen_rand_all(s);
	s->idx = 0;
    }

    r = s->sfmt[s->idx/4].si;
    s->idx+=4;
    return r;
}


/**
 * This function generates and returns 64-bit pseudorandom number.
 * rm_init() or rm_init_array() must be called before this function.
 * The function rm_rand64() should not be called after rm_rand32(),
 * without re-initializing.
 *
 * @return 64-bit pseudorandom number
 */
uint64_t rm_rand64(rm_state_t *s) {
#if defined(BIG_ENDIAN64) && !defined(ONLY64)
    uint32_t r1, r2;
#else
    uint64_t r;
#endif

    assert(NULL!=s);
    assert(s->initialized);
    assert(s->idx % 2 == 0);

    if (s->idx >= RM_N32) {
	_gen_rand_all(s);
	s->idx = 0;
    }
#if defined(BIG_ENDIAN64) && !defined(ONLY64)
    r1 = s->p32[s->idx];
    r2 = s->p32[s->idx + 1];
    s->idx += 2;
    return ((uint64_t)r2 << 32) | r1;
#else
    r = s->p64[s->idx / 2];
    s->idx += 2;
    return r;
#endif
}

#ifndef ONLY64

/**
 * fills 'array[]' with 'size' 32-bit unsigned integers in one call.
 *
 * 'size' must be at least 624 and a multiple of four. This function
 * is much faster than rm_rand() for the same number of numbers.
 *
 * Initialization: rm_init() or rm_init_array() must be called before
 * the first call to rm_rand32_array(); it must not be called after
 * calling rm_rand() without an intervening initialization call.
 *
 * @param array an array where pseudorandom 32-bit integers are filled
 * by this function.  The pointer to the array must be \b "aligned"
 * (namely, must be a multiple of 16) in the SIMD version, since it
 * refers to the address of a 128-bit integer.  In the standard C
 * version, the pointer is arbitrary.
 *
 * @param size the number of 32-bit pseudorandom integers to be
 * generated.  size must be a multiple of 4, and greater than or equal
 * to (MEXP / 128 + 1) * 4.
 *
 * @note \b memalign or \b posix_memalign is available to get aligned
 * memory. Mac OSX doesn't have these functions, but \b malloc of OSX
 * returns the pointer to the aligned memory block.
 */
void rm_rand32_array(rm_state_t *s, uint32_t *array, int size) {
    assert(NULL!=s);
    assert(s->initialized);
    assert(s->idx == RM_N32);
    assert(size % 4 == 0);
    assert(size >= RM_N32);

    _gen_rand_array(s, (uv_t *)array, size / 4);
    s->idx = RM_N32;
}
#endif


/**
 * This function generates pseudorandom 64-bit integers in the
 * specified array[] by one call. The number of pseudorandom integers
 * is specified by the argument size, which must be at least 312 and a
 * multiple of two.  The generation by this function is much faster
 * than the following rm_rand function.
 *
 * For initialization, init_gen_rand or init_by_array must be called
 * before the first call of this function. This function can not be
 * used after calling rm_rand function, without initialization.
 *
 * @param array an array where pseudorandom 64-bit integers are filled
 * by this function.  The pointer to the array must be "aligned"
 * (namely, must be a multiple of 16) in the SIMD version, since it
 * refers to the address of a 128-bit integer.  In the standard C
 * version, the pointer is arbitrary.
 *
 * @param size the number of 64-bit pseudorandom integers to be
 * generated.  size must be a multiple of 2, and greater than or equal
 * to (MEXP / 128 + 1) * 2
 *
 * @note \b memalign or \b posix_memalign is available to get aligned
 * memory. Mac OSX doesn't have these functions, but \b malloc of OSX
 * returns the pointer to the aligned memory block.
 */
void rm_rand64_array(rm_state_t *s, uint64_t *array, int size) {
    assert(NULL!=s);
    assert(s->initialized);
    assert(s->idx == RM_N32);
    assert(size % 2 == 0);
    assert(size >= RM_N64);

    _gen_rand_array(s,(uv_t *)array, size / 2);
    s->idx = RM_N32;

#if defined(BIG_ENDIAN64) && !defined(ONLY64)
    _swap((uv_t *)array, size /2);
#endif
}

/**
 * This function initializes the internal state array with a 32-bit
 * integer seed.
 *
 * @param  s     a pointer to a rm_state_t struct
 * @param  seed  a 32-bit integer used as the seed.
 *
 * @note   To avoid creating a memory-mangement contract with the
 *         caller, the caller must supply a pointer to a rm_state_t
 *         struct, allocating aligned memory if necessary, and
 *         deallocating as necessary.
 */
void rm_init(rm_state_t *s, uint32_t seed) {
    int i;

    assert(NULL!=s);
//    s->p32=(uint32_t *)s->sfmt;
//    s->p64=(uint64_t *)s->sfmt;

    s->p32[_idxof(0)] = seed;
    for (i = 1; i < RM_N32; i++) {
	s->p32[_idxof(i)] = 1812433253UL * (s->p32[_idxof(i - 1)] 
				       ^ (s->p32[_idxof(i - 1)] >> 30))
	    + i;
    }
    s->idx = RM_N32;
    _period_certification(s);
    s->initialized=1;
}

/**
 * This function initializes a state struct with an array
 * of 32-bit integers used as the seeds.
 *
 * @param s          the state struct
 * @param init_key   the array of 32-bit integers, used as a seed.
 * @param key_length the length of init_key.
 *
 * @note   To avoid creating a memory-mangement contract with the
 *         caller, the caller must supply a pointer to a rm_state_t
 *         struct, allocating aligned memory as necessary.
 */
void rm_init_array(rm_state_t *s, uint32_t *init_key, int key_length) {
    int i, j, count;
    uint32_t r;
    int lag;
    int mid;
    int size = RM_N * 4;

    assert(NULL!=s);
//    s->p32=(uint32_t *)s->sfmt;
//    s->p64=(uint64_t *)s->sfmt;

    if (size >= 623) {
	lag = 11;
    } else if (size >= 68) {
	lag = 7;
    } else if (size >= 39) {
	lag = 5;
    } else {
	lag = 3;
    }
    mid = (size - lag) / 2;

    memset(s->sfmt, 0x8b, 16*RM_N);
    if (key_length + 1 > RM_N32) {
	count = key_length + 1;
    } else {
	count = RM_N32;
    }
    r = _func1(s->p32[_idxof(0)] ^ s->p32[_idxof(mid)] 
	      ^ s->p32[_idxof(RM_N32 - 1)]);
    s->p32[_idxof(mid)] += r;
    r += key_length;
    s->p32[_idxof(mid + lag)] += r;
    s->p32[_idxof(0)] = r;

    count--;
    for (i = 1, j = 0; (j < count) && (j < key_length); j++) {
	r = _func1(s->p32[_idxof(i)] ^ s->p32[_idxof((i + mid) % RM_N32)] 
		  ^ s->p32[_idxof((i + RM_N32 - 1) % RM_N32)]);
	s->p32[_idxof((i + mid) % RM_N32)] += r;
	r += init_key[j] + i;
	s->p32[_idxof((i + mid + lag) % RM_N32)] += r;
	s->p32[_idxof(i)] = r;
	i = (i + 1) % RM_N32;
    }
    for (; j < count; j++) {
	r = _func1(s->p32[_idxof(i)] ^ s->p32[_idxof((i + mid) % RM_N32)] 
		  ^ s->p32[_idxof((i + RM_N32 - 1) % RM_N32)]);
	s->p32[_idxof((i + mid) % RM_N32)] += r;
	r += i;
	s->p32[_idxof((i + mid + lag) % RM_N32)] += r;
	s->p32[_idxof(i)] = r;
	i = (i + 1) % RM_N32;
    }
    for (j = 0; j < RM_N32; j++) {
	r = _func2(s->p32[_idxof(i)] + s->p32[_idxof((i + mid) % RM_N32)] 
		  + s->p32[_idxof((i + RM_N32 - 1) % RM_N32)]);
	s->p32[_idxof((i + mid) % RM_N32)] ^= r;
	r -= i;
	s->p32[_idxof((i + mid + lag) % RM_N32)] ^= r;
	s->p32[_idxof(i)] = r;
	i = (i + 1) % RM_N32;
    }

    s->idx = RM_N32;
    _period_certification(s);
    s->initialized=1;
}

#endif /* RSMT_C */
