#ifndef APP_SUPPORT_H
#define APP_SUPPORT_H

#include <stdio.h>	/* For FILE, fprintf(), etc.	*/
#include "sim-info.h"	/* For sim_info_t		*/

/*
 * What the PPU/SPU intercommunication block-size must be a multiple
 * of in bytes (e.g. 128)
 */
#if defined(__PPU__)
#   define AS_BLKSZ_MULT 128
#endif

void as_usage(const char *msg);
void as_helptext(FILE *fp);
void as_dump(sim_info_t * const si);
char *as_join(const int argc, char * argv[], const char * const sep);

#endif /* APP_SUPPORT_H */
