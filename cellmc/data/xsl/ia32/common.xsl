<?xml version="1.0"?>
<!--
 | @file    common.xsl
 | @brief   Common templates
 | @author  Emmet Caulfield.
 | @version 0.1
 |
 | $Id: common.xsl 53 2009-01-26 03:13:46Z emmet $
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
<!--      <xsl:message>$LPR != 'none'</xsl:message> -->
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
<!--    <xsl:message>adjust-prop</xsl:message> -->
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
