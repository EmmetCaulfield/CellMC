#ifndef ARCH_SUPPORT_H
#define ARCH_SUPPORT_H

#include <cellmc.h>
#if THR==CMC_THR_ON
#   include <pthread.h>
#   include <sched.h>
#   include <sys/types.h>
#   include <linux/unistd.h>
#endif


#if PREC==CMC_PREC_SINGLE
#   include "prec-single.h"
#elif PREC==CMC_PREC_DOUBLE
#   include "prec-double.h"
#endif

/*
 * Whether an cumulative array of propensity sums is maintained
 * while summing and used for reaction choice
 */
#undef CUMULATIVE_SUM_ARRAY

#endif /* ARCH_SUPPORT_H */
