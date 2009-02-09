#ifndef FPULOG_C
#define FPULOG_C

#include "univec.h"

static inline 
v4sf _fpu_logf4(v4sf x)
{
    asm (
	"fldln2			\n"
	"fld      (%[xp])	\n"
	"fyl2x			\n"
	"fstp     (%[xp])	\n"
	"fldln2			\n"
	"fld     4(%[xp])	\n"
	"fyl2x			\n"
	"fstp    4(%[xp])	\n"
	"fldln2			\n"
	"fld     8(%[xp])	\n"
	"fyl2x			\n"
	"fstp    8(%[xp])	\n"
	"fldln2			\n"
	"fld    12(%[xp])	\n"
	"fyl2x			\n"
	"fstp   12(%[xp])	\n"
	:
        : [xp]"r"(&x)
	: "st", "st(1)", "memory"
    );
    return x;
}


static inline 
v2df _fpu_logd2(v2df x)
{
    __asm__ __volatile__ (
	"fldln2			\n"
	"fld      (%[xp])	\n"
	"fyl2x			\n"
	"fstp     (%[xp])	\n"
	"fldln2			\n"
	"fld     8(%[xp])	\n"
	"fyl2x			\n"
	"fstp    8(%[xp])	\n"
	:
	: [xp]"p"((double *)&x)
	: "st", "st(1)", "memory"
    );
    return x;
}

#endif /* FPULOG_C */
