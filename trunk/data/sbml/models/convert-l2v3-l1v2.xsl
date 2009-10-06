<?xml version="1.0"?>
<!--
 | @file    convert-l2v3-l1v2.xsl
 | @brief   Converts an SBML model from SBML L2V3 to L1V2
 | @author  Emmet Caulfield.
 | @version 0.1
 |
 | This does a half-reasonable job on the models we have, but it
 | is not expected to be generally useful.
 |
 | $Id$
-->

<!-- Preamble: -->
<xsl:transform
   xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
   xmlns="http://www.sbml.org/sbml/level1"
   xmlns:l2="http://www.sbml.org/sbml/level2/version3"
   xmlns:svn="http://svnbook.red-bean.com/en/1.4/svn.advanced.props.special.keywords.html"
   xmlns:k="http://polacksbacken.net/wiki/SSACBE"
   xmlns:m="http://www.w3.org/1998/Math/MathML"
   version="1.0"
   exclude-result-prefixes="l2 svn k m"
>

  <xsl:import href="kineticLaw.xsl" />

  <xsl:output
    method="xml"
    indent="no"
    media-type="text/xml" />

  <xsl:strip-space elements="*"/>

  <!-- Zap all the crud we've added and annotations -->
  <xsl:template match="svn:*|k:*"/>
  <xsl:template match="l2:annotation"/>


  <!-- Correct the attribute values on the root -->
  <xsl:template match="/l2:sbml">
    <sbml level="1" version="2">
      <xsl:apply-templates/>
    </sbml>
  </xsl:template>


  <!-- 
       The next 4 templates handle translating around the lack of
       modifiers in L1 by duplicating a modifier in the products
       and reactants that are available. This is horrible because
       of the (completely unnecessary) ordering constraints on
       the listOf* elements.
    -->
  <xsl:template match="l2:reaction">
    <xsl:element name="reaction">
      <xsl:call-template name="dupe-attrs"/>
      <xsl:apply-templates select="l2:listOfReactants"/>
      <xsl:apply-templates select="l2:listOfModifiers"/>
      <xsl:apply-templates select="l2:listOfProducts"/>
    </xsl:element>
  </xsl:template>

  <xsl:template match="l2:listOfReactants|l2:listOfProducts">
    <xsl:element name="{local-name()}">
      <xsl:apply-templates select="./l2:speciesReference"/>
      <xsl:apply-templates select="../l2:listOfModifiers/l2:modifierSpeciesReference"/>
    </xsl:element>
  </xsl:template>

  <xsl:template match="l2:modifierSpeciesReference">
    <speciesReference species="{@species}"/>
  </xsl:template>

  <xsl:template match="l2:listOfModifiers">
    <xsl:if test="not(../l2:listOfReactants)">
      <listOfReactants>
	<xsl:apply-templates select="./l2:modifierSpeciesReference"/>
      </listOfReactants>
    </xsl:if>
    <xsl:if test="not(../l2:listOfProducts)">
      <listOfProducts>
	<xsl:apply-templates select="./l2:modifierSpeciesReference"/>
      </listOfProducts>
    </xsl:if>
  </xsl:template>

  <!-- End of modifier handling -->


  <xsl:template match="l2:kineticLaw">
    <xsl:element name="kineticLaw">
      <xsl:attribute name="formula">
	<xsl:apply-templates/>
      </xsl:attribute>
    </xsl:element>
  </xsl:template>


<!-- 
     We do this lest a terrible plague of utterly unecessary namespace
     declarations be rained down upon us like locusts
-->
  <xsl:template match="l2:*">
    <xsl:element name="{local-name()}">
      <xsl:call-template name="dupe-attrs"/>
      <xsl:apply-templates />
    </xsl:element>
  </xsl:template>


  <xsl:template name="dupe-attrs">
    <xsl:for-each select="@*">
      <xsl:choose>
	<xsl:when test="local-name()='name' or local-name()='size'"/> <!-- Discard -->
	<xsl:when test="local-name()='id'">    <!-- Rename  -->
          <xsl:attribute name="name">
	    <xsl:value-of select="."/>
	  </xsl:attribute>
	</xsl:when>
	<xsl:otherwise>
	  <xsl:copy-of select="." />		<!-- Copy  -->
	</xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
  </xsl:template>


</xsl:transform>
