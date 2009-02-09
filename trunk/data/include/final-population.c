/*============================================================*\
 *                   RESULT BLOCK DUMPING                     *
\*------------------------------------------------------------*/

/*
 * On CELL, results are computed in fixed-size blocks, which, in
 * general, have some amount of waste at the top. The result pointer
 * points to the first of a series of blocks, each with its own little
 * bit of waste at the top, so one must print out results block-wise
 * so that the garbage in the wasted space isn't printed out.
 *
 * On IA32, this is not the case -- there is, in effect, a single
 * block with as many result-sets as were requested.
 *
 * SIMD-slot optimization complicates the picture somewhat, and
 * separate result-block printing functions are supplied for the SSO
 * and non-SSO cases.
 */
#include <stdio.h>	/* for fprintf(), etc.	*/

#include "final-population.h"
#include "types.h"
#include "error-macros.h"
#include "sim-info.h"


/*
 * Forward-declaration: one of the variants of the _fp_dump_block()
 * primitive is essential to the sole function in the public
 * interface, fp_print()
 */
static void _fp_dump_block(void *, int, FILE*);


#if SSO==CMC_SSO_ON
/*
 * If SSO is on, a whole population (every species) is saved at once
 * to contiguous memory, so successive integer positions in memory
 * correspond to successive species. This make printing out results
 * under these circumstances very easy.
 */
static void _fp_dump_block(
    void *rblk,		/* Result block				*/
    int  n_trjs,	/* Number of trajectories in the block	*/
    FILE *fp)		/* File to print out to			*/
{
    int k;	/* Species position	*/
    int tro;    /* Trajectory offset    */

    T_PIS *rb = (T_PIS *)rblk;

    PING();
    CLOG("Scalar block dump of %d trjs @ %08lx\n", n_trjs, (uint32_t)rblk);

    if(fp==NULL)
	fp=stdout;

    for(tro=0; tro < (n_trjs*N_SPXS); tro+=N_SPXS) {
        for(k=0; k < N_SPXS-1; k++) {
	    fprintf(fp, T_PIS_FMT"\t", rb[tro+k]);
        }
	fprintf(fp, T_PIS_FMT"\n", rb[tro+k]);
    }

    PING();
}
#else /* SSO OFF */
/*
 * If SSO is off, a single block has 'n_rsets' *sets* of trajectories
 * (and possibly some irrelevant unused space at the end), where the
 * results have been generated in vectors of size T_NVS -- four (SP)
 * or two (DP).
 *
 * This means that T_NVS consecutive integer positions correspond to
 * the same species in T_NVS different trajectories. One cannot,
 * therefore, simply print out integers sequentially since consecutive
 * int positions DO NOT correspond to different species. So one has to
 * print a population of a species, then skip T_NVS positions to get
 * the next species in the same trajectory, print that one, etc., then
 * go back to the beginning, move one slot forward, do the same again.
 */
static void _fp_dump_block(
    void *rblk,
    int n_rsets,	/* Number of result sets in the block	*/
    FILE *fp)		/* File to print out to			*/
{
    uv_t *rb;
    int k;	/* Species position	*/
    int s;	/* SIMD slot		*/
    int rso;    /* Result-set offset    */

    PING();
    CLOG("Vector block dump of %d rsets @ %08p\n", n_rsets, rblk);

    if(fp==NULL)
	fp=stdout;

    rb = (uv_t *)rblk;


    for(rso=0; rso<(N_SPXS*n_rsets); rso+=N_SPXS) {
	for(s=0; s < T_NVS; s++) {
	    for(k=0; k < N_SPXS-1; k++) {
		fprintf(fp, T_PIS_FMT"\t", rb[rso+k].T_PIS_UM[s]);
	    }
	    fprintf(fp, T_PIS_FMT"\n", rb[rso+k].T_PIS_UM[s]);
	}
    }


    PING();
}
#endif
/*==============^^^ RESULT BLOCK DUMPING ^^^ =================*/



/*============================================================*\
 *                 RESULT ENTIRETY DUMPING                    *
\*------------------------------------------------------------*/
#if defined(__SSE2__)
/*
 * On IA32, results are a single block
 */
void fp_print(sim_info_t *si, FILE *fp) {
    PING();
#if SSO==CMC_SSO_ON
    _fp_dump_block(si->results, si->n_trjs, fp);
#else
    _fp_dump_blocks(si->results, si->n_trjs/T_NVS, fp);
#endif
    PING();
}

#elif defined(__PPU__)
/*
 * On CELL, results are in blocks
 */
#if SSO==CMC_SSO_ON
/*
 * Sets of blocks of plain trajectories (belonging to a single SPU) if
 * SSO is on
 */
static void _fp_print_blockset(sim_info_t *si, void *start, FILE *fp)
{
    unsigned b;	/* Block number		*/

    if(si->verbose) {
	CLOG("Printing SPU scalar block-set at 0x%08x\n", (uint32_t)start);
    }

    for(b=0; b<si->n_blks_per_thr-1; b++) {
	_fp_dump_block( start+(b*si->blksz), si->n_trjs_per_blk, fp);
    }
    b=si->n_blks_per_thr-1; /* t'be sure */
    _fp_dump_block( start+(b*si->blksz), si->n_trjs_residual, fp);
    PING();
}
#else /* SSO is OFF */
/*
 * Sets of blocks of result-sets (belonging to a single SPU) if SSO is
 * off.
 */
static void _fp_print_blockset(sim_info_t *si, void *start, FILE *fp)
{
    int b; /* Block number */

    if(si->verbose) {
	CLOG("Printing SPU vector block-set at 0x%08x\n", (uint32_t)start);
    }

    for(b=0; b<(si->n_blks_per_thr-1); b++) {
	_fp_dump_block( start+(b*si->blksz), si->n_rsets_per_blk, fp);
    }
    b=si->n_blks_per_thr-1; /* t'be sure */
    _fp_dump_block( start+(b*si->blksz), si->n_rsets_residual, fp);

}
#endif /* SSO ON/OFF */

void fp_print(sim_info_t *si, FILE *fp) {
    int     t;	/* Thread/SPU number			*/
    void   *s;	/* Start of blockset for a thread/spu	*/
    size_t  k;	/* Blockset size			*/

    PING();
    k = (si->n_blks_per_thr+1) * si->blksz;

    for(t=0; t<si->n_thrs; t++) {
	s = si->results + (t * k);
	_fp_print_blockset(si, s, fp);
    }
    PING();
}

#endif /* defined(__SSE2__)... */
