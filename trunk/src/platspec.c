#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <config.h>

#include <error-macros.h>
#include <option-mapping.h>

#include <platspec.h>
#include <utils.h>
#include <appconfig.h>
#include <runconfig.h>


/* 
 * FIXME: put package -I and -L directories and install equivalents
 * into separate arrays and include them in gcc or spu-gcc's argument
 * list conditionally on whether 'cellmc' is run before it's
 * installed. Maybe do this on the basis of the presence of
 * 'cellmc.c'?
 */

#define PS_GCC_ARGS_COMMON "-Wextra", "-Wall", "-Winline", "-Wno-main"

/*
 * Stub names of C source code that has to be compiled for each
 * model. The appropriate extensions are added on as needed.
 */
static const char *_ps_aux_files[] = {
#if defined(ARCH_CELL)
    "main"	,
    "ctrlblk"	,
#endif
    "sim-info"		,
    "app-support"	,
    "option-mapping"    
};
#define PS_AUX_FILES_LEN ((int)(sizeof(_ps_aux_files)/sizeof(char *)))

#define PS_AUX_PROF_OFF "final-population"
#define PS_AUX_PROF_ON  "reaction-stats"


#if defined(ARCH_IA32)
static const char *_ps_gcc_args[] = {
    PS_GCC_ARGS_COMMON			,
    "-fomit-frame-pointer"		,
    "-msse2"				,
    "--param", "max-inline-insns-single=" AC_GCC_PARAM_MAX_INLINE_INSNS_SINGLE,
    "-I" TOPDIR "/data/include"		,
    "-I" TOPDIR "/data/include/ia32"	,
    "-DHAVE_SSE"			,
    "-DMEXP=19937"
};
#define PS_GCC_ARGS_LEN ((int)(sizeof(_ps_gcc_args)/sizeof(char *)))

#elif defined(ARCH_CELL)

static const char *_ps_spu_gcc_args[] = {
    PS_GCC_ARGS_COMMON                          ,
    "-I" TOPDIR "/data/include"                 ,
    "-I" TOPDIR "/data/include/cell"            ,
    "-I" TOPDIR "/data/include/cell/spu"        ,
//    "-I/usr/spu/include"                        ,
    "-I" AC_SDKDIR "/usr/spu/include"           ,
    "-I" AC_SDKDIR "/prototype/usr/spu/include" ,
/* Linker flags */
//    "-L/usr/spu/lib"				,
    "-L" AC_SDKDIR "/usr/spu/lib"               ,
    "-L" AC_SDKDIR "/prototype/usr/spu/lib"     ,
    "-Wl,-N"
};
#define PS_SPU_GCC_ARGS_LEN ((int)(sizeof(_ps_spu_gcc_args)/sizeof(char *)))


static const char *_ps_spu_gcc_libs[] = {
/* Link libraries */
    "-lmc_rand",
    "-lmisc"
};
#define PS_SPU_GCC_LIBS_LEN ((int)(sizeof(_ps_spu_gcc_libs)/sizeof(char *)))


static const char *_ps_ppu_embedspu_args[] = {
    "-m32",
    AC_SPU_HANDLE
};
#define PS_PPU_EMBEDSPU_ARGS_LEN ((int)(sizeof(_ps_ppu_embedspu_args)/sizeof(char *)))



static const char *_ps_ppu_ar_args[] = {
    "-qcs"
};
#define PS_PPU_AR_ARGS_LEN ((int)(sizeof(_ps_ppu_ar_args)/sizeof(char *)))


static const char *_ps_ppu_gcc_args[] = {
    "-m32"					,
    "-maltivec"					,
    "-I" TOPDIR "/data/include"                 ,
    "-I" TOPDIR "/data/include/cell"            ,
    "-I" TOPDIR "/data/include/cell/ppu"        ,
    "-I" AC_SDKDIR "/usr/include"		,
    "-I" AC_SDKDIR "/prototype/usr/include"	,
    "-L" AC_SDKDIR "/usr/lib"			,
    "-L" AC_SDKDIR "/prototype/usr/lib"
};
#define PS_PPU_GCC_ARGS_LEN ((int)(sizeof(_ps_ppu_gcc_args)/sizeof(char *)))


#endif /* defined(ARCH_CELL) */


/* FIXME: this has to be rationalised a bit */
char * ps_choose_xsltfile(const runconfig_t * const conf)
{
    if( NULL != conf->xslfile ) {
	return( conf->xslfile );
    }
#if defined(ARCH_IA32)
    if( conf->app.prec == CMC_PREC_SINGLE ) {
	return u_search_path(AC_XSL_PATH, "simd-spfp-ssog.xsl", conf->verbose);
    }
    if( conf->app.prec == CMC_PREC_DOUBLE ) {
	return u_search_path(AC_XSL_PATH, "simd-double.xsl", conf->verbose);
    }
    DIE("Unknown precision constant");
#elif defined(ARCH_CELL)
    return u_search_path(AC_XSL_PATH, "simd.xsl", conf->verbose);
#endif
}


const char ** ps_xslt_params(runconfig_t * const conf) {
    static char parbuf[_POSIX_ARG_MAX]={0};
    static char *sv[2*(AC_XSL_PARAMS_MAX+1)]={NULL};

    char *p;	/* Pointer to current parameter in buffer	*/
    int   l;    /* Length of current parameter in buffer	*/
    int   n=0;  /* Current parameter number			*/

    p = parbuf;

    PS_CATON( "EXE" ); PS_FMTON("'%s'",  rc_exe_file(conf) );
    PS_CATON( "VER" ); PS_FMTON("'%s'",  VERSION );
    PS_CATON( "LBL" ); PS_FMTON("'%s'",  conf->app.lbl  );

    PS_CATON( "PREC" ); PS_FMTON("'%s'", om_prec_str(conf->app.prec));
    PS_CATON( "LOG"  ); PS_FMTON("'%s'", om_log_str(conf->app.log));
    PS_CATON( "RNG"  ); PS_FMTON("'%s'", om_rng_str(conf->app.rng));
    PS_CATON( "LPR"  ); PS_FMTON("'%s'", om_lpr_str(conf->app.lpr));
    PS_CATON( "SSO"  ); PS_FMTON("'%s'", om_bool_str(conf->app.sso));

    /*
     * "ARCH" is omitted and "CORE" used instead. The reason for this
     * is that ARCH relates to the -march= parameter of gcc. This is
     * currently rather meaningless on the SPU, but it might not
     * always be so, so I don't want to pollute the meaning of "ARCH"
     * by overloading it on Cell/BE. Right now, it's convenient that
     * uses of "ARCH" should generate errors in XSL-T.
     */
//    PS_CATON( "ARCH"  ); PS_FMTON("'%s'", conf->app.arch);

#if defined(ARCH_CELL)
    PS_CATON( "CORE" ); PS_CATON("'SPU'");
    PS_CATON( "MPI"  ); PS_FMTON("'%s'", om_bool_str(conf->app.mpi));
#elif defined(ARCH_IA32)
    PS_CATON( "CORE" ); PS_CATON("'IA32'");
    PS_CATON( "THR"  ); PS_FMTON("'%s'", om_bool_str(conf->app.thr));
#endif

    PS_CATON( "PROF" ); PS_FMTON("'%s'", om_bool_str(conf->app.prof));

    /*
     * These are not really necessary since the XSL "knows" how many
     * reactions and species there are because it handles the SBML
     * model directly. They're only here for completeness.
     */
    PS_CATON( "N_RXNS"  ); PS_FMTON("'%d'", conf->app.n_rxns );
    PS_CATON( "N_SPXS"  ); PS_FMTON("'%d'", conf->app.n_rxns );

    sv[n]=NULL;
    sv[n+1]=NULL;

    return (const char **)sv;
}


void _gcc_argv_common(runconfig_t * const conf, char *sv[], char **pp, int *nn)
{
    char *p = *pp;
    int   n = *nn;
    int l;

    if( conf->verbose ) {
	PS_CATON( "-v" );
    }

    if( conf->gcc_opt_O && conf->gcc_opt_O != '-') {
	PS_FMTON( "-O%c", conf->gcc_opt_O);
    } else {
	PS_CATON( "-O" );
    }

    if( conf->save_temps ) {
	PS_CATON("-save-temps");
    }

    if ( conf->gcc_opt_g != NULL ) {
	if( *(conf->gcc_opt_g) == '-' ) {
	    PS_CATON( "-g" );	
	    PS_CATON("-DNDEBUG");
	} else {
	    PS_FMTON( "-g%s", conf->gcc_opt_g );
	}
    } else {
	PS_CATON("-DNDEBUG");
    }

    PS_FMTON( "-DEXE=\"%s\"",  conf->app.exe    );
    PS_FMTON( "-DCCMD=\"%s\"", conf->app.ccmd   );
    PS_FMTON( "-DVER=\"%s\"",  conf->app.ver    );
    PS_FMTON( "-DLBL=\"%s\"",  conf->app.lbl    );
    PS_FMTON( "-DPREC=%d",     conf->app.prec   );
    PS_FMTON( "-DLOG=%d",      conf->app.log    );
    PS_FMTON( "-DRNG=%d",      conf->app.rng    );
    PS_FMTON( "-DLPR=%d",      conf->app.lpr    );
    PS_FMTON( "-DSSO=%d",      conf->app.sso    );
    PS_FMTON( "-DARCH=\"%s\"", conf->app.arch   );
    PS_FMTON( "-DPROF=%d",     conf->app.prof   );
#if defined(ARCH_CELL)
    PS_FMTON( "-DMPI=%d",      conf->app.mpi    );
#else
    PS_FMTON( "-DMPI=%d",      CMC_MPI_OFF      );
#endif

    /*
     * These *are* really necessary here since (on Cell/BE) the
     * PPU-side code is *not* generated via XSL, so there's no way of
     * the PPU code knowing much about the model without querying the
     * SPUs, which is the way we used to do it but want to move
     * away from to simplify the PPU code.
     */
    PS_FMTON( "-DN_RXNS=%d",   conf->app.n_rxns );
    PS_FMTON( "-DN_SPXS=%d",   conf->app.n_spxs );

    *pp = p;
    *nn = n;
}


static void _ps_obj_src_pair(runconfig_t *conf, const char *name, char **ofile, char **cfile)
{
    char buf[PATH_MAX];
    char *tmp;

    strncpy(buf, name, PATH_MAX-1);
    strncat(buf, ".c", 2);
    tmp = u_search_path(AC_AUX_PATH, buf, conf->verbose);
    DIE_IF(tmp==NULL, "Support file '%s' not found", buf);
    if( conf->verbose ) {
	CLOG("Found '%s'\n", tmp);
    }
    *cfile=tmp;

    strncpy(buf, rc_filestub(conf), PATH_MAX-1);
    strncat(buf, "-", 1);
    strncat(buf, name, PATH_MAX-1);
    strncat(buf, ".o", 2);
    *ofile = strdup(buf);
    RCHECK(ofile, !=NULL, strdup);
}



char **ps_aux_files(runconfig_t *const conf, int *len)
{
    static char *sv[AC_GCC_ARGS_MAX+1];
    int i;

    for(i=0; i<PS_AUX_FILES_LEN; i++) {
	_ps_obj_src_pair( conf, _ps_aux_files[i], &sv[2*i], &sv[2*i+1]);
    }
    if( conf->app.prof ) {
	_ps_obj_src_pair( conf, PS_AUX_PROF_ON, &sv[2*i], &sv[2*i+1]);
    } else {
	_ps_obj_src_pair( conf, PS_AUX_PROF_OFF, &sv[2*i], &sv[2*i+1]);
    }
    i++;
    *len=i;

    sv[2*i]=NULL;
    sv[2*i+1]=NULL;

    return sv;
}


#if defined(ARCH_IA32)
void ps_gcc_argv(runconfig_t * const conf, char *sv[], char **pp, int *nn)
{
    char *p = *pp;	/* Pointer to start of buffer		*/
    int   n = *nn;	/* Current argument number in sv	*/
    int   i;		/* G.P. Iterator			*/
    int   l;		/* Scratch (used by PS_CATON)		*/

    PS_CATON( AC_GCC );

    _gcc_argv_common(conf, sv, &p, &n);

    PS_FMTON("-DTHR=%d", conf->app.thr);

    if( conf->app.thr ) {
	PS_CATON("-pthread");
    }

    if( conf->app.arch != NULL ) {
	PS_FMTON("-march=%s", conf->app.arch);
    }

    for(i=0; i<PS_GCC_ARGS_LEN; i++) {
	PS_CATON(_ps_gcc_args[i]);
    }

    sv[n]=NULL;

    *pp = p;
    *nn = n;
}


const char ** ps_strip_argv(runconfig_t * const conf)
{
    static char argbuf[_POSIX_ARG_MAX]={0};
    static char *sv[AC_GCC_ARGS_MAX+1];

    char *p;	/* Pointer to current argument in buffer	*/
    int   l;    /* Length of current argument in buffer		*/
    int   n=0;  /* Current argument number			*/

    p = argbuf;

    PS_CATON( AC_STRIP );
    PS_CATON( rc_exe_file(conf) );

    sv[n]=NULL;

    return (const char **)sv;
}




#elif defined(ARCH_CELL)

const char ** ps_spu_gcc_argv(runconfig_t * const conf)
{
    static char argbuf[_POSIX_ARG_MAX]={0};
    static char *sv[AC_GCC_ARGS_MAX+1];

    char *p;	/* Pointer to current argument in buffer	*/
    int   i;    /* G.P. Iterator				*/
    int   l;    /* Length of current argument in buffer		*/
    int   n=0;  /* Current argument number			*/

    p = argbuf;

    PS_CATON( AC_SPU_GCC );

    _gcc_argv_common(conf, sv, &p, &n);

    for(i=0; i<PS_SPU_GCC_ARGS_LEN; i++) {
	PS_CATON(_ps_spu_gcc_args[i]);
    }

    PS_CATON("-o"); PS_CATON( rc_spu_exe_file(conf) );
    PS_CATON( rc_c_file(conf) );

    for(i=0; i<PS_SPU_GCC_LIBS_LEN; i++) {
	PS_CATON(_ps_spu_gcc_libs[i]);
    }

    sv[n]=NULL;

    return (const char **)sv;
}


const char ** ps_spu_strip_argv(runconfig_t * const conf)
{
    static char argbuf[_POSIX_ARG_MAX]={0};
    static char *sv[AC_GCC_ARGS_MAX+1];

    char *p;	/* Pointer to current argument in buffer	*/
    int   l;    /* Length of current argument in buffer		*/
    int   n=0;  /* Current argument number			*/

    p = argbuf;

    PS_CATON( AC_SPU_STRIP );
    PS_CATON( rc_spu_exe_file(conf) );

    sv[n]=NULL;

    return (const char **)sv;
}


const char ** ps_ppu_embedspu_argv(runconfig_t * const conf)
{
    static char argbuf[_POSIX_ARG_MAX]={0};
    static char *sv[AC_GCC_ARGS_MAX+1];

    char *p;	/* Pointer to current argument in buffer	*/
    int   i;    /* G.P. Iterator				*/
    int   l;    /* Length of current argument in buffer		*/
    int   n=0;  /* Current argument number			*/

    p = argbuf;

    PS_CATON( AC_PPU_EMBEDSPU );

    for(i=0; i<PS_PPU_EMBEDSPU_ARGS_LEN; i++) {
	PS_CATON(_ps_ppu_embedspu_args[i]);
    }

    PS_CATON( rc_spu_exe_file(conf) );
    PS_CATON( rc_ppu_emb_file(conf) );

    sv[n]=NULL;

    return (const char **)sv;
}


const char ** ps_ppu_ar_argv(runconfig_t * const conf)
{
    static char argbuf[_POSIX_ARG_MAX]={0};
    static char *sv[AC_GCC_ARGS_MAX+1];

    char *p;	/* Pointer to current argument in buffer	*/
    int   i;    /* G.P. Iterator				*/
    int   l;    /* Length of current argument in buffer		*/
    int   n=0;  /* Current argument number			*/

    p = argbuf;

    PS_CATON( AC_PPU_AR );

    for(i=0; i<PS_PPU_AR_ARGS_LEN; i++) {
	PS_CATON(_ps_ppu_ar_args[i]);
    }

    PS_CATON( rc_ppu_ar_file(conf)  );
    PS_CATON( rc_ppu_emb_file(conf) );

    sv[n]=NULL;

    return (const char **)sv;
}



void ps_ppu_gcc_argv(runconfig_t * const conf, char *sv[], char **pp, int *nn)
{
    char *p = *pp;	/* Pointer to start of buffer		*/
    int   n = *nn;	/* Current argument number in sv	*/
    int   i;		/* G.P. Iterator			*/
    int   l;		/* Scratch (used by PS_CATON)		*/

    PS_CATON( AC_PPU_GCC );

    _gcc_argv_common(conf, sv, &p, &n);

    for(i=0; i<PS_PPU_GCC_ARGS_LEN; i++) {
	PS_CATON(_ps_ppu_gcc_args[i]);
    }

    sv[n]=NULL;

    *pp = p;
    *nn = n;
}



#endif /* defined(ARCH_CELL) */


