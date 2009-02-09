/**
 * @file  test.c
 * @brief test program for 32-bit and 64-bit output of SFMT.
 *
 * @author Mutsuo Saito (Hiroshima-univ)
 *
 * Copyright (C) 2007 Mutsuo Saito, Makoto Matsumoto and Hiroshima
 * University. All rights reserved.
 *
 * The new BSD License is applied to this software, see LICENSE.txt
 */

#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "rsmt.c"

#define BLOCK_SIZE 100000
#define BLOCK_SIZE64 50000
#define COUNT 1000

void check32(void);
void speed32(void);
void check64(void);
void speed64(void);

#if defined(HAVE_ALTIVEC)
static vector unsigned int array1[BLOCK_SIZE / 4];
static vector unsigned int array2[10000 / 4];
#elif defined(HAVE_SSE2)
static __m128i array1[BLOCK_SIZE / 4];
static __m128i array2[10000 / 4];
#else
static uint64_t array1[BLOCK_SIZE / 4][2];
static uint64_t array2[10000 / 4][2];
#endif

#define MISMATCH(A,I,V) {					\
	fprintf(stderr, "Mismatch at %s:%d:%s() %s[%d]:%x != %s:%x\n",	\
		__FILE__,__LINE__,__func__,				\
		#A, I, A[I], #V, V );					\
	exit(1);							\
}
#define MISMATCH64(A,I,V) {						\
	fprintf(stderr, "Mismatch at %s:%d%s() %s[%d]:%lx != %s:%lx\n",	\
		__FILE__,__LINE__,__func__,				\
		#A, I, A[I], #V, V );					\
	exit(1);							\
}


#ifndef ONLY64
void check32(void) {
    rm_state_t s;
    int i;
    uint32_t *array32 = (uint32_t *)array1;
    uint32_t *array32_2 = (uint32_t *)array2;
    uint32_t ini[4] = {0x1234, 0x5678, 0x9abc, 0xdef0};
    uint32_t r32;

    if (rm_min_array_size32() > 10000) {
	printf("array size too small!\n");
	exit(1);
    }
    printf("%s\n32 bit generated randoms\n", rm_idstring());
    printf("rm_init________________\n");
    /* 32 bit generation */
    rm_init(&s,1234);
    rm_rand32_array(&s,array32, 10000);
    rm_rand32_array(&s,array32_2, 10000);
    rm_init(&s,1234);
    for (i = 0; i < 10000; i++) {
	if (i < 1000) {
	    printf("%10u ", array32[i]);
	    if (i % 5 == 4) {
		printf("\n");
	    }
	}
	r32 = rm_rand32(&s);
	if (r32 != array32[i])
	    MISMATCH(array32, i, r32);
    }
    for (i = 0; i < 700; i++) {
	r32 = rm_rand32(&s);
	if (r32 != array32_2[i])
	    MISMATCH(array32_2, i, r32);
    }
    printf("\n");
    rm_init_array(&s,ini, 4);
    printf("rm_init_array__________\n");
    rm_rand32_array(&s,array32, 10000);
    rm_rand32_array(&s,array32_2, 10000);
    rm_init_array(&s,ini, 4);
    for (i = 0; i < 10000; i++) {
	if (i < 1000) {
	    printf("%10u ", array32[i]);
	    if (i % 5 == 4) {
		printf("\n");
	    }
	}
	r32 = rm_rand32(&s);
	if (r32 != array32[i])
	    MISMATCH(array32, i, r32);
    }

    for (i = 0; i < 700; i++) {
	r32 = rm_rand32(&s);
	if (r32 != array32_2[i])
	    MISMATCH(array32_2, i, r32);
    }
}

void speed32(void) {
    rm_state_t s;
    int i, j;
    clock_t clo;
    clock_t min = LONG_MAX;
    uint32_t *array32 = (uint32_t *)array1;

    if (rm_min_array_size32() > BLOCK_SIZE) {
	printf("array size too small!\n");
	exit(1);
    }
    /* 32 bit generation */
    rm_init(&s,1234);
    for (i = 0; i < 10; i++) {
	clo = clock();
	for (j = 0; j < COUNT; j++) {
	    rm_rand32_array(&s, array32, BLOCK_SIZE);
	}
	clo = clock() - clo;
	if (clo < min) {
	    min = clo;
	}
    }
    printf("32 bit BLOCK:%.0f", (double)min * 1000/ CLOCKS_PER_SEC);
    printf("ms for %u randoms generation\n",
	   BLOCK_SIZE * COUNT);
    min = LONG_MAX;
    rm_init(&s,1234);
    for (i = 0; i < 10; i++) {
	clo = clock();
	for (j = 0; j < BLOCK_SIZE * COUNT; j++) {
	    rm_rand32(&s);
	}
	clo = clock() - clo;
	if (clo < min) {
	    min = clo;
	}
    }
    printf("32 bit SEQUE:%.0f", (double)min * 1000 / CLOCKS_PER_SEC);
    printf("ms for %u randoms generation\n",
	   BLOCK_SIZE * COUNT);
}
#endif

void check64(void) {
    rm_state_t s;
    int i;
    uint64_t *array64;
    uint64_t *array64_2;
    uint64_t r;
    uint32_t ini[] = {5, 4, 3, 2, 1};

    array64 = (uint64_t *)array1;
    array64_2 = (uint64_t *)array2;
    if (rm_min_array_size64() > 5000) {
	printf("array size too small!\n");
	exit(1);
    }
    printf("%s\n64 bit generated randoms\n", rm_idstring());
    printf("rm_init________________\n");
    /* 64 bit generation */
    rm_init(&s,4321);
    rm_rand64_array(&s,array64, 5000);
    rm_rand64_array(&s,array64_2, 5000);
    rm_init(&s,4321);
    for (i = 0; i < 5000; i++) {
	if (i < 1000) {
	    printf("%20lu", array64[i]);
	    if (i % 3 == 2) {
		printf("\n");
	    }
	}
	r = rm_rand64(&s);
	if (r != array64[i])
	    MISMATCH64(array64, i, r);
    }
    printf("\n");
    for (i = 0; i < 700; i++) {
	r = rm_rand64(&s);
	if (r != array64_2[i])
	    MISMATCH64(array64_2, i, r);
    }
    printf("rm_init_array__________\n");
    /* 64 bit generation */
    rm_init_array(&s, ini, 5);
    rm_rand64_array(&s, array64, 5000);
    rm_rand64_array(&s, array64_2, 5000);
    rm_init_array(&s, ini, 5);
    for (i = 0; i < 5000; i++) {
	if (i < 1000) {
	    printf("%20lu ", array64[i]);
	    if (i % 3 == 2) {
		printf("\n");
	    }
	}
	r = rm_rand64(&s);
	if (r != array64[i])
	    MISMATCH64(array64, i, r);
    }
    printf("\n");
    for (i = 0; i < 700; i++) {
	r = rm_rand64(&s);
	if (r != array64_2[i])
	    MISMATCH64(array64_2, i, r);
    }
}

void speed64(void) {
    rm_state_t s;
    int i, j;
    uint64_t clo;
    uint64_t min = LONG_MAX;
    uint64_t *array64 = (uint64_t *)array1;

    if (rm_min_array_size64() > BLOCK_SIZE64) {
	printf("array size too small!\n");
	exit(1);
    }
    /* 64 bit generation */
    rm_init(&s, 1234);
    for (i = 0; i < 10; i++) {
	clo = clock();
	for (j = 0; j < COUNT; j++) {
	    rm_rand64_array(&s, array64, BLOCK_SIZE64);
	}
	clo = clock() - clo;
	if (clo < min) {
	    min = clo;
	}
    }
    printf("64 bit BLOCK:%.0f", (double)min * 1000/ CLOCKS_PER_SEC);
    printf("ms for %u randoms generation\n",
	   BLOCK_SIZE64 * COUNT);
    min = LONG_MAX;
    rm_init(&s,1234);
    for (i = 0; i < 10; i++) {
	clo = clock();
	for (j = 0; j < BLOCK_SIZE64 * COUNT; j++) {
	    rm_rand64(&s);
	}
	clo = clock() - clo;
	if (clo < min) {
	    min = clo;
	}
    }
    printf("64 bit SEQUE:%.0f", (double)min * 1000 / CLOCKS_PER_SEC);
    printf("ms for %u randoms generation\n",
	   BLOCK_SIZE64 * COUNT);
}

int main(int argc, char *argv[]) {
    int i;
    int speed = 0;
    int bit64 = 0;
#ifndef ONLY64
    int bit32 = 0;
#endif

    for (i = 1; i < argc; i++) {
	if (strncmp(argv[1],"-s", 2) == 0) {
	    speed = 1;
	}
	if (strncmp(argv[1],"-b64", 4) == 0) {
	    bit64 = 1;
	}
#ifndef ONLY64
	if (strncmp(argv[1],"-b32", 4) == 0) {
	    bit32 = 1;
	}
#endif
    }
#ifdef ONLY64
    if (speed + bit64 == 0) {
	printf("usage:\n%s [-s | -b64]\n", argv[0]);
	return 0;
    }
#else
    if (speed + bit32 + bit64 == 0) {
	printf("usage:\n%s [-s | -b32 | -b64]\n", argv[0]);
	return 0;
    }
#endif
    if (speed) {
#ifndef ONLY64
	speed32();
#endif
	speed64();
    }
#ifndef ONLY64
    if (bit32) {
	check32();
    }
#endif
    if (bit64) {
	check64();
    }
    return 0;
}
