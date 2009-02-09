#ifndef PLATSPEC_H
#define PLATSPEC_H

#include <config.h>
#include "runconfig.h"

#include <string.h>
#include <stdio.h>

/*
 * These are handy, but rather nastily assume that sv, n, p, and l are
 * in scope whenever the macro is used:
 */
#define PS_CATON(S)     { sv[n++]=p; l=strlen(S)+1; strncpy(p, S, l); p+=l; }
#define PS_FMTON(S,...) { sv[n++]=p; l=sprintf(p, S, __VA_ARGS__)+1;  p+=l; }


char *  ps_choose_xsltfile   (const runconfig_t * const conf);
const char ** ps_xslt_params(runconfig_t * const conf);
char **ps_aux_files(runconfig_t *const conf, int *len);

#if defined(ARCH_IA32)
void ps_gcc_argv(runconfig_t * const conf, char *sv[], char **pp, int *nn);
char **ps_aux_files(runconfig_t *const conf, int *len);
const char ** ps_strip_argv(runconfig_t * const conf);
#elif defined(ARCH_CELL)
const char ** ps_spu_gcc_argv(runconfig_t * const conf);
const char ** ps_spu_strip_argv(runconfig_t * const conf);
const char ** ps_ppu_embedspu_argv(runconfig_t * const conf);
const char ** ps_ppu_ar_argv(runconfig_t * const conf);
void ps_ppu_gcc_argv(runconfig_t * const conf, char *sv[], char **pp, int *nn);
#else
#   error Architecture not defined
#endif

#endif /* PLATSPEC_H */
