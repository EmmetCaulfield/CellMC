#ifndef RSMT_PARAMS_H
#define RSMT_PARAMS_H

#if !defined(MEXP)
  #warning "MEXP is not defined. I assume MEXP is 19937."
  #define MEXP 19937
#endif

/*==================================================*\
 * BASIC DEFINITIONS
\*--------------------------------------------------*/
/**
 * Mersenne Exponent. The period of the sequence 
 *  is a multiple of 2^MEXP-1.
 */

/**
 * Size of internal state array when regarded as an array of 128-bit
 * integers.
 */
#define RM_N (MEXP / 128 + 1)

/**
 * Size of internal state array when regarded as an array of 32-bit
 * integers.
 */
#define RM_N32 (RM_N * 4)

/**
 * Size of internal state array when regarded as an array of 64-bit
 * integers.
 */
#define RM_N64 (RM_N * 2)

#if MEXP == 607
  #include "607.h"
#elif MEXP == 1279
  #include "1279.h"
#elif MEXP == 2281
  #include "2281.h"
#elif MEXP == 4253
  #include "4253.h"
#elif MEXP == 11213
  #include "11213.h"
#elif MEXP == 19937
  #include "19937.h"
#elif MEXP == 44497
  #include "44497.h"
#elif MEXP == 86243
  #include "86243.h"
#elif MEXP == 132049
  #include "132049.h"
#elif MEXP == 216091
  #include "216091.h"
#else
  #error "MEXP is not valid."
  #undef MEXP
#endif


//#define RM_IDSTR "RSMT-" MEXP ":" RM_POS1 "-" RM_SL1 "-" RM_SL2 "-" RM_SR1 "-" RM_SR2 ":" 


#endif /* RSMT_PARAMS_H */
