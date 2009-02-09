/**
 * @file   cdssak.c
 * @brief  Model-independent PPU program for direct-method SSA
 * @author Emmet Caulfield
 *
 * $Id: main.c 22 2009-01-20 00:18:29Z emmet $
 */
#include <cellmc.h>
#include <inttypes.h>	/* uint32_t	*/
#include <stdlib.h>	/* exit()	*/
#include <unistd.h>	/* getpid()	*/
#include <string.h>

/*
 * #includes for pretty much every PPU program.
 */
#include <altivec.h>
#include <vec_types.h>
#include <libspe2.h>
#include <pthread.h>
#include <libmisc.h>

#include <resblk.h>   /* Common result block struct definition */
#include <ctrlblk.h>  /* Common control block struct */
#include <cb_utils.h> /* PPU-only control-block utilities */
#include <propstat.h> /* Propensity statistics handling */
#include <sanity_checks.h>

/*
 * Maximum number of available SPUs:
 */
#define N_SPUS_MAX (16)

/*
 * Number of populations in a vector:
 */ 
#define POPS_PER_VECTOR (4)

/*
 * SPU program handle.
 */
extern spe_program_handle_t CMC_SPU_CODE_HANDLE_NAME;


/*
 * Control blocks for each SPU
 */
ctrlblk_t cb[N_SPUS_MAX] __attribute__((aligned(128)));


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


/*
 * Handy utility functions
 */
static void _dump_resblk(FILE *fp, resblk_t *rb, uint16_t rpb, uint16_t n_species) 
{
    uint8_t   i;
    uint16_t  t, s;
    int32_t  *u;

    /* rpb = results per block */
//    fprintf(fp, "=======\n");
//    fprintf(fp, "%3u:%u:%u\n", rpb, POPS_PER_VECTOR, n_species);
//    fprintf(fp, "-------\n");
    for(t=0; t<rpb; t++) {
        for(i=0; i<POPS_PER_VECTOR; i++) {
            for(s=0; s<n_species; s++) {
                u=(int32_t *)&(rb->data[ t*n_species+s ]);
                fprintf(fp, "%u\t", u[i]);
//		fprintf(fp, "%u:%u:%u\t", t,i,s);
            }
            fputc('\n', fp);
        }
    }
}


static void fprint_vec_uint4_array(FILE* fp, vec_uint4 *v, int len) {
    int i;
    uint32_t *a;

    fprintf(fp, "PPU| Printing %d vectors >\n", len);
    for(i=0; i<len; i++) {
	a=(uint32_t *)&v[i];
	fprintf(fp, "PPU| %u %u %u %u\n", a[0], a[1], a[2], a[3]); 
    }
}


static void _usage(const char const *msg) {
    fprintf(stderr, "PPU| USAGE: cdssak n_trjs t_stop\n\t%s\n", msg); 
    exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
    int i, j, n_spus;

    spe_context_ptr_t spu_context[N_SPUS_MAX];
    pthread_t         spu_thread[N_SPUS_MAX];
    thread_args_t     thread_args[N_SPUS_MAX];
    uint32_t          mail[N_SPUS_MAX];

    vec_int4         *vu;
    vec_int4          zero = (vec_int4){0,0,0,0};

    propstat_t       *propstats;

    uint32_t n_trjs;
    float    t_stop;

    uint32_t total_blocks;
    uint32_t blocks_per_spu;
    uint16_t n_species;
    uint16_t n_reactions;

    volatile resblk_t *result_block;
    size_t   rb_size;
    uint16_t rsets_per_blk;
    uint16_t rsets_residual;

    uint32_t tmp;

    char buf[128];
    FILE *fp;


    /*
     * Get the number of trajectories and steps from the command-line
     */
    if( argc != 3 ) {
	_usage("Wrong number of arguments");
    }
    n_trjs  = (uint32_t)strtol(argv[1], NULL, 10);
    if( n_trjs < 1 ) {
	_usage("n_trjs cannot be less than one");
    }
    t_stop = (float)strtod(argv[2], NULL); 
    if( t_stop <= 0 ) {
	_usage("t_stop cannot be less than zero");
    }


    /*
     * Will use *rand48() family to generate seeds for the MT RNGs on
     * the SPUs
     */ 
    srand48( getpid()*getppid() );

    /*
     * Find actual number of available SPUs and use that, provided
     * it's not greater than the maximum number.
     */
    n_spus=spe_cpu_info_get(SPE_COUNT_USABLE_SPES, -1);
    if( n_spus > N_SPUS_MAX ) {
        n_spus = N_SPUS_MAX;
    }
    fprintf(stderr, "PPU| %u available SPUs\n", n_spus);

    /*
     * Do common initialisation for SPUs
     */
    for(i=0; i<n_spus; i++) {
	if( NULL==(spu_context[i]=spe_context_create(0,NULL)) ) {
	    perror("Failed creating SPU context");
	    exit(EXIT_FAILURE);
	}
	thread_args[i].spu_context = spu_context[i];
	thread_args[i].spu_argp    = &cb[i];

	/*
	 * The SPU programs wait for the "starting gun", so we can load
	 * and start them here.
	 */
	if( spe_program_load(spu_context[i], &spu_model_text) ) {
	    perror("Failed to load SPU program");
	    exit(EXIT_FAILURE);
	}
	if( pthread_create(&spu_thread[i], NULL, &ppu_thread_mgr, &thread_args[i]) ) {
	    perror("Failed to create SPU thread");
	    exit(EXIT_FAILURE);
	}
    }
    fprintf(stderr, "PPU| Preliminary initialization complete\n");


    /*
     * Query the first SPU (zero) for model parameters.
     */
    for(i=0; i<n_spus; i++) {
	mail[i]=i;
	spe_in_mbox_write(spu_context[i], &mail[i], 1, SPE_MBOX_ANY_NONBLOCKING);
    }
    spe_out_intr_mbox_read(spu_context[0], &mail[0], 1, SPE_MBOX_ALL_BLOCKING);
    fprintf(stderr, "PPU| Model parameters received\n");


    /*
     * The model-specific data is now in cb[0], fill in computed values and
     * copy the results to the other CBs.
     */
    cb_fillin(&cb[0], n_trjs, n_spus, t_stop);
    for(i=1; i<n_spus; i++) {
	cb_copy(&cb[i], &cb[0]);
    }
    n_species      = cb[0].n_species;
    n_reactions    = cb[0].n_reactions;
    blocks_per_spu = cb[0].n_blocks;
    total_blocks   = n_spus * blocks_per_spu;
    rsets_per_blk  = cb[0].rsets_per_block;
    rsets_residual = cb[0].rsets_residual;

    /*
     * Allocate memory for the results, allowing an extra block for
     * reaction counts.
     */
    rb_size = (total_blocks+n_spus) * cb[0].block_size;
    result_block = (volatile resblk_t *)malloc_align( rb_size, 7 );
    fprintf(stderr, "PPU| %u bytes allocated at %p\n", rb_size, result_block);
    if( NULL == result_block ) {
	perror("Failed to allocate memory for results");
	exit(EXIT_FAILURE);
    }

    /*
     * Do SPU-specific initialization, and start the SPUs
     */
    for(i=0; i<n_spus; i++) {
	cb[i].rng_seed    = (uint32_t)lrand48();
	tmp = (uint32_t)&result_block[ i * (blocks_per_spu+1) ];
	cb[i].result_base = 0x00000000ffffffff & (uint64_t)tmp;
	spe_in_mbox_write(spu_context[i], &tmp, 1, SPE_MBOX_ANY_NONBLOCKING);

	/* Tell the SPUs we want reaction summary data */
	mail[i]=i+1;
	spe_in_mbox_write(spu_context[i], &mail[i], 1, SPE_MBOX_ANY_NONBLOCKING);
    }
    fprintf(stderr, "PPU| SPUs started\n");
    cb_dump(stderr, &cb[0]);


    /*
     * Now, wait for all the SPU programs to finish
     */
    for(i=0; i<n_spus; i++) {
	if( pthread_join(spu_thread[i], NULL) ) {
	    perror("Thread join failed");
	    exit(EXIT_FAILURE);
	}
	if( 0!=(spe_context_destroy(spu_context[i])) ) {
	    perror("Failed to destroy SPU context");
	    exit(EXIT_FAILURE);
	}
    }
    fprintf(stderr, "PPU| SPUs finished\n");


    /*
     * Print out XSL to reorder reactions:
     */
    cb_hr_hash(&cb[0], buf);
    strncat(buf, ".xsl", 4);
    fp=fopen(buf, "w");
    DIE_UNLESS(fp!=NULL, "fopen");

    propstats=propstat_alloc(n_reactions);
    for(i=0; i<n_spus; i++) {
	vu=(vec_int4 *)&result_block[i*(blocks_per_spu+1)+blocks_per_spu];
	for(j=0; j<n_reactions; j++) {
	    propstats[j].sum += vec_extract(vec_sums(vu[j], zero), 3);
	}
    }
    fprint_sro_xsl(fp, propstats, n_reactions, cb[0].model_name);
    fclose(fp);


    /*
     * Print out raw reaction propensity statistics
     */
    strncpy(buf+strlen(buf)-4, ".cnt", 4);
    fp=fopen(buf, "w");
    DIE_UNLESS(fp!=NULL, "fopen");
    fprint_prop_counts(fp, propstats, n_reactions);
    fclose(fp);

    propstat_free(propstats);


    /*
     * Print out results
     */
    strncpy(buf+strlen(buf)-4, ".dat", 4);
    fp=fopen(buf, "w");
    DIE_UNLESS(fp!=NULL, "fopen");
    for(i=0; i<n_spus; i++) {
	for(j=0; j<blocks_per_spu-1; j++) {
	    _dump_resblk( fp, &result_block[i*(blocks_per_spu+1)+j], rsets_per_blk, n_species);
	}
	_dump_resblk( fp, &result_block[i*(blocks_per_spu+1)+j], rsets_residual, n_species);
    }
    fclose(fp);




    fprintf(stderr, "PPU| base=%p\n", result_block);
    free_align( (void *)result_block );

    return 0;
}

