#include "XmlUtils.h"
#include "SeqUtil.h"


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
