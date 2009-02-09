#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <stdio.h>
#include <config.h>
#include <autodirs.h>
#include <cellmc.h>

#define AC_EXE			"a.out"
#define AC_DIR_SEP		":"
#define AC_PATH_SEP		"/"
#define AC_TMPDIR		P_tmpdir

#define AC_SDKDIR		"/opt/cell/sdk"

#define AC_XSD_PATH		DATADIR "/sbml/schema"		AC_DIR_SEP \
				TOPDIR "/data/sbml/schema" 

#if defined(ARCH_CELL)
#   define AC_XSL_PATH		DATADIR "/xsl/cell"		AC_DIR_SEP \
				TOPDIR "/data/xsl/cell"

#   define AC_AUX_PATH		DATADIR "/include/cell/ppu"	AC_DIR_SEP \
				DATADIR "/include"		AC_DIR_SEP \
				TOPDIR "/data/include/cell/ppu" AC_DIR_SEP \
				TOPDIR "/data/include"
#elif defined(ARCH_IA32)
#   define AC_XSL_PATH		TOPDIR "/data/xsl/ia32"		AC_DIR_SEP \
				DATADIR "/xsl/ia32"

#   define AC_AUX_PATH		DATADIR "/include"		AC_DIR_SEP \
				TOPDIR "/data/include" 
#endif

/*
 * Default optimization-leve for 'gcc'
 */
#define AC_GCC_OPTIM            '3'

#define AC_XSL_PARAMS_MAX	16
#define AC_GCC_ARGS_MAX         64
#define AC_LBL_MAXLEN           64	/* At most 96 bytes free in CELL control block */

#define AC_SBML_XSD_FILE	"sbml-l2v3.xsd"
#define AC_CFLAGS		NULL

#define AC_C_FMT		"%s.c"

#if defined(ARCH_IA32)
#   define AC_GCC		"gcc"
#   define AC_STRIP		"strip"
#elif defined(ARCH_CELL)
#   define _ac_QUOTEME(s) #s
#   define _ac_QQ(s) _ac_QUOTEME(s)
#   define AC_SPU_GCC		"spu-gcc"
#   define AC_SPU_STRIP         "spu-strip"
#   define AC_PPU_EMBEDSPU	"ppu-embedspu"
#   define AC_PPU_AR		"ppu-ar"
#   define AC_PPU_GCC		"ppu-gcc"
#   define AC_SPU_HANDLE	_ac_QQ(CMC_SPU_CODE_HANDLE_NAME)
#   define AC_SPU_EXE_FMT	"%s.spu"
#   define AC_PPU_EMB_FMT	"%s.spu.o"
#   define AC_PPU_AR_FMT	"%s.spu.a"
#else
#   error Architecture not specified
#endif


/*
 * max-inline-instns-single is too small by default with the gcc that
 * ships with some popular Linux distributions (it seems to be 300
 * with the gcc-4.1.2 on CentOS/RHEL 5). The problem is that gcc's
 * tree-inliner can give horrible compile-time performance if this
 * parameter is set too high, and the value changes from version to
 * version. The default on the latest gcc used is 450 and there's no
 * problem, but 450 is not high enough with gcc-4.1.2 on RHEL/Centos,
 * so I'm guessing that 600 is not unreasonable.
 */
#define AC_GCC_PARAM_MAX_INLINE_INSNS_SINGLE "600"


/*
 * Content MathML namespace inside SBML documents
 */
#define AC_SBML_NS_URI   "http://www.sbml.org/sbml/level2/version3"
#define AC_SBML_NS_PFX   "s"

#define AC_MATHML_NS_URI "http://www.w3.org/1998/Math/MathML"
#define AC_MATHML_NS_PFX "m"

#define AC_SVN_NS_URI    "http://svnbook.red-bean.com/en/1.4/svn.advanced.props.special.keywords.html"
#define AC_SVN_NS_PFX    "svn"

/*
 * XPath expressions, using the above prefixes, tried in order until a
 * label is found:
 */
#define AC_LBL_XPATH_1   "/s:sbml/s:annotation/svn:subversion/svn:Id"
#define AC_LBL_XPATH_2   "/s:sbml/s:model/@name"
#define AC_LBL_XPATH_3   "/s:sbml/s:model/@id"

/*
 * XPath expression that finds all the reactions for determining N_RXNS
 */
#define AC_N_RXNS_XPATH  "/s:sbml/s:model/s:listOfReactions/s:reaction"
#define AC_N_RXNS_MIN 2
#define AC_N_RXNS_MAX 100

/*
 * XPath expression that finds the species for determining N_SPXS
 */
#define AC_N_SPXS_XPATH  "/s:sbml/s:model/s:listOfSpecies/s:species"
#define AC_N_SPXS_MIN 1
#define AC_N_SPXS_MAX 100


#endif /* APPCONFIG_H */
