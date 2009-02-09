<?xml version="1.0"?>
<!--
 | @file    kineticLaw.xsl
 | @brief   To transform a kineticLaw element containing prefix
 |		Content MathML to infix C. We let gcc worry about
 |              the conversion to SSE2/SPU instructions, which
 |              (by decompilation) we know it does fine.
 |
 |           Because libxslt only supports XSL-T 1.0, we don't
 |           have support for XSL-T 2.0 "modes", so we must
 |           duplicate some templates. This is less unwieldy
 |           than trying to route a parameter to duplicate the
 |           missing functionality. The empty/default mode
 |           is implicitly scalar and the vector mode is
 |           explicitly "vector".
 |           
 | @author  Emmet Caulfield.
 | @version 0.1
 |
 | $Id: kineticLaw.xsl 25 2009-01-20 21:23:21Z emmet $
-->

<xsl:transform
  xmlns:s="http://www.sbml.org/sbml/level2/version3"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:m="http://www.w3.org/1998/Math/MathML"
  version="1.0"
>

<!--
  <xsl:param name="PREC">float</xsl:param>
-->

  <xsl:template match="s:kineticLaw">
    <xsl:apply-templates select="child::*"/>
  </xsl:template>

  <xsl:template match="s:kineticLaw" mode="vector">
    <xsl:apply-templates select="child::*" mode="vector"/>
  </xsl:template>


  <xsl:template match="m:apply">
    <xsl:text>(</xsl:text>
      <xsl:apply-templates select="m:times|m:plus|m:minus|m:divide"/>
    <xsl:text>)</xsl:text>
  </xsl:template>

  <xsl:template match="m:apply" mode="vector">
    <xsl:text>(</xsl:text>
      <xsl:apply-templates select="m:times|m:plus|m:minus|m:divide" mode="vector"/>
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

  <xsl:template match="m:times" mode="vector">
    <xsl:for-each select="following-sibling::*">
      <xsl:apply-templates select="self::*" mode="vector"/>
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

  <xsl:template match="m:divide" mode="vector">
    <xsl:for-each select="following-sibling::*">
      <xsl:apply-templates select="." mode="vector"/>
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

  <xsl:template match="m:plus" mode="vector">
    <xsl:for-each select="following-sibling::*">
      <xsl:apply-templates select="." mode="vector"/>
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

  <xsl:template match="m:minus" mode="vector">
    <xsl:for-each select="following-sibling::*">
      <xsl:apply-templates select="." mode="vector"/>
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

  <xsl:template match="m:cn" mode="vector">
    <xsl:choose>
      <xsl:when test=". = 1">
	<xsl:choose>
	  <xsl:when test="$PREC='single'">
            <xsl:text>UV_1_4sf</xsl:text>
	  </xsl:when>
	  <xsl:when test="$PREC='double'">
            <xsl:text>UV_1_2df</xsl:text>
	  </xsl:when>
	  <xsl:otherwise>
	    <xsl:message terminate="yes">Unrecognised PREC parameter</xsl:message>
	  </xsl:otherwise>
	</xsl:choose>
      </xsl:when>
      <xsl:when test=". = 0">
	<xsl:choose>
	  <xsl:when test="$PREC='single'">
            <xsl:text>UV_0_4sf</xsl:text>
	  </xsl:when>
	  <xsl:when test="$PREC='double'">
            <xsl:text>UV_0_2df</xsl:text>
	  </xsl:when>
	  <xsl:otherwise>
	    <xsl:message terminate="yes">Unrecognised PREC parameter</xsl:message>
	  </xsl:otherwise>
	</xsl:choose>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="vectorize">
          <xsl:with-param name="type"  select="'fpn'" />
	  <xsl:with-param name="value" select="translate(.,' ','')"/>
	</xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>


  <!-- 
       These assume that species populations are integers, which must
       be cast to FPNs, and parameters are FPNs, which ought to be
       left as-is
   -->
  <xsl:template match="m:ci">
    <xsl:choose>
      <xsl:when test="//s:species/@id = normalize-space(.)">
         <xsl:text>((</xsl:text>
	 <xsl:choose>
	   <xsl:when test="$PREC='single'">
	     <xsl:text>float</xsl:text>
	   </xsl:when>
	   <xsl:when test="$PREC='double'">
	     <xsl:text>double</xsl:text>
	   </xsl:when>
	   <xsl:otherwise>
	     <xsl:message terminate="yes">Unrecognised PREC parameter</xsl:message>
	   </xsl:otherwise>
	 </xsl:choose>
	 <xsl:text>)SPOP_</xsl:text>
	 <xsl:value-of select="translate(.,' ','')"/>
	 <xsl:text>(SLOT))</xsl:text>
      </xsl:when>
      <xsl:otherwise> <!-- type='real|rational|e-notation' -->
         <xsl:text>SP_</xsl:text>
	 <xsl:value-of select="translate(.,' ','')"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

<!-- Gratuitous ASCII bunny:

	(\_/)
	('.')
	(")_(")
-->
  <xsl:template match="m:ci" mode="vector">
    <xsl:choose>
      <xsl:when test="//s:species/@id = normalize-space(.)">
         <xsl:text>CVT_VEC_ITOF(VPOP_</xsl:text>
	 <xsl:value-of select="translate(.,' ','')"/>
	 <xsl:text>)</xsl:text>
      </xsl:when>
      <xsl:otherwise> <!-- type='real|rational|e-notation' -->
         <xsl:text>VP_</xsl:text>
	 <xsl:value-of select="translate(.,' ','')"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- This only makes sense in vector mode -->
  <xsl:template name="vectorize" mode="vector">
    <xsl:param name="value" />
    <xsl:param name="type"  />

    <xsl:text>((</xsl:text>
      <xsl:choose>
	<xsl:when test="$type='fpn'">
	  <xsl:choose>
	    <xsl:when test="$PREC='single'">
	      <xsl:text>v4sf</xsl:text>
	    </xsl:when>
	    <xsl:when test="$PREC='double'">
	      <xsl:text>v2df</xsl:text>
	    </xsl:when>
	    <xsl:otherwise>
	      <xsl:message terminate="yes">Unrecognised PREC parameter</xsl:message>
	    </xsl:otherwise>
	  </xsl:choose>
	</xsl:when>
	<xsl:otherwise>
	  <xsl:choose>
	    <xsl:when test="$PREC='single'">
	      <xsl:text>v4si</xsl:text>
	    </xsl:when>
	    <xsl:when test="$PREC='double'">
	      <xsl:choose>
		<xsl:when test="$ARCH='IA32'">
		  <xsl:text>v2si</xsl:text>
		</xsl:when>
		<xsl:otherwise>
		  <xsl:text>v2di</xsl:text>
		</xsl:otherwise>
	      </xsl:choose>
	    </xsl:when>
	    <xsl:otherwise>
	      <xsl:message terminate="yes">Unrecognised PREC parameter</xsl:message>
	    </xsl:otherwise>
	  </xsl:choose>
	</xsl:otherwise>
      </xsl:choose>
    <xsl:text>){</xsl:text>

    <xsl:choose>
      <xsl:when test="$type='fpn' and string(number($value))!='NaN'">
	<xsl:if test="PREC='double'">
          <xsl:call-template name="format-as-fpn">
	    <xsl:with-param name="value" select="$value"/>
          </xsl:call-template>
          <xsl:text>,</xsl:text>
          <xsl:call-template name="format-as-fpn">
	    <xsl:with-param name="value" select="$value"/>
          </xsl:call-template>
          <xsl:text>,</xsl:text>
	</xsl:if>
        <xsl:call-template name="format-as-fpn">
	  <xsl:with-param name="value" select="$value"/>
        </xsl:call-template>
        <xsl:text>,</xsl:text>
        <xsl:call-template name="format-as-fpn">
	  <xsl:with-param name="value" select="$value"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
	<xsl:if test="$PREC='single'">
          <xsl:value-of select="$value"/>
          <xsl:text>,</xsl:text>
          <xsl:value-of select="$value"/>
          <xsl:text>,</xsl:text>
	</xsl:if>
        <xsl:value-of select="$value"/>
        <xsl:text>,</xsl:text>
        <xsl:value-of select="$value"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>})</xsl:text>
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
    <xsl:choose>
      <xsl:when test="$PREC='single'">
        <xsl:text>f</xsl:text>
      </xsl:when>
      <xsl:when test="$PREC='double'"/>
      <xsl:otherwise>
	<xsl:message terminate="yes">Unrecognised PREC parameter</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:transform>
