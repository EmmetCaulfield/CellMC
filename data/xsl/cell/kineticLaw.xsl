<?xml version="1.0"?>
<!--
 | @file    kineticLaw.xsl
 | @brief   To transform a kineticLaw element containing prefix
 |		Content MathML to infix C. We let the compiler
 |              worry about the conversion to SPU intrinsics.
 | @author  Emmet Caulfield.
 | @version 0.1
 |
 | $Id: kineticLaw.xsl 17 2009-01-19 15:44:44Z emmet $
-->

<!-- Preamble: -->
<xsl:transform
  xmlns:s="http://www.sbml.org/sbml/level2/version3"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:m="http://www.w3.org/1998/Math/MathML"
  version="1.0"
>

  <xsl:template match="s:kineticLaw">
    <xsl:apply-templates select="child::*"/>
  </xsl:template>

  <xsl:template match="m:apply">
    <xsl:text>(</xsl:text>
      <xsl:apply-templates select="m:times|m:plus|m:minus|m:divide"/>
    <xsl:text>)</xsl:text>
  </xsl:template>

  <xsl:template match="m:times">
    <xsl:for-each select="following-sibling::*">
      <xsl:apply-templates select="self::*"/>
      <xsl:if test="position()!=last()">
        <xsl:text>*</xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="m:divide">
    <xsl:for-each select="following-sibling::*">
      <xsl:apply-templates select="."/>
      <xsl:if test="position()!=last()">
        <xsl:text>/</xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="m:plus">
    <xsl:for-each select="following-sibling::*">
      <xsl:apply-templates select="."/>
      <xsl:if test="position()!=last()">
        <xsl:text>+</xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="m:minus">
    <xsl:for-each select="following-sibling::*">
      <xsl:apply-templates select="."/>
      <xsl:if test="position()!=last()">
        <xsl:text>-</xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>


  <xsl:template match="m:cn">
    <xsl:choose>
      <xsl:when test=". = 1">
        <xsl:text>vf_1</xsl:text>
      </xsl:when>
      <xsl:when test=". = 0">
        <xsl:text>vf_0</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="quadify">
          <xsl:with-param name="type"  select="'vec_float4'" />
	  <xsl:with-param name="value" select="translate(.,' ','')"/>
	</xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>


  <!-- In the context of the Content MathML used for kineticLaw -->
  <!-- elements, vectors of integers must be cast to vectors of -->
  <!-- floats with the SPU intrinsic 'spu_convtf'. It should be -->
  <!-- safest to leave floats uncast                            -->
  <xsl:template match="m:ci">
    <xsl:choose>
      <xsl:when test="//s:species/@id = normalize-space(.)">
         <xsl:text>spu_convtf(s_</xsl:text>
	 <xsl:value-of select="translate(.,' ','')"/>
	 <xsl:text>,0)</xsl:text>
      </xsl:when>
      <xsl:otherwise> <!-- type='real|rational|e-notation' -->
         <xsl:text>C_</xsl:text>
	 <xsl:value-of select="translate(.,' ','')"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>



  <xsl:template name="quadify">
    <xsl:param name="value" />
    <xsl:param name="type"  />

    <xsl:text>((</xsl:text>
    <xsl:value-of select="$type" />
    <xsl:text>){</xsl:text>

    <xsl:choose>
      <xsl:when test="$type='vec_float4'">
        <xsl:call-template name="format-as-float">
	  <xsl:with-param name="value" select="$value"/>
        </xsl:call-template>
        <xsl:text>,</xsl:text>
        <xsl:call-template name="format-as-float">
	  <xsl:with-param name="value" select="$value"/>
        </xsl:call-template>
        <xsl:text>,</xsl:text>
        <xsl:call-template name="format-as-float">
	  <xsl:with-param name="value" select="$value"/>
        </xsl:call-template>
        <xsl:text>,</xsl:text>
        <xsl:call-template name="format-as-float">
	  <xsl:with-param name="value" select="$value"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$value"/>
        <xsl:text>,</xsl:text>
        <xsl:value-of select="$value"/>
        <xsl:text>,</xsl:text>
        <xsl:value-of select="$value"/>
        <xsl:text>,</xsl:text>
        <xsl:value-of select="$value"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>})</xsl:text>
  </xsl:template>


  <xsl:template name="format-as-float">
    <xsl:param name="value"/>
    <xsl:choose>
      <xsl:when test="contains($value, '.')">
        <xsl:value-of select="$value"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat($value, '.0')"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>f</xsl:text>
  </xsl:template>

</xsl:transform>
