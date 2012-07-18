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

void XmlUtils_resolveEnv (char *_docname, xmlXPathContextPtr _context) {
  xmlXPathObjectPtr result;
  xmlNodeSetPtr nodeset = NULL;
  xmlNodePtr nodePtr = NULL;
  char *substr,*var=NULL,*post=NULL,*env=NULL,*saveptr1,*saveptr2;
  char *nodeContent=NULL;
  int i,start,isvar;
  static char newContent[4096];

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
      env = getenv(var);
      post = strtok_r(NULL," ",&saveptr2);
      if (env == NULL){
	if (isvar > 0){
	  raiseError("Environment variable %s referenced by %s but is not set\n",var,_docname);}
	else{
	  strncpy(newContent+start,var,strlen(var));
	  start += strlen(var);
	}
      }
      else{
	SeqUtil_TRACE("XmlUtils_resolve(): replacing %s with ENV value %s\n",var,env);
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
}
