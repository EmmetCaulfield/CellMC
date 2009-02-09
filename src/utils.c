#define _GNU_SOURCE

#include <limits.h>		/* For PATH_MAX		*/
#include <string.h>		/* For strncat(), etc.	*/
#include <stdio.h>		/* For fputs(), etc.	*/

#include <unistd.h>		/* For open(), close()	*/
#include <fcntl.h>		/* For O_RDONLY		*/
#include <ctype.h>		/* For isspace()	*/

#include "utils.h"
#include <config.h>		/* For PACKAGE_NAME	*/
#include <appconfig.h>		/* For AC_ constants	*/
#include <error-macros.h>	/* For CLOG, RCHECK	*/

/*
 * Warning: returned string must be free()d 
 *
 * FIXME: Will choke on filenames with AC_DIR_SEP (colon) characters
 * in them
 */
char * u_search_path(const char path[], 
		     const char file[], 
		     bool verbose)
{
    char buf[PATH_MAX];
    char *list;
    char *dir;
    char *state=NULL;
    int fd;

    list = strdup(path);
    RCHECK(list, !=NULL, strdup);

    dir = strtok_r(list, AC_DIR_SEP, &state);
    do {
	strncpy(buf, dir, PATH_MAX-1);
	strncat(buf, AC_PATH_SEP, 1);
	strncat(buf, file, strlen(file));
	if( verbose ) {
	    CLOG("Checking for '%s' in '%s'\n", file, dir);
	}
	fd=open(buf, O_RDONLY);
	if( fd != -1 ) {
	    close(fd);
	    free(list);
	    return(strdup(buf));
	}
    } while( NULL!=(dir=strtok_r(NULL, AC_DIR_SEP, &state)) );

    /*
     * If we get here, we've failed to find the file
     */
    if( verbose ) {
	CLOG("Failed to find essential file '%s'\n", file);
    }
    free(list);
    return NULL;
}

/*
 * Trim leading and trailing whitespace.
 */
char *u_trim(char *s) {
    char *n;
    int  i, j, l;

    l = strlen(s);
    n = (char *)malloc((l+1)*sizeof(char));
    RCHECK(n, !=NULL, malloc);
    
    j=0;
    for(i=0; i<l; i++) {
	if( isspace(s[i]) )
	    continue;
	n[j]=s[i];
	j++;
    }
    n[j]='\0';
    return n;
}

char *u_join(int argc, char *argv[], const char *sep)
{
    int i;
    char buf[_POSIX_ARG_MAX] = { '\0' } ;

    for(i=0; i<argc-1; i++) {
	strncat(buf, argv[i], strlen(argv[i]));
	strncat(buf, sep, strlen(sep));
    }
    strncat(buf, argv[argc-1], strlen(argv[argc-1]));
    return strdup(buf);
}


void u_dump_strvec(const char **sv, char oddsep, char evensep) {
    int i=0;

    fputs(PACKAGE_NAME ": [", stdout);
    while( sv[i] != NULL ) {
	fputs(sv[i], stdout);
	if( sv[i+1] != NULL ) {
	    if( i%2 ) 
		putchar(oddsep);
	    else
		putchar(evensep);
	}
	i++;
    }
    fputs("]\n", stdout);
}

