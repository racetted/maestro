#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/tree.h>
#include <libxml/xpathInternals.h>

#ifndef _XMLUTILS
#define _XMLUTILS
xmlDocPtr XmlUtils_getdoc (char *_docname);

xmlXPathObjectPtr
XmlUtils_getnodeset (xmlChar *_xpathQuery, xmlXPathContextPtr _context);

void XmlUtils_resolve (char *_docname, xmlXPathContextPtr _context, char *_deffile);

#endif
