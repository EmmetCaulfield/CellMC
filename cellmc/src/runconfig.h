#ifndef RUNCONFIG_H
#define RUNCONFIG_H
#include <stdbool.h>
#include <stdio.h>
#include <getopt.h>

#include <config.h>
#include <cellmc.h>
#include <appconfig.h>

typedef struct {
    cellmc_app_t app;

    bool verbose;
    bool save_temps;
    bool validate;
    bool strip;

    char  gcc_opt_O;	/* gcc -O level     (optimization level)  */
    char *gcc_opt_g;	/* gcc -g           (debug info level)    */

    char *sbmlfile;
    char *filestub;
    char *xslfile;

    const char * tmpdir;
    
} runconfig_t;

#if defined(HAVE_XSD_VALIDATION)
#   define RC_VALIDATION 1
#else
#   define RC_VALIDATION 0
#endif

#define RC_STRUCT_DEFAULT {CMC_APP_STRUCT_DEFAULT, false, false, RC_VALIDATION, true,   AC_GCC_OPTIM, NULL,   NULL, NULL, NULL,   NULL}

void rc_getopts(runconfig_t * conf, int argc, char *argv[]);
void rc_cleanup(runconfig_t * const conf);
void rc_getenvars(runconfig_t * const conf);
void rc_dump(runconfig_t const *conf);

const char *rc_filestub(runconfig_t *const conf);
const char *rc_c_file(runconfig_t * const conf);
const char *rc_exe_file(runconfig_t * const conf);
#if defined(ARCH_CELL)
const char *rc_spu_exe_file(runconfig_t * const conf);
const char *rc_ppu_emb_file(runconfig_t * const conf);
const char *rc_ppu_ar_file(runconfig_t * const conf);
#endif

#endif /* RUNCONFIG_H */
