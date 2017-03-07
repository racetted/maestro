/* XmlUtils.c - Xml reader utility functions used by the Maestro sequencer software package.
 * Copyright (C) 2011-2015  Operations division of the Canadian Meteorological Centre
 *                          Environment Canada
 *
 * Maestro is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * version 2.1 of the License.
 *
 * Maestro is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#include "XmlUtils.h"
#include "SeqUtil.h"
#include <string.h>

xmlDocPtr XmlUtils_getdoc (const char *_docname) {
   xmlDocPtr doc;
   if ( _docname == NULL ) return NULL;
   doc = xmlParseFile(_docname);
   
   if (doc == NULL ) {
      fprintf(stderr,"Document %s not parsed successfully. \n", _docname );
      return NULL;
   }

   return doc;
}

xmlXPathObjectPtr
XmlUtils_getnodeset (const xmlChar *_xpathQuery, xmlXPathContextPtr _context) {
   
   xmlXPathObjectPtr result;
   SeqUtil_TRACE( TL_FULL_TRACE,"XmlUtils_getnodeset(): xpath query: %s\n", _xpathQuery );
   result = xmlXPathEvalExpression(_xpathQuery, _context);

   if(xmlXPathNodeSetIsEmpty(result->nodesetval)) {
      xmlXPathFreeObject(result);
      SeqUtil_TRACE(TL_FULL_TRACE, "XmlUtils_getnodeset(): No result\n");
      return NULL;
   }
   return result;
}


/********************************************************************************
 * Wrapper around xmlGetProp that returns empty string if the property is not
 * found.
 *
 * This is used for objects use an empty string to mark a property that is not
 * defined.
 *
 * Note that the empty string is dynamically allocated because objects like
 * SeqDepData have a free routine that frees all of it's fields.
********************************************************************************/
char * XmlUtils_getProp_ES(xmlNodePtr nodePtr, const char *name)
{
   char * value = (char *) xmlGetProp( nodePtr, name );
   if( value == NULL )
      return strdup("");
   else
      return value;
}

/* Resolve keywords in xml files.  To use a definition file (format defined by
   SeqUtils_getdef(), provide the _deffile name; a NULL value passed to _deffile 
   causes the resolver to search in the environment for the key definition.*/
void XmlUtils_resolve (const char *_docname, xmlXPathContextPtr _context, const char *_deffile, const char* _seq_exp_home) {
  xmlXPathObjectPtr result;
  xmlNodeSetPtr nodeset = NULL;
  xmlNodePtr nodePtr = NULL;
  char *nodeContent=NULL;
  int i;

  result =  xmlXPathEvalExpression("//@*",_context);
  nodeset = result->nodesetval;
  for (i=0; i < nodeset->nodeNr; i++){
    nodePtr = nodeset->nodeTab[i];
    nodeContent = (char *) xmlNodeGetContent(nodePtr);
    if (strstr(nodeContent, "${") != NULL) {
       xmlNodeSetContent(nodePtr,(xmlChar *)SeqUtil_keysub(nodeContent,_deffile,_docname,_seq_exp_home));
    }
    free(nodeContent);
  }
  xmlXPathFreeObject(result);
}
