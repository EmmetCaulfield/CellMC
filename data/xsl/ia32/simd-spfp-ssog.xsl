<?xml version="1.0"?>
<!--
 | @file    simd.xsl
 | @brief   To transform an SBML model to C code
 | @author  Emmet Caulfield.
 | @version 0.1
 |
 | $Id: simd-spfp-ssog.xsl 90 2009-02-03 09:28:56Z emmet $
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
#if defined(SIG_DUMPERS)
uint64_t *absp, *conp;
uv_t *psump, *tp, *flgp, *popnp; 
int *kp;
static void _catch_usr1(int arg) {
    (void)arg;
    fprintf(stderr, "-----\nRC: %llu/%llu @%d\n", *conp, *absp, *kp);
    uv_print_sf("psum ", (*psump) );
    uv_print_sf("t    ", (*tp)    );
    uv_print_si("flg  ", (*flgp)  );
}

static void _catch_abrt(int arg) {
    int i;
    (void)arg;
    for(i=0; i&lt;N_SPECIES; i++) {
        printf("%3d ", i);
        uv_print_si("-", popnp[i]);
    }
}
#endif


#if PROF==CMC_PROF_OFF
    static T_PIS *saved = NULL;	/* Saved populations		*/
#   define SAVED(T,S) saved[T*N_SPECIES+S]
#else
    static T_PIS saved[N_REACTIONS] = {0};
#   if THR==CMC_THR_ON
        static pthread_mutex_t saved_mutex = PTHREAD_MUTEX_INITIALIZER;
#   endif
#endif

typedef struct {
    int thrid;		/* Thread ID				*/
    uint32_t seed;      /* RNG seed				*/
    float t_stop;	/* Stop-time				*/
    int k_lo;		/* Starting index			*/
    int k_hi;		/* Ending index	+ 1			*/
    uint64_t absrc;	/* Absolute number of reactions		*/
    uint64_t conrc;	/* Number of contributing reactions	*/
} ssa_thread_args_t;


void *ssa_thread(void *args)
{
    ssa_thread_args_t *arg;
    T_FPV t_stopv;		/* Stop-time as a 4xSPFP vector */

#if RNG == CMC_RNG_RSMT
    rm_state_t rs_obj;          /* RSMT RNG state object        */
    rm_state_t *rs;             /* RSMT RNG state =&amp;rs_obj  */
#elif RNG == CMC_RNG_STDLIB
    unsigned short rs[3];       /* erand48()-family state       */
#endif

#if THR == CMC_THR_ON &amp;&amp; defined(OS_LINUX)
    cpu_set_t affty;		/* Thread/CPU affinity mask	*/
#endif

    uv_t popn[N_SPECIES];	/* Populations			*/
    uv_t prop[N_REACTIONS];	/* Propensities			*/

#if PROF==CMC_PROF_ON &amp;&amp; THR==CMC_THR_ON
    T_PIS rcount[N_REACTIONS]={0};	/* Reaction counts	*/
#endif

    uv_t t;			/* Elapsed simulation time	*/
    uv_t tau;			/* Timestep			*/
    uv_t tea;			/* Kahan tau error accumulator	*/

    uv_t r_sum, choice;

    int nr[UV_4]={0,0,0,0};	/* Number of reactions in slot	*/
    uint64_t absrc = 0LL;	/* Absolute number of reactions	*/
    uint64_t conrc = 0LL;	/* Contributing number of rxns	*/

    uv_t flg;
    float r;

    int e;			/* SIMD vector element/slot iterator	*/
    int i;			/* G.P. Iterator			*/
    int k = 0;			/* Saved population index		*/

</xsl:text><xsl:if test="$LPR = 'full'">
    register float oldr; /* Temporary partial propensity sum accumulator */
    register float newr; /* Temporary partial propensity sum accumulator */
</xsl:if><xsl:text>
    arg     = (ssa_thread_args_t *)args;

#if defined(SIG_DUMPERS)
    absp     = &amp;absrc;
    conp     = &amp;conrc;
    kp       = &amp;k;
    psump    = &amp;r_sum;
    flgp     = &amp;flg;
    tp       = &amp;t;
    popnp    = popn;
    signal(SIGUSR1, _catch_usr1);
    signal(SIGABRT, _catch_abrt);
#endif

/*
 * If we're on Linux, we can use CPU affinity to avoid
 * scaling problems on the last processor.
 */
#if THR==CMC_THR_ON &amp;&amp; defined(OS_LINUX)
    CPU_ZERO(&amp;affty);
    CPU_SET(arg->thrid, &amp;affty);
    sched_setaffinity(syscall(SYS_gettid), sizeof(cpu_set_t), &amp;affty);
#endif


#if RNG==CMC_RNG_RSMT
    rs = &amp;rs_obj;
#endif

    t.sf    = UV_0_4sf;
    tau.sf  = UV_0_4sf;
    tea.sf  = UV_0_4sf;

    flg.si = UV_0_4si;

    t_stopv = (v4sf){arg->t_stop,arg->t_stop,arg->t_stop,arg->t_stop};

    // Seed the PRNG
    _vutil_srand( rs, (uint32_t)arg-&gt;seed );

</xsl:text>
    <!-- Initialize the populations -->
    <xsl:call-template name="init-popn"/>
    <xsl:text>
    _update_props(prop,popn);
    PING();
    for(k=arg->k_lo; k&lt;arg->k_hi; /* Yes, I mean it. */) {
</xsl:text>
        /*
         * flg.i32[e] will be set iff t in slot e is larger than t_stop
         */
        flg.sf = _mm_cmple_ps(t_stopv,t.sf);
        if( flg.i32[0] || flg.i32[1] || flg.i32[2] || flg.i32[3] ) {
            for(e=0; e&lt;UV_4; e++) {
                if( flg.i32[e] ) {
                    // Trajectory in e has reached its final time
	            k++;
#if PROF==CMC_PROF_OFF
	            // Save the population in slot e and reset it to the initial
                    // value
                    for(i=0; i&lt;N_SPECIES; i++) {
                        SAVED((k-1),i)=SPOP(i,e);
		        SPOP(i,e)=ipop[i];
                    }
#endif
                    _update_props(prop,popn);
                    // Reset the time and reaction counters associated with slot e
                    conrc   += nr[e];
		    nr[e]    = 0;
		    t.f[e]   = 0.0f;
		    tea.f[e] = 0.0f;
	        }
            }
        }

    <!-- Compute the propensities and sum them -->
    <xsl:if test="$LPR = 'none'">
        <xsl:text>        _update_props(prop, popn);&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="$LPR != 'full'">
        <xsl:text>        r_sum.sf=_sum_props(prop);&#10;</xsl:text>
    </xsl:if>

    <xsl:text>
        /* Generate reaction-selector value, a PRN on (0,r_sum) */
        choice.sf = r_sum.sf * _vutil_rand(rs);

	/*
         * Compute a timestep
         */
        tau.sf = _vutil_tau(rs, r_sum.sf);

	/* For each slot, e, of the SIMD vector */
	for(e=0; e&lt;UV_4; e++) {
	    nr[e]++;
            r=choice.f[e]; // reaction-selector in slot e

	    /*
             * Successively subtract propensities from the reaction-selector
	     * value (RSV) until it becomes negative: the reaction index is then
             * one less. For example, if the RSV is 0.5 and the first propensity
             * (at index 0) is 1.0, 'i' will be 1 after exiting the loop and
             * has to be decremented.
             */
            for(i=0; r&gt;=0.0f &amp;&amp; i&lt;N_REACTIONS; i++) {
		r -= prop[i].f[e];
	    }
	    i--;

            absrc++;
#if PROF==CMC_PROF_ON
#   if THR==CMC_THR_OFF
            saved[i]++;
#   else
            rcount[i]++;
#   endif
#endif
</xsl:text>
                <xsl:call-template name="switch"/>
<xsl:text>
	} /* for(e=1:FPV) */

        /*
         * Update time using Kahan compensation: we use inline assembly mostly to
         * defend against aggressive optimization problems.
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
    }  /* k=for(k_lo:k_hi) */

    arg->absrc=absrc;
    arg->conrc=conrc;

#if THR==CMC_THR_ON &amp;&amp; PROF==CMC_PROF_ON
    pthread_mutex_lock( &amp;saved_mutex );
    for(i=0; i&lt;N_REACTIONS; i++) {
       saved[i]+=rcount[i];
    }
    pthread_mutex_unlock(&amp;saved_mutex);
#endif

    PING();

#if THR==CMC_THR_ON
    pthread_exit( NULL );
#endif
    return(NULL);
}
</xsl:text>
  </xsl:template>

  <xsl:template name="main">
    <xsl:call-template name="ssa-thread"/>
    <xsl:text>
/*
 * [name="main"]
 */
int main(int argc, char *argv[])
{
    sim_info_t info;		/* Info, CLAs, etc.		*/    

#if THR==CMC_THR_OFF
    ssa_thread_args_t  targs;
#else
    ssa_thread_args_t *targs;	/* Thread arguments	*/
    pthread_t         *thread;
    pthread_attr_t     tattr;

    int i;			/* G.P iterator		*/
#endif

#if PROF==CMC_PROF_OFF
    void *vpp;
#endif
#if THR==CMC_THR_ON
    int rc;
#endif

    si_init(&amp;info, argc, argv);

#if PROF==CMC_PROF_OFF
    MALLOC_ALIGN16(vpp, (info.n_trjs*N_SPECIES*sizeof(T_PIS)));
    RCHECK(vpp, !=NULL, MALLOC_ALIGN);
    saved = vpp;
#endif

    info.results=(void *)saved;

#if THR==CMC_THR_ON
    thread = (pthread_t *)malloc(info.n_thrs*sizeof(pthread_t));
    RCHECK(thread, !=NULL, malloc);

    targs = (ssa_thread_args_t *)malloc(info.n_thrs*sizeof(ssa_thread_args_t));
    RCHECK(targs, !=NULL, malloc);
#endif

#if THR==CMC_THR_OFF
    targs = (ssa_thread_args_t){0, info.seed, info.t_stop, 0, info.n_trjs, 0LL, 0LL};

    si_start(&amp;info);
    ssa_thread((void *)&amp;targs);
    si_stop(&amp;info);
    info.abs_rxn_total = targs.absrc;
    info.con_rxn_total = targs.conrc;
#else
    srand48(info.seed);
    rc = info.n_trjs_per_thr;
    for(i=0; i&lt;info.n_thrs; i++) {
        targs[i] = (ssa_thread_args_t){i, lrand48(), info.t_stop, i*rc, (i+1)*rc, 0LL, 0LL};
    }

    pthread_attr_init(&amp;tattr);
    pthread_attr_setdetachstate(&amp;tattr, PTHREAD_CREATE_JOINABLE);
//    pthread_attr_setstacksize(&amp;tattr, (N_SPECIES+N_REACTIONS)*sizeof(uv_t)+2*PTHREAD_STACK_MIN);

    si_start(&amp;info);
    for(i=0; i&lt;info.n_thrs; i++) {
        rc=pthread_create(&amp;thread[i], &amp;tattr, ssa_thread, (void *)&amp;targs[i]);
        RCHECK(rc, ==0, pthread_create);
    }

    for(i=0; i&lt;info.n_thrs; i++) {
        rc = pthread_join(thread[i], NULL);
        RCHECK(rc, ==0, pthread_join);

	info.abs_rxn_total += targs[i].absrc;
	info.con_rxn_total += targs[i].conrc;
    }
    si_stop(&amp;info);

    pthread_attr_destroy(&amp;tattr);
#endif

    si_print_results(&amp;info);
    si_cleanup(&amp;info);

#if PROF==CMC_PROF_OFF
    FREE_ALIGN16(saved);
#endif

#if THR==CMC_THR_ON
    free(thread);
    free(targs);
    pthread_exit( NULL );
#endif

    return EXIT_SUCCESS;
}
</xsl:text>

  </xsl:template> <!-- main -->
<!--================================================-->


<!--
=======================================================
Generate the update_props function
======================================================= 
-->
  <xsl:template name="update-props">
    <!-- Function header -->
    <xsl:text>
/*
 * [name="update-props"]
 */

/*
 * Sum an array of reaction propensities:
 */
static inline v4sf _sum_props(const uv_t prop[])
{
    int i;
    v4sf r_sum=UV_0_4sf;

    for(i=N_REACTIONS-1; i&gt;=0; i--) {
        r_sum += prop[i].sf;
    }
    return r_sum;
}


/*
 * Update reaction propensities
 */
static inline void _update_props(uv_t prop[], uv_t popn[])
{
</xsl:text>
    <xsl:apply-templates match="/s:sbml/s:model/s:listOfReactions/s:reaction" mode="props" />
<xsl:text>
}
</xsl:text>
  </xsl:template>

  <xsl:template match="s:reaction" mode="props">
    <xsl:text>    prop[i_</xsl:text>
    <xsl:value-of select="./@id"/>
    <xsl:text>].sf=VEQN_</xsl:text>
    <xsl:value-of select="./@id"/>
    <xsl:text>;&#10;</xsl:text>
  </xsl:template>
<!--================================================-->


</xsl:transform>
