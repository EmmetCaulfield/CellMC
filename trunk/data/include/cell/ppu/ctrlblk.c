#include "../ctrlblk.h"

#include <sim-info.h>
#include <stdio.h>

/**
 * Fills in a control-block struct from a simulation information
 * struct.
 */

/*
 * In order to compute the number of blocks that each SPU must
 * compute, we need to know:
 *
 *    1. The number of trajectories to compute (from command-line)
 *    2. The number of species (from model)
 *    3. The size of a block (from command-line)
 *    4. The number of SPUs (in n_thrs)
 *    5. The number of MPI nodes (if applicable)
 *
 * From the block-size and number of species, we can compute the
 * number of results that fit in a block. Recall that T_NVS
 * populations of the same species are in each 128b (16B) register.
 *
 * We assume that we start the number of SPUs from the command-line on
 * each MPI node, and that every SPU computes the same number of sets
 * of results, and this number is at least one.
 *
 * Therefore, on an 8-node PS3 cluster, even if we only ask for 1
 * trajectory, we will actually get 192! (in the default case: 8*6*4).
 * By the time cb_init() is called, the number of trajectories will
 * already have been "rounded up" to a multiple of this number.
 * 
 * We assume that each SPU computes zero or more full blocks plus
 * exactly one residual block having one or more result sets.
 *
 */
void cb_init( ctrlblk_t* __restrict__ cb, sim_info_t* __restrict__ si)
{
    cb->blksz   = si->blksz;
    cb->n_trjs  = si->n_trjs_per_thr;
    cb->t_stop  = si->t_stop;
    cb->n_blks  = si->n_blks_per_thr;

#if SSO==CMC_SSO_OFF
    cb->n_rsets_per_blk  = si->n_rsets_per_blk;
    cb->n_rsets_residual = si->n_rsets_residual;
#else
    cb->n_trjs_per_blk  = si->n_trjs_per_blk;
    cb->n_trjs_residual = si->n_trjs_residual;
#endif

    cb->steps_per_trj = 0;
    cb->result_base = 0;
}


/**
 * Dumps a control block to the given stream.
 */
void cb_dump( const ctrlblk_t * const cb, FILE *fp )
{
    if( fp == NULL ) {
	fp = stderr;
    }

    fprintf(fp, EXE ": ctrlblk_t@%p = {\n", cb);

    fprintf(fp, "\tthr_id           : %02"PRIx8"\n",     cb->thr_id	          );
    fprintf(fp, "\tresult_base      : 0x%016"PRIx64"\n", cb->result_base      );
    fprintf(fp, "\tblksz            : %"PRIu32"\n",      cb->blksz            );
    fprintf(fp, "\tseed             : 0x%08"PRIx32"\n",  cb->seed             );
    fprintf(fp, "\tsteps_per_trj    : %"PRIu32"\n",      cb->steps_per_trj    );
    fprintf(fp, "\tn_trjs           : %"PRIu32"\n",      cb->n_trjs           );
    fprintf(fp, "\tn_blks           : %"PRIu16"\n",      cb->n_blks           );
#if SSO==CMC_SSO_OFF
    fprintf(fp, "\tn_rsets_per_blk  : %"PRIu16"\n",      cb->n_rsets_per_blk  );
    fprintf(fp, "\tn_rsets_residual : %"PRIu16"\n",      cb->n_rsets_residual );
#else
    fprintf(fp, "\tn_trjs_per_blk   : %"PRIu16"\n",      cb->n_trjs_per_blk   );
    fprintf(fp, "\tn_trjs_residual  : %"PRIu16"\n",      cb->n_trjs_residual  );
#endif
    fprintf(fp, "\tt_stop           : "T_FPS_FMT"\n",    cb->t_stop           );
    fprintf(fp, "\tpadding[]        : %uB\n",	     sizeof(cb->padding)  );
    fprintf(fp, "} (%uB)\n", sizeof(ctrlblk_t) );
}


