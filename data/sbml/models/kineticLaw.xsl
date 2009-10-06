<?xml version="1.0"?>
<!--
 | @file    kineticLaw.xsl
 | @brief   To transform a kineticLaw element containing prefix
 |		Content MathML to infix for SBML L1V2 'formula'
 |		attribute.
 |           
 | @author  Emmet Caulfield.
 | @version 0.1
 |
 | $Id$
-->

<xsl:transform
  xmlns="http://www.sbml.org/sbml/level1"
  xmlns:s="http://www.sbml.org/sbml/level2/version3"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:m="http://www.w3.org/1998/Math/MathML"
  version="1.0"
>

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
    <xsl:call-template name="format-as-fpn">
      <xsl:with-param name="value" select="normalize-space(.)"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="m:ci">
    <xsl:value-of select="normalize-space(.)"/>
  </xsl:template>

  <!-- This only makes sense in scalar mode -->
  <xsl:template name="format-as-fpn">
    <xsl:param name="value"/>
    <xsl:choose>
      <xsl:when test="contains($value, '.')">
        <xsl:value-of select="$value"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat($value, '.0')"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:transform>
