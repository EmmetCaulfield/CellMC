#ifndef LIBLOG_C
#define LIBLOG_C

#include <math.h>
#include "univec.h"

static inline v4sf _lib_logf4(v4sf x) {
    uv_t u, v;
    int i;

    u.sf=x;
    for(i=0; i<UV_4; i++) {
	v.f[i]=logf( u.f[i] );
    }
    return v.sf;
}

//static inline 
v2df _lib_logd2(v2df x) {
    uv_t u;

    u.df=x;
    u.d[0]=log( u.d[0] );
    u.d[1]=log( u.d[1] );

    return u.df;
}

#endif /* LIBLOG_C */
