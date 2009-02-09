<?xml version="1.0"?>
<!--
 | @file    simd.xsl
 | @brief   To transform an SBML model to C code
 | @author  Emmet Caulfield.
 | @version 0.1
 |
 | $Id: simd-single.xsl 28 2009-01-20 23:45:04Z emmet $
-->

<!-- Preamble: -->
<xsl:transform
  xmlns:s="http://www.sbml.org/sbml/level2/version3"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:svn="http://svnbook.red-bean.com/en/1.4/svn.advanced.props.special.keywords.html"
  xmlns:k="http://polacksbacken.net/wiki/SSACBE"
  xmlns:m="http://www.w3.org/1998/Math/MathML"
  version="1.0"
>

  <xsl:import href="common.xsl" />

<!--
=======================================================
Generate the main function
======================================================= 
-->
  <xsl:template name="ssa-thread">
    <xsl:text>
static int *saved = NULL;	/* Saved populations		*/
static int spi = 0;		/* Saved population index       */

static pthread_mutex_t spi_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * We only ever write these variables *before* we launch other
 * threads, so it should be safe to read them, although it might
 * not be a good idea anyway.
 */
static v4sf t_stopv;		/* Stop time			*/
static int  n_trjs;		/* Number of trajectories (CLA) */

#define SAVED(T,S) saved[T*N_SPECIES+S]

typedef struct {
    uint32_t seed;
} ssa_thread_args_t;

void *ssa_thread(void *args)
{
    ssa_thread_args_t *arg;

#if RNG == CMC_RNG_RSMT
    rm_state_t rs_obj;          /* RSMT RNG state object        */
    rm_state_t *rs;             /* RSMT RNG state =&amp;rs_obj  */
#elif RNG == CMC_RNG_STDLIB
    unsigned short rs[3];       /* erand48()-family state       */
#endif

    uv_t popn[N_SPECIES];	/* */
    uv_t rate[N_REACTIONS];	/* Propensities			*/

    uv_t t;			/* Elapsed simulation time	*/
    uv_t tau;			/* Timestep			*/
    uv_t tea;			/* Kahan tau error accumulator	*/

    uv_t r_sum, choice;

    int nr[UV_4]={0,0,0,0};	/* Number of reactions in slot	*/
    uint64_t rc = 0L;		/* Total number of reactions	*/

    uv_t flg;
    float r;

    int e;			/* SIMD vector element/slot iterator */

</xsl:text><xsl:if test="$LPR = 'full'">
    register float oldr; /* Temporary partial propensity sum accumulator */
    register float newr; /* Temporary partial propensity sum accumulator */
</xsl:if><xsl:text>
    int i;

#if RNG==CMC_RNG_RSMT
    rs = &amp;rs_obj;
#endif

    t.sf    = UV_0_4sf;
    tau.sf  = UV_0_4sf;
    tea.sf  = UV_0_4sf;

    flg.si = UV_1_4si;

    arg = (ssa_thread_args_t *) args;
    
    _vutil_srand( rs, (uint32_t)arg->seed );
</xsl:text>
    <xsl:call-template name="init-popn"/>
    <xsl:if test="$LPR != 'none'">
        <xsl:text>    _update_rates(rate, popn);&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="$LPR = 'full'">
        <xsl:text>    r_sum.sf=SUM_RATES;&#10;</xsl:text>
    </xsl:if>

    <xsl:text>
    
    for( ;; ) { /* We pthread_exit() explicitly */
</xsl:text>
    <xsl:if test="$LPR = 'none'">
        <xsl:text>        _update_rates(rate, popn);&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="$LPR != 'full'">
        <xsl:text>        r_sum.sf=SUM_RATES;&#10;</xsl:text>
    </xsl:if>

    <xsl:text>
        choice.sf = r_sum.sf * _vutil_rand(rs);

	for(e=0; e&lt;UV_4; e++) {
	    nr[e]++;
            r=choice.f[e];

	    /*
             * Fast reactions are at low indices
             */
#if defined(CUMULATIVE_SUM_ARRAY)
            for(i=1; r&lt;cumr[i].f[e]; i++)
                ; // Yes, I mean it!
#else
            for(i=0; r&gt;0.0f &amp;&amp; i&lt;=N_REACTIONS; i++) {
		r -= rate[i].f[e];
	    }
#endif
	    --i;

</xsl:text>
                <xsl:call-template name="switch"/>
<xsl:text>

            if( ! flg.i32[e] ) {
                pthread_mutex_lock( &amp;spi_mutex );
#if defined(REPORT_ALL)
	        fprintf(stderr, "%d %d %d\n", spi, e, nr[e]);
#endif
	        if( spi &lt; n_trjs ) {
	            for(i=0; i&lt;N_SPECIES; i++) {
			  SAVED(spi,i)=SPOP(i,e);
                    }
		    SPOP(i,e)=ipop[i];
                    spi++;
	        }
                if( spi &lt; n_trjs ) {
                    pthread_mutex_unlock( &amp;spi_mutex );
                } else {
		    pthread_mutex_unlock( &amp;spi_mutex );
		    pthread_exit(NULL);
		}
</xsl:text>
    <xsl:if test="$LPR != 'none'">
      <xsl:text>                _update_rates(rate, popn);&#10;</xsl:text>
    </xsl:if>
<xsl:text>
		rc += nr[e];
		nr[e]    = 0;
		t.f[e]   = 0.0f;
		tea.f[e] = 0.0f;
	    }

	} /* for(e=1:FPV) */

        tau.sf = _vutil_tau(rs, r_sum.sf);

        /*
         * Kahan time summing: we use inline assembly mostly to defend
         * against aggressive optimization problems.
         */
	__asm__ (
            "subps   %[tea],  %[tau]   \n" // ty -= tea [ty = tau-tea]
            "movaps    %[t],  %%xmm7   \n" // tt  = t   
            "addps   %[tau],  %%xmm7   \n" // tt += ty  [tt = t+ty]
	    "movaps  %%xmm7,  %[tea]   \n" // tea = tt
            "subps     %[t],  %[tea]   \n" // tt -= t
            "subps   %[tau],  %[tea]   \n" // tt -= ty  [tea=(tt-t)-ty]
            "movaps  %%xmm7,    %[t]   \n" // t   = tt  [t=tt]
            : [t]"+&amp;x"(t.sf), [tea]"+&amp;x"(tea.sf)
            : [tau]"x"(tau.sf)
	    : "%xmm7"
        );

        flg.sf = _mm_cmpgt_ps(t_stopv,t.sf);

    }  /* n=for(1:n_trjs) */

    /*
     * We should never actually get here.
     */
    pthread_exit(NULL);
}
</xsl:text>
  </xsl:template>

  <xsl:template name="main">
    <xsl:call-template name="ssa-thread"/>
    <xsl:text>
/*
 * [name="main"]
 */
int main(const int argc, const char *const argv[])
{
    unsigned seed=3987654321U;	/* RNG seed			*/
    float t_stop;		/* Stop time read for CLAs	*/
    int n_thrs;			/* Number of threads to launch  */
    int i, j;
    int rc;

    ssa_thread_args_t *targs;
    pthread_t         *thread;
    pthread_attr_t     tattr;


</xsl:text>
    <!-- This will set t_stop, n_trjs, and (maybe) seed -->
    <xsl:call-template name="getopts"/>
<xsl:text>
    t_stopv = (v4sf){t_stop,t_stop,t_stop,t_stop};

    saved=(int *)malloc(n_trjs*N_SPECIES*sizeof(int));
    RCHECK(saved!=NULL, malloc);

    n_thrs=1;
    
    thread = (pthread_t *)malloc(n_thrs*sizeof(pthread_t));
    RCHECK(thread!=NULL, malloc);

    targs = (ssa_thread_args_t *)malloc(n_thrs*sizeof(ssa_thread_args_t));
    RCHECK(targs!=NULL, malloc);

    srand48(seed);
    for(i=0; i&lt;n_thrs; i++) {
        targs[i].seed = (uint32_t)lrand48();
    }

    <!-- ************************************ -->
    <!-- * WRITE PTHREAD STARTUP CODE HERE! * -->
    <!-- ************************************ -->
    pthread_attr_init(&amp;tattr);
    pthread_attr_setdetachstate(&amp;tattr, PTHREAD_CREATE_JOINABLE);
//    pthread_attr_setstacksize(&amp;tattr, (N_SPECIES+N_REACTIONS)*sizeof(uv_t)+2*PTHREAD_STACK_MIN);

    for(i=0; i&lt;n_thrs; i++) {
        rc=pthread_create(&amp;thread[i], &amp;tattr, ssa_thread, (void *)&amp;targs[i]);
        RCHECK(rc==0, pthread_create);
    }

    for(i=0; i&lt;n_thrs; i++) {
        rc = pthread_join(thread[i], NULL);
        RCHECK(rc==0, pthread_join);
    }

    pthread_attr_destroy(&amp;tattr);


#if defined(DUMP_RESULTS)
    for(i=0; i&lt;n_trjs; i++) {
        for(j=0; j&lt;N_SPECIES-1; j++) {
            printf("%d ", SAVED(i,j));
        }
        printf("%d\n", SAVED(i,j));
    }
#else
    fprintf(stderr, "Total reactions: " FS_U64 "\n", rc);
#endif

    // Free the memory we've allocated:
    free(saved);
    free(thread);
    free(targs);

    pthread_exit(NULL);
    return EXIT_SUCCESS;
}
</xsl:text>

  </xsl:template> <!-- main -->
<!--================================================-->


<!--
=======================================================
Generate the update_rates function
======================================================= 
-->
  <xsl:template name="update-rates">
    <!-- Function header -->
    <xsl:text>
/*
 * [name="update-rates"]
 */

/*
 * Sum an array of reaction propensities:
 */
#if defined(CUMULATIVE_SUM_ARRAY)
static inline v4sf _sum_rates(uv_t rate[], uv_t cumr[]) 
#else
static inline v4sf _sum_rates(uv_t rate[])
#endif
{
    int i;
    v4sf r_sum=UV_0_4sf;

    for(i=N_REACTIONS-1; i&gt;=0; i--) {
        r_sum += rate[i].sf;
#if defined(CUMULATIVE_SUM_ARRAY)
        cumr[i].sf = r_sum;
#endif
    }
    return r_sum;
}
#if defined(CUMULATIVE_SUM_ARRAY)
#   define SUM_RATES _sum_rates(rate, cumr) 
#else
#   define SUM_RATES _sum_rates(rate)
#endif


/*
 * Update reaction propensities
 */
static inline void _update_rates(uv_t rate[], uv_t popn[])
{
</xsl:text>
    <xsl:apply-templates match="/s:sbml/s:model/s:listOfReactions/s:reaction" mode="rates" />
<xsl:text>
}
</xsl:text>
  </xsl:template>

  <xsl:template match="s:reaction" mode="rates">
    <xsl:text>    rate[i_</xsl:text>
    <xsl:value-of select="./@id"/>
    <xsl:text>].sf=VEQN_</xsl:text>
    <xsl:value-of select="./@id"/>
    <xsl:text>;&#10;</xsl:text>
  </xsl:template>
<!--================================================-->


</xsl:transform>
