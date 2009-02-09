#include <cb_utils.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <debug_utils.h>
#include <ctrlblk.h>
#include <revision.h>

/*
 * In order to compute the number of blocks that each SPU must
 * compute, we need to know:
 *
 *    1. The number of trajectories to compute (from command-line)
 *    2. The number of species (from model)
 *    3. The size of a block (currently fixed)
 *    4. The number of SPUs
 *
 * From the block-size and number of species, we can compute the
 * number of results that fit in a block. Recall that 4 populations of
 * the same species are in each 128b (16B) register, referred to as a
 * "quad". The following is simple arithmetic, but it's easy to get
 * wrong, so we do it very explicitly.
 *
 * We assume that we always start all SPUs, each computes the same
 * number of sets of results, and this number is at least
 * one. Therefore, even if we only ask for 1 trajectory, we will
 * actually get 24: #SPUs * #ints-per-vector = 6*4
 * 
 * This "multiple" is dubbed a "granule".
 *
 * We assume that each SPU computes zero or more full blocks plus
 * exactly one residual block having one or more quajectories.
 *
 */
static void _cb_nblocks(ctrlblk_t *__restrict__ cb, uint32_t n_trjs, uint8_t n_spus)
{
    uint32_t quajectory_bytes;	// In bytes
    uint32_t quajectories_per_block;
    uint32_t trjs_per_granule;
    uint32_t blocks_per_spu;
    uint32_t quajectories_per_spu;
    uint32_t residual_quajectories;
    uint32_t actual_trjs;

    DU_PRINTF("--==[ Block computation ]==--\n");
    DU_PRINTF("\t#SPUs: %u\n", n_spus);
    DU_PRINTF("\t#Trajectories: %u\n", n_trjs);

    /*
     * How many trajectories in a granule?
     */ 
    trjs_per_granule = ( 4 * n_spus );
    DU_PRINTF("\t#Trajectories per granule: %u\n", trjs_per_granule);
    
    /*
     * How many result sets must be computed in total?
     */
    quajectories_per_spu = n_trjs / trjs_per_granule;
    if( n_trjs % trjs_per_granule ) {
	quajectories_per_spu++;
    }
    DU_PRINTF("\t#Quajectories per SPU: %u\n", quajectories_per_spu);


    /*
     * How many quajectories fit in a block:
     */
    DU_PRINTF("\tBlock size: %u B\n", cb->block_size);
    quajectory_bytes = 16*cb->n_species;
    DU_PRINTF("\tQuajectory size: %u B\n", quajectory_bytes);
    quajectories_per_block = cb->block_size / quajectory_bytes;
    DU_PRINTF("\tQuajectories per block %u\n", quajectories_per_block);

    /*
     * Now, how many blocks must each SPU compute:
     */
    residual_quajectories=quajectories_per_block;
    blocks_per_spu = quajectories_per_spu / quajectories_per_block;
    if( quajectories_per_spu % quajectories_per_block ) {
	residual_quajectories = quajectories_per_spu - (blocks_per_spu * quajectories_per_block);
	blocks_per_spu++;
    }
    DU_PRINTF("\tTotal blocks per SPU: %u\n", blocks_per_spu);
    DU_PRINTF("\tQuajectories in last block: %u\n", residual_quajectories);

    actual_trjs = ((blocks_per_spu-1)*quajectories_per_block+residual_quajectories) * trjs_per_granule;

    DU_PRINTF("\tActual #trajectories: %u\n", actual_trjs);

    cb->n_blocks        = blocks_per_spu;
    cb->rsets_per_block = quajectories_per_block;
    cb->rsets_residual  = residual_quajectories;
    cb->n_trjs          = actual_trjs;
}

/**
 * Fills in a control-block.
 */
void cb_fillin( ctrlblk_t* __restrict__ cb, uint32_t n_trjs, uint8_t n_spus, float t_stop)
{
    cb->t_stop = t_stop;
    _cb_nblocks(cb, n_trjs, n_spus);
}


/**
 * Duplicates a control block exactly, including all the fields that
 * (arguably) it shouldn't: i.e. rng_seed and result_base.
 */
void cb_copy( ctrlblk_t *__restrict__ tgt, ctrlblk_t *__restrict__ src)
{
    memcpy(tgt, src, sizeof(ctrlblk_t));
}


/**
 * Dumps a control block to the given stream.
 */
void cb_dump( FILE *fp, ctrlblk_t *__restrict__ cb) {
    fprintf(fp, "--==[ Control Block %p Dump ]==--\n", cb);
    fprintf(fp, "\tresult_base     (P3:u64): 0x%llx\n", cb->result_base     );
    fprintf(fp, "\trng_seed        (P3:u32): 0x%x\n",   cb->rng_seed        );
    fprintf(fp, "\tn_blocks        (P3:u16): %u\n",     cb->n_blocks        );
    fprintf(fp, "\trsets_per_block (P3:u16): %u\n",     cb->rsets_per_block );
    fprintf(fp, "\trsets_residual  (P3:u16): %u\n",     cb->rsets_residual  );
    fprintf(fp, "\tsteps_per_trj   (P1:u32): %u\n",     cb->steps_per_trj   );
    fprintf(fp, "\tn_species       (S2:u16): %u\n",     cb->n_species       );
    fprintf(fp, "\tn_reactions     (S2:u16): %u\n",     cb->n_reactions     );
    fprintf(fp, "\tblock_size      (S2:u16): %u\n",     cb->block_size      );
    fprintf(fp, "\tt_stop          (P3:f32): %f\n",     cb->t_stop          );
    fprintf(fp, "\tn_trjs          (P3:u32): %u\n",     cb->n_trjs          );
    fprintf(fp, "\tmodel_name      (S2:str): %s\n",     cb->model_name      );
}


/**
 * A human-readable hash with the essential details
 */
void cb_hr_hash( ctrlblk_t *__restrict__ cb, char *buf ) {
    snprintf(buf, 128, "%s-r%s-%ux%f", cb->model_name, SVN_REV, cb->n_trjs, cb->t_stop);
}
