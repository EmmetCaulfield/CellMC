#ifndef XML_UTILS_H
#define XML_UTILS_H

#include <stdbool.h>
#include <runconfig.h>

#include <libxslt/transform.h>
#include <libxml/xmlreader.h>


xmlDocPtr xu_open_sbmlfile(char const * const sbmlfile, runconfig_t * const conf);
xsltStylesheetPtr xu_open_stylesheet(char const * const filename, const runconfig_t * const conf);
void xu_transform(xsltStylesheetPtr xsp, xmlDocPtr xdp, runconfig_t * const conf);


#endif /* XML_UTILS_H */
