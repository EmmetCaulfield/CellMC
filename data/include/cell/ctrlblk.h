/**
 * @file    ctrlblk.h
 * @brief   Model-independent control-block definition common to 
 *          SPU and PPU code.
 * @author  Emmet Caulfield
 *
 * $Id: ctrlblk.h 85 2009-02-02 03:31:39Z emmet $
 */
#ifndef CTRLBLK_H
#define CTRLBLK_H

#include <types.h>	/* For T_FPS */
#include <inttypes.h>	/* For exact integer types */


/*
 * The ctrlblk_t must be 128 bytes:
 */
#define CB_PAD_BYTES (79)

typedef struct {
    /* PPU to SPU */
    uint8_t  thr_id;		/* This SPU's thread-number from the PPU perspective			*/
    uint64_t result_base;	/* The base address in main memory where this SPU writes results	*/
    uint32_t blksz;		/* The size of the block used for PPU/SPU transfers			*/
    uint32_t seed;		/* The seed this SPU will use for its RNG				*/
    uint32_t steps_per_trj;	/* Unused								*/
    uint32_t n_trjs;		/* The number of trajectories to compute (used?)			*/
    uint16_t n_blks;		/* The number of blocks to compute					*/
#if SSO==CMC_SSO_OFF
    uint16_t n_rsets_per_blk;	/* The number of result-sets that fit in a block of size blksz		*/
    uint16_t n_rsets_residual;  /* The number of result-sets to be computed in the final block		*/
#else
    uint16_t n_trjs_per_blk;	/* The number of populations that fit in a block of size blksz		*/
    uint16_t n_trjs_residual;   /* The number of populations to be computed in the final block		*/
#endif
    T_FPS    t_stop;		/* The stop-time for each trajectory					*/

    char     padding[CB_PAD_BYTES];	/* Padding out to 128 bytes					*/
} ctrlblk_t;


/*
 * We use these functions ONLY on the PPU side, never on the SPU side
 * nor with the regular 'gcc' compiler.
 */
#if defined(__PPU__)

#include <sim-info.h>
#include <stdio.h>

/**
 * Fills in a control-block.
 */
void cb_init( ctrlblk_t* __restrict__ cb, sim_info_t* __restrict__ si);


/**
 * Dumps a control block to the given stream.
 */
void cb_dump( const ctrlblk_t *const cb, FILE *fp);

#endif /* defined(__PPU__) */


#endif /* CTRLBLK_H */
