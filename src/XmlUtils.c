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

/* Resolve ${.} formatted keywords in xml files.  To use a definition file
   (format A=B), provide the _deffile name; a NULL value passed to _deffile 
   causes the resolver to search in the environment for the key definition*/
void XmlUtils_resolve (char *_docname, xmlXPathContextPtr _context, char *_deffile) {
  xmlXPathObjectPtr result;
  xmlNodeSetPtr nodeset = NULL;
  xmlNodePtr nodePtr = NULL;
  char *substr,*var=NULL,*post=NULL,*env=NULL,*saveptr1,*saveptr2,*source=NULL;
  char *nodeContent=NULL;
  int i,start,isvar;
  static char newContent[SEQ_MAXFIELD];

  if (_deffile == NULL){
    SeqUtil_stringAppend( &source, "environment" );}
  else{
    SeqUtil_stringAppend( &source, "definition" );
  }
  SeqUtil_TRACE("XmlUtils_resolve(): performing %s replacements\n",source);

  result =  xmlXPathEvalExpression("//@*",_context);
  nodeset = result->nodesetval;
  for (i=0; i < nodeset->nodeNr; i++){
    nodePtr = nodeset->nodeTab[i];
    nodeContent = (char *) xmlNodeGetContent(nodePtr);
    substr = strtok_r(nodeContent,"${",&saveptr1);
    start=0;
    while (substr != NULL){
      isvar = (strstr(substr,"}") == NULL) ? 0 : 1;
      var = strtok_r(substr,"}",&saveptr2);
      if (strcmp(source,"environment") == 0){
	env = getenv(var);}
      else{
	env = SeqUtil_getdef(_deffile,var);
      }
      post = strtok_r(NULL," ",&saveptr2);
      if (env == NULL){
	if (isvar > 0){
	  raiseError("Variable %s referenced by %s but is not set in %s\n",var,_docname,source);}
	else{
	  strncpy(newContent+start,var,strlen(var));
	  start += strlen(var);
	}
      }
      else{
	SeqUtil_TRACE("XmlUtils_resolve(): replacing %s with %s value %s\n",var,source,env);
	strncpy(newContent+start,env,strlen(env));
	start += strlen(env);
      }
      if (post != NULL){
	strncpy(newContent+start,post,strlen(post));
	start += strlen(post);
      }
      newContent[start]=0;
      substr = strtok_r(NULL,"${",&saveptr1);
    }
    xmlNodeSetContent(nodePtr,(xmlChar *)newContent);
    free(nodeContent);
  }
  xmlXPathFreeObject(result);
  free(source);
}
