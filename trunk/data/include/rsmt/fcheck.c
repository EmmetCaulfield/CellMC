#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "../SSE/univec128.h"

#define MEXP 19937
#include "rsmt.c"

#define MANT ((v4su){0x007fffff,0x007fffff,0x007fffff,0x007fffff})
#define EXPO ((v4su){0x3f800000,0x3f800000,0x3f800000,0x3f800000})

#define N 100000

int main(void) 
{
    rm_state_t s;
    uv_t u[N];
    uv_t m, e, v, o;
    int i;
    m.su = MANT;
    e.su = EXPO;
    o.sf = (v4sf){1.0f, 1.0f, 1.0f, 1.0f};

    rm_init(&s, 1234);
    rm_rand32_array(&s, (int *)u, 4*N);
    v=u[0];

    for(i=0; i<N; i++) {
	v=u[i];
	v=uv_and(v, m);
	v=uv_or(v,  e); 
	v.sf -= o.sf;
	printf("%10e %10e %10e %10e\n", v.f[0], v.f[1], v.f[2], v.f[3]);
    }

    return 0;
}
