#include <limits.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#if defined(__PPU__)
#   include <libspe2.h>
#endif

#include "cellmc.h"
#include "option-mapping.h"
#include "sim-info.h"
#include "app-support.h"
#include "error-macros.h"

#if PROF==CMC_PROF_OFF
#   include "final-population.h"
#else
#   include "reaction-stats.h"
#endif


#if defined(__PPU__)
#   define CORE "SPU"
#elif defined(__SSE2__)
#   define CORE "compute thread"
#endif

#define SI_TS_BUFLEN  128	/* Character buffer length for strftime() */

#define SI_OPTSTRING   "hb:s:c:io:v"
#define SI_OPT_HELP    'h'
#define SI_OPT_BLKSZ   'b'
#define SI_OPT_SEED    's'
#define SI_OPT_CORES   'c'
#define SI_OPT_INFO    'i'
#define SI_OPT_FILE    'o'
#define SI_OPT_VERBOSE 'v'

#define QUOTEME(s) #s
#define QQ(s) QUOTEME(s)


/*
 * SI multipliers for time computations that avoid magic constants
 * with a missing zero! Laugh now, but when one of your "millions" is
 * 100,000 the joke will be on you :o)
 */
#define _us_per_s  ( 1000000 ) /* One million  */
#define _us_per_ms (    1000 ) /* One thousand */
#define _ms_per_s  (    1000 ) /* One thousand */



static void _si_calc_n_trjs(sim_info_t * const si)
{
#if defined(__PPU__)
    int m;	/* Number of result-sets */
    int k;	/* Size of a result-set  */
#endif

#if defined(__SSE2__) && THR==CMC_THR_OFF
    si->n_trjs = si->n_trjs_reqd;
#else
    int n;	/* Nr. of trajectories computed at once */

    n = si->n_thrs * T_NVS;

#   if MPI==CMC_MPI_ON
        n *= si->n_mpi_nodes;
#   endif

    if( si->n_trjs_reqd % n ) {
	si->n_trjs = si->n_trjs_reqd + (n - si->n_trjs_reqd % n);
    } else {
	si->n_trjs = si->n_trjs_reqd;
    }
    si->n_trjs_per_thr = si->n_trjs / si->n_thrs;

#   if MPI==CMC_MPI_ON
        si->n_trjs_per_thr /= si->n_mpi_nodes;
#   endif
#endif


#if defined(__PPU__)
#   if SSO==CMC_SSO_OFF
        m = si->n_trjs_per_thr / T_NVS;		/* m is the number of result-sets */
        k = sizeof(T_PIV)*N_SPXS;		/* k is the size of a result-set  */

        DIE_IF(si->blksz < k, "Block-size (%d) too small (min: %d)", si->blksz, k);

        si->n_rsets_per_blk  = si->blksz/k;
        si->n_blks_per_thr   = m / si->n_rsets_per_blk + 1;
        si->n_rsets_residual = m % si->n_rsets_per_blk;
#   else
        k = sizeof(T_PIS)*N_SPXS;		/* k is the size of a population  */

        DIE_IF(si->blksz < k, "Block-size (%d) too small (min: %d)", si->blksz, k);

        si->n_trjs_per_blk  = si->blksz / k;
	si->n_blks_per_thr  = si->n_trjs_per_thr / si->n_trjs_per_blk + 1;
        si->n_trjs_residual = si->n_trjs_per_thr % si->n_trjs_per_blk;
#   endif   
    si->n_blks = si->n_blks_per_thr * si->n_thrs;
#endif
}



void si_init(sim_info_t *si, int argc, char * argv[])
{
    int c;			/* CLA flag character from getopt()	*/
    int flags[128]={ 0 };

    extern char *optarg;
    extern int optind, opterr, optopt;

    struct timeval tv;

    FILE *fp;			/* To test if outfile exists		*/

    if( argc <= 2 ) {
	as_helptext(stdout);
	exit(EXIT_SUCCESS);
    }

    si->rcmd          = as_join(argc, argv, " ");
    si->header        = true;
    si->verbose	      = false;
    si->outfile       = NULL;
    si->abs_rxn_total = 0LL;
    si->con_rxn_total = 0LL;
    si->results       = NULL;
    si->seed          = SI_SEED_DEFAULT;
#if defined(__PPU__)
    si->blksz	      = SI_BLKSZ_DEFAULT;
#endif
    si->ding	      = (struct timeval){0,0};
    si->dong	      = (struct timeval){0,0};

#if PROF==CMC_PROF_ON
    si->helper_data       = NULL;
    si->helper_cleanup_fn = NULL;
#endif

#if (defined(__SSE2__) && THR==CMC_THR_ON)
    si->n_thrs = 2;
#elif defined(__PPU__)
    si->n_thrs = 16;
#endif

    /*
     * Command-line processing.
     */
    opterr=0;
    while( -1 != (c=getopt(argc, argv, SI_OPTSTRING)) ) {
	switch(c) {
	case SI_OPT_HELP:
	    as_helptext(stdout);
	    exit(EXIT_SUCCESS);

	case SI_OPT_BLKSZ:
#if defined(__PPU__)
	    si->blksz = (size_t)strtol(optarg, NULL, 10);
	    if( errno == EINVAL || errno == ERANGE || si->blksz <= 0) {
		DIE("Invalid value for block size (-b).");
	    }
	    if( si->blksz % AS_BLKSZ_MULT ) {
		DIE("Block size (-b) must be a multible of " QQ(AS_BLKSZ_MULT) "B.");
	    }
#else
	    DIE("Block size (-b) not applicable to this platform.");
#endif
	    flags[c]++;
	    break;;

	case SI_OPT_SEED:
	    if( 0==strncmp(optarg, "tippid", 7) ) {
		gettimeofday(&tv, NULL);
		si->seed = (uint32_t)(getpid()*getppid()*tv.tv_usec);
	    } else {
		si->seed = (uint32_t)strtol(optarg, NULL, 10);
		if( errno == EINVAL || errno == ERANGE || si->seed == 0) {
		    DIE("Invalid value for PRNG seed (-s).");
		}
	    }
	    flags[c]++;
	    break;;

	case SI_OPT_CORES:
#if defined(__PPU__) || (defined(__SSE2__) && THR==CMC_THR_ON)
	    si->n_thrs = (uint32_t)strtol(optarg, NULL, 10);
	    if( errno == EINVAL || errno == ERANGE || si->n_thrs < 1 ) {
		DIE("Invalid value for number of " CORE "s (-c).");
	    }
	    flags[c]++;
#else
	    DIE("Built without thread support (with '" CCMD "')");
#endif
	    break;;

	case SI_OPT_INFO:
	    si->header = false;
	    flags[c]++;
	    break;;

	case SI_OPT_VERBOSE:
	    si->verbose = true;
	    flags[c]++;
	    break;;

	case SI_OPT_FILE:
	    si->outfile = strdup(optarg);
	    flags[c]++;
	    break;;

        case ':':
            DIE("Missing argument to option (-%c)", optopt);

        case '?':
            DIE_IF(optopt==0, "Unrecognised option (%s)", argv[optind-1]);
            DIE("Unrecognised option (-%c)", optopt);

        default:
            DIE("Option processing error");
	}
    }
    
    DIE_IF(optind >= argc-1, "Too few remaining arguments after option processing");

    si->n_trjs_reqd = (uint32_t)strtol(argv[optind], NULL, 10);
    if( errno == EINVAL || errno == ERANGE || si->n_trjs_reqd < 1 ) {
	DIE("Invalid value for number of trajectories.");
    }

    si->t_stop = (T_FPS)strtod(argv[optind+1], NULL);
    if( errno == EINVAL || errno == ERANGE || si->t_stop < (T_FPS)0.0 ) {
	DIE("Invalid value for stop time.");
    }

    if( si->outfile != NULL ) {
	fp=fopen(si->outfile, "r");
	if( fp!=NULL ) {
	    fclose(fp);
	    DIE("Output file '%s' already exists", si->outfile);
	}
    }


    /*
     * Now, populate the compile-time members of the struct:
     */
    si->app.exe  = EXE;
    si->app.ccmd = CCMD;
    si->app.ver  = VER;
    si->app.lbl  = LBL;
    si->app.prec = PREC;
    si->app.log  = LOG;
    si->app.rng  = RNG;
    si->app.lpr  = LPR;
    si->app.sso  = SSO;
    si->app.arch = ARCH;
    si->app.prof = PROF;
#if defined(__SSE2__)
    si->app.thr  = THR;
#elif defined(__PPU__)
    si->app.mpi  = MPI;
#endif

    si->app.n_rxns = N_RXNS;
    si->app.n_spxs = N_SPXS;


#if defined(__PPU__)
    /*
     * Find actual number of available SPUs and use that, provided
     * it's not greater than the number requested
     */
    c=spe_cpu_info_get(SPE_COUNT_USABLE_SPES, -1);
    if( c < si->n_thrs ) {
        si->n_thrs = c;
    }
    
    if( si->verbose )
	CLOG("Using %d SPUs, %d available.\n", si->n_thrs, c);
#endif

    _si_calc_n_trjs(si);

    PING();
}



void si_start(sim_info_t *si)
{
    gettimeofday( &(si->ding), NULL );
}


void si_stop(sim_info_t *si)
{
    gettimeofday( &(si->dong), NULL );
}


/* 
 * Helper to subtract the `struct timeval' values X and Y, storing the
 * result in 'result'.  Return 1 if the difference is negative,
 * otherwise 0.
 *
 * Nicked from http://www.gnu.org/software/libtool/manual/libc/Elapsed-Time.html
 */
static int _timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_usec < y->tv_usec) {
	int nsec = (y->tv_usec - x->tv_usec) / _us_per_s + 1;
	y->tv_usec -= _us_per_s * nsec;
	y->tv_sec  += nsec;
    }
    if (x->tv_usec - y->tv_usec > _us_per_s) {
	int nsec = (x->tv_usec - y->tv_usec) / _us_per_s;
	y->tv_usec += _us_per_s * nsec;
	y->tv_sec -= nsec;
    }
     
    /* Compute the time remaining to wait. tv_usec is certainly positive. */
    result->tv_sec  = x->tv_sec  - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;
    
    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}


void si_header(sim_info_t *si, FILE *fp, const char * const pre, const char * const post)
{
    struct timeval diff;		/* Elapsed time		*/
    double ets;				/* Elapsed time in secs */
    char ts[SI_TS_BUFLEN] = {'\0'};	/* Time string		*/
    int rc;				/* G.P. return code	*/

    if( fp==NULL ) {
	fp=stdout;
    }

    rc=strftime(ts, SI_TS_BUFLEN-1, "%F %T", localtime(&si->ding.tv_sec));
    RCHECK(rc, >0, strftime);
    fprintf(fp, "%sSimulation start time             = %s%s\n", pre, ts, post);

    rc=strftime(ts, SI_TS_BUFLEN-1, "%F %T", localtime(&si->dong.tv_sec));
    RCHECK(rc, >0, strftime);
    fprintf(fp, "%sSimulation end time               = %s%s\n", pre, ts, post);

    rc=_timeval_subtract( &diff, &si->dong, &si->ding );
    RCHECK(rc, ==0, _timeval_subtract);
    ets = (double)diff.tv_sec + ((double)diff.tv_usec/(double)_us_per_s);

    fprintf(fp, "%sExecutable name                   = %s%s\n", pre, si->app.exe, post);
    fprintf(fp, "%sExecutable invoked with command   = %s%s\n", pre, si->rcmd, post);
    fprintf(fp, "%sBuilt with CellMC version         = %s%s\n", pre, si->app.ver, post);    
    fprintf(fp, "%sExecutable built with command     = %s%s\n", pre, si->app.ccmd, post);    
    fprintf(fp, "%sModel label                       = %s%s\n", pre, si->app.lbl, post);
    fprintf(fp, "%sFP precision                      = %s%s\n", pre, om_prec_str(si->app.prec), post);
    fprintf(fp, "%slog() implementation              = %s%s\n", pre, om_log_str(si->app.log), post);
    fprintf(fp, "%sPRNG                              = %s%s\n", pre, om_rng_str(si->app.rng), post);
    fprintf(fp, "%sPropensity recalculation limiting = %s%s\n", pre, om_lpr_str(si->app.lpr), post);
    fprintf(fp, "%sSIMD slot optimization            = %s%s\n", pre, om_bool_str(si->app.sso), post);
    fprintf(fp, "%sProfiling                         = %s%s\n", pre, om_bool_str(si->app.prof), post);

#if defined(__SSE2__)
    fprintf(fp, "%sBuilt with multi-threading        = %s%s\n", pre, om_bool_str(si->app.thr), post);
#else
    fprintf(fp, "%sBuilt with multi-threading        = (not available)%s\n", pre, post);
#endif

#if defined(__PPU__)
    fprintf(fp, "%sNumber of SPUs used               = %d%s\n",  pre, si->n_thrs, post);
    fprintf(fp, "%sSPU/PPU block size (bytes)        = %lu%s\n", pre, si->blksz, post);
    fprintf(fp, "%sTotal number of blocks            = %d%s\n",  pre, si->n_blks, post);
    fprintf(fp, "%sNumber of blocks per SPU          = %d%s\n",  pre, si->n_blks_per_thr, post);
#   if SSO==CMC_SSO_OFF
        fprintf(fp, "%sPopulations in result-set         = %d%s\n",  pre, T_NVS, post);
        fprintf(fp, "%sNumber of result-sets per block   = %d%s\n",  pre, si->n_rsets_per_blk, post);
        fprintf(fp, "%sResidual (last block)             = %d%s\n",  pre, si->n_rsets_residual, post);
#   else
        fprintf(fp, "%sNumber of populations per block   = %d%s\n",  pre, si->n_trjs_per_blk, post);
        fprintf(fp, "%sResidual (last block)             = %d%s\n",  pre, si->n_trjs_residual, post);
#   endif
#elif defined(__SSE2__) && THR==CMC_THR_ON
    fprintf(fp, "%sNumber of compute threads         = %d%s\n", pre, si->n_thrs, post);
#endif







#if defined(__PPU__)
    fprintf(fp, "%sBuilt with MPI support            = %s%s\n", pre, om_bool_str(si->app.mpi), post);
#else
    fprintf(fp, "%sBuilt with MPI support            = (not available)%s\n", pre, post);
#endif

    fprintf(fp, "%sPRNG master seed                  = 0x%08"PRIx32"%s\n", pre, si->seed, post);

    fprintf(fp, "%sNumber of reactions in model      = %d%s\n", pre, si->app.n_rxns, post);
    fprintf(fp, "%sNumber of species in model        = %d%s\n", pre, si->app.n_spxs, post);

    fprintf(fp, "%sNumber of trajectories requested  = %d%s\n", pre, si->n_trjs_reqd, post);
    if( si->n_trjs_reqd != si->n_trjs ) {
	fprintf(fp, "%sNumber of trajectories computed   = %d%s\n", pre, si->n_trjs, post);
    }
#if defined(__SSE2__) && THR==CMC_THR_ON
    fprintf(fp, "%sNumber of trajectories per thread = %d%s\n", pre, si->n_trjs_per_thr, post);
#endif

    fprintf(fp, "%sSimulated time                    = %f%s\n", pre, si->t_stop, post);

    fprintf(fp, "%sElapsed simulation time (seconds) = %.3f%s\n", pre, ets, post);

    fprintf(fp, "%sEffective time per trajectory (s) = %.3fs%s\n", pre, ets/(double)si->n_trjs, post);

    if( si->abs_rxn_total != 0 ) {
	fprintf(fp, "%sAbsolute total reactions executed = %"PRIu64"%s\n", pre, si->abs_rxn_total, post);
	fprintf(fp, "%sAbsolute simulation speed (Rps)   = %.0f%s\n", pre, (double)si->abs_rxn_total/ets, post);
    }
    else
	fprintf(fp, "%sAbsolute total reactions executed = (not recorded)%s\n", pre, post);

    if( si->con_rxn_total != 0 ) {
	fprintf(fp, "%sContributing total reactions      = %"PRIu64"%s\n", pre, si->con_rxn_total, post);
	fprintf(fp, "%sEffective simulation speed (Rps)  = %.0f%s\n", pre, (double)si->con_rxn_total/ets, post);
	fprintf(fp, "%sReactions per trajectory (RpT)    = %.0f%s\n", pre, (double)si->con_rxn_total/(double)si->n_trjs, post);
    }
    else
	fprintf(fp, "%sContributing total reactions      = (not recorded)%s\n", pre, post);

    if( si->con_rxn_total && si->abs_rxn_total )
	fprintf(fp, "%sReaction sim. overhead (%%)        = %2.2f%s\n", pre, 100.0*((double)si->abs_rxn_total/(double)si->con_rxn_total-1.0), post);

}



void si_print_results(sim_info_t * const si)
{
    FILE *fp = stdout;

    PING();

    if( si->outfile != NULL ) {
	fp=fopen(si->outfile, "w");
	RCHECK(fp, !=NULL, fopen);
    }

#if PROF==CMC_PROF_OFF
    if( si->header ) {
	si_header(si, fp, "# ", "");
    }

    fp_print(si, fp);
#else
    rs_init(si); 
    rs_print_sro_xslt(si, fp);
#endif

    if( fp != stdout ) {
	fclose(fp);
    }

    return;
}



void si_cleanup(sim_info_t * const si)
{
    free(si->rcmd);
#if PROF==CMC_PROF_ON
    if( si->helper_cleanup_fn != NULL ) {
	si->helper_cleanup_fn((void *)si);
	si->helper_data = NULL;
	si->helper_cleanup_fn = NULL;
    }
#endif
    PING();
}
