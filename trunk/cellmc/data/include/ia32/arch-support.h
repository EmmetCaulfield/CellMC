#ifndef ARCH_SUPPORT_H
#define ARCH_SUPPORT_H

#include <cellmc.h>

#if THR==1
#   include <pthread.h>
#endif


#if PREC==CMC_PREC_SINGLE
#   include "prec-single.h"
#elif PREC==CMC_PREC_DOUBLE
#   include "prec-double.h"
#endif

/*
 * #define if you want results
 */
#undef DUMP_RESULTS

/*
 * #define if you want a brief report when each trajectory ends.
 * Good for HSR. Horrible for ME.
 */
#undef REPORT_ALL

/*
 * Whether an cumulative array of propensity sums is maintained
 * while summing and used for reaction choice
 */
#undef CUMULATIVE_SUM_ARRAY

#endif /* ARCH_SUPPORT_H */
