#include <limits.h>
#include <stdbool.h>
#include <string.h>

#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlschemas.h>
#include <libxml/xpathInternals.h>

#include <config.h>
#include <error-macros.h>
#include <utils.h>
#include <xml-utils.h>
#include <runconfig.h>
#include <appconfig.h>
#include <platspec.h>

/*
 * Sets 'conf->app.lbl' to the string retrieved from XPath 'path'.
 */
static void _set_lbl_from_sbml(runconfig_t * const conf, xmlXPathContextPtr xpath_ctx, const char *path)
{
    xmlXPathObjectPtr  xpath_obj;
    xmlNodeSetPtr      nodes;
    xmlChar           *xs;
    int                nr;

    if(conf->app.lbl!=NULL) {
	return;
    }

    xpath_obj = xmlXPathEvalExpression( (xmlChar *)path, xpath_ctx );
    RCHECK(xpath_obj, !=NULL, xmlXPathEvalExpression);
    nodes = xpath_obj->nodesetval;
    nr    = (nodes) ? nodes->nodeNr : 0;
    if( nr ) {
	xs = xmlNodeGetContent(nodes->nodeTab[0]);
	if( xs == NULL ) {
	    return;
	}
	conf->app.lbl = strdup((const char *)xs);
	RCHECK(conf->app.lbl, !=NULL, strdup);
	xmlFree(xs);
    }

    xmlXPathFreeObject(xpath_obj);
}

static void _set_n_rxns_from_sbml(runconfig_t * const conf, xmlXPathContextPtr xpath_ctx)
{
    xmlXPathObjectPtr  xpath_obj;
    xmlNodeSetPtr      nodes;
    int                nr;

    if(conf->app.n_rxns!=0) {
	return;
    }

    xpath_obj = xmlXPathEvalExpression( (xmlChar *)AC_N_RXNS_XPATH, xpath_ctx );
    RCHECK(xpath_obj, !=NULL, xmlXPathEvalExpression);
    nodes = xpath_obj->nodesetval;
    nr    = (nodes) ? nodes->nodeNr : 0;
    DIE_IF(nr<AC_N_RXNS_MIN, "Too few reactions in model");
    DIE_IF(nr>AC_N_RXNS_MAX, "Too many reactions in model");
    conf->app.n_rxns = nr;
	
    xmlXPathFreeObject(xpath_obj);
}

static void _set_n_spxs_from_sbml(runconfig_t * const conf, xmlXPathContextPtr xpath_ctx)
{
    xmlXPathObjectPtr  xpath_obj;
    xmlNodeSetPtr      nodes;
    int                nr;

    if(conf->app.n_spxs!=0) {
	return;
    }

    xpath_obj = xmlXPathEvalExpression( (xmlChar *)AC_N_SPXS_XPATH, xpath_ctx );
    RCHECK(xpath_obj, !=NULL, xmlXPathEvalExpression);
    nodes = xpath_obj->nodesetval;
    nr    = (nodes) ? nodes->nodeNr : 0;
    DIE_IF(nr<AC_N_SPXS_MIN, "Too few species in model");
    DIE_IF(nr>AC_N_SPXS_MAX, "Too many species in model");
    conf->app.n_spxs = nr;
	
    xmlXPathFreeObject(xpath_obj);
}

/*
 * Dies if the schema is not valid
 */
xmlDocPtr xu_open_sbmlfile(char const * const sbml_file, runconfig_t * const conf) 
{
    xmlDocPtr               sbml_doc;
    xmlDocPtr               xsd_doc;
    char                   *xsd_file;
    xmlSchemaParserCtxtPtr  parser_ctx;
    xmlSchemaValidCtxtPtr   valid_ctx;
    xmlSchemaPtr            schema;
    xmlXPathContextPtr      xpath_ctx;
    xmlXPathObjectPtr       xpath_obj;
    xmlNodeSetPtr           nodes;
    xmlChar                *xs;
    bool		    is_valid=false;

    int rc;
    int i, nr;
    char *s;


    sbml_doc = xmlParseFile(sbml_file);
    RCHECK(sbml_doc, !=NULL, xmlParseFile);

    if( conf->validate ) {
	xsd_file=u_search_path(AC_XSD_PATH, AC_SBML_XSD_FILE, conf->verbose);
	RCHECK(xsd_file, !=NULL, u_search_path);

	xsd_doc = xmlParseFile(xsd_file);
	RCHECK(xsd_doc, !=NULL, xmlParseFile);

	parser_ctx = xmlSchemaNewDocParserCtxt(xsd_doc);
	RCHECK(parser_ctx, !=NULL, xmlSchemaNewDocParserCtxt);
	
	schema = xmlSchemaParse(parser_ctx);
	RCHECK(schema, !=NULL, xmlSchemaParse);

	valid_ctx = xmlSchemaNewValidCtxt(schema);
	RCHECK(valid_ctx, !=NULL, xmlSchemaNewValidCtxt);
	
	is_valid = 0==xmlSchemaValidateDoc(valid_ctx, sbml_doc);
	DIE_IF(!is_valid, "SBML failed to validate against schema");

	xmlSchemaFreeValidCtxt(valid_ctx);
	xmlSchemaFree(schema);
	xmlSchemaFreeParserCtxt(parser_ctx);
	xmlFreeDoc(xsd_doc);
	free(xsd_file);
    }

    /*
     * Get model ID string - prefer SVN Id over
     */
    xpath_ctx = xmlXPathNewContext(sbml_doc);
    RCHECK(xpath_ctx,  != NULL, xmlXPathNewContext);
    rc=xmlXPathRegisterNs( xpath_ctx, (xmlChar *)AC_SBML_NS_PFX, (xmlChar *)AC_SBML_NS_URI );
    RCHECK(rc, ==0, xmlXPathRegisterNs);
    rc=xmlXPathRegisterNs( xpath_ctx, (xmlChar *)AC_SVN_NS_PFX, (xmlChar *)AC_SVN_NS_URI );
    RCHECK(rc, ==0, xmlXPathRegisterNs);

    _set_lbl_from_sbml(conf, xpath_ctx, AC_LBL_XPATH_1);
    _set_lbl_from_sbml(conf, xpath_ctx, AC_LBL_XPATH_2);
    _set_lbl_from_sbml(conf, xpath_ctx, AC_LBL_XPATH_3);


    /*
     * To prevent the label being destroyed later, if the output is
     * stored in Subversion, we replace all occurrences of '$' with
     * '%'. It's a little crude, but (for the time being at least)
     * solves more problems than it causes.
     */
    nr = strlen(conf->app.lbl);
    for(i=0; i<nr; i++) {
	if( conf->app.lbl[i] == '$' )
	    conf->app.lbl[i] = '%';
    }


    /*
     * Set number of reactions and species:
     */
    _set_n_rxns_from_sbml(conf, xpath_ctx);
    _set_n_spxs_from_sbml(conf, xpath_ctx);


    /*
     * Whitespace in <ci> elements causes huge headaches in the XSL-T,
     * so we programmatically strip it here:
     */
    rc=xmlXPathRegisterNs( xpath_ctx, (xmlChar *)AC_MATHML_NS_PFX,  (xmlChar *)AC_MATHML_NS_URI );
    RCHECK(rc, ==0, xmlXPathRegisterNs);

    /* Find MathML 'ci' nodes */
    xpath_obj = xmlXPathEvalExpression((xmlChar *)"//" AC_MATHML_NS_PFX ":ci", xpath_ctx);
    RCHECK(xpath_obj,  != NULL, xmlXPathEvalExpression);

    /* Update any nodes found */
    nodes = xpath_obj->nodesetval;
    nr    = (nodes) ? nodes->nodeNr : 0;

    for(i=nr-1; i>=0; i--) {
	xs = xmlNodeGetContent(nodes->nodeTab[i]);
	s  = u_trim( (char *)xs );
	xmlFree(xs);
	xmlNodeSetContent(nodes->nodeTab[i], (xmlChar *)s );
	free(s);
	if( nodes->nodeTab[i]->type != XML_NAMESPACE_DECL ) {
	    nodes->nodeTab[i] = NULL;
	}
    }

    /* Cleanup of LibXML stuff */
    xmlXPathFreeObject(xpath_obj);
    xmlXPathFreeContext(xpath_ctx); 
    
//    xmlDocDump(stdout, sbml_doc);

    return sbml_doc;
}


xsltStylesheetPtr xu_open_stylesheet(char const * const filename, const runconfig_t * const conf) 
{
    xsltStylesheetPtr xsp;

    (void)conf;

    xsp = xsltParseStylesheetFile( (const xmlChar*)filename );
    RCHECK(xsp, !=NULL, xsltParseStylesheetFile);
    return xsp;
}


void xu_transform(xsltStylesheetPtr xsp, xmlDocPtr xdp, runconfig_t * const conf)
{
    xmlDocPtr res;
    int rc;
    
    const char **params;

    params=ps_xslt_params(conf);
    if( conf->verbose ) {
	u_dump_strvec(params, ',', '=');
    }

    res = xsltApplyStylesheet(xsp, xdp, params);
    RCHECK(res, !=NULL, xsltApplyStylesheet);

    rc = xsltSaveResultToFilename( rc_c_file(conf), res, xsp, 0);
    RCHECK(rc, !=-1, xsltSaveResultToFilename);

    xmlFreeDoc(res);
}
