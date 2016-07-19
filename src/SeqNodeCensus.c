#include <string.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/tree.h>
#include <libxml/xpathInternals.h>

#include "XmlUtils.h"
#include "FlowVisitor.h"
#include "SeqUtil.h"

#include "SeqNodeCensus.h"
void getNodeList_internal(FlowVisitorPtr fv, PathArgNodePtr *lp,
                            const char * basePath, const char * baseSwitchArgs,
                                                                     int depth);
void gnl_module(FlowVisitorPtr fv, PathArgNodePtr *pathArgList ,
                        const char *basePath,const char *baseSwitchArgs,
                        int depth,xmlNodePtr xmlNode);
void gnl_task(FlowVisitorPtr fv, PathArgNodePtr *pathArgList ,
                        const char *basePath,const char *baseSwitchArgs,
                        int depth,xmlNodePtr xmlNode);
void gnl_container(FlowVisitorPtr fv, PathArgNodePtr *pathArgList ,
                        const char *basePath,const char *baseSwitchArgs,
                        int depth,xmlNodePtr xmlNode);
void gnl_switch_item(FlowVisitorPtr fv, PathArgNodePtr *pathArgList ,
                        const char *basePath,const char *baseSwitchArgs,
                        int depth,xmlNodePtr xmlNode);

int PathArgNode_pushFront(PathArgNodePtr *list_head, const char *path, const char *switch_args);


/********************************************************************************
 * Returns a list of nodes for a given experiment.
 * The format of the list is as a pair consisting of
 * path : The path to the node
 * switch_args : The branches of switches that must be taken to get to the node.
 * NOTE: The caller is responsible for deleting the list.
********************************************************************************/
PathArgNodePtr getNodeList(const char * seq_exp_home)
{
   PathArgNodePtr pap = NULL;
   FlowVisitorPtr fv = Flow_newVisitor(NULL,seq_exp_home,NULL);

   const char * basePath = (const char *) xmlGetProp(fv->context->node,
                                                      (const xmlChar *)"name");

   /*
    * Base step of recursion
    */
   PathArgNode_pushFront(&pap, basePath, "" );

   /*
    * Start recursion
    */
   getNodeList_internal(fv, &pap ,basePath,"", 0);

out_free:
   free((char*)basePath);
   Flow_deleteVisitor(fv);
   return pap;
}

/********************************************************************************
 * Inserts a PathArgNodePtr at the front of the list pointed to by *list_head
 * and stores the address of the new element in list_head.
********************************************************************************/
int PathArgNode_pushFront(PathArgNodePtr *list_head, const char *path, const char *switch_args)
{
   PathArgNodePtr newNode = NULL;
   if( (newNode = malloc(sizeof *newNode)) == NULL ){
      SeqUtil_TRACE(TL_CRITICAL,"PathArgPair_pushFront() No memory available.\n");
      return 1;
   }
   newNode->path = strdup(path);
   newNode->switch_args = strdup(switch_args);
   newNode->nextPtr = *list_head;

   *list_head = newNode;
   return 0;
}

/********************************************************************************
 * Frees the entire list pointed to by *list_head.  For good measure, we set
 * *list_head to NULL.
********************************************************************************/
int PathArgNode_deleteList(PathArgNodePtr *list_head)
{
   if(*list_head == NULL )
      return 0;

   PathArgNodePtr current = *list_head, tmp_next = *list_head;

   while( current != NULL ){
      tmp_next = current->nextPtr;
      free((char*)current->path);
      free((char*)current->switch_args);
      free(current);
      current = tmp_next;
   }
   *list_head = NULL;
   return 0;
}

/********************************************************************************
 * Prints the nodes of the list if the trace level is sufficient
********************************************************************************/
void PathArgNode_printList(PathArgNodePtr list_head, int trace_level)
{
   if( trace_level < SeqUtil_getTraceLevel())
      return;

   for_pap_list(itr,list_head){
      PathArgNodePtr pap_itr = (PathArgNodePtr) itr;
      fprintf(stderr,"%s {%s}\n", pap_itr->path, pap_itr->switch_args);
   }
}

/********************************************************************************
 * Main internal recursive routine for parsing the flow tree of an experiment.
 * It works in two steps since the recursive step is different depending on the
 * type of xmlNode encountered.
 * We get the children of the current xmlNode, then we redirect the recursion
 * through one of the pft_int_${nodeType}() functions, and these functions
 * continue the recursion.
********************************************************************************/
void getNodeList_internal(FlowVisitorPtr fv, PathArgNodePtr *pathArgList,
                            const char * basePath, const char * baseSwitchArgs,
                                                                     int depth)
{
   /*
    * Oh heavenly Father, I pray to thee to forgive me for this inelegant query
    * which amounts to saying "all children except SUBMITS".  There has to be a
    * way of saying something like child::!SUBMITS or something.
    */
   xmlXPathObjectPtr results = XmlUtils_getnodeset((const xmlChar*)
                                                    "(child::FAMILY|child::TASK\
                                                     |child::SWITCH|child::SWITCH_ITEM\
                                                     |child::MODULE|child::LOOP\
                                                     |child::NPASS_TASK|child::FOR_EACH)"
                                                      , fv->context);
   for_results( xmlNode, results ){
      xmlNodePtr previousNode = fv->context->node;
      fv->context->node = xmlNode;

      if( strcmp(xmlNode->name, "TASK") == 0 || strcmp(xmlNode->name, "NPASS_TASK") == 0)
      {
         gnl_task(fv,pathArgList,basePath,baseSwitchArgs,depth,xmlNode);
      }
      else if( strcmp(xmlNode->name, "MODULE") == 0)
      {
         gnl_module(fv,pathArgList,basePath,baseSwitchArgs,depth,xmlNode);
      }
      else if(   strcmp(xmlNode->name, "LOOP") == 0
                || strcmp(xmlNode->name, "FAMILY") == 0
                || strcmp(xmlNode->name, "SWITCH") == 0)
      {
         gnl_container(fv,pathArgList,basePath,baseSwitchArgs,depth,xmlNode);
      }
      else if( strcmp(xmlNode->name, "SWITCH_ITEM") == 0 )
      {
         gnl_switch_item(fv,pathArgList,basePath,baseSwitchArgs,depth,xmlNode);
      }

      fv->context->node = previousNode;
   }

   xmlXPathFreeObject(results);
}
/********************************************************************************
 * Recursion stop for container nodes (except modules)
********************************************************************************/
void gnl_container(FlowVisitorPtr fv, PathArgNodePtr *pathArgList ,
                        const char *basePath,const char *baseSwitchArgs,
                        int depth,xmlNodePtr xmlNode)
{
   char path[SEQ_MAXFIELD];
   const char * container_name = (const char *)xmlGetProp( xmlNode, (const xmlChar *)"name");

   /*
    * Append current node name to basePath and add list entry
    */
   sprintf( path, "%s/%s", basePath,container_name);
   PathArgNode_pushFront( pathArgList, path, baseSwitchArgs );

   /*
    * Continue recursion
    */
   getNodeList_internal(fv, pathArgList ,path,baseSwitchArgs,depth+1);

out_free:
   free((char*)container_name);
}

/********************************************************************************
 * Recursion step for modules
********************************************************************************/
void gnl_module(FlowVisitorPtr fv, PathArgNodePtr *pathArgList,
                        const char *basePath,const char *baseSwitchArgs,
                        int depth,xmlNodePtr xmlNode)
{
   char path[SEQ_MAXFIELD];
   const char * module_name = (const char *)xmlGetProp( xmlNode, (const xmlChar *)"name");

   /*
    * Append current node name to basePath and add list entry
    */
   sprintf( path, "%s/%s", basePath,module_name);
   PathArgNode_pushFront( pathArgList, path, baseSwitchArgs );

   /*
    * Continue recursion
    */
   Flow_changeModule(fv, module_name);
   getNodeList_internal(fv, pathArgList ,path,baseSwitchArgs, depth+1);
   Flow_restoreContext(fv);

out_free:
   free((char*)module_name);
}

/********************************************************************************
 * Recursion step for Task and NpassTask nodes
********************************************************************************/
void gnl_task(FlowVisitorPtr fv, PathArgNodePtr *pathArgList,
                        const char *basePath,const char *baseSwitchArgs,
                        int depth,xmlNodePtr xmlNode)
{
   char path[SEQ_MAXFIELD];
   const char * name = (const char *)xmlGetProp( xmlNode, (const xmlChar *)"name");

   /*
    * Append current node name to basePath and add list entry
    */
   sprintf( path, "%s/%s", basePath,name);
   PathArgNode_pushFront( pathArgList , path, baseSwitchArgs );

   /*
    * Recursion does ends with Task or NpassTask nodes
    */
   free((char *)name);
}

/********************************************************************************
 * Recursion step for switch items
********************************************************************************/
void gnl_switch_item(FlowVisitorPtr fv, PathArgNodePtr *pathArgList,
                        const char *basePath,const char *baseSwitchArgs,
                        int depth,xmlNodePtr xmlNode)
{
   char switch_args[SEQ_MAXFIELD];
   const char * switch_item_name = (const char *)xmlGetProp(xmlNode, (const xmlChar *)"name");
   const char * switch_name = SeqUtil_getPathLeaf(basePath);

   /*
    * Append current switch_item_name=first_switch_arg' to baseSwitchArgs
    */
   char * first_comma = strstr(switch_item_name,",");
   if( first_comma != NULL ) *first_comma = 0;
   sprintf( switch_args, "%s%s=%s,",baseSwitchArgs,switch_name,switch_item_name );

   /*
    * Continue recursion
    */
   getNodeList_internal(fv, pathArgList ,basePath,switch_args, depth+1);

out:
   free((char*)switch_item_name);
   free((char*)switch_name);
}

