/**
 *
 * $Id: cellmc.c 82 2009-02-01 22:50:41Z emmet $
 */
#include <stdio.h>
#include <getopt.h>

#include <config.h>
#include <cellmc.h>
#include <error-macros.h>

#include "runconfig.h" 
#include "xml-utils.h"
#include "platspec.h"
#include "compile.h"

int main(int argc, char *argv[]) {
    runconfig_t conf = RC_STRUCT_DEFAULT;
    char *sbmlfile;	/* SBML input file containing model	*/
    char *xsltfile;	/* XSL-T transformation chosen		*/

    /*
     * Pointers for LibXML's in-memory datastructures for the
     * SBML file and the XSLT transformation. We regard these
     * as opaque, since we don't particularly care about their
     * internals.
     */
    xmlDocPtr         sbml_obj;	/* LibXML document object	*/
    xsltStylesheetPtr xslt_obj; /* LibXSLT document object	*/

    rc_getopts(&conf, argc, argv);

    if( conf.verbose )
	CLOG("This is %s version %s\n", PACKAGE_NAME, PACKAGE_VERSION);

    DIE_IF(argc<=1, "No SBML file specified");

    rc_getenvars(&conf);

    sbmlfile = conf.sbmlfile;

    sbml_obj = xu_open_sbmlfile(sbmlfile, &conf);
    RCHECK(sbml_obj, !=NULL, xu_open_sbmlfile);

    if( conf.verbose ) {
	CLOG("Read SBML file '%s'\n", sbmlfile);
    }

    xsltfile = ps_choose_xsltfile(&conf);
    RCHECK(xsltfile, !=NULL, ps_choose_xsltfile);

    xslt_obj = xu_open_stylesheet(xsltfile, &conf);
    RCHECK(xslt_obj, !=NULL, xu_open_stylesheet);

    if( conf.verbose )
	CLOG("Parsed XSL-T file '%s'\n", xsltfile);

    xu_transform(xslt_obj,sbml_obj,&conf);

    if( conf.verbose ) {
	CLOG("Generated C code in '%s'\n", rc_c_file(&conf));
	rc_dump(&conf);
    }


    /*
     * Finished with the XML processing phase. Might as well do
     * cleanup now.
     */
    xsltFreeStylesheet(xslt_obj);
    xmlFreeDoc(sbml_obj);
    xsltCleanupGlobals();
    xmlCleanupParser();

    c_compile(&conf);

    rc_cleanup(&conf);

    return 0;
}
