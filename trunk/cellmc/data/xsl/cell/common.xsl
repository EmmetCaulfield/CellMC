<?xml version="1.0"?>
<!--
 | @file    common.xsl
 | @brief   Common templates
 | @author  Emmet Caulfield.
 | @version 0.1
 |
 | $Id: common.xsl 83 2009-02-02 00:24:46Z emmet $
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

  <xsl:import href="../neutral.xsl" />


  <xsl:template name="ident">
    <xsl:value-of select='/s:sbml/s:model/@name'/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="substring-after(substring-before(/s:sbml/s:annotation/svn:subversion/svn:Rev,' $'),': ')"/>
    <xsl:for-each select="/s:sbml/s:annotation/k:provenance/k:via">
      <xsl:text>.</xsl:text>
      <xsl:value-of select="substring-after(.,'-')"/>
    </xsl:for-each>
  </xsl:template>


  <xsl:template name="file-scope-c">
    <xsl:text>
/*
 * Global (static) data [!XSL]:
 */
static volatile  ctrlblk_t cb         __attribute__((aligned(128)));
static volatile  T_PIS     *resblk[2] __attribute__((aligned(128)));
static uint8_t   crb=0;    /* Current result block */

#ifdef DEBUG
    static uint8_t   _thr_id;  /* This SPU's ID back on the PPU side */
#endif

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
     * see make sure we don't start computing a new block until the old
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
</xsl:text>
  </xsl:template>


  <xsl:template name="get-control-block">
    /* Read the complete control-block from the PPU */
    mfc_get(&amp;cb, argp, sizeof(ctrlblk_t), 0, 0, 0);
    mfc_write_tag_mask( 1 );
    mfc_read_tag_status_all();

#ifdef DEBUG
    _thr_id=cb.thr_id;    
#endif

    DPRINTF("SPU %"PRIx64" got control block\n", speid);
    DPRINTF("\tseed = %"PRIx32"\n", cb.seed);

  </xsl:template>


  <xsl:template name="complete-outstanding-dma">
    <xsl:text>
    /*
     * Make sure that all transfers are complete before exit.
     * We've only used channels 0 and 1.
     */
    mfc_write_tag_mask( 3 );
    mfc_read_tag_status_all();
</xsl:text>
  </xsl:template>



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
    <xsl:if test="$LPR != 'none'">
      <xsl:for-each select="/s:sbml/s:model/s:listOfReactions/s:reaction[s:kineticLaw//m:ci = current()//s:speciesReference/@species]">
	<xsl:sort select="position()" data-type="number" order="descending"/>
        <xsl:call-template name="adjust-prop"/>
      </xsl:for-each>
    </xsl:if>
    <xsl:if test="$LPR = 'full'">
      <xsl:text>                    r_sum.f[e] += (newr - oldr);&#10;</xsl:text>
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


<!--================================================-->

  <xsl:template name="adjust-prop">
    <xsl:if test="$LPR='full'">
      <xsl:text>                    /* Adjusting </xsl:text>
      <xsl:value-of select="./@id"/>
      <xsl:text> */&#10;</xsl:text>
      <xsl:text>                    oldr += rate[i_</xsl:text>
      <xsl:value-of select="./@id"/>
      <xsl:text>].f[e];&#10;</xsl:text>
    </xsl:if>
    <xsl:text>                    rate[i_</xsl:text>
    <xsl:value-of select="./@id"/>
    <xsl:text>].f[e]=SEQN_</xsl:text>
    <xsl:value-of select="./@id"/>
    <xsl:text>(e);&#10;</xsl:text>
    <xsl:if test="$LPR='full'">
      <xsl:text>                    newr += rate[i_</xsl:text>
      <xsl:value-of select="./@id"/>
      <xsl:text>].f[e];&#10;</xsl:text>
    </xsl:if>
  </xsl:template>

</xsl:transform>
