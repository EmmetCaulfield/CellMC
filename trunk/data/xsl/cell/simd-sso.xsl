<?xml version="1.0"?>
<!--
 | @file    simd.xsl
 | @brief   Transforms an SBML model to C code for the SPU.
 | @author  Emmet Caulfield.
 | @version 0.1
 |
 | $Id$
-->

<!-- Preamble: -->
<xsl:transform
  xmlns:s="http://www.sbml.org/sbml/level2/version3"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:fn="http://www.w3.org/2005/02/xpath-functions"
  xmlns:m="http://www.w3.org/1998/Math/MathML"
  xmlns:svn="http://svnbook.red-bean.com/en/1.4/svn.advanced.props.special.keywords.html"
  xmlns:k="http://polacksbacken.net/wiki/SSACBE"
  version="1.0"
>

  <xsl:import href="kineticLaw.xsl" />

  <xsl:output
    method="text"
    indent="no"
    media-type="text/plain" />

  <xsl:strip-space elements="*"/>

  <xsl:variable name="UCASE">ABCDEFGHIJKLMNOPQRSTUVWXYZ</xsl:variable>
  <xsl:variable name="LCASE">abcdefghijklmnopqrstuvwxyz</xsl:variable>


  <xsl:template match="/">
    <xsl:call-template name="header"    />
    <xsl:call-template name="main"      />
  </xsl:template>

  <xsl:template match="svn:*"/>


  <xsl:template name="header">
#include &lt;support.h&gt;

/*
 *Defines for number of reactions and number of species 
 */
#define N_SPECIES <xsl:value-of select="count(/s:sbml/s:model/s:listOfSpecies/s:species)"/>
#define N_REACTIONS <xsl:value-of select="count(/s:sbml/s:model/s:listOfReactions/s:reaction)"/>
  </xsl:template>


<!--================================================-->


<xsl:template name="negtau">
  <xsl:text>_logf4(as_rand())/r_sum</xsl:text>
</xsl:template>


<xsl:template name="print-loop-logicals">
        uv_dprint_sx(flag, "SPU %llu|\t", speid);
	exit(0);
</xsl:template>



<!--
=======================================================
Output constants
======================================================= 
-->
  <xsl:template name="constants">
    /*
     * Constant model parameters [name="constants"]:
     */
    <xsl:apply-templates select="//s:parameter" mode="const"/>
    <xsl:text>&#10;&#10;</xsl:text>
    <xsl:apply-templates select="//s:species"   mode="const"/>
  </xsl:template>

  <xsl:template match="s:parameter" mode="const">
    <xsl:choose>
      <xsl:when test="preceding::s:parameter/@value = ./@value">
    #define C_<xsl:value-of select="./@id"/> vf_<xsl:value-of select="translate(@value,'.','_')"/>
      </xsl:when>
      <xsl:otherwise>
    register const vec_float4 vf_<xsl:value-of select="translate(@value,'.','_')"/> = <xsl:call-template name="quadify">
          <xsl:with-param name="type"  select="'vec_float4'" />
          <xsl:with-param name="value" select="./@value" />
        </xsl:call-template>;
    #define RA_vf_<xsl:value-of select="translate(@value,'.','_')"/>
    #define C_<xsl:value-of select="./@id"/> vf_<xsl:value-of select="translate(@value,'.','_')"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>



  <xsl:template match="s:species" mode="const">
    <xsl:choose>
      <xsl:when test="preceding::s:species/@initialAmount = @initialAmount">
    #define C_<xsl:value-of select="@id"/> vi_<xsl:value-of select="@initialAmount"/>
      </xsl:when>
      <xsl:otherwise>
    const vec_int4 vi_<xsl:value-of select="@initialAmount"/> = <xsl:call-template name="quadify">
          <xsl:with-param name="type"  select="'vec_int4'" />
          <xsl:with-param name="value" select="@initialAmount" />
        </xsl:call-template>;
    #define RA_vi_<xsl:value-of select="@initialAmount"/>
    #define C_<xsl:value-of select="@id"/> vi_<xsl:value-of select="@initialAmount"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>


<!--================================================-->


  <xsl:template name="modvars">
/*
 * Population and reaction propensity globals [name="modvars"]
 */
    <xsl:apply-templates select="/s:sbml/s:model/s:listOfSpecies"   mode="vars"/>
    <xsl:apply-templates select="/s:sbml/s:model/s:listOfReactions" mode="vars"/>
  </xsl:template>

  <xsl:template match="s:listOfSpecies|s:listOfReactions" mode="vars">
    <xsl:apply-templates select="s:species|s:reaction" mode="vars"/>
  </xsl:template>

  <xsl:template match="s:species" mode="vars">
    register vec_int4 s_<xsl:value-of select="./@id"/>;
  </xsl:template>

  <xsl:template match="s:reaction" mode="vars">
    register vec_float4 r_<xsl:value-of select="./@id"/>;
  </xsl:template>


<!--================================================-->

  <xsl:template name="reset-population">
    <xsl:for-each select="//s:species">
	    s_<xsl:value-of select="@id"/> = C_<xsl:value-of select="@id"/>;
    </xsl:for-each>
  </xsl:template>

<!--================================================-->
  
  <xsl:template name="save-pops">
    <xsl:for-each select="//s:species">
            popn[<xsl:value-of select="position()-1"/>] = s_<xsl:value-of select="@id"/>;
    </xsl:for-each>
  </xsl:template>


  <xsl:template name="update-propensities">
    // WTF: <xsl:apply-templates match="//s:reaction" mode="propensities" />
		r_sum=vf_0;
    <xsl:for-each select="/s:sbml/s:model/s:listOfReactions/s:reaction">
      <xsl:sort select="position()" data-type="number" order="descending"/>
      <xsl:apply-templates select="." mode="sum"/>      
    </xsl:for-each>
  </xsl:template> <!-- name="update-propensities" -->

  <xsl:template match="s:reaction" mode="propensities">
		r_<xsl:value-of select="./@id"/>=<xsl:apply-templates match="./s:kineticLaw"/>;
  </xsl:template>

  <xsl:template match="s:reaction" mode="sum">
		r_sum = spu_add(r_sum, r_<xsl:value-of select="./@id"/>);
  </xsl:template>



<!--================================================-->


  <xsl:template name="update-times">  
	    /* Compute tau and add it on to dt */
	    tau = as_tau(r_sum);
	    uv_dprint_sf((uv_t)tau, "SPU| tau     = ");

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
  </xsl:template>

 
  <xsl:template name="ident">
    <xsl:text>ident</xsl:text>
  </xsl:template>

<!--================================================-->




<!--
=======================================================
Generate the compute_trajectory function
======================================================= 
-->
  <xsl:template name="compute-trajectory">
	    t   = vf_0;
	    kea = vf_0;

    <xsl:call-template name="reset-population"/>
            while( (flg=spu_extract(spu_gather(flags=spu_cmpgt(t_stopv,t)),0)) ) {
	        DU_PRINTF("----------------------------------\n");
        <xsl:call-template name="update-propensities"/>
		/* Compute tau and add it on to dt */
		<xsl:call-template name="update-times"/>

		/* Can re-use r_sum now */
                r_sum *= as_rand();


	        /*
		 * Unrolled iteration over the four slots of each SIMD vector
		 */ 

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


	        /* FIXME: Check for underflow here */
	    }


  </xsl:template> <!-- name="compute-trajectory" -->



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
		    DU_PRINTF("Skipping trj %d\n", <xsl:value-of select="$slot"/>);
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
		tmp -= spu_extract(r_<xsl:value-of select="@id"/>, <xsl:value-of select="$slot"/>);
		if( 0.0f > tmp ) {
		    DU_PRINTF("%10s++ (%2u) in slot %2u\n", "<xsl:value-of select='@id'/>", <xsl:value-of select="position()"/>, <xsl:value-of select="$slot"/>);
		    rcount[<xsl:value-of select="position()-1"/>] += c_1_in_pos_<xsl:value-of select="$slot"/>;
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
		    s_<xsl:value-of select="@species"/> -= c_1_in_pos_<xsl:value-of select="$slot"/>;
    <xsl:call-template name="lpr-opt">
      <xsl:with-param name="species" select="@species"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="s:listOfProducts/s:speciesReference" mode="case">
    <xsl:param name="slot"/>
		    s_<xsl:value-of select="@species"/> += c_1_in_pos_<xsl:value-of select="$slot"/>;
    <xsl:call-template name="lpr-opt">
      <xsl:with-param name="species" select="@species"/>
    </xsl:call-template>
  </xsl:template>


<!--================================================-->

  <xsl:template name="lpr-opt"/>

  <xsl:template name="lpr-opt-disabled">
    <xsl:param name="species"/>
            /* Reactions affected by <xsl:value-of select="$species"/> */
      <xsl:for-each select="//m:ci[normalize-space(.) = $species]">
        <xsl:apply-templates select="ancestor::s:reaction" mode="adjust-prop"/>
      </xsl:for-each>
  </xsl:template>

  <xsl:template match="s:reaction" mode="adjust-prop">
            r_sum -= r_<xsl:value-of select="./@id"/>;
            r_<xsl:value-of select="./@id"/>=<xsl:apply-templates match="./s:kineticLaw"/>;
            r_sum += r_<xsl:value-of select="./@id"/>;
  </xsl:template>

  <xsl:template name="main">
/*
 * Global (static) data [!XSL]:
 */
static volatile ctrlblk_t cb         __attribute__((aligned(128)));
static volatile T_PIV     *resblk[2] __attribute__((aligned(128)));
static uint8_t  crb=0;    /* Current result block */


static void _send_result_block(int dblk, volatile void *src, size_t len) {
    uint64_t dest;

    PING();

    if( src == NULL ) {
        src=resblk[crb];
    }
    if( len == 0 ) {
        len=cb.blksz;
    }

    /*
     * Compute target address in main memory from base address.
     */
    dest=cb.result_base+(dblk*cb.blksz);

    DPRINTF("block %d xfr from:%p to 0x%08llx, len=%lu\n", dblk, src, dest, len);

    /*
     * Initiate transfer from current just-filled buffer. A barrier/fence
     * is *not* necessary because we wait (in the following stanza) to 
     * make sure we don't start computing a new block until the old
     * transfer is complete.
     */
    mfc_put(src, dest, len, crb, 0, 0);

    /*
     * Switch to the other buffer and make sure that the previous 
     * transfer from it has actually completed
     */
    crb = (crb==0);
    mfc_write_tag_mask( 1 &lt;&lt; crb );
    mfc_read_tag_status_all();

    PING();
}


int main(uint64_t speid, uint64_t argp)
{
    T_PIV *popn;
    summblk_t *sb;		/* Summary block		*/
    int b, s;
    int n_rsets;
    uint64_t nr_abs;

    register vec_uint4 flags;
    uint32_t flg;

    /*
     * Common constants: these are used a lot
     */ 
    register const vec_int4 c_1_in_pos_0 = {1, 0, 0, 0};
    register const vec_int4 c_1_in_pos_1 = {0, 1, 0, 0};
    register const vec_int4 c_1_in_pos_2 = {0, 0, 1, 0};
    register const vec_int4 c_1_in_pos_3 = {0, 0, 0, 1};

    /*
     * Common global variables
     */
    register v4sf r_sum;

    register v4sf t;
    register v4sf tau;
    register v4sf t_stopv;
    register v4sf kea, tmp1, tmp2;

    register float tmp;

    v4su rcount[N_REACTIONS];


    /***********************************************\
     * BEGIN MODEL-DEPENDENT VARIABLE DECLARATIONS *
    \***********************************************/
    <xsl:call-template name="constants" />
    <xsl:call-template name="modvars"   />
    /*********************************************\
     * END MODEL-DEPENDENT VARIABLE DECLARATIONS *
    \*********************************************/

    #ifndef RA_vf_1
    register const v4sf vf_1 = {1.0f, 1.0f, 1.0f, 1.0f};
    #define RA_vf_1
    #endif

    #ifndef RA_vf_0
    register const v4sf vf_0 = {0.0f, 0.0f, 0.0f, 0.0f};
    #define RA_vf_0
    #endif

    (void)speid; /* Suppress warning */

    resblk[0] = (T_PIV *)malloc_align( cb.blksz, 7 ); /* 7=lg(128) */
    resblk[1] = (T_PIV *)malloc_align( cb.blksz, 7 ); /* 7=lg(128) */

    /* Initialize the reaction counters */
    for(b=0; b&lt;N_REACTIONS; b++) {
	rcount[b]=(v4su){0,0,0,0};
    }
    
    /* Read the complete control-block from the PPU */
    mfc_get(&amp;cb, argp, sizeof(ctrlblk_t), 0, 0, 0);
    mfc_write_tag_mask( 1 );
    mfc_read_tag_status_all();


    DU_PRINTF("Read complete control block\n");
    as_srand( cb.seed );

    t_stopv = spu_splats(cb.t_stop);


    /* Get going! */
    for(b=0; b &lt; cb.n_blks; b++) {
        n_rsets=cb.n_rsets_per_blk;
	if(b==cb.n_blks-1) {
	    n_rsets=cb.n_rsets_residual;
	}
        for(s=0; s &lt; n_rsets; s++) {
	    <xsl:call-template name="compute-trajectory" />

	    /*
	     * Save values from registers into current block
             */
	    popn = resblk[crb]+s*N_SPECIES;//*sizeof(v4si);

            <xsl:call-template name="save-pops"/>
        }

	_send_result_block(b, NULL, 0);
    }

    nr_abs=0LL;
    for(b=0; b&lt;N_REACTIONS; b++) {
        nr_abs += spu_extract(rcount[b],0);
        nr_abs += spu_extract(rcount[b],1);
        nr_abs += spu_extract(rcount[b],2);
        nr_abs += spu_extract(rcount[b],3);
    }

    /*
     * The summary data is sent in the extra block
     */
    PING();
    sb = (summblk_t *)resblk[crb];
    sb->nr_abs = nr_abs;
//    sb->nr_con = nr_con;
    _send_result_block(cb.n_blks, NULL, 0);


    /*
     * Make sure that all transfers are complete before exit.
     * We've only used channels 0 and 1.
     */
    mfc_write_tag_mask( 3 );
    mfc_read_tag_status_all();

    return 0;
}
&#10;</xsl:template>
</xsl:transform>
