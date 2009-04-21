<?xml version="1.0"?>
<!--
 | @file    simd.xsl
 | @brief   To transform an SBML model to C code
 | @author  Emmet Caulfield.
 | @version 0.1
 |
 | $Id$
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
/*
 * [name="main"]
 */
    <xsl:call-template name="file-scope-c"/>
    <xsl:text>
int main(uint64_t speid, uint64_t argp)
{
    T_PIS *saved = NULL;	/* Current save area		*/

#define SAVED(TRJ,SPX) saved[TRJ*N_SPECIES+SPX]

    uv_t popn[N_SPECIES];	/* Working populations          */
    uv_t rate[N_REACTIONS];	/* Propensities			*/
#if defined(CUMULATIVE_SUM_ARRAY)
    uv_t cumr[N_REACTIONS];	/* Cumulative propensity sums	*/
#endif
    uv_t t;			/* Elapsed simulation time	*/
    uv_t tau;			/* Timestep			*/
    uv_t kea;			/* Kahan tau error accumulator	*/
    v4sf t_stopv;		/* Stop time			*/
    v4sf tmp1, tmp2;

    summblk_t *sb;		/* Summary block		*/
    T_PIS nr_rxn[N_REACTIONS];	/* Reaction counts by reaction	*/
    uv_t nr_slot;		/* Reaction count by slot	*/
    uint64_t nr_abs = 0LL;	/* Absolute number of reactions	*/
    uint64_t nr_con = 0LL;	/* Contributing number of rxns	*/

    uv_t r_sum, choice;
    uv_t slot;
    uv_t flg;

    float r;
//    float t_stop;

</xsl:text><xsl:if test="$LPR = 'full'">
    register float oldr; /* Temporary partial propensity sum accumulator */
    register float newr; /* Temporary partial propensity sum accumulator */
</xsl:if><xsl:text>
    int b, i, n, e;

#if SSO==CMC_SSO_OFF
    int n_rsets;
#else
    int n_trjs;
#endif

    (void)speid;	/* Just to suppress "unused" warning */

    /* Initialize the reaction counters */
    for(i=0; i&lt;N_REACTIONS; i++) {
        nr_rxn[i] = (T_PIS)0;
    }

    PING();
</xsl:text>
    <xsl:call-template name="get-control-block"/>
<xsl:text>

    PING();

    t_stopv = spu_splats(cb.t_stop);
    
    resblk[0] = (T_PIS *)malloc_align( cb.blksz, 7 ); /* 7=lg(128) */
    resblk[1] = (T_PIS *)malloc_align( cb.blksz, 7 ); /* 7=lg(128) */

    t.sf    = UV_0_4sf;
    tau.sf  = UV_0_4sf;
    kea.sf  = UV_0_4sf;
    flg.si  = UV_1_4si;

    as_srand( (uint32_t)cb.seed );

</xsl:text>
    <xsl:call-template name="init-popn"/>
    <xsl:text>
    for(b=0; b &lt; cb.n_blks; b++) {
        if( b &lt; cb.n_blks-1 ) {
	    n_trjs=cb.n_trjs_per_blk;
        } else {
            n_trjs=cb.n_trjs_residual;
        }
        for(n=0; n&lt;n_trjs; ) {
</xsl:text>
    <xsl:if test="$LPR = 'none'">
        <xsl:text>        _update_rates(rate, popn);&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="$LPR != 'full'">
        <xsl:text>        r_sum.sf=SUM_RATES;&#10;</xsl:text>
    </xsl:if>

    <xsl:text>
//        DPRINTF(stderr, "------------------------------\n");
        choice.sf = r_sum.sf * as_rand();
        uv_dprint_sf(r_sum,  "r_sum  ");
	uv_dprint_sf(choice, "choice ");

	for(e=0; e&lt;T_NVS; e++) {
	    nr_slot.i32[e]++;
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

	    slot.i32[e]=i;
	    nr_rxn[i]++;
	    nr_abs++;
</xsl:text>
                <xsl:call-template name="switch"/>
<xsl:text>
            if( ! flg.i32[e] ) {
	        saved = (T_PIS *)&amp;(resblk[crb][ n * N_SPECIES ]);
                for(i=0; i&lt;N_SPECIES; i++) {
                    saved[i]  = SPOP(i,e);
                    SPOP(i,e) = ipop[i];
                }
                n++;
		nr_con         += nr_slot.i32[e];
		nr_slot.i32[e]  = 0;
		t.f[e]          = 0.0f;
		kea.f[e]        = 0.0f;
</xsl:text>
    <xsl:if test="$LPR != 'none'">
      <xsl:text>                _update_rates(rate, popn);&#10;</xsl:text>
    </xsl:if>
<xsl:text>
	    }

	} /* for(e=1:T_NVS) */

	uv_dprint_si(slot, "slot   ");

        tau.sf = as_tau(r_sum.sf);
	uv_dprint_sf(tau, "tau    ");

        /*
         * Kahan time summing: we use inline assembly mostly to defend
         * against aggressive optimization problems.
         *    x   = tau - kea
         *    y   = t + x
         *    kea = (y - t) - x     ( kea=y-t; kea-=x )
         *    t   = y
         */
	 tmp1   = spu_sub(tau.sf, kea.sf);
	 tmp2   = spu_add(t.sf, tmp1);
	 kea.sf = spu_sub(tmp2, t.sf);
	 kea.sf = spu_sub(kea.sf, tmp1);
	 t.sf   = tmp2;
/*
	__asm__ (
            "fs        $75,  %[tau],  %[kea]   \n"
	    "fa        $76,    %[t],    $75	\n"
	    "fs      %[kea],    $76,    %[t]	\n"
	    "fs      %[kea],  %[kea],    $75   \n"
            "xor       %[t],    %[t],    %[t]	\n"
	    "or        %[t],    %[t],    $76	\n"
            : [t]"+&amp;v"(t.sf), [kea]"+&amp;v"(kea.sf)
            : [tau]"v"(tau.sf)
	    : "$75", "$76"
        );
*/

	uv_dprint_se(t,   "t      ");
	uv_dprint_se(tau, "tau    ");
	uv_dprint_se(kea, "kea    ");

        flg.su = spu_cmpgt(t_stopv,t.sf);

      }  /* n=for(1:n_rsets) */

      _send_result_block(b, NULL, 0);

    } /* b=(1:n_blks) */

    DPRINTF("NR: abs=%"PRIu64", con=%"PRIu64"\n", nr_abs, nr_con);
    
    /*
     * The summary data is sent in the extra block
     */
    sb = (summblk_t *)resblk[crb];
    sb->nr_abs = nr_abs;
    sb->nr_con = nr_con;
    _send_result_block(b, NULL, 0);

</xsl:text>
    PING();
    <xsl:call-template name="complete-outstanding-dma"/>
    PING();
    <xsl:text>
    return 0;
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
