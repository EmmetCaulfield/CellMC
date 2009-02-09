/** 
 * @file  rsmt-sse2.h
 * @brief SIMD oriented Fast Mersenne Twister(rsmt) for Intel SSE2
 *
 * @author Mutsuo Saito (Hiroshima University)
 * @author Makoto Matsumoto (Hiroshima University)
 *
 * @note We assume LITTLE ENDIAN in this file
 *
 * Copyright (C) 2006, 2007 Mutsuo Saito, Makoto Matsumoto and Hiroshima
 * University. All rights reserved.
 *
 * The new BSD License is applied to this software, see LICENSE.txt
 */

#ifndef RSMT_SSE2_H
#define RSMT_SSE2_H



static inline v1wi _rm_recursion(v1wi a, v1wi b, v1wi c,
				   v1wi d, v1wi mask);

/**
 * Recursion formula.
 *
 * @param a    a 128-bit part of the interal state array
 * @param b    a 128-bit part of the interal state array
 * @param c    a 128-bit part of the interal state array
 * @param d    a 128-bit part of the interal state array
 * @param mask 128-bit mask
 * @return output
 */
static inline v1wi _rm_recursion(v1wi x, v1wi y, 
				   v1wi z, v1wi v, v1wi mask) {
/*
    y = _mm_srli_epi32( y, RM_SR1 ); // I1
    z = _mm_srli_si128( z, RM_SR2 ); // I2
    v = _mm_slli_epi32( v, RM_SL1 ); // I3
    z = _mm_xor_si128 ( z,      x ); // I4
    z = _mm_xor_si128 ( z,      v ); // I5
    x = _mm_slli_si128( x, RM_SL2 ); // I6
    y = _mm_and_si128 ( y,   mask ); // I7
    z = _mm_xor_si128 ( z,      x ); // I8
    z = _mm_xor_si128 ( z,      y ); // I9

*/
    __asm__ __volatile__ (
	"psrld   %[SR1],   %[y]		\n\t" // y >>= RM_SR1	   I1
	"psrldq  %[SR2],   %[z]		\n\t" // z = c >> RM_SR2   I2
	"pslld   %[SL1],   %[v]		\n\t" // v = d << RM_SL1   I3
	"pxor      %[x],   %[z]		\n\t" // z ^= x		   I4
	"pxor      %[v],   %[z]		\n\t" // z ^= v		   I5
	"pslldq  %[SL2],   %[x]		\n\t" // x <<= RM_SL2      I6
	"pand      %[m],   %[y]		\n\t" // y &= mask	   I7
	"pxor      %[x],   %[z]		\n\t" // z ^= x		   I8
	"pxor      %[y],   %[z]		\n\t" // z ^= y		   I9

	: [x]"+x"(x), [y]"+x"(y), [z]"+x"(z), [v]"+x"(v)	// OUT
	: [SR1]"i"(RM_SR1), [SR2]"i"(RM_SR2), [SL1]"i"(RM_SL1), // IN
	  [SL2]"i"(RM_SL2), [m]"x"(mask)
    );

    return z;
}

/**
 * This function fills the internal state array with pseudorandom
 * integers.
 */
inline static void _gen_rand_all(rm_state_t *s) {
    int i;
    v1wi r, r1, r2, mask;
    mask = _mm_set_epi32(RM_MSK4, RM_MSK3, RM_MSK2, RM_MSK1);

    r1 = _mm_load_si128(&s->sfmt[RM_N - 2].wi);
    r2 = _mm_load_si128(&s->sfmt[RM_N - 1].wi);
    for (i = 0; i < RM_N - RM_POS1; i++) {
	r = _rm_recursion(s->sfmt[i].wi, s->sfmt[i + RM_POS1].wi, r1, r2, mask);
	_mm_store_si128(&s->sfmt[i].wi, r);
	r1 = r2;
	r2 = r;
    }
    for (; i < RM_N; i++) {
	r = _rm_recursion(s->sfmt[i].wi, s->sfmt[i + RM_POS1 - RM_N].wi, r1, r2, mask);
	_mm_store_si128(&s->sfmt[i].wi, r);
	r1 = r2;
	r2 = r;
    }
}

/**
 * This function fills the user-specified array with pseudorandom
 * integers.
 *
 * @param array an 128-bit array to be filled by pseudorandom numbers.  
 * @param size number of 128-bit pesudorandom numbers to be generated.
 */
inline static void _gen_rand_array(rm_state_t *s, uv_t *array, int size) {
    int i, j;
    v1wi r, r1, r2, mask;
    mask = _mm_set_epi32(RM_MSK4, RM_MSK3, RM_MSK2, RM_MSK1);

    r1 = _mm_load_si128(&s->sfmt[RM_N - 2].wi);
    r2 = _mm_load_si128(&s->sfmt[RM_N - 1].wi);
    for (i = 0; i < RM_N - RM_POS1; i++) {
	r = _rm_recursion(s->sfmt[i].wi, s->sfmt[i + RM_POS1].wi, r1, r2, mask);
	_mm_store_si128(&array[i].wi, r);
	r1 = r2;
	r2 = r;
    }
    for (; i < RM_N; i++) {
	r = _rm_recursion(s->sfmt[i].wi, array[i + RM_POS1 - RM_N].wi, r1, r2, mask);
	_mm_store_si128(&array[i].wi, r);
	r1 = r2;
	r2 = r;
    }
    /* main loop */
    for (; i < size - RM_N; i++) {
	r = _rm_recursion(array[i - RM_N].wi, array[i + RM_POS1 - RM_N].wi, r1, r2,
			 mask);
	_mm_store_si128(&array[i].wi, r);
	r1 = r2;
	r2 = r;
    }
    for (j = 0; j < 2 * RM_N - size; j++) {
	r = _mm_load_si128(&array[j + size - RM_N].wi);
	_mm_store_si128(&s->sfmt[j].wi, r);
    }
    for (; i < size; i++) {
	r = _rm_recursion(array[i - RM_N].wi, array[i + RM_POS1 - RM_N].wi, r1, r2,
			 mask);
	_mm_store_si128(&array[i].wi, r);
	_mm_store_si128(&s->sfmt[j++].wi, r);
	r1 = r2;
	r2 = r;
    }
}

#endif /* RSMT_SSE_H */

