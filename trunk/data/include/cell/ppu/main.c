/**
 * @file   cdssak.c
 * @brief  Model-independent PPU program for direct-method SSA
 * @author Emmet Caulfield
 *
 * $Id: main.c 86 2009-02-02 04:10:23Z emmet $
 */
#include <stdint.h>	/* uint32_t	*/
#include <stdlib.h>	/* exit()	*/
#include <string.h>

/*
 * #includes for pretty much every PPU program.
 */
#include <altivec.h>
#include <vec_types.h>
#include <libspe2.h>
#include <pthread.h>
#include <libmisc.h>

#include <support.h>
#include <cellmc.h>
#include <sim-info.h>
#include <app-support.h>

#include <error-macros.h>

#include <ctrlblk.h>  /* Common control block struct */
#include <summblk.h>  /* Common summary data struct  */

/*
 * SPU program handle.
 */
extern spe_program_handle_t CMC_SPU_CODE_HANDLE_NAME;


/*
 * Thread data and management:
 */
typedef struct {
    spe_context_ptr_t  spu_context;
    void              *spu_argp;
} thread_args_t;


void *ppu_thread_mgr(void *arg)
{
    thread_args_t *ta;
    uint32_t entry_point = SPE_DEFAULT_ENTRY;

    ta = (thread_args_t *)arg;

    if( 0>spe_context_run(ta->spu_context, &entry_point, 0, ta->spu_argp, NULL, NULL) ) {
	perror("Failed to start SPU thread");
	exit(EXIT_FAILURE);
    }
    pthread_exit(NULL);

}

int main(int argc, char *argv[])
{
    sim_info_t info;
    ctrlblk_t *cb;
    summblk_t *sb;

    int i, j, rc;

    uint32_t m, n;

    spe_context_ptr_t *spu_context;
    pthread_t         *spu_thread;
    thread_args_t     *thread_args;

    vec_int4         *vu;
    vec_int4          zero = (vec_int4){0,0,0,0};

    uint32_t tmp;


    si_init(&info, argc, argv);
    if( info.verbose )
	CLOG("Command-line arguments processed.\n");

    /*
     * Allocate data-structure arrays now that we know the number of
     * SPUs (info.n_thrs).
     */
    cb = (ctrlblk_t *)malloc_align(info.n_thrs*sizeof(ctrlblk_t), 7);

    spu_context=(spe_context_ptr_t *)malloc(info.n_thrs*sizeof(spe_context_ptr_t));
    RCHECK(spu_context, !=NULL, malloc);
    
    spu_thread=(pthread_t *)malloc(info.n_thrs*sizeof(pthread_t));
    RCHECK(spu_thread, !=NULL, malloc);
    
    thread_args=(thread_args_t *)malloc(info.n_thrs*sizeof(thread_args_t));
    RCHECK(thread_args, !=NULL, malloc);

    if( info.verbose )
	CLOG("Main memory allocated for housekeeping.\n");
    

    /*
     * Allocate memory for the results, with one extra block per SPU
     * for summary data at the end.
     */
    n = info.n_thrs * (info.n_blks_per_thr+1) * info.blksz;
    info.results = malloc_align( n, 7 );
    RCHECK(info.results, !=NULL, malloc_align);
    DPRINTF("base: %08p\n", info.results);
    if( info.verbose )
	CLOG("Aligned main memory allocated for results.\n");


    /*
     * Fill in the control blocks. Each has its own id, seed, and
     * result_base, but they otherwise contain the same data.
     */
    cb_init(&cb[0], &info);
    cb[0].thr_id=0;
    srand48(info.seed);
    cb[0].seed=(uint32_t)lrand48();
    n = (uint32_t)info.results;
    cb[0].result_base = 0x00000000ffffffff & (uint64_t)n;

    for(i=1; i<info.n_thrs; i++) {
	memcpy(&cb[i], &cb[0], sizeof(ctrlblk_t));
	cb[i].thr_id = i;
	cb[i].seed   = (uint32_t)lrand48();
	n = (uint32_t)info.results + (uint32_t)i*info.blksz*(info.n_blks_per_thr+1);
	cb[i].result_base = 0x00000000ffffffff & (uint64_t)n;
    }
    if( info.verbose ) {
	CLOG("%d control blocks initialized.\n", info.n_thrs);
	cb_dump(&cb[0], stdout);
    }

    si_start(&info);
    if( info.verbose )
	CLOG("Clock started.\n");

    /*
     * Start the SPUs!
     */
    for(i=0; i<info.n_thrs; i++) {
	spu_context[i]=spe_context_create(0,NULL);
	RCHECK(spu_context[i], !=NULL, spe_context_create);
	thread_args[i].spu_context = spu_context[i];
	thread_args[i].spu_argp    = &cb[i];

	if( spe_program_load(spu_context[i], &spu_model_text) ) {
	    perror("Failed to load SPU program");
	    exit(EXIT_FAILURE);
	}
	if( pthread_create(&spu_thread[i], NULL, &ppu_thread_mgr, &thread_args[i]) ) {
	    perror("Failed to create SPU thread");
	    exit(EXIT_FAILURE);
	}
    }

    if(info.verbose)
	CLOG("SPUs started.\n");


    /*
     * Now, wait for all the SPU programs to finish
     */
    for(i=0; i<info.n_thrs; i++) {
	rc = pthread_join(spu_thread[i], NULL);
	RCHECK(rc, ==0, pthread_join);

	rc=spe_context_destroy(spu_context[i]);
	RCHECK(rc, ==0, spe_context_destroy);
    }
    if(info.verbose)
	CLOG("SPUs finished.\n");

    /*
     * The summary block is at the end of the results and isn't
     * counted in the number of blocks per thread
     */
    info.abs_rxn_total = 0LL;
    info.con_rxn_total = 0LL;
    m = info.blksz*(info.n_blks_per_thr);
    for(i=0; i<info.n_thrs; i++) {
	n  = (uint32_t)(cb[i].result_base + m);
	sb = (summblk_t *)n;
	info.abs_rxn_total += sb->nr_abs;
	info.con_rxn_total += sb->nr_con;
    }
    if( info.verbose )
	CLOG("Retrieved summary data.\n");


    si_stop(&info);
    if( info.verbose )
	CLOG("Clock stopped.\n");


    /*
     * Print out results
     */
    si_print_results(&info);
    if( info.verbose )
	CLOG("Results printed.\n");

    /*
     * Cleanup
     */
    if( info.verbose )
	CLOG("Cleaning up.\n");
    free_align( info.results );
    free( spu_context );
    free( spu_thread  );
    free( thread_args );

    if( info.verbose )
	CLOG("Quitting...\n");

    return 0;
}

