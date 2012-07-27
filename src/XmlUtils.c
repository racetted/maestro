#include "XmlUtils.h"
#include "SeqUtil.h"
#include <string.h>

xmlDocPtr XmlUtils_getdoc (char *_docname) {
   xmlDocPtr doc;
   doc = xmlParseFile(_docname);
   
   if (doc == NULL ) {
      fprintf(stderr,"Document not parsed successfully. \n");
      return NULL;
   }

   return doc;
}

xmlXPathObjectPtr
XmlUtils_getnodeset (xmlChar *_xpathQuery, xmlXPathContextPtr _context) {
   
   xmlXPathObjectPtr result;
    SeqUtil_TRACE ("XmlUtils_getnodeset(): xpath query: %s\n", _xpathQuery );
   result = xmlXPathEvalExpression(_xpathQuery, _context);

   if(xmlXPathNodeSetIsEmpty(result->nodesetval)) {
      SeqUtil_TRACE("XmlUtils_getnodeset(): No result\n");
      return NULL;
   }
   return result;
}

/* Resolve keywords in xml files.  To use a definition file (format defined by
   SeqUtils_getdef(), provide the _deffile name; a NULL value passed to _deffile 
   causes the resolver to search in the environment for the key definition.*/
void XmlUtils_resolve (char *_docname, xmlXPathContextPtr _context, char *_deffile) {
  xmlXPathObjectPtr result;
  xmlNodeSetPtr nodeset = NULL;
  xmlNodePtr nodePtr = NULL;
  char *source=NULL,*nodeContent=NULL;
  int i;
  static char newContent[SEQ_MAXFIELD];

  result =  xmlXPathEvalExpression("//@*",_context);
  nodeset = result->nodesetval;
  for (i=0; i < nodeset->nodeNr; i++){
    nodePtr = nodeset->nodeTab[i];
    nodeContent = (char *) xmlNodeGetContent(nodePtr);
    xmlNodeSetContent(nodePtr,(xmlChar *)SeqUtil_keysub(nodeContent,_deffile,_docname));
    free(nodeContent);
  }
  xmlXPathFreeObject(result);
}
