#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

char * u_search_path(const char path[], const char file[], bool verbose);
void u_dump_strvec(const char **sv, const char oddsep, const char evensep);
char *u_join(int argc, char *argv[], const char *sep);
char *u_trim(char *s);

#endif /* UTILS_H */
