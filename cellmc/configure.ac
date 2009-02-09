# -*- Autoconf -*-
# $Id: configure.ac 89 2009-02-02 20:26:06Z emmet $

# Warning! Warning! Warning!
#

# Failure to distinguish between the 'cellmc' application and the what
# it uses to compile models, and extra programs that it needs to do
# this, can cause huge confusion!  

# On IA32, 'cellmc' is built with 'gcc' and uses it to compile models.
# HOWEVER, on CELL, 'cellmc' is built with 'gcc', but uses 4 different
# programs to build the models, NONE OF WHICH are the regular 'gcc':
# 'spu-gcc', 'ppu-gcc', 'ppu-embedspu', and 'ppu-ar'. What this means
# is that on IA32, a single check below can serve a dual-role of
# making sure that 'cellmc' will compile and that 'cellmc' will be
# likely to be able to compile a model. HOWEVER, on CELL, the two are
# more distinct. We *DO* perform some additional checks here to make
# sure that 'cellmc' has the programs that it needs at runtime, in
# spite of the fact that their absence will not inhibit 'cellmc'
# itself from being built. To make this clearer, checks are tagged as
# 'NART' (for, "needed at runtime") and/or 'RCTC' (for "regular
# compile-time check").

# Define an m4 macro for the SVN revision
define([svnversion], esyscmd([sh -c "svnversion|tr -d '\n'"]))

# Basic setup:
AC_PREREQ([2.59])
AC_INIT([cellmc], [0.2.svnversion], [emmet-cellmc@caulfield.info])
AC_DEFINE(SVN_REVISION, "svnversion", [SVN Revision])
AC_REVISION([$Rev$])
AC_CONFIG_AUX_DIR([config])

AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([1.9.2 foreign -Wall])

# RCTC
AC_CONFIG_SRCDIR([src/cellmc.c])
# RCTC
AC_CONFIG_HEADERS([config.h data/include/cellmc.h])

AC_SUBST([WARN_CFLAGS], ["-Wall -Wextra"])

# The 'cellmc' application is in pure C
AC_LANG([C])# RTCT

# The 'cellmc' application itself is built with the regular 'gcc'
AC_PROG_CC([gcc]) # RCTC, NART:IA32

# Check for packages
PKG_CHECK_MODULES([libXML], [libxml-2.0 >= 2.6.16])
PKG_CHECK_MODULES([libXML], [libxml-2.0 >= 2.6.26], 
	AC_DEFINE(HAVE_XSD_VALIDATION, [1], [LibXML XSD validation works]), 
	AC_MSG_WARN([XML schema validation of SBML models turned off by default.]))
PKG_CHECK_MODULES([libxslt], [libxslt >= 1.1.11])


# Checks for typedefs, structures, and compiler characteristics
# required on both platforms.
AC_HEADER_STDBOOL
AC_C_CONST
AC_CHECK_HEADERS([limits.h getopt.h])
AC_CHECK_FUNCS([getopt_long], [], AC_MSG_ERROR([getopt_long is required]))
AC_CHECK_FUNCS([posix_memalign memalign malloc_align]) 

AC_CONFIG_FILES([Makefile src/Makefile])         # RCTC

# We need this to decide what to build in Makefile.am

case $target_cpu in
    i686|x86_64)
	AC_DEFINE(ARCH_IA32, 1, [For Intel or AMD])	# RCTC
	CFLAGS="$CFLAGS -msse2"				# Necessary for older gccs
	AC_CHECK_HEADERS([emmintrin.h mm_malloc.h])	# NART
	AM_CONDITIONAL([ARCH_CELL], [false])
    ;;
    powerpc64)
	AC_DEFINE(ARCH_CELL, 1, [For Cell/BE])               # RCTC
	AC_CHECK_PROG([SPU_GCC],      [spu-gcc],      [yes]) # NART
	AC_CHECK_PROG([PPU_GCC],      [ppu-gcc],      [yes]) # NART
	AC_CHECK_PROG([PPU_EMBEDSPU], [ppu-embedspu], [yes]) # NART
	AC_CHECK_PROG([PPU_AR],       [ppu-ar],       [yes]) # NART
	AC_CHECK_HEADERS([libspe2.h])                        # NART
	AM_CONDITIONAL([ARCH_CELL], [true])
#	AC_CONFIG_FILES([data/lib/ppu/Makefile])
    ;;
    * )
        AC_MSG_ERROR([$target_cpu not recognised])
    ;;
esac

case $target_os in
     darwin*)
         AC_DEFINE(OS_DARWIN, 1, [For Mac OS])
     ;;
     linux-*)
         AC_DEFINE(OS_LINUX, 1, [For Linux])
     ;;
     *)
         AC_MSG_ERROR([$target_os not supported])
     ;;
esac


AM_PROG_CC_C_O

#AC_TYPE_SIZE_T
#AC_HEADER_TIME
#AC_STRUCT_TM

# Checks for library functions.
#AC_FUNC_ERROR_AT_LINE
#AC_FUNC_MALLOC
#AC_FUNC_REALLOC
#AC_FUNC_STRTOD
#AC_CHECK_FUNCS([])

AC_OUTPUT
