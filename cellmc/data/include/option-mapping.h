#ifndef OPTION_MAPPING_H
#define OPTION_MAPPING_H

#include <stdbool.h>

const char * om_prec_str(int i);
int om_prec_id(const char *const s);

const char * om_log_str(int i);
int om_log_id(const char *const s);

const char * om_rng_str(int i);
int om_rng_id(const char *const s);

const char * om_lpr_str(int i);
int om_lpr_id(const char * const s);

const char * om_bool_str(bool b);
bool om_bool_val(const char *const s);

#endif /* OPTION_MAPPING_H */
