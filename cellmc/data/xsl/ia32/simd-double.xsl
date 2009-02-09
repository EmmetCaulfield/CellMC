<?xml version="1.0"?>
<!--
 | @file    sbml.xsl
 | @brief   To transform an SBML model to C code
 | @author  Emmet Caulfield.
 | @version 0.1
 |
 | $Id: simd-double.xsl 38 2009-01-23 19:22:35Z emmet $
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
int main(const int argc, const char *const argv[])
{
#if RNG == CMC_RNG_RSMT
    rm_state_t rs_obj;          /* RSMT RNG state object        */
    rm_state_t *rs;             /* RSMT RNG state =&amp;rs_obj  */
#elif RNG == CMC_RNG_STDLIB
    unsigned short rs[3];       /* erand48()-family state       */
#endif

    uv64_t popn[N_SPECIES];
    uv_t rate[N_REACTIONS];	/* Propensities			*/
#if defined(CUMULATIVE_SUM_ARRAY)
    uv_t cumr[N_REACTIONS];	/* Cumulative propensity sums	*/
#endif
    uv_t t;			/* Elapsed simulation time	*/
    uv_t tau;			/* Time step			*/
    v2df t_stopv;		/* Stop time			*/

    int32_t *saved;		/* Saved populations		*/
    unsigned seed=3987654321U;  /* RNG seed                     */

    int nr[UV_2]={0,0};		/* Number of reactions in slot  */
    uint64_t rc = 0L;           /* Total number of reactions    */

    uv_t r_sum, choice;
    uv_t flg;

    double r;
    double t_stop;
</xsl:text><xsl:if test="$LPR = 'full'">
    register double oldr; /* Temporary partial propensity sum accumulator */
    register double newr; /* Temporary partial propensity sum accumulator */
</xsl:if><xsl:text>
    int i, n, e, n_trjs;

#if RNG==CMC_RNG_RSMT
    rs = &amp;rs_obj;
#endif
</xsl:text>
    <xsl:call-template name="getopts"/>
<xsl:text>
    t.df   = UV_0_2df;
//    t.d[0] = 0.0;
//    t.d[1] = 0.0;
    tau.df = UV_0_2df;
//    tau.d[0] = 0.0;
//    tau.d[1] = 0.0;
    t_stopv = (v2df){t_stop, t_stop};

    flg.di = UV_f_2di;
    rc     = 0L;


    saved=(int32_t *)malloc(n_trjs*N_SPECIES*sizeof(int32_t));
    if( NULL==saved ) {
        fprintf(stderr, "malloc() failed!");
        exit(1);
    }
  #define SAVED(T,S) saved[T*N_SPECIES+S]

    _vutil_srand( rs, (uint32_t)seed );


</xsl:text>
    <xsl:call-template name="init-popn"/>
    <xsl:if test="$LPR != 'off'">
        <xsl:text>    _update_rates(rate, popn);&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="$LPR = 'full'">
        <xsl:text>    r_sum.df=SUM_RATES;&#10;</xsl:text>
    </xsl:if>

    <xsl:text>
    __asm__ __volatile__("emms\n");
    for(n=0; n&lt;n_trjs; ) {
/*
        printf("-----------------------------------\n");
        uv_print_dx("flg", flg);
	uv_print_df("tau", tau);
	uv_print_df("  t", t);
*/
</xsl:text>
    <xsl:if test="$LPR = 'off'">
        <xsl:text>        _update_rates(rate, popn);&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="$LPR != 'full'">
        <xsl:text>        r_sum.df=SUM_RATES;&#10;</xsl:text>
    </xsl:if>

    <xsl:text>
        choice.df = r_sum.df * _vutil_rand(rs);

	for(e=0; e&lt;UV_2; e++) {
            nr[e]++;
            r=choice.d[e];

	    /*
             * Fast reactions are at low indices
             */
#if defined(CUMULATIVE_SUM_ARRAY)
            for(i=1; r&lt;cumr[i].d[e]; i++)
                ; // Yes, I mean it!
#else
            for(i=0; r&gt;0.0f &amp;&amp; i&lt;=N_REACTIONS; i++) {
		r -= rate[i].d[e];
	    }
#endif
	    --i;
//	    fprintf(stderr, "(%02d,%d) (%le:%le) @ %le\n", i, e, rate[i].d[e], r_sum.d[e], t.d[e]);
</xsl:text>
                <xsl:call-template name="switch"/>
<xsl:text>

            if( ! flg.i64[e] ) {
#if defined(REPORT_ALL)
	        fprintf(stderr, "%d %d %d\n", n, e, nr[e]);
#endif
	        for(i=0; i&lt;N_SPECIES; i++) {
		    SAVED(n,i)=SPOP(i,e);
		    SPOP(i,e)=ipop[i];
		}
</xsl:text>
    <xsl:if test="$LPR != 'off'">
      <xsl:text>                _update_rates(rate, popn);&#10;</xsl:text>
    </xsl:if>
<xsl:text>
                n++;
		rc   += nr[e];
                nr[e] = 0;
		t.d[e]   = 0.0;
	    }

	} /* for(e=1:UV_2) */

	tau.df = _vutil_tau(rs, r_sum.df);
        t.df  += tau.df;

        flg.df = _mm_cmpgt_pd(t_stopv,t.df);

    }  /* n=for(1:n_trjs) */

#if defined(DUMP_RESULTS)
    for(n=0; n&lt;n_trjs; n++) {
        for(i=0; i&lt;N_SPECIES-1; i++) {
            printf(FS_D32" ", SAVED(n,i));
        }
        printf(FS_D32"\n", SAVED(n,i));
    }
#else
    fprintf(stderr, "Total reactions: " FS_U64 "\n", rc);
#endif

    // Free the memory we've allocated:
    free(saved);

    return EXIT_SUCCESS;
}
</xsl:text>

  </xsl:template> <!-- main -->


  <xsl:template name="switch">
    <xsl:text>
	    // Execute selected reaction [name="switch"]:
            switch(i) {&#10;</xsl:text>
    <xsl:apply-templates select="//s:reaction" mode="case"/>
    <xsl:text>
            }
</xsl:text>
  </xsl:template>


  <xsl:template match="s:reaction" mode="case">
    <xsl:text>                case i_</xsl:text>
    <xsl:value-of select="./@id"/>
    <xsl:text>:&#10;</xsl:text>
    <xsl:apply-templates select="(s:listOfReactants|s:listOfProducts)/s:speciesReference" mode="case"/>
    <xsl:if test="$LPR = 'full'">
      <xsl:text>                    oldr=0.0f; newr=0.0f;&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="$LPR != 'off'">
      <xsl:for-each select="/s:sbml/s:model/s:listOfReactions/s:reaction[s:kineticLaw//m:ci = current()//s:speciesReference/@species]">
	<xsl:sort select="position()" data-type="number" order="descending"/>
        <xsl:call-template name="adjust-prop"/>
      </xsl:for-each>
    </xsl:if>
    <xsl:if test="$LPR = 'full'">
      <xsl:text>                    r_sum.d[e] += (newr - oldr);&#10;</xsl:text>
    </xsl:if>
    <xsl:text>                break;&#10;&#10;</xsl:text>
  </xsl:template>

  <xsl:template match="s:listOfReactants/s:speciesReference" mode="case">
    <xsl:text>                    SPOP(i_</xsl:text>
    <xsl:value-of select="./@species"/>
    <xsl:text>,e)--;&#10;</xsl:text>
  </xsl:template>


  <xsl:template match="s:listOfProducts/s:speciesReference" mode="case">
    <xsl:text>                    SPOP(i_</xsl:text>
    <xsl:value-of select="./@species"/>
    <xsl:text>,e)++;&#10;</xsl:text>
  </xsl:template>

  <xsl:template name="popn-const">
    <xsl:text>const int ipop[N_SPECIES] = {&#10;</xsl:text>
    <xsl:apply-templates select="//s:species" mode="const"/>
    <xsl:text>&#10;};&#10;</xsl:text>
  </xsl:template>

  <xsl:template match="s:species" mode="const">
    <xsl:text>    SS_</xsl:text>
    <xsl:value-of select="./@id"/>
    <xsl:if test="position() != last()">
      <xsl:text>,&#10;</xsl:text>
    </xsl:if>
  </xsl:template>

  
  <xsl:template name="init-popn">
    <xsl:text>    /* [name="init-popn"] */&#10;</xsl:text>
    <xsl:apply-templates select="//s:species" mode="main"/>
  </xsl:template>

  <xsl:template match="s:species" mode="main">
    <xsl:text>    VPOP_</xsl:text>
    <xsl:value-of select="./@id"/>
    <xsl:text> = VS_</xsl:text>
    <xsl:value-of select="./@id"/>
    <xsl:text>;&#10;</xsl:text>
  </xsl:template>
<!--================================================-->


<!--
=======================================================
Generate the propensity recalculation functions
======================================================= 
-->
  <xsl:template name="prop-funcs">
<xsl:text>
/*
 * [name="prop-funcs"]
 * 
 * SEQN_ = Scalar Equation
 * VEQN_ = Vector Equation
 */
</xsl:text>
    <xsl:apply-templates select="/s:sbml/s:model/s:listOfReactions/s:reaction" mode="funcs" />
  </xsl:template>

  <xsl:template match="s:reaction" mode="funcs">
    <xsl:text>#define SEQN_</xsl:text>
    <xsl:value-of select="./@id"/>
    <xsl:text>(SLOT) (</xsl:text>
    <!-- The s:kineticLaw template is in 'kineticLaw.xsl' -->
    <xsl:apply-templates match="./s:kineticLaw"/>
    <xsl:text>)&#10;</xsl:text>

    <xsl:text>#define VEQN_</xsl:text>
    <xsl:value-of select="./@id"/>
    <xsl:text> (</xsl:text>
    <!-- The s:kineticLaw template is in 'kineticLaw.xsl' -->
    <xsl:apply-templates match="./s:kineticLaw" mode="vector"/>
    <xsl:text>)&#10;</xsl:text>

  </xsl:template>


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
static inline v2df _sum_rates(uv_t rate[], uv_t cumr[]) 
#else
static inline v2df _sum_rates(uv_t rate[])
#endif
{
    int i;
    v2df r_sum=UV_0_2df;

    for(i=N_REACTIONS-1; i&gt;=0; i--) {
        r_sum += rate[i].df;
#if defined(CUMULATIVE_SUM_ARRAY)
        cumr[i].df = r_sum;
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
static inline void _update_rates(uv_t rate[], uv64_t popn[])
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
    <xsl:text>].df=VEQN_</xsl:text>
    <xsl:value-of select="./@id"/>
    <xsl:text>;&#10;</xsl:text>
  </xsl:template>
<!--================================================-->


</xsl:transform>
