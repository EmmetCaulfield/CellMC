#ifndef ERROR_MACROS_H
#define ERROR_MACROS_H
/**
 * @file    error-macros.h
 * @brief   Generally useful error-check macros.
 * @author  Emmet Caulfield
 * @version 0.1
 *
 * Contains fairly crude error-checking macros to enable bailing with
 * a reasonably consistent-looking and meaningful error message under
 * a variety of circumstances.
 *
 * $Id: error-macros.h 83 2009-02-02 00:24:46Z emmet $
 */

#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <errno.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

/** @def RCHECK(C,F)
 *
 * A crude macro intended to be called directly after a library/system
 * call with condition, C, and the name of the function called, F. If
 * C fails, a pretty message is produced and the program aborts. We
 * assume that the function-call is on the previous line.
 */
#define RCHECK(VAR,TST,FN) if(!(VAR TST)) {				\
        int errsv=errno;						\
	fprintf(stderr, "ERROR: Test " #VAR #TST " failed ("		\
		#VAR " was %ld) at %s:%d:%s() in call to " #FN "(): ",	\
		(long)VAR, __FILE__, __LINE__-1, __func__);		\
        if( errsv > 0 )							\
  	    fputs(strerror(errsv), stderr);				\
	else if ( (long)VAR > 0 )					\
  	    fputs(strerror((long)VAR), stderr);				\
        else								\
            fputs("[API-specific error]", stderr);			\
        fputc('\n', stderr);						\
	exit(1);							\
    }

/** @def DIE_IF(C,M)
 *
 * A crude macro which fails with message M if condition C is true.
 */
#define DIE(...) {						\
	fprintf(stderr, "FATAL: %s:%d:%s(): ",			\
		basename(__FILE__), __LINE__, __func__);	\
	fprintf(stderr, __VA_ARGS__);				\
	fputs(".\n", stderr);					\
	exit(EXIT_FAILURE);					\
    }
#define DIE_IF(C,...) if(C) DIE(__VA_ARGS__)


/** @def WARN_IF(C,M)
 *
 * A crude macro which warns and continues with message M if condition
 * C is true.
 */
#define WARN(...) {						\
	fprintf(stderr, "WARNING: %s:%d:%s(): ",		\
		basename(__FILE__), __LINE__, __func__);	\
	fprintf(stderr, __VA_ARGS__);				\
	fputs(".\n", stderr);					\
    }
#define WARN_IF(C,...) if(C) WARN(__VA_ARGS__)


/** @def CLOG(M)
 *
 * Console logs: logs a message to stdout with the name of this
 * executable prepended.
 */
#if defined(PACKAGE_NAME)
#   define CLOG(...) printf(PACKAGE_NAME ": " __VA_ARGS__)
#elif defined(EXE)
#   define CLOG(...) printf(EXE ": " __VA_ARGS__)
#else
#   error "No name defined in CLOG()"
#endif


/** @def PING()
 *
 * Logs filename, function, and line number to the console
 */
#if defined(PONG)
#   define PING() fprintf(stderr, "PING: %s:%d:%s()\n", basename(__FILE__), __LINE__, __func__)
#else
#   define PING()
#endif


/** @def DPRINTF()
 *
 * Logs filename, function, and line number to the console
 */
#if defined(DEBUG)
#   if defined(__PPU__)
#       define DPRINTF(...) fprintf(stderr, EXE "(PPU)> " __VA_ARGS__)
#   elif defined(__SPU__)
#       define DPRINTF(...) { fprintf(stderr, EXE "(SPU %02d)> ", _thr_id); fprintf(stderr, __VA_ARGS__); }
#   else
#       define DPRINTF(...) fprintf(stderr, PACKAGE_NAME "> " __VA_ARGS__)
#   endif
#else
#   define DPRINTF(...)
#endif

#endif /* ERROR_MACROS_H */
