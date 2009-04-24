#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <config.h>
#include <error-macros.h>
#include <runconfig.h>
#include <platspec.h>
#include <utils.h>



static const char *_ps_ppu_ld_args[] = {
    "-Wl,-m,elf32ppc"	,
    "-lmisc"		,
    "-lspe2"
};
#define PS_PPU_LD_ARGS_LEN ((int)(sizeof(_ps_ppu_ld_args)/sizeof(char *)))


static void _run(char *exe, const char **argv, runconfig_t *conf) {
    pid_t pid;
    int status;

    if(conf->verbose) {
	u_dump_strvec(argv, ' ', ' ');
    }

    pid=fork();
    RCHECK(pid, !=-1, fork);
    if( pid ) {
	/* Parent: wait for the child */
	waitpid(pid, &status, 0);
    } else {
	/* Child: run the compiler */
	status=execvp(exe, (char **)argv);
	RCHECK(status, !=-1, execvp);
    }

    DIE_IF( ! WIFEXITED(status), exe );
    status=WEXITSTATUS(status);
    DIE_IF( status!=0, exe );

    if( conf->verbose ) {
	CLOG("%s exited with status %d\n", exe, status);
    }
}

static void _zap(runconfig_t * const conf, const char *file) {
    int rc;
    if( !conf->save_temps ) {
	rc=unlink(file);
	WARN_IF(rc==-1, "Failed to delete '%s'", file);
	RCHECK(rc, !=-1, unlink);
    }
}


#if defined(ARCH_IA32)
void c_compile(runconfig_t * const conf)
{
    static char argbuf[_POSIX_ARG_MAX]={0};
    static char *sv[AC_GCC_ARGS_MAX+1];

    char *p;	/* Pointer to end of buffer		*/
    int   l;    /* Length of current argument in buffer	*/
    int   n=0;  /* Current argument number		*/

    char *sp;	/* Saved p				*/
    int   sn;	/* Saved n				*/

    int   i;    /* G.P. Iterator			*/

    char **sf;	/* Array of support files		*/
    int   nsf;	/* Number of support files to compile	*/

    p=argbuf;

    ps_gcc_argv(conf, sv, &p, &n);
    sp=p;
    sn=n;

    sf = ps_aux_files(conf, &nsf);

    for(i=0; i<nsf; i++) {
	p=sp;
	n=sn;
	PS_CATON("-c");
	PS_CATON("-o"); PS_CATON( sf[2*i] );
	PS_CATON( sf[2*i+1]);
	sv[n]=NULL;
	_run(AC_GCC, (const char **)sv, conf);
    }

    p=sp;
    n=sn;
    PS_CATON("-o"); PS_CATON( rc_exe_file(conf) );

    for(i=0; i<nsf; i++) {
	PS_CATON( sf[2*i] );
    }

    PS_CATON( rc_c_file(conf) );

    if( conf->app.log == CMC_LOG_LIB ) {
	PS_CATON("-lm");
    }

    _run(AC_GCC, (const char **)sv, conf);

    for(i=0; i<nsf; i++) {
	_zap(conf, sf[2*i] );
    }

    _zap( conf, rc_c_file(conf) );

    if( conf->strip ) {
	_run(AC_STRIP, ps_strip_argv(conf), conf);
    }
}

#elif defined(ARCH_CELL)

/*
 * For the Cell/BE architecture, the build process is somewhat
 * convoluted. The following steps are taken:
 *
 * 1) The C code (previously generated) is compiled with spu-gcc into
 *    a "SPU executable"
 *
 * 2) ppu-embedspu is used to generate a .o file which can be linked
 *    to PPU code.
 * 
 * 3) ppu-ar is used to make an .a archive of the .o file
 *
 * 4) ppu-gcc is used to compile/link the PPU-side C code with 
 *
 * There is no established convention for what extensions to use,
 * so they are configurable in app-config.h
 * 
 */
void c_compile(runconfig_t *conf) 
{
    static char argbuf[_POSIX_ARG_MAX]={0};
    static char *sv[AC_GCC_ARGS_MAX+1];

    char *p;	/* Pointer to end of buffer		*/
    int   l;    /* Length of current argument in buffer	*/
    int   n=0;  /* Current argument number		*/

    char *sp;	/* Saved p				*/
    int   sn;	/* Saved n				*/

    int   i;    /* G.P. Iterator			*/

    char **sf;	/* Array of support files		*/
    int   nsf;	/* Number of support files to compile	*/


    _run(AC_SPU_GCC, ps_spu_gcc_argv(conf), conf);
    _zap( conf, rc_c_file(conf) );

    if( conf->strip ) {
	_run(AC_SPU_STRIP, ps_spu_strip_argv(conf), conf);
    }

    _run(AC_PPU_EMBEDSPU, ps_ppu_embedspu_argv(conf), conf);
    _zap( conf, rc_spu_exe_file(conf) );

    _run(AC_PPU_AR, ps_ppu_ar_argv(conf), conf);
    _zap( conf, rc_ppu_emb_file(conf) );


    /*
     * Run ppu-gcc multiple times:
     */
    p=argbuf;

    ps_ppu_gcc_argv(conf, sv, &p, &n);
    sp=p;
    sn=n;

    sf = ps_aux_files(conf, &nsf);

    for(i=0; i<nsf; i++) {
	p=sp;
	n=sn;
	PS_CATON("-c");
	PS_CATON("-o"); PS_CATON( sf[2*i] );
	PS_CATON( sf[2*i+1]);
	sv[n]=NULL;
	_run(AC_PPU_GCC, (const char **)sv, conf);
    }

    p=sp;
    n=sn;
    PS_CATON("-o"); PS_CATON( rc_exe_file(conf) );

    for(i=0; i<nsf; i++) {
	PS_CATON( sf[2*i] );
    }

    PS_CATON( rc_ppu_ar_file(conf) );

    for(i=0; i<PS_PPU_LD_ARGS_LEN; i++) {
	PS_CATON(_ps_ppu_ld_args[i]);
    }
#if defined(HAVE_MPI_H)
    if( conf->app.mpi ) {
      PS_CATON("-lmpi");
    }
#endif

    sv[n]=NULL;

    _run(AC_PPU_GCC, (const char **)sv, conf);

    if( !conf->save_temps ) {
	for(i=0; i<nsf; i++) {
	    _zap(conf, sf[2*i] );
	}
    }

/*
    if( conf->strip ) {
	_run(AC_PPU_STRIP, ps_strip_argv(conf), conf);
    }
*/

}

#endif /* ARCH_CELL */
