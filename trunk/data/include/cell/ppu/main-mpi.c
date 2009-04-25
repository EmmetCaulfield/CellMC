/**
 * @file   main-mpi.c
 * @brief  MPI driver for cellmc
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
#include <libspe2.h>
#include <pthread.h>
#include <libmisc.h>

/*
 * For MPI
 */
#include <mpi.h>

#include "support.h"
#include "univec.h"
#include "cellmc.h"
#include "sim-info.h"
#include "app-support.h"

#include "error-macros.h"

#include "cell/ctrlblk.h"  /* Common control block struct */
#include "cell/summblk.h"  /* Common summary data struct  */

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

void aggregate_metadata(sim_info_t *ninf, sim_info_t *info)
{
    int i;

    info->abs_rxn_total=0LL;
    info->con_rxn_total=0LL;
    info->n_trjs_computed=0LL;
    for(i=0; i<info->n_mpi_nodes; i++) {
	info->abs_rxn_total   += ninf[i].abs_rxn_total;
	info->con_rxn_total   += ninf[i].con_rxn_total;
	info->n_trjs_computed += ninf[i].n_trjs_computed;
    }

    return;
}


void do_sim(sim_info_t *info) {
    ctrlblk_t *cb;
    summblk_t *sb;

    int i, j, rc;

    uint32_t m, n;

    spe_context_ptr_t *spu_context;
    pthread_t         *spu_thread;
    thread_args_t     *thread_args;

    v4si *vu;

    /*
     * Allocate data-structure arrays now that we know the number of
     * SPUs (info->n_thrs).
     */
    cb = (ctrlblk_t *)malloc_align(info->n_thrs*sizeof(ctrlblk_t), 7);

    spu_context=(spe_context_ptr_t *)malloc(info->n_thrs*sizeof(spe_context_ptr_t));
    RCHECK(spu_context, !=NULL, malloc);
    
    spu_thread=(pthread_t *)malloc(info->n_thrs*sizeof(pthread_t));
    RCHECK(spu_thread, !=NULL, malloc);
    
    thread_args=(thread_args_t *)malloc(info->n_thrs*sizeof(thread_args_t));
    RCHECK(thread_args, !=NULL, malloc);

    if( info->verbose )
	CLOG("Main memory allocated for housekeeping.\n");
    

    /*
     * Fill in the control blocks. Each has its own id, seed, and
     * result_base, but they otherwise contain the same data.
     */
    cb_init(&cb[0], info);
    cb[0].thr_id=0;
    srand48(info->seed);
    cb[0].seed=(uint32_t)lrand48();
    n = (uint32_t)info->results;
    cb[0].result_base = 0x00000000ffffffff & (uint64_t)n;

    for(i=1; i<info->n_thrs; i++) {
	memcpy(&cb[i], &cb[0], sizeof(ctrlblk_t));
	cb[i].thr_id = i;
	cb[i].seed   = (uint32_t)lrand48();
	n = (uint32_t)info->results + (uint32_t)i*info->blksz*(info->n_blks_per_thr+1);
	cb[i].result_base = 0x00000000ffffffff & (uint64_t)n;
    }
    if( info->verbose ) {
	CLOG("%d control blocks initialized.\n", info->n_thrs);
	cb_dump(&cb[0], stdout);
    }

    si_start(info);
    if( info->verbose )
	CLOG("Clock started.\n");

    /*
     * Start the SPUs!
     */
    for(i=0; i<info->n_thrs; i++) {
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

    if(info->verbose)
	CLOG("SPUs started.\n");


    /*
     * Now, wait for all the SPU programs to finish
     */
    for(i=0; i<info->n_thrs; i++) {
	rc = pthread_join(spu_thread[i], NULL);
	RCHECK(rc, ==0, pthread_join);

	rc=spe_context_destroy(spu_context[i]);
	RCHECK(rc, ==0, spe_context_destroy);
    }
    if(info->verbose)
	CLOG("SPUs finished.\n");

    /*
     * The summary block is at the end of the results and isn't
     * counted in the number of blocks per thread
     */
    info->abs_rxn_total = 0LL;
    info->con_rxn_total = 0LL;
    m = info->blksz*(info->n_blks_per_thr);
    for(i=0; i<info->n_thrs; i++) {
	n  = (uint32_t)(cb[i].result_base + m);
	sb = (summblk_t *)n;
	info->abs_rxn_total += sb->nr_abs;
	info->con_rxn_total += sb->nr_con;
    }
    if( info->verbose )
	CLOG("Retrieved summary data.\n");

    /*
     * Cleanup, but don't deallocate info.results as it was allocated
     * in the caller (and we deallocate there) and we have yet to send
     * the results to the master and deallocate in the caller.
     */
    free_align( cb );
    free( spu_context );
    free( spu_thread  );
    free( thread_args );

    si_stop(info);
    if( info->verbose )
	CLOG("Clock stopped.\n");

    return;
}

void slave(void) 
{
    sim_info_t info;
    int size;
    int rc;

    MPI_Status status;

    /*
     * Because our nodes are homogeneous, we cheat and don't bother
     * packing/unpacking the struct
     */
    size=sizeof(sim_info_t)/sizeof(char);
    rc=MPI_Scatter(NULL, size, MPI_CHAR, &info, size, MPI_CHAR, 0, MPI_COMM_WORLD);
    RCHECK(rc, ==MPI_SUCCESS, MPI_Scatter);


    /*
     * Let's allocate memory for the results here, so we know what
     * size the result-block is to pass back, with one extra block per
     * SPU for the summary data at the end.
     */
    size = info.n_thrs * (info.n_blks_per_thr+1) * info.blksz;
    info.results = malloc_align( size, 7 );
    RCHECK(info.results, !=NULL, malloc_align);
    DPRINTF("base: %08p\n", info.results);
    if( info.verbose )
	CLOG("Aligned main memory allocated for results.\n");

    /*
     * Now, we can do our (the slave's ) share of the computation.
     */
    do_sim(&info);

    /*
     * Let's send the result back to the master:
     */
    rc=MPI_Gather(info.results, size, MPI_CHAR, NULL, size, MPI_CHAR, 0, MPI_COMM_WORLD);
    RCHECK(rc, ==MPI_SUCCESS, MPI_Gather);

    /*
     * Let's send the metadata gathered in 'info' back to the master:
     */
    rc=MPI_Gather(&info, sizeof(sim_info_t), MPI_CHAR, NULL, sizeof(sim_info_t), MPI_CHAR, 0, MPI_COMM_WORLD);
    RCHECK(rc, ==MPI_SUCCESS, MPI_Gather);

    /*
     * Now, let's clean up
     */
    free_align(info.results);

    return;
}




void master(int argc, char *argv[])
{
    sim_info_t *info;	/* Simulation info for each node	*/
    sim_info_t init;	/* Initialisation and aggregate info	*/
    sim_info_t dummy;	/* Dummy receive buffer for MPI		*/
    struct timeval tic;	/* Start-time of simulation		*/
    struct timeval toc;	/* End-time of simulation		*/
    uint32_t seed;
    int i;
    int size;
    int rc;
    FILE *fp;

    MPI_Status   status;
    MPI_Request  request;

    si_init(&init, argc, argv);
    if( init.verbose )
	CLOG("Command-line arguments processed.\n");

    si_start(&init);

    /*
     * Allocate space for sim_info_t structs for each node. We can do
     * this in this very simplistic way because the slaves don't use
     * any of the 4 pointers in a sim_info_t, except result, which we
     * handle explicitly, so we don't need fancy serialisation/packing.
     */
    info=(sim_info_t *)malloc((init.n_mpi_nodes)*sizeof(sim_info_t));
    RCHECK(info, !=NULL, malloc);

    size=sizeof(sim_info_t)/sizeof(char);
    srand48(init.seed);
    for(i=0; i<init.n_mpi_nodes; i++) {
	info[i]=init;
	info[i].seed=(uint32_t)lrand48();
    }
    
    /*
     * Because our nodes are homogeneous, we cheat and don't bother
     * packing/unpacking the struct, which would be a royal PITA.  The
     * content of info[0] ends up in dummy, which we don't care about.
     */
    rc=MPI_Scatter(info, size, MPI_CHAR, &dummy, size, MPI_CHAR, 0, MPI_COMM_WORLD);
    RCHECK(rc, ==MPI_SUCCESS, MPI_Scatter);
    
    /*
     * We have to allocate enough memory for *all* of the results in
     * the master:
     */
    size = init.n_thrs * (init.n_blks_per_thr+1) * init.blksz;
    init.results = malloc_align( init.n_mpi_nodes*size, 7 );
    RCHECK(init.results, !=NULL, malloc_align);

    /*
     * We also have to allocate space for our local computation:
     */
    info[0].results = malloc_align( size, 7 );

    /*
     * Now we can do our (the master's) share of the computation
     */
    do_sim(&info[0]);

    /*
     * Now, collect the results from the nodes
     */
    rc=MPI_Gather(info[0].results, size, MPI_CHAR, init.results, size, MPI_CHAR, 0, MPI_COMM_WORLD);
    RCHECK(rc, ==MPI_SUCCESS, MPI_Gather);

    /*
     * Let's send the metadata gathered in 'info' back to the master:
     */
    rc=MPI_Gather(info, sizeof(sim_info_t), MPI_CHAR, &info[0], sizeof(sim_info_t), MPI_CHAR, 0, MPI_COMM_WORLD);
    RCHECK(rc, ==MPI_SUCCESS, MPI_Gather);

    si_stop(&init);

    aggregate_metadata(info, &init);

    /*
     * Print out results
     */
    fp=si_print_results(&init);
    RCHECK(fp, !=NULL, si_print_results);

    /*
     * si_print_results will only have printed out results from the
     * first info struct, so we must arrange for the remaining results
     * to be printed out by re-pointing the results element to the
     * right place (right now, these are addresses on the remote
     * nodes, and are invalid here) and calling fp_print()
     */
    for(i=1; i<init.n_mpi_nodes; i++) {
	info[i].results = init.results + i*size;
	fp_print(&info[i], fp);
    }


    if( init.verbose )
	CLOG("Results printed.\n");

    /*
     * Cleanup
     */
    if( init.verbose )
	CLOG("Cleaning up.\n");

    free(info);
    free_align( init.results );

    return;
}



/*
 * Essentially, we need to:
 *
 *    Read CLAs and bootstrap on one node (master)
 *    Pass the sim_info_t structure to the other nodes (slaves)
 *    Do the calculations on the slaves
 *    Pass the results from the slaves back to the masters
 *    Collate the results on the master
 *    Print out the results
 *    Quit
 */
int main(int argc, char *argv[])
{
    int myid;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&myid);

    if( myid==0 )
      master(argc, argv);
    else
      slave();

    MPI_Finalize();

    return 0;
}

