#include <string.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/tree.h>
#include <libxml/xpathInternals.h>

#include "XmlUtils.h"
#include "FlowVisitor.h"
#include "SeqUtil.h"
#include "nodeinfo.h"

#include "SeqNodeCensus.h"
/********************************************************************************
 * DOCUMENTATION: Implementation.
 *
 * The function getNodeList() retrieves the list of nodes of an experiment by
 * using a FlowVisitor object to visit the Flow.xml files of an experiment in a
 * depth first search manner using recursion.
 *
 * The base step of the recursion is to point the visitor on the root node of
 * the entry module's Flow.xml file, and add it's path (just the name) to the
 * new list.
 *
 * It then calls getNodeList_internal which obtains all the children of that
 * node (except SUBMITS).  For each child, depending on the type, it calls the
 * right subroutine (gnl_module, gnl_task, gnl_container, gnl_switch_item) which
 * carry out the right recursion step depending on the type, then continue the
 * recursion by calling getNodeList_internal.
 *
********************************************************************************/
void getNodeList_internal(FlowVisitorPtr fv, PathArgNodePtr *lp,
                            const char * basePath, const char * baseSwitchArgs,
                                                                     int depth);
void gnl_module(FlowVisitorPtr fv, PathArgNodePtr *pathArgList ,
                     const char *basePath,const char *baseSwitchArgs, int depth);

void gnl_task(FlowVisitorPtr fv, PathArgNodePtr *pathArgList ,
                     const char *basePath,const char *baseSwitchArgs, int depth);

void gnl_container(FlowVisitorPtr fv, PathArgNodePtr *pathArgList ,
                     const char *basePath,const char *baseSwitchArgs, int depth);

void gnl_switch_item(FlowVisitorPtr fv, PathArgNodePtr *pathArgList ,
                     const char *basePath,const char *baseSwitchArgs, int depth);

int PathArgNode_pushFront(PathArgNodePtr *list_head, const char *path, const char *switch_args, SeqNodeType type);


/********************************************************************************
 * Returns a list of nodes for a given experiment.
 * The format of the list is as a pair consisting of
 * path : The path to the node
 * switch_args : The branches of switches that must be taken to get to the node.
 * NOTE: The caller is responsible for deleting the list.
********************************************************************************/
PathArgNodePtr getNodeList(const char * seq_exp_home, const char *datestamp)
{
   PathArgNodePtr list_head = NULL;
   FlowVisitorPtr fv = Flow_newVisitor(NULL,seq_exp_home,NULL);
   fv->datestamp = datestamp;

   const char * basePath = (const char *) xmlGetProp(fv->context->node,
                                                      (const xmlChar *)"name");
   const char * fixedBasePath = SeqUtil_fixPath(basePath);
   /*
    * Base step of recursion
    */
   PathArgNode_pushFront(&list_head, fixedBasePath, "", Module );

   /*
    * Start recursion
    */
   getNodeList_internal(fv, &list_head ,fixedBasePath,"", 0);

out_free:
   free((char*)fixedBasePath);
   free((char*)basePath);
   Flow_deleteVisitor(fv);
   return list_head;
}

/********************************************************************************
 * Inserts a PathArgNodePtr at the front of the list pointed to by *list_head
 * and stores the address of the new element in list_head.
********************************************************************************/
int PathArgNode_pushFront(PathArgNodePtr *list_head, const char *path,
                                       const char *switch_args,SeqNodeType type)
{
   PathArgNodePtr newNode = NULL;
   if( (newNode = malloc(sizeof *newNode)) == NULL ){
      SeqUtil_TRACE(TL_CRITICAL,"PathArgPair_pushFront() No memory available.\n");
      return 1;
   }
   newNode->path = strdup(path);
   newNode->switch_args = strdup(switch_args);
   newNode->type = type;
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
      fprintf(stderr,"%s {%s} [type %d]\n", itr->path, itr->switch_args,itr->type);
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
      /*
       * Just noticed some redundance in the code, we set
       *    fv->context->node = xmlNode
       * yet we pass the xmlNode to the functions.  For now, I want to get my
       * thing working, but in the future, the extra parameter should be removed
       * in favor of the more elegant way of using the visitor the way it was
       * designed to be used.
       */
      fv->context->node = xmlNode;

      if( strcmp(xmlNode->name, "TASK") == 0 || strcmp(xmlNode->name, "NPASS_TASK") == 0)
      {
         gnl_task(fv,pathArgList,basePath,baseSwitchArgs,depth);
      }
      else if( strcmp(xmlNode->name, "MODULE") == 0)
      {
         gnl_module(fv,pathArgList,basePath,baseSwitchArgs,depth);
      }
      else if(   strcmp(xmlNode->name, "LOOP") == 0
                || strcmp(xmlNode->name, "FAMILY") == 0
                || strcmp(xmlNode->name, "SWITCH") == 0)
      {
         gnl_container(fv,pathArgList,basePath,baseSwitchArgs,depth);
      }
      else if( strcmp(xmlNode->name, "SWITCH_ITEM") == 0 )
      {
         if( fv->datestamp != NULL ){
            /*
             * This because if a datestamp is set, gnl_container() should move
             * the flow visitor into the correct switch item based on the
             * datestamp.
             *
             * An extension to this would be to have a
             * struct SwitchContext {
             *    char * datestamp
             *    char * color
             *    char * mood
             * }
             * if we want to add properties that change the the switch branch
             * that is taken. See comments at the top of FlowVisitor.c.
             */
            raiseError("ERROR: A datestamp was specified but getNodeList_internal reached a SWITCH_ITEM\n");
         }
         gnl_switch_item(fv,pathArgList,basePath,baseSwitchArgs,depth);
      }

      fv->context->node = previousNode;
   }

   xmlXPathFreeObject(results);
}

/********************************************************************************
 * Enter the right switch item based on the datestamp of the flow visitor.
 * NOTE the use of a dummy SeqNodeData since we need it to pass a datestamp to
 * SwitchReturn()
********************************************************************************/
static int gnl_enterSwitchItem(FlowVisitorPtr fv)
{
   int retval = FLOW_FAILURE;

   /* Get the switch type */
   const char *switch_type = Flow_findSwitchType(fv);

   /* Get the switch answer */
   SeqNodeData nd; nd.datestamp = fv->datestamp;
   const char *switch_value = switchReturn(&nd,switch_type);

   /* Enter the switch item corresponding to switch_value */
   retval = Flow_findSwitchItem(fv,switch_value);

out_free:
   free((char *)switch_value);
   free((char *)switch_type);
   return retval;
}

/********************************************************************************
 * Recursion stop for container nodes (except modules).
 * NOTE: If the visitor has a datestamp set (and in the future, a struct
 * SwitchContext), then when we are on a switch, we want to use to find the
 * right SWITCH_ITEM to enter.  At the next step in the recursion we will have
 * already entered the SWITCH_ITEM and will query the children of that node.
********************************************************************************/
void gnl_container(FlowVisitorPtr fv, PathArgNodePtr *pathArgList ,
                     const char *basePath,const char *baseSwitchArgs, int depth)
{
   char path[SEQ_MAXFIELD];
   const char * container_name = (const char *)xmlGetProp( fv->context->node, (const xmlChar *)"name");
   SeqNodeType type = getNodeType(fv->context->node->name);

   /*
    * Append current node name to basePath and add list entry
    */
   sprintf( path, "%s/%s", basePath,container_name);
   PathArgNode_pushFront( pathArgList, path, baseSwitchArgs, type);

   /*
    * If a datestamp is specified, use it to enter the correct switch item
    */
   if( fv->datestamp != NULL && strcmp( fv->context->node->name, "SWITCH" ) == 0){
      if( gnl_enterSwitchItem(fv) == FLOW_FAILURE ){
         /* If no switch item was found, stop recursion */
         goto out_free;
      }
   }

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
                     const char *basePath,const char *baseSwitchArgs, int depth)
{
   char path[SEQ_MAXFIELD];
   const char * module_name = (const char *)xmlGetProp( fv->context->node, (const xmlChar *)"name");

   /*
    * Append current node name to basePath and add list entry
    */
   sprintf( path, "%s/%s", basePath,module_name);
   PathArgNode_pushFront( pathArgList, path, baseSwitchArgs,Module);

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
 * Recursion step for Task and NpassTask nodes.
********************************************************************************/
void gnl_task(FlowVisitorPtr fv, PathArgNodePtr *pathArgList,
                     const char *basePath,const char *baseSwitchArgs, int depth)
{
   char path[SEQ_MAXFIELD];
   const char * name = (const char *)xmlGetProp( fv->context->node, (const xmlChar *)"name");
   SeqNodeType type = getNodeType(fv->context->node->name);

   /*
    * Append current node name to basePath and add list entry
    */
   sprintf( path, "%s/%s", basePath,name);
   PathArgNode_pushFront( pathArgList , path, baseSwitchArgs,type );

   /*
    * Recursion does ends with Task or NpassTask nodes
    */
   free((char *)name);
}

/********************************************************************************
 * Recursion step for switch items.
 * NOTE that The SWITCH_ITEM may have a name attribute like name="0,1,2" which
 * contains three values that will cause the switch to direct flow through this
 * switch item.  We only need one of these values, therefore we take the first
 * one.
********************************************************************************/
void gnl_switch_item(FlowVisitorPtr fv, PathArgNodePtr *pathArgList,
                     const char *basePath,const char *baseSwitchArgs, int depth)
{
   char switch_args[SEQ_MAXFIELD];
   const char * switch_item_name = (const char *)xmlGetProp(fv->context->node, (const xmlChar *)"name");
   const char * switch_name = SeqUtil_getPathLeaf(basePath);

   /*
    * Append current 'switch_item_name=first_switch_arg' to baseSwitchArgs
    */
   char * first_comma = strstr(switch_item_name,",");
   if( first_comma != NULL ) *first_comma = '\0';
   sprintf( switch_args, "%s%s=%s,",baseSwitchArgs,switch_name,switch_item_name );

   /*
    * Continue recursion
    */
   getNodeList_internal(fv, pathArgList ,basePath,switch_args, depth+1);

out:
   free((char*)switch_item_name);
   free((char*)switch_name);
}

