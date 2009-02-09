#ifndef SIM_INFO_H
#define SIM_INFO_H

#include <inttypes.h>		/* for uintXX_t		*/
#include <sys/time.h>		/* for struct timeval	*/
#include <stdio.h>		/* for FILE		*/
#include <stdbool.h>		/* for bool		*/

#include "types.h"		/* for T_FPS etc.	*/	
#include "cellmc.h"		/* for cellmc_app_t	*/	

typedef struct sim_info_t sim_info_t;

struct sim_info_t {
/* Compile-time options and parameters */
    cellmc_app_t app;

/* Runtime info */
    char *rcmd;			/* Invocation name, i.e. argv[0]	*/
    char *outfile;		/* Output filename			*/

/* Runtime simulation parameters */
    int      n_trjs_reqd;	/* No. of trajectories requested (CLA)	*/
    int	     n_trjs;		/* Actual no. of trajectories to do	*/
    T_FPS    t_stop;		/* Final time of trajectories (CLA)	*/
    uint32_t seed;		/* PRNG seed (CLA)			*/

/* Runtime results */
    struct timeval ding;	/* Start-time of simulation		*/
    struct timeval dong;	/* End-time of simulation		*/
    uint64_t abs_rxn_total;	/* Absolute number of rxns (if counted) */
    uint64_t con_rxn_total;	/* No. of contributing rxns (if c'nted)	*/
    void    *results;           /* Final trajectory populations		*/

#if PROF==CMC_PROF_ON
/* Helper support */
    void  *helper_data;		/* Helper data pointer			*/
    void (*helper_cleanup_fn)(sim_info_t *); /* Helper's cleanup func'n	*/
#endif

/* Runtime options */
    bool     header;		/* Whether to print an info header	*/
    bool     verbose;		/* Whether to be verbose or not		*/
#if defined(__PPU__)
    size_t   blksz;		/* Block size in bytes			*/
    size_t   n_blks;
# if MPI==CMC_MPI_ON
    int      n_mpi_nodes;	/* Number of MPI nodes used		*/
# endif
#endif

#if (defined(__PPU__) || (defined(__SSE2__) && THR==CMC_THR_ON))
    int      n_thrs;		/* Number of compute threads or SPUs	*/
    int      n_trjs_per_thr;	/* Number of trajectories per thread	*/
#endif

/* Runtime computed parameters */
    int      n_trjs_computed;	/* Not less than n_trjs			*/
#if defined(__PPU__)
    int      n_blks_per_thr;	/* Number of blocks per SPU		*/
#  if SSO==CMC_SSO_OFF
    int      n_rsets_per_blk;	/* Number of result sets per block	*/
    int      n_rsets_residual;	/* Number of result sets in last block	*/
#  elif SSO==CMC_SSO_ON
    int      n_trjs_per_blk;	/* Number of trajectories per block	*/
    int      n_trjs_residual;	/* Number of trjs in last block		*/
#  endif
#endif
};

#define SI_SEED_DEFAULT  3987654321U	/* Default RNG seed		*/
#define SI_BLKSZ_DEFAULT 8192

void si_init(sim_info_t *si, int argc, char * argv[]);
void si_start(sim_info_t *si);
void si_stop(sim_info_t  *si);
void si_print_results(sim_info_t * const si);
void si_header(sim_info_t *si, FILE *fp, const char * const pre, const char * const post);
void si_cleanup(sim_info_t *si);

#endif /* SIM_INFO_H */
