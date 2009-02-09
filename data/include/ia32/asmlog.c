#ifndef ASMLOG_C
#define ASMLOG_C

#include "univec.h"
/*
 * This implementation is largely stolen from IBM's simdmath.h files
 * for Cell/BE.
 */
/**
 * computes log (base 2) of a vector of single-precision floats.
 *
 * The log2 is approximated as a polynomial of order 8 (C. Hastings,
 * Jr, 1955) according to:
 *
 *                    8
 *	log2f(1+x) = Sum( Ci*x^i )
 *                   i=1
 *
 * for 0.0 < x <= 1.0
 *
 * @param   x  A 16-byte aligned 4-vector of floats (SSE2 v4sf)
 * @returns    The log (base 2) of x
 */
#define C1 (  1.4426898816672f    )
#define	C2 ( -0.72116591947498f   )
#define C3 (  0.47868480909345f   )
#define C4 ( -0.34730547155299f   )
#define C5 (  0.24187369696082f   )
#define C6 ( -0.13753123777116f   )
#define C7 (  0.052064690894143f  )
#define C8 ( -0.0093104962134977f )

#define C1v ((v4sf){C1,C1,C1,C1})
#define C2v ((v4sf){C2,C2,C2,C2})
#define C3v ((v4sf){C3,C3,C3,C3})
#define C4v ((v4sf){C4,C4,C4,C4})
#define C5v ((v4sf){C5,C5,C5,C5})
#define C6v ((v4sf){C6,C6,C6,C6})
#define C7v ((v4sf){C7,C7,C7,C7})
#define C8v ((v4sf){C8,C8,C8,C8})


/*
 * IEEE-754 SPFP exponent bias
 */
#define EXP_BIAS ((v4si){127,127,127,127})

/*
 * IEEE-754 exponent mask 0111 1111 1000...
 */
#define EXP_MASK ((v4si){0x7F800000,0x7F800000,0x7F800000,0x7F800000})

/*
 * IEEE-754 significand length
 */
#define MANT_LEN (23)

/*
 * Log base 2 of e
 */
#define LN2S (0.693147180559945f)
#define LN2V ((v4sf){LN2S,LN2S,LN2S,LN2S})

/*
 * Constant array to make it easy to load coefficients at runtime and
 * avoid requiring too many SSE registers.
 */
static const v4sf coeff[9] = {
    LN2V,	/*     (c) */
    C1v,	/* 0x10(c) */
    C2v,	/* 0x20(c) */
    C3v,	/* 0x30(c) */
    C4v,	/* 0x40(c) */
    C5v,        /* 0x50(c) */
    C6v,	/* 0x60(c) */
    C7v,	/* 0x70(c) */
    C8v,	/* 0x80(c) */
};




static inline 
v4sf _asm_logf4(v4sf x)
{
    v4sf y;

    asm (
	"movaps	        %[x],       %[y]    \n\t" // Copy x, use y as temp.
	"andps         %[ma],       %[y]    \n\t" // Extract exponent
	"psrld	      %[mln],	    %[y]    \n\t" // Shift all the way right	
	"psubd 	       %[bi],       %[y]    \n\t" // Remove bias
	"movaps         %[y],     %%xmm7    \n\t" // Save exponent for later
	"pslld        %[mln],       %[y]    \n\t" // Shift back
        "psubd          %[y],       %[x]    \n\t" // Calculate remainder
	"subps        %[one],	    %[x]    \n\t" // Subtract ones from X
	// Summation
	"movaps         %[x],       %[y]    \n\t" // y = x*c8+c7
	"mulps    0x80(%[c]),	    %[y]    \n\t"
	"addps    0x70(%[c]),       %[y]    \n\t"
	"mulps          %[x],       %[y]    \n\t" // y = x*y+c6
	"addps    0x60(%[c]),       %[y]    \n\t"
	"mulps          %[x],       %[y]    \n\t" // y = x*y+c5 
	"addps    0x50(%[c]),       %[y]    \n\t"
	"mulps          %[x],       %[y]    \n\t" // y = x*y+c4 
	"addps    0x40(%[c]),       %[y]    \n\t"
	"mulps          %[x],       %[y]    \n\t" // y = x*y+c3 
	"addps    0x30(%[c]),       %[y]    \n\t"
	"mulps          %[x],       %[y]    \n\t" // y = x*y+c2 
	"addps    0x20(%[c]),       %[y]    \n\t"
	"mulps          %[x],       %[y]    \n\t" // y = x*y+c1 
	"addps    0x10(%[c]),       %[y]    \n\t"
	"mulps          %[x],       %[y]    \n\t" // y = x*y
	"cvtdq2ps     %%xmm7,     %%xmm7    \n\t" // Convert exponent
	"addps        %%xmm7,       %[y]    \n\t" // Add onto result
	"mulps        (%[c]),	    %[y]    \n\t"
        : [y]"=&x"(y), [x]"+x"(x)
        : [c]"r"(coeff),
	  [ma]"x"(EXP_MASK), [bi]"x"(EXP_BIAS), 
	  [one]"x"(UV_1_4sf),
	  [mln]"I"(MANT_LEN)
	: "%xmm7"
    );

    return y;
}
#endif /* ASMLOG_C */
