#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <getopt.h>

#include <config.h>

#include <cellmc.h>
#if defined(OS_CYGWIN)
#   define OPT_CDECL __attribute__((dllimport))
#else
#   define OPT_CDECL
#endif

#include <error-macros.h>
#include <option-mapping.h>

#include <appconfig.h>
#include <helptext.h>
#include <runconfig.h>
#include <utils.h>


/*
 * Option flags. We use numbers for those with no short
 * equivalent. Macro names are chosen for similarity with constants in
 * cellmc.h where possible.
 */
#define RC_IA32_OPTS "l:n:"
#define RC_OPT_LOG   'l'
#define RC_OPT_RNG   'n'
#define RC_OPT_ARCH  '1'

#define RC_CELL_OPTS "Ms"
#define RC_OPT_MPI   'M'
#define RC_OPT_SSO   's'

/*
 * We allow an optional argument to -h and -V so that getopts doesn't
 * complain under certain circumstances. These are "one trick" options
 * that ignore the rest of the line.
 */
#define RC_COMMON_OPTS     "h::V::r:o:pmdvO::g::"
#define RC_OPT_HELP        'h'
#define RC_OPT_VER         'V'
#define RC_OPT_LPR         'r'
#define RC_OPT_EXE         'o'
#define RC_OPT_PROF        'p'
#define RC_OPT_THR         'm'
#define RC_OPT_PREC_DOUBLE 'd'
#define RC_OPT_VERBOSE     'v'
#define RC_OPT_GCC_OPT_O   'O'
#define RC_OPT_GCC_OPT_g   'g'

/* Common options with no short version */
#define RC_OPT_SAVETF     '2'
#define RC_OPT_VALIDATION '3'
#define RC_OPT_XSLFILE    '4'
#define RC_OPT_NOSTRIP    '5'


#if defined(ARCH_CELL)
#   define RC_OPTSTRING RC_COMMON_OPTS RC_CELL_OPTS
#elif defined(ARCH_IA32)
#   define RC_OPTSTRING RC_COMMON_OPTS RC_IA32_OPTS
#endif

static struct option _longopts[] = {
//#if defined(ARCH_IA32) 
    { "log",           required_argument, NULL, RC_OPT_LOG         },
    { "rng",           required_argument, NULL, RC_OPT_RNG         },
//#elif defined(ARCH_CELL)
    { "mpi",           no_argument,       NULL, RC_OPT_MPI         },
//#endif
    { "help",          optional_argument, NULL, RC_OPT_HELP        },
    { "version",       optional_argument, NULL, RC_OPT_VER         },
    { "sso",           no_argument,       NULL, RC_OPT_SSO         },
    { "lpr",	       required_argument, NULL, RC_OPT_LPR         },
    { "output",	       required_argument, NULL, RC_OPT_EXE         },
    { "xslfile",       required_argument, NULL, RC_OPT_XSLFILE     },
    { "profile",       no_argument,       NULL, RC_OPT_PROF        },
    { "double",        no_argument,       NULL, RC_OPT_PREC_DOUBLE },
    { "save-temps",    no_argument,       NULL, RC_OPT_SAVETF      },
#if defined(HAVE_XSD_VALIDATION)
    { "no-validation", no_argument,       NULL, RC_OPT_VALIDATION  },
#else
    { "validate",      no_argument,       NULL, RC_OPT_VALIDATION  },
#endif
    { "verbose",       no_argument,       NULL, RC_OPT_VERBOSE     },
    { "no-strip",      no_argument,       NULL, RC_OPT_NOSTRIP     },
    { "threading",     no_argument,       NULL, RC_OPT_THR         },
    { "gcc-optim",     optional_argument, NULL, RC_OPT_GCC_OPT_O   },
    { "gcc-debug",     optional_argument, NULL, RC_OPT_GCC_OPT_g   },
    { "arch",          required_argument, NULL, RC_OPT_ARCH        },
    { 0, 0, 0, 0 } /* Required sentinel */
};


#if defined(ARCH_CELL)
#   define NOT_ON_CELL(MSG) DIE(MSG " is not available on Cell/BE")
#   define NOT_ON_IA32(MSG)
#elif defined(ARCH_IA32)
#   define NOT_ON_IA32(MSG) DIE(MSG " is not available on IA32")
#   define NOT_ON_CELL(MSG)
#endif


void rc_getopts(runconfig_t *conf, int argc, char *argv[])
{
    extern char *optarg OPT_CDECL;
    extern int optind OPT_CDECL;
    extern int opterr OPT_CDECL;
    extern int optopt OPT_CDECL;
    int c;
    unsigned int flags[128]={0};
    FILE *fp;

    if( argc <= 1 ) {
	helptext(stdout);
	exit(EXIT_SUCCESS);
    }

    opterr=0;
    while( -1 != (c=getopt_long(argc, argv, RC_OPTSTRING, _longopts, NULL)) ) {
	switch(c) {
	case RC_OPT_HELP:
	    helptext(stdout);
	    exit(EXIT_SUCCESS);

	case RC_OPT_VER:
	    puts( PACKAGE_VERSION );
	    exit(EXIT_SUCCESS);

	case RC_OPT_LOG:
	    NOT_ON_CELL("Choice of log() implementation");
	    conf->app.log = om_log_id(optarg);
	    DIE_IF(conf->app.log < 0, "Log option '%s' is not valid", optarg);
	    flags[c]++;
	    break;

	case RC_OPT_RNG:
	    NOT_ON_CELL("Choice of PRNG");
	    conf->app.rng = om_rng_id(optarg);
	    DIE_IF(conf->app.rng < 0, "RNG option '%s' is not valid", optarg);
	    flags[c]++;
	    break;

	case RC_OPT_THR:
#if defined(ARCH_CELL)
	    WARN("Threading switch is vacuous on Cell/BE");
#else
	    conf->app.thr = true;
#endif
	    flags[c]++;
	    break;

	case RC_OPT_MPI:
	    NOT_ON_IA32("MPI support");
	    conf->app.mpi = false;
#if defined(HAVE_MPI_H)
	    conf->app.mpi = true;
	    flags[c]++;
#else
	    DIE("MPI requested but unavailable");
#endif
	    break;

	case RC_OPT_SSO:
	    conf->app.sso = true;
	    flags[c]++;
	    break;

	case RC_OPT_LPR:
	    conf->app.lpr = om_lpr_id(optarg);
	    DIE_IF(conf->app.lpr<0, "LPR option '%s' is not valid", optarg);
	    flags[c]++;
	    break;

	case RC_OPT_EXE:
	    conf->app.exe = strdup(optarg);
	    RCHECK(conf->app.exe, !=NULL, strdup);
	    flags[c]++;
	    break;

	case RC_OPT_XSLFILE:
	    conf->xslfile = strdup(optarg);
	    RCHECK(conf->xslfile, !=NULL, strdup);
	    flags[c]++;
	    break;

	case RC_OPT_PROF:
	    conf->app.prof = true;
	    flags[c]++;
	    break;

	case RC_OPT_PREC_DOUBLE:
	    conf->app.prec=CMC_PREC_DOUBLE;
	    flags[c]++;
	    break;

	case RC_OPT_VERBOSE:
	    conf->verbose=true;
	    flags[c]++;
	    break;

	case RC_OPT_NOSTRIP:
	    conf->strip=false;
	    flags[c]++;
	    break;

	case RC_OPT_GCC_OPT_g:
	    if( optarg == NULL ) {
		conf->gcc_opt_g = "-";
	    } else {
 		conf->gcc_opt_g = strdup(optarg);
		RCHECK(conf->gcc_opt_g, !=NULL, strdup);
	    }
	    flags[c]++;
	    break;

	case RC_OPT_GCC_OPT_O:
	    if( optarg == NULL ) {
		conf->gcc_opt_O = '-';
	    } else if ( *optarg == 's' ) {
		conf->gcc_opt_O = 's';
	    } else {
		conf->gcc_opt_O = (int)strtol(optarg, NULL, 10);
		if( errno == EINVAL || errno == ERANGE ) {
		    DIE("Failed to parse argument '%s' to '-O' option", optarg)
		}
		if( conf->gcc_opt_O > 3 ) {
		    DIE("Argument '%s' is out-of-range for -O option", optarg);
		}
		conf->gcc_opt_O += '0';
	    }
	    flags[c]++;
	    break;

        /*
	 * Long options with no short equivalent
	 */
	case RC_OPT_SAVETF:
	    conf->save_temps = true;
	    flags[c]++;
	    break;

	case RC_OPT_VALIDATION:
#if defined(HAVE_XSD_VALIDATION)
	    conf->validate = false;
#else
	    conf->validate = true;
#endif
	    flags[c]++;
	    break;

	case RC_OPT_ARCH:
	    conf->app.arch = strdup(optarg);
	    RCHECK(conf->app.arch, !=NULL, strdup);
	    flags[c]++;
	    break;

        /*
	 * Error-handling
	 */
	case ':':
	    DIE("Missing argument to option (-%c)", optopt);

	case '?':
	    DIE_IF(optopt==0, "Unrecognised option (%s)", argv[optind-1]);
	    DIE("Unrecognised option (-%c)", optopt);

	default:
	    DIE("Option processing error");
	}
    }

    /*
     * Exactly one remaining non-option argument should be a file that
     * exists and is readable.
     */
    DIE_IF(optind >= argc,   "No SBML file specified");
    if( argc-optind != 1 ) {
	fprintf(stderr, "%s: argc=%d, optind=%d\n", PACKAGE_NAME, argc, optind);
	for(c=optind; c<argc; c++) {
	    fprintf(stderr, "\t%d: [%s]\n", c, argv[c]);
	}
	DIE("Too many non-option arguments");
    }
    conf->sbmlfile = strdup(argv[optind]);
    fp=fopen(conf->sbmlfile, "r");
    DIE_IF(fp==NULL, "Cannot open file '%s'", conf->sbmlfile);
    fclose(fp);


    for(c=33; c<127; c++) {
	DIE_IF(flags[c] > 1, "Option '-%c' specified more than once\n", c);
    }


    /*
     * Make sure that if we're debugging, we don't strip out the
     * debugging symbols or have optimisation on too high.
     */
    if( flags[RC_OPT_GCC_OPT_g] ) {
	conf->strip = false;
	if( flags[RC_OPT_GCC_OPT_O] && conf->gcc_opt_O > '1' ) {
	    conf->gcc_opt_O = '0';
	    fprintf(stderr, "WARNING: Argument '-O %c' overridden by '-g|--gcc-debug'\n", conf->gcc_opt_O);
	}
    }

    if( conf->app.exe == NULL ) {
	conf->app.exe = strdup(AC_EXE);
	RCHECK(conf->app.exe, !=NULL, strdup);
    }
    conf->app.ver = strdup(VERSION);
    RCHECK(conf->app.ver, !=NULL, strdup);

#if defined(ARCH_IA32)
    if( flags['d'] ) {
	conf->app.log = CMC_LOG_LIB;
	WARN_IF( conf->verbose, "defaulting to stdlib log() function in double precision");
    }
#endif

    conf->app.ccmd = u_join(argc, argv, " ");
}


void rc_getenvars(runconfig_t * const conf)
{
    (void)conf;
    return;
}


/*
 * The file stub used for all intermediate files (of which there can
 * be many, depending on the platform) depends on the '-o' option and
 * '--save-temps' options.  If '-o' option, specifying the output
 * filename, is specified. If it is, then it is used as the stub. If
 * not, a random filename is used. If '--save-temps' is specified, the
 * stub DOES NOT include the temporary directory, and all intermediate
 * files end up in the CWD. If '--save-temps' is NOT specified, then
 * all intermediate files are created in the specified temporary
 * directory.
 */
const char *rc_filestub(runconfig_t * const conf)
{
    if( conf->filestub == NULL ) {
	if( conf->save_temps ) {
	    conf->tmpdir=".";
	    if( conf->app.exe == NULL ) {
		conf->filestub=tempnam(conf->tmpdir, PACKAGE_NAME);
		RCHECK(conf->filestub, !=NULL, tempnam);
	    } else {
		conf->filestub=strdup(conf->app.exe);
		RCHECK(conf->filestub, !=NULL, strdup);
	    }
	} else {
	    conf->filestub=tempnam(conf->tmpdir, PACKAGE_NAME);
	    RCHECK(conf->filestub, !=NULL, tempnam);
	}

	if( conf->app.exe == NULL ) {
	    conf->app.exe = strdup(AC_EXE);
	    RCHECK(conf->app.exe, !=NULL, strdup);
	}
    }
    return conf->filestub;
}

const char *rc_exe_file(runconfig_t * const conf)
{
    if( conf->app.exe==NULL ) {
	rc_filestub(conf);
    }
    return conf->app.exe;
}


void rc_cleanup(runconfig_t * const conf)
{
    /*
     * Allocated above:
     */
    if( conf->app.arch  !=NULL ) free(conf->app.arch);
    if( conf->app.exe   !=NULL ) free(conf->app.exe);
    if( conf->app.ccmd  !=NULL ) free(conf->app.ccmd);
    if( conf->app.ver   !=NULL ) free(conf->app.ver);
    if( conf->filestub  !=NULL ) free(conf->filestub);
    if( conf->sbmlfile  !=NULL ) free(conf->sbmlfile);
    if( conf->xslfile   !=NULL ) free(conf->xslfile);

    /*
     * Somewhat special handling to facilitate recognising that the
     * "-g" option has been given, but without an argument.
     */
    if( conf->gcc_opt_g !=NULL && *conf->gcc_opt_g != '-' ) 
	free(conf->gcc_opt_g);

    /*
     * Allocated in xml-utils.c
     */
    if( conf->app.lbl   !=NULL ) free(conf->app.lbl);


}


#define RC_COOK_FILENAME_WITH_FORMAT(F) {				\
	static char buf[PATH_MAX]={ '\0' };				\
	if( buf[0] == '\0' ) {						\
	    int rc;							\
	    rc=snprintf(buf, PATH_MAX, F, rc_filestub(conf));		\
	    RCHECK(rc, >1, snprintf);					\
	}								\
	return buf;							\
    }

const char *rc_c_file(runconfig_t * const conf) RC_COOK_FILENAME_WITH_FORMAT(AC_C_FMT)

#if defined(ARCH_CELL)
const char *rc_spu_exe_file(runconfig_t * const conf) RC_COOK_FILENAME_WITH_FORMAT(AC_SPU_EXE_FMT)
const char *rc_ppu_emb_file(runconfig_t * const conf) RC_COOK_FILENAME_WITH_FORMAT(AC_PPU_EMB_FMT)
const char *rc_ppu_ar_file(runconfig_t * const conf)  RC_COOK_FILENAME_WITH_FORMAT(AC_PPU_AR_FMT)
#endif



#define RC_DUMP_APP_STRING(M) fprintf(stderr, "\t\t%-6s = \"%s\"\n", #M, conf->app.M)
#define RC_DUMP_RUN_STRING(M) fprintf(stderr, "\t%-11s = \"%s\"\n", #M, conf->M)

#define RC_DUMP_APP_LOOKUP(M) fprintf(stderr, "\t\t%-6s = %s (%d)\n", #M, om_##M##_str(conf->app.M), conf->app.M)

#define RC_DUMP_APP_INT(M)    fprintf(stderr, "\t\t%-6s = %d\n", #M, conf->app.M)

#define RC_DUMP_APP_BOOL(M) fprintf(stderr, "\t\t%-6s = %s\n", #M, om_bool_str(conf->app.M))
#define RC_DUMP_RUN_BOOL(M) fprintf(stderr, "\t%-11s = %s\n", #M, om_bool_str(conf->M))

#define RC_DUMP_RUN_CHAR(M) fprintf(stderr, "\t%-11s = '%c'\n", #M, conf->M)

void rc_dump(runconfig_t const *conf)
{
    fprintf(stderr, "conf = {\n\tapp = {\n");
    RC_DUMP_APP_STRING(exe);
    RC_DUMP_APP_STRING(ver);
    RC_DUMP_APP_STRING(lbl);
    RC_DUMP_APP_LOOKUP(prec);
    RC_DUMP_APP_LOOKUP(log);
    RC_DUMP_APP_LOOKUP(rng);
    RC_DUMP_APP_LOOKUP(lpr);
    RC_DUMP_APP_BOOL(sso);
    RC_DUMP_APP_STRING(arch);
    RC_DUMP_APP_BOOL(prof);
#if defined(ARCH_IA32)
    RC_DUMP_APP_BOOL(thr);
#elif defined(ARCH_CELL)
    RC_DUMP_APP_BOOL(mpi);
#endif
    RC_DUMP_APP_INT(n_rxns);
    RC_DUMP_APP_INT(n_spxs);
    fprintf(stderr, "\t}\n");

    RC_DUMP_RUN_BOOL(verbose);
    RC_DUMP_RUN_BOOL(save_temps);
    RC_DUMP_RUN_BOOL(validate);
    RC_DUMP_RUN_BOOL(strip);

    RC_DUMP_RUN_CHAR(gcc_opt_O);
    RC_DUMP_RUN_STRING(gcc_opt_g);
    
    RC_DUMP_RUN_STRING(sbmlfile);
    RC_DUMP_RUN_STRING(filestub);
    RC_DUMP_RUN_STRING(xslfile);

    RC_DUMP_RUN_STRING(tmpdir);

    fprintf(stderr, "}\n");
}
