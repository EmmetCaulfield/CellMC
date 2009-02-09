#include <stdio.h>

#include <config.h>
#include <helptext.h>


void helptext(FILE *fp) {
    if( fp == NULL ) {
	fp = stderr;
    }
    fputs(
	"USAGE:\n"
	"  " PACKAGE " [options] sbmlfile\n"
	"\n"
	"OPTIONS:\n"
	"  Options that affect " PACKAGE " behaviour, but don't affect the\n"
	"  generated program\n"
	"    -h|--help                    show this help text\n"
	"    -V|--version                 show version information and exit\n"
	"    -v|--verbose                 make " PACKAGE " more verbose\n"
	"    -o|--output <filename>       set output filename (default: 'a.out')\n"
	"    --xslfile <filename>         override internal choice of XSL-T file\n"
	"    --save-temps                 save temporary files (as gcc)\n"
#if defined(HAVE_XSD_VALIDATION)
	"    --no-validation              don't validate SBML model\n"
#else
	"    --validate                   attempt SBML model validation\n"
#endif
	"\n"
	"  Options that affect the generated program:\n"
	"    -d|--double                  use double-precision (default: single)\n"
	"    -p|--profile                 generate profiling code\n"
	"    -g|--gcc-debug [lbl]         pass -g flag (debugging symbols) to gcc (implies --no-strip)\n"
	"    -O|--gcc-optim [0|1|2|3|s]   pass -O flag (optimization level) to gcc\n"
	"    --no-strip                   don't strip executable\n"
	"\n"
#if defined(ARCH_IA32)
	"  Options specific to this platform (IA32)\n"
	"    -m|--multicore               generate pthreads code for multiprocessor PCs\n"
	"    -l|--log <asm|lib|fpu>       select log() implementation (default: 'asm')\n"
	"    -r|--lpr <none|semi|full>    select LPR method (default: 'semi')\n"
	"    -n|--rng <rsmt|stdlib>       select PRNG implementation (default: 'rsmt')\n"
	"    --march <gcc-march-label>    pass machine architecture label to gcc\n"
#elif defined(ARCH_CELL)
	"  Options specific to this platform (Cell/BE)\n"
	"    -s|--sso                     turn SIMD slot optimization on (default: off)\n"
	"    -M|--mpi                     generate MPI code for a cluster\n"
#endif
        "\n", fp);
}
