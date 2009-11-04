<?xml version="1.0"?>
<!--
 | @file    simd.xsl
 | @brief   To transform an SBML model to C code
 | @author  Emmet Caulfield.
 | @version 0.1
 |
 | $Id: simd-single.xsl 75 2009-01-29 22:44:23Z emmet $
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
  <xsl:template name="main">
    <xsl:text>
/*
 * [name="main"]
 */
int main(int argc, char *argv[])
{
//    char   outfile[FILENAME_MAXLEN];

#if RNG == CMC_RNG_RSMT
    rm_state_t rs_obj;		/* RSMT RNG state object	*/
    rm_state_t *rs;		/* RSMT RNG state =&amp;rs_obj	*/
#elif RNG == CMC_RNG_STDLIB
    unsigned short rs[3];	/* erand48()-family state	*/
#endif

    sim_info_t info;            /* Common configuration struct  */

    uv_t popn[N_SPECIES];	/* Current population		*/
    uv_t prop[N_REACTIONS];	/* Propensities			*/
#if defined(CUMULATIVE_SUM_ARRAY)
    uv_t cumr[N_REACTIONS];	/* Cumulative propensity sums	*/
#endif
    uv_t t;			/* Elapsed simulation time	*/
    uv_t tau;			/* Timestep			*/
    uv_t tea;			/* Kahan tau error accumulator	*/
    S_FPV t_stopv;		/* Stop time			*/

    int *saved;			/* Saved populations		*/

#if S_NVS==2
    int nr[S_NVS]={0,0,0,0};	/* Number of reactions in slot	*/
#elif S_NVS==4
    int nr[S_NVS]={0,0,0,0};	/* Number of reactions in slot	*/
#endif
    uint64_t rc = 0L;		/* Total number of reactions	*/

    uv_t r_sum, choice;
    uv_t flg;

    S_FPS r;

</xsl:text><xsl:if test="$LPR = 'full'">
    S_FPS oldr; /* Temporary partial propensity sum accumulator */
    S_FPS newr; /* Temporary partial propensity sum accumulator */
</xsl:if><xsl:text>
    int i, n, e;

    si_init(&amp; info, argc, argv); /* Process CLAs */

    assert(N_REACTIONS == N_RXNS);
    assert(N_SPECIES   == N_SPXS);

#if RNG==CMC_RNG_RSMT
    rs = &amp;rs_obj;
#endif
</xsl:text>

<xsl:text>

    _vutil_srand( rs, info.seed );
    
#if S_NVS==2
    t_stopv = (S_FPV){info.t_stop,info.t_stop};
    t.sf    = UV_0_2df;
    tau.sf  = UV_0_2df;
    tea.sf  = UV_0_2df;
    flg.si  = UV_1_2di;
#elif S_NVS==4
    t_stopv = (S_FPV){info.t_stop,info.t_stop,info.t_stop,info.t_stop};
    t.sf    = UV_0_4sf;
    tau.sf  = UV_0_4sf;
    tea.sf  = UV_0_4sf;
    flg.si  = UV_1_4si;
#endif

    saved=(int *)malloc(info.n_trjs*N_SPECIES*sizeof(int));
    RCHECK(NULL, !=saved, malloc);
    info.results=(void *)saved;

    #define SAVED(T,S) saved[T*N_SPECIES+S]

    si_start(&amp;info);	     /* Start timing */
</xsl:text>
    <xsl:call-template name="init-popn"/>
    <xsl:if test="$LPR != 'none'">
        <xsl:text>    _update_props(prop, popn);&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="$LPR = 'full'">
        <xsl:text>    r_sum.sf=SUM_RATES;&#10;</xsl:text>
    </xsl:if>

    <xsl:text>
    for(n=0; n&lt;info.n_trjs; ) {
</xsl:text>
    <xsl:if test="$LPR = 'none'">
        <xsl:text>        _update_props(prop, popn);&#10;</xsl:text>
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
		r -= prop[i].f[e];
	    }
#endif
	    --i;

</xsl:text>
                <xsl:call-template name="switch"/>
<xsl:text>

            if( ! flg.i32[e] ) {
#if defined(REPORT_ALL)
	        fprintf(stderr, "%d %d %d\n", n, e, nr[e]);
#endif
	        for(i=0; i&lt;N_SPECIES; i++) {
		    SAVED(n,i)=SPOP(i,e);
		    SPOP(i,e)=ipop[i];
		}
</xsl:text>
    <xsl:if test="$LPR != 'none'">
      <xsl:text>                _update_props(prop, popn);&#10;</xsl:text>
    </xsl:if>
<xsl:text>
                n++;
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
    si_stop(&amp;info);		/* Stop timing  */

    info.rxn_total = rc;
    
    as_dump(&amp;info);		/* Dump results */


    /*
     * Cleanup:
     */
    free(saved);
    info.results=NULL;

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
#if defined(CUMULATIVE_SUM_ARRAY)
static inline v4sf _sum_props(uv_t prop[], uv_t cumr[]) 
#else
static inline v4sf _sum_props(uv_t prop[])
#endif
{
    int i;
    v4sf r_sum=UV_0_4sf;

    for(i=N_REACTIONS-1; i&gt;=0; i--) {
        r_sum += prop[i].sf;
#if defined(CUMULATIVE_SUM_ARRAY)
        cumr[i].sf = r_sum;
#endif
    }
    return r_sum;
}
#if defined(CUMULATIVE_SUM_ARRAY)
#   define SUM_RATES _sum_props(prop, cumr) 
#else
#   define SUM_RATES _sum_props(prop)
#endif


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
