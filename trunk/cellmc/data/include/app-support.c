#include <limits.h>	/* for _POSIX_ARG_MAX	*/
#include <stdio.h>	/* for puts(), etc.	*/
#include <stdlib.h>	/* for exit()		*/

#include <string.h>	/* for strncpy(), etc.  */

#include "error-macros.h"
#include "app-support.h"

void as_usage(const char *msg)
{
    
    fprintf(stderr, "FATAL: %s\n", msg);
    fprintf(stderr, "\t(Try '" EXE " -h' for help.)\n");
    exit(1);
}


void as_helptext(FILE *fp)
{
    if(fp==NULL) {
	fp=stdout;
    }
    fputs("USAGE:\n", fp);
    fputs("    " EXE " [options] <number of trajectories> <stop time>\n", fp);

    fputs("OPTIONS:\n", fp);
    fputs("    -h           shows this help text.\n", fp);
    fputs("    -o <file>    print output to file (instead of stdout).\n", fp);
    fputs("    -s <value>   sets PRNG seed to 'value'.\n", fp);
#if defined(__PPU__)
    fputs("    -c <n>       use 'n' SPUs (mnemonic: c='cores').\n", fp);
    fputs("    -b <n>       set block-size to 'n'.\n", fp);
#elif defined(__SSE2__) && THR==CMC_THR_ON
    fputs("    -c <n>       use 'n' compute threads (mnemonic: 'c'='cores').\n", fp);
#endif
    fputs("    -i           suppress info header in results (mnemonic: '-i'='info off').\n", fp);
    putc('\n', fp);
}


char *as_join(const int argc, char * argv[], const char * const sep)
{
    int i=0;
    char buf[_POSIX_ARG_MAX] = { '\0' } ;

    for(i=0; i<argc-1; i++) {
	strncat(buf, argv[i], strlen(argv[i]));
	strncat(buf, sep, strlen(sep));
    }
    strncat(buf, argv[argc-1], strlen(argv[argc-1]));
    return strdup(buf);
}
