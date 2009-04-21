<?xml version="1.0"?>
<!--
 | @file    simd.xsl
 | @brief   To transform an SBML model to C code
 | @author  Emmet Caulfield.
 | @version 0.1
 |
 | $Id: simd.xsl 86 2009-02-02 04:10:23Z emmet $
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
//    T_PIS *saved = NULL;	/* Current save area		*/

#define SAVED(TRJ,SPX) saved[TRJ*N_SPECIES+SPX]

    uv_t *popn;			/* Working populations          */
    uv_t rate[N_REACTIONS];	/* Propensities			*/
    register v4sf t;		/* Elapsed simulation time	*/
    register v4sf tau;		/* Timestep			*/
    register v4sf kea;		/* Kahan tau error accumulator	*/
    register v4sf t_stopv;	/* Stop time			*/
    register v4sf tmp1, tmp2;
    register v4su flags;

    summblk_t *sb;		/* Summary block		*/
    uv_t nr_rxn[N_REACTIONS];	/* Reaction counts by reaction	*/
    uint64_t nr_abs = 0LL;	/* Absolute number of reactions	*/
    uint64_t nr_con = 0LL;	/* Contributing number of rxns	*/

    register v4sf r_sum;
    int flg;

   float tmp;

    /*
     * Common constants: these are used a lot
     */ 
    const v4si c_1_in_pos_0 = {1, 0, 0, 0};
    const v4si c_1_in_pos_1 = {0, 1, 0, 0};
    const v4si c_1_in_pos_2 = {0, 0, 1, 0};
    const v4si c_1_in_pos_3 = {0, 0, 0, 1};



</xsl:text><xsl:if test="$LPR = 'full'">
    register float oldr; /* Temporary partial propensity sum accumulator */
    register float newr; /* Temporary partial propensity sum accumulator */
</xsl:if><xsl:text>
    int b, i, n;//, e;

#if SSO==CMC_SSO_OFF
    int n_rsets;
#else
    int n_trjs;
#endif

    (void)speid;	/* Just to suppress "unused" warning */

    /* Initialize the reaction counters */
    for(i=0; i&lt;N_REACTIONS; i++) {
        nr_rxn[i].su = (v4su){0,0,0,0};
    }

    PING();
</xsl:text>
    <xsl:call-template name="get-control-block"/>
<xsl:text>

    PING();

    resblk[0] = (T_PIS *)malloc_align( cb.blksz, 7 ); /* 7=lg(128) */
    resblk[1] = (T_PIS *)malloc_align( cb.blksz, 7 ); /* 7=lg(128) */

    as_srand( (uint32_t)cb.seed );
    t_stopv = spu_splats(cb.t_stop);

</xsl:text>
    <xsl:text>
    for(b=0; b &lt; cb.n_blks; b++) {
        if( b &lt; cb.n_blks-1 ) {
	    n_rsets=cb.n_rsets_per_blk;
        } else {
            n_rsets=cb.n_rsets_residual;
        }
        for(n=0; n&lt;n_rsets; n++) {
	    popn = (uv_t *)resblk[crb]+n*N_SPECIES;

	    // equiv:compute-trajectory
  	    t    = UV_0_4sf;
	    kea  = UV_0_4sf;

</xsl:text>
    <xsl:call-template name="init-popn"/>
<xsl:text>
	    while( (flg=spu_extract(spu_gather(flags=spu_cmpgt(t_stopv,t)),0)) ) {

	        // equiv:update-propensities
	        _update_rates(rate, popn);
		r_sum=SUM_RATES;

		// equiv:update-times
		tau = as_tau(r_sum);
		uv_dprint_sf(tau, "tau    ");

		/*
		* Kahan time summing: we use inline assembly mostly to defend
		* against aggressive optimization problems.
		*    x   = tau - kea
		*    y   = t + x
		*    kea = (y - t) - x     ( kea=y-t; kea-=x )
		*    t   = y
		*/
		tmp1 = spu_sub(tau, kea);
		tmp2 = spu_add(t, tmp1);
		kea  = spu_sub(tmp2, t);
		kea  = spu_sub(kea, tmp1);
		t    = tmp2;

		r_sum *= as_rand();;
	        /*
		 * Unrolled iteration over the four slots of each SIMD vector
		 */ 
</xsl:text>
    <xsl:call-template name="each-vec-el">
      <xsl:with-param name="slot" select="'0'"/>
    </xsl:call-template>

    <xsl:call-template name="each-vec-el">
      <xsl:with-param name="slot" select="'1'"/>
    </xsl:call-template>

    <xsl:call-template name="each-vec-el">
      <xsl:with-param name="slot" select="'2'"/>
    </xsl:call-template>

    <xsl:call-template name="each-vec-el">
      <xsl:with-param name="slot" select="'3'"/>
    </xsl:call-template>
<xsl:text>
            }

	    /* NO NEED TO DO THIS:
	     * Save values from scratch registers into current block
             */
      }  /* n=for(1:n_rsets) */

      _send_result_block(b, NULL, 0);

    /* Initialize the reaction counters */
    for(i=0; i&lt;N_REACTIONS; i++) {
        nr_abs += nr_rxn[i].i32[0];
        nr_abs += nr_rxn[i].i32[1];
        nr_abs += nr_rxn[i].i32[2];
        nr_abs += nr_rxn[i].i32[3];
        nr_rxn[i].su = (v4su){0,0,0,0};
    }

    } /* b=(1:n_blks) */


    /*
     * The summary data is sent in the extra block
     */
    sb = (summblk_t *)resblk[crb];
    sb->nr_abs = nr_abs;
    sb->nr_con = nr_con;
    _send_result_block(b, NULL, 0);

    PING();
</xsl:text>
    <xsl:call-template name="complete-outstanding-dma"/>
<xsl:text>
    PING();
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



<!--================================================-->
<!-- SLOT ITERATION				    -->
<!--================================================-->
  <xsl:template name="next-element">
    <xsl:param name="slot"/>
    <xsl:choose>
      <xsl:when test="$slot=3">
		    continue;
      </xsl:when>
      <xsl:otherwise>
		    goto ELEMENT_<xsl:value-of select="$slot+1"/>;
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>


  <xsl:template name="each-vec-el">
    <xsl:param name="slot"/>
      <xsl:if test="$slot!=0">
ELEMENT_<xsl:value-of select="$slot"/>:
      </xsl:if>
		/* if *this* time is already &lt;= 0 (!&gt;), skip this element */
		if( !spu_extract(flags, <xsl:value-of select="$slot"/>) ) {
	            <xsl:call-template name="next-element">
		      <xsl:with-param name="slot" select="$slot"/>
		    </xsl:call-template>
	        }

		tmp = spu_extract(r_sum,<xsl:value-of select="$slot"/>);
    <xsl:call-template name="switch">
      <xsl:with-param name="slot" select="$slot"/>
    </xsl:call-template>
  </xsl:template>


  <xsl:template name="switch">
    <xsl:param name="slot"/>
<!--    <xsl:message><xsl:value-of select="$slot"/></xsl:message> -->
    <xsl:apply-templates select="//s:reaction" mode="case">
      <xsl:with-param name="slot" select="$slot"/>
    </xsl:apply-templates>
  </xsl:template>


  <xsl:template match="s:reaction" mode="case">
    <xsl:param name="slot"/>
		tmp -= spu_extract(rate[i_<xsl:value-of select="@id"/>].sf, <xsl:value-of select="$slot"/>);
		if( 0.0f > tmp ) {
		    DU_PRINTF("SPU| %10s++ (%2u) in slot %2u\n", "<xsl:value-of select='@id'/>", <xsl:value-of select="position()"/>, <xsl:value-of select="$slot"/>);
		    nr_rxn[<xsl:value-of select="position()-1"/>].su += c_1_in_pos_<xsl:value-of select="$slot"/>;
    <xsl:apply-templates select="(s:listOfReactants|s:listOfProducts)/s:speciesReference" mode="case">
      <xsl:with-param name="slot" select="$slot"/>
    </xsl:apply-templates>
    <xsl:call-template name="next-element">
      <xsl:with-param name="slot" select="$slot"/>
    </xsl:call-template>
		}
  </xsl:template>


  <xsl:template match="s:listOfReactants/s:speciesReference" mode="case">
    <xsl:param name="slot"/>
		    VPOP(i_<xsl:value-of select="@species"/>) -= c_1_in_pos_<xsl:value-of select="$slot"/>;
    <xsl:call-template name="lpr-opt">
      <xsl:with-param name="species" select="@species"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="s:listOfProducts/s:speciesReference" mode="case">
    <xsl:param name="slot"/>
		    VPOP(i_<xsl:value-of select="@species"/>) += c_1_in_pos_<xsl:value-of select="$slot"/>;
    <xsl:call-template name="lpr-opt">
      <xsl:with-param name="species" select="@species"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="lpr-opt"/>

  <xsl:template name="lpr-opt-disabled">
    <xsl:param name="species"/>
            /* Reactions affected by <xsl:value-of select="$species"/> */
      <xsl:for-each select="//m:ci[normalize-space(.) = $species]">
        <xsl:apply-templates select="ancestor::s:reaction" mode="adjust-prop"/>
      </xsl:for-each>
  </xsl:template>

  <xsl:template match="s:reaction" mode="adjust-prop">
            r_sum -= rate[i_<xsl:value-of select="./@id"/>].sf;
            rate[i_<xsl:value-of select="./@id"/>].sf=<xsl:apply-templates match="./s:kineticLaw"/>;
            r_sum += rate[i_<xsl:value-of select="./@id"/>].sf;
  </xsl:template>



</xsl:transform>


