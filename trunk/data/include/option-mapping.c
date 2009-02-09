#include "option-mapping.h"
#include "error-macros.h"
#include <stdbool.h>
#include <string.h>

/*
 * Linear search through a given array of strings for a particular
 * string. In practice, the arrays are very short (2-3 elements), so a
 * linear search is fine.
 */
static int _om_id_search(const char * const search[], const int len, const char *key) {
    int i;
    for(i=0; i<len; i++) {
	if( search[i]==NULL )
	    continue;
	if( 0==strncmp(search[i], key, strlen(search[i])+1) )
	    return i;
    }
    return -1;
}
#define LEN(TBL) ((int)(sizeof(_om_##TBL)/sizeof(char *)))
#define _om_id_find(FLD,STR) _om_id_search(_om_##FLD, LEN(FLD), STR)


/*
 * Floating-point precision specifiers, reverse mapping:
 */
static const char *_om_prec[] = {
    NULL	,
    "single"	,
    "double"
};


/*
 * Logarithm function selectors, reverse mapping:
 */
static const char *_om_log[] = {
    NULL	,
    "asm"	,
    "lib"	,
    "fpu"
};


/*
 * PRNG function selectors, reverse mapping:
 */
static const char *_om_rng[] = {
    NULL	,
    "stdlib"	,
    "rsmt"	,
    "mc_rand"
};


/*
 * Propensity recalculation selectors, reverse mapping:
 */
static const char *_om_lpr[] = {
    "none"	,
    "semi"	,
    "full"
};


/*
 * For Boolean options, reverse mapping
 */
static const char *_om_boolean[]={
    "off"	,
    "on"	,
    "false"	,
    "true"	,
    "no"	,
    "yes"
};


const char * om_prec_str(int i) {
    DIE_IF( i<0 || i>=LEN(prec), "%d is not a valid precision ID", i);
    return _om_prec[i];
}
int om_prec_id(const char *const s) {
    int rc=_om_id_find(prec,s);
    WARN_IF(rc<0, "'%s' is not a valid precision specifier", s);
    return rc;
}


const char * om_log_str(int i) {
    DIE_IF( i<0 || i>=LEN(log), "%d is not a valid log function ID", i);
    return _om_log[i];
}
int om_log_id(const char *const s) {
    int rc=_om_id_find(log,s);
    WARN_IF(rc<0, "'%s' is not a valid log function specifier",s);
    return rc;
}


const char * om_rng_str(int i) {
    DIE_IF( i<0 || i>=LEN(rng), "%d is not a valid PRNG ID", i);
    return _om_rng[i];
}
int om_rng_id(const char *const s) {
    int rc=_om_id_find(rng,s);
    WARN_IF(rc<0, "'%s' is not a valid PRNG specifier",s);
    return rc;
}


const char * om_lpr_str(int i) {
    DIE_IF( i<0 || i>=LEN(lpr), "No LPR level ID has value %d", i);
    return _om_lpr[i];
}
int om_lpr_id(const char * const s) {
    int rc=_om_id_find(lpr,s);
    WARN_IF(rc<0, "'%s' is not a valid LPR level specifier",s);
    return rc;
}


const char * om_bool_str(bool b) {
    return _om_boolean[b?1:0];
}
bool om_bool_val(const char *const s) {
    int rc=_om_id_find(boolean,s);
    WARN_IF(rc<0, "'%s' is not a valid boolean specifier",s);
    return rc%2;
}

