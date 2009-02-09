<?xml version="1.0"?>
<!--
 | @file    common.xsl
 | @brief   Common templates
 | @author  Emmet Caulfield.
 | @version 0.1
 |
 | $Id: neutral.xsl 28 2009-01-20 23:45:04Z emmet $
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

  <xsl:import href="kineticLaw.xsl" />

  <xsl:output
    method="text"
    indent="no"
    media-type="text/plain" />

  <xsl:strip-space elements="*"/>

  <xsl:variable name="UCASE">ABCDEFGHIJKLMNOPQRSTUVWXYZ</xsl:variable>
  <xsl:variable name="LCASE">abcdefghijklmnopqrstuvwxyz</xsl:variable>

  <xsl:template match="/">
    <xsl:call-template name="boiler"       />
    <xsl:call-template name="includes"     />
    <xsl:call-template name="defines"      />
    <xsl:call-template name="enums"        />
    <xsl:call-template name="popn-const"   />
    <xsl:call-template name="macros"       />
    <xsl:call-template name="prop-funcs"   />
    <xsl:call-template name="update-rates" />
    <xsl:call-template name="main"         />
  </xsl:template>


<!-- Suppresses a weird appearance of svn: data in
     apply-templates match=".../s:reaction" mode="rates" -->

  <xsl:template match="svn:*|k:*"/>
  <xsl:template match="svn:*|k:*" mode="rates"/>
  <xsl:template match="svn:*|k:*" mode="const"/>


<!-- 
=======================================================
Output #includes
=======================================================
--> 
  <xsl:template name="includes">
    <xsl:text>
/*
 * Included libraries [name="includes"]:
 */
#include &lt;support.h&gt;
</xsl:text>
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
Output enums for species and reactions. Not strictly
necessary, but makes C more readable.
======================================================= 
-->
  <xsl:template name="enums">
    <xsl:text>
/*
 * Enumerations [name="enums"]
 *
 * i_ => index of
 */
</xsl:text>

    <xsl:apply-templates select="/s:sbml/s:model/s:listOfSpecies"   mode="enum"/>
    <xsl:apply-templates select="/s:sbml/s:model/s:listOfReactions" mode="enum"/>
    <xsl:text>&#10;</xsl:text>
  </xsl:template>

  <xsl:template match="s:listOfSpecies|s:listOfReactions" mode="enum">
    <xsl:text>enum {&#10;</xsl:text>
    <xsl:apply-templates select="s:species|s:reaction" mode="enum"/>
    <xsl:text>    N_</xsl:text>
    <xsl:value-of select="translate(substring-after(local-name(), 'listOf'), $LCASE, $UCASE)"/>
    <xsl:text>&#10;};&#10;</xsl:text>
  </xsl:template>

  <xsl:template match="s:species|s:reaction" mode="enum">
    <xsl:text>    i_</xsl:text>
    <xsl:value-of select="./@id"/>
    <xsl:text>,		/* </xsl:text>
    <xsl:value-of select="position()-1"/>
    <xsl:text> */&#10;</xsl:text>
  </xsl:template>
<!--================================================-->


<!--
=======================================================
Output parameters as manifest constants (#defines).
======================================================= 
-->
  <xsl:template name="defines">
    <xsl:text>
/* 
 * [name="defines"] 
 *
 * SP_ => Scalar Parameter (constant macro)
 * VP_ => Vector Parameter (constant macro)
 * SS_ => Scalar Species (constant macro)
 * VS_ => Vector Species (constant macro)
 */
</xsl:text>
    <xsl:apply-templates select="//s:parameter" mode="define"/>
    <xsl:text>&#10;</xsl:text>
    <xsl:apply-templates select="//s:species"   mode="define"/>
  </xsl:template>

  <!-- Output as preprocessor #defines -->
  <xsl:template match="s:parameter" mode="define">
    <xsl:text>#define SP_</xsl:text>
    <xsl:value-of select="./@id"/>
    <xsl:text> </xsl:text>
    <xsl:call-template name="format-as-fpn">
      <xsl:with-param name="value" select="./@value"/>
    </xsl:call-template>
    
    <xsl:text>&#10;#define VP_</xsl:text>
    <xsl:value-of select="./@id"/>
    <xsl:text> </xsl:text>
    <xsl:call-template name="vectorize">
      <xsl:with-param name="value" select="concat('SP_',./@id)"/>
      <xsl:with-param name="type"  select="'fpn'"/>
    </xsl:call-template>
    <xsl:text>&#10;</xsl:text>
  </xsl:template>

  <xsl:template match="s:species" mode="define">
    <xsl:text>#define SS_</xsl:text>
    <xsl:value-of select="./@id"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="./@initialAmount"/>
    <xsl:text>&#10;#define VS_</xsl:text>
    <xsl:value-of select="./@id"/>
    <xsl:text> </xsl:text>
    <xsl:call-template name="vectorize">
      <xsl:with-param name="value" select="concat('SS_',./@id)"/>
      <xsl:with-param name="type"  select="'int'"/>
    </xsl:call-template>
    <xsl:text>&#10;</xsl:text>
  </xsl:template>


  <xsl:template name="macros">
    <xsl:text>
/* 
 * [name="macros"] 
 *
 * SPOP => Scalar Population Macro (variable macro)
 * VPOP => Vector Population Macro (variable macro)
 *
 * These are macros which depend on a vector of species
 * population vectors, called 'popn', being in scope.
 * They evaluate to the current value of a particular
 * species population, rather than the constant value
 * in the corresponding SS_ and VS_ #defines.
 */
</xsl:text>
    <xsl:text>#define SPOP(SPECIES,SLOT) popn[SPECIES].i32[SLOT]&#10;</xsl:text>
    <xsl:text>#define VPOP(SPECIES)      popn[SPECIES].si&#10;</xsl:text>
    <xsl:apply-templates select="//s:species" mode="macro"/>
  </xsl:template>

  <xsl:template match="s:species" mode="macro">
    <xsl:text>#define SPOP_</xsl:text>
    <xsl:value-of select="./@id"/>
    <xsl:text>(SLOT) SPOP(i_</xsl:text>
    <xsl:value-of select="./@id"/>
    <xsl:text>,SLOT)&#10;</xsl:text>

    <xsl:text>#define VPOP_</xsl:text>
    <xsl:value-of select="./@id"/>
    <xsl:text> VPOP(i_</xsl:text>
    <xsl:value-of select="./@id"/>
    <xsl:text>)&#10;</xsl:text>
  </xsl:template>


<!--================================================-->


<!-- 
=======================================================
Top boilerplate for output code
=======================================================
--> 
  <xsl:template name="boiler">
    <xsl:text>
/*****************************************************\
 * Code generated by CellMC $Rev$ 
\******************************************************/
</xsl:text>
  </xsl:template>
<!--================================================-->

</xsl:transform>
