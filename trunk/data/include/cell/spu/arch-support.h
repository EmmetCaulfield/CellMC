#ifndef ARCH_SUPPORT_H
#define ARCH_SUPPORT_H

#if !defined(__SPU__)
    error "SPU support file included, but not on SPU"
#endif

#include <stdint.h>		/* For uint64_t, etc.		*/
#include <spu_intrinsics.h>	/* For spu_convtf(), etc.	*/
#include <spu_mfcio.h>		/* For mfc_get(), etc.		*/
#include <mc_rand_mt.h>		/* For IBM MT PRNG		*/
#include <simdmath/logf4.h>	/* For _logf4()			*/
#include <libmisc.h>		/* For malloc_align()		*/

    // #define DEBUG 1
    // #define UV_DEBUG 1

#include "../../univec.h"
#include "../../types.h"
#include "../../error-macros.h"
#include "../ctrlblk.h"
#include "../summblk.h"

#define CVT_VEC_ITOF(X) spu_convtf(X,0)

#define as_srand(SEED)  mc_rand_mt_init(SEED)
#define as_rand()	mc_rand_mt_0_to_1_f4()
#define as_tau(R)	(-_logf4( as_rand() )/R)

#endif /* ARCH_SUPPORT_H */
