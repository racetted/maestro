/* FlowVisitor.c - Visits flow.xml files to assign flow information to a
 * nodeDataPtr.
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/tree.h>
#include <libxml/xpathInternals.h>

#include "SeqNode.h"
#include "nodeinfo.h"
#include "SeqUtil.h"
#include "XmlUtils.h"
#include "SeqLoopsUtil.h"

#include "FlowVisitor.h"
#include "ResourceVisitor.h"


/********************************************************************************
 * Initializes the flow_visitor to the entry module;
 * Caller should check if the return pointer is NULL.
********************************************************************************/
FlowVisitorPtr Flow_newVisitor(const char *nodePath, const char *seq_exp_home,
                                                     const char *switch_args)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_newVisitor() begin\n");
   FlowVisitorPtr new_flow_visitor = (FlowVisitorPtr) malloc(sizeof(FlowVisitor));
   if( new_flow_visitor == NULL )
      goto out;

   {
      char * postfix = "/EntryModule/flow.xml";
      char * xmlFilename = (char *) malloc ( strlen(seq_exp_home) + strlen(postfix) + 1 );
      sprintf(xmlFilename, "%s%s", seq_exp_home,postfix);
      xmlDocPtr doc = XmlUtils_getdoc(xmlFilename);
      new_flow_visitor->context = xmlXPathNewContext(doc);
      free(xmlFilename);
   }

   new_flow_visitor->nodePath = (nodePath ? strdup(nodePath) : NULL );
   new_flow_visitor->expHome = seq_exp_home;
   new_flow_visitor->datestamp = NULL;
   new_flow_visitor->switch_args = (switch_args && strlen(switch_args) ? strdup(switch_args): NULL);


   new_flow_visitor->context->node = new_flow_visitor->context->doc->children;

   new_flow_visitor->currentFlowNode = NULL;
   new_flow_visitor->suiteName = NULL;
   new_flow_visitor->taskPath = NULL;
   new_flow_visitor->module = NULL;
   new_flow_visitor->intramodulePath = NULL;
   new_flow_visitor->currentNodeType = Task;

   new_flow_visitor->_stackSize = 0;

out_free:
out:
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_newVisitor() end\n");
   return new_flow_visitor;
}

static void _freeStack(FlowVisitorPtr fv)
{
   xmlXPathContextPtr context;
   while ( (context = _popContext(fv)) != NULL){
      xmlFreeDoc(context->doc);
      xmlXPathFreeContext(context);
   }
}

/********************************************************************************
 * Destructor for the flow_visitor object.
********************************************************************************/
int Flow_deleteVisitor(FlowVisitorPtr _flow_visitor)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_deleteVisitor() begin\n");
   if( _flow_visitor->context->doc != NULL )
      xmlFreeDoc(_flow_visitor->context->doc);
   if( _flow_visitor->context != NULL )
      xmlXPathFreeContext(_flow_visitor->context);

   _freeStack(_flow_visitor);

   /*
    * Same as with exp_home, I don't copy the string, so the memory for
    * datestamp belongs to the caller of Flow_newVisitor(), so I don't free it
    * here.
    */

   free(_flow_visitor->currentFlowNode);
   free(_flow_visitor->taskPath);
   free(_flow_visitor->module);
   free(_flow_visitor->intramodulePath);
   free(_flow_visitor->suiteName);
   free(_flow_visitor->nodePath);
   free(_flow_visitor->switch_args);

   free(_flow_visitor);
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_deleteVisitor() end\n");
   return FLOW_SUCCESS;
}

/********************************************************************************
 * Parses the given nodePath while gathering information and adding attributes
 * to the nodeDataPtr;
 * returns FLOW_SUCCESS if it is able to completely parse the path
 * returns FLOW_FAILURE otherwise.
********************************************************************************/
int Flow_parsePath(FlowVisitorPtr _flow_visitor, SeqNodeDataPtr _nodeDataPtr,
                                                         const char * _nodePath)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_parsePath() begin nodePath=%s\n", _nodePath);
   int retval = FLOW_SUCCESS;
   int totalCount = SeqUtil_tokenCount( _nodePath, "/" ) - 1;/* count is 0-based */
   int count = 0;

   for_tokens(pathToken, _nodePath, "/", sp){

      if ( Flow_doNodeQuery(_flow_visitor, pathToken, count == 0) == FLOW_FAILURE){
         retval = FLOW_FAILURE;
         goto out;
      }

      if( _flow_visitor->currentNodeType == Module ){
         if ( Flow_changeModule(_flow_visitor, pathToken) == FLOW_FAILURE ){
            retval = FLOW_FAILURE;
            goto out;
         }
      }

      Flow_updatePaths(_flow_visitor, pathToken, count != totalCount );

      /* retrieve node specific attributes */
      if(    _flow_visitor->currentNodeType != Task
          && _flow_visitor->currentNodeType != NpassTask )
         Flow_checkWorkUnit(_flow_visitor, _nodeDataPtr);

      if( _flow_visitor->currentNodeType == Switch )
         Flow_parseSwitchAttributes(_flow_visitor, _nodeDataPtr,
                                                  count == totalCount );

      if( _flow_visitor->currentNodeType == Loop && count != totalCount ){
         getNodeLoopContainersAttr(_nodeDataPtr, _flow_visitor->expHome,
                                                 _flow_visitor->currentFlowNode);
      }

      count++;
   }

out:
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_parsePath() end\n");
   return retval;
}


/********************************************************************************
 * Tries to get to the node specified by nodePath.
 * Returns FLOW_FAILURE if it can't get to it,
 * Returns FLOW_SUCCESS if it can go through the whole path without a query
 * failing.
 * Note that if the last query was successful, the path walk is considered
 * successful and we don't have to move to the next switch item.
********************************************************************************/
int Flow_walkPath(FlowVisitorPtr _flow_visitor, SeqNodeDataPtr _nodeDataPtr,
                                                         const char * nodePath)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_walkPath() begin\n");
   int count = 0;
   int totalCount = SeqUtil_tokenCount(nodePath,"/") - 1 ;
   int retval = FLOW_SUCCESS;
   for_tokens(pathToken, nodePath , "/", sp){
      if( Flow_doNodeQuery(_flow_visitor, pathToken, count == 0) == FLOW_FAILURE ){
         retval = FLOW_FAILURE;
         goto out;
      }

      if( count == totalCount ){
         SeqUtil_TRACE(TL_FULL_TRACE,"Flow_walkPath setting node %s to type %d\n",
                                          nodePath,_flow_visitor->currentNodeType);
         _nodeDataPtr->type = _flow_visitor->currentNodeType;
         retval = FLOW_SUCCESS;
         goto out;
      }

      if( _flow_visitor->currentNodeType == Switch ){
         const char * switchType = Flow_findSwitchType(_flow_visitor);
         const char * switchValue = switchReturn(_nodeDataPtr,switchType);
         if( Flow_findSwitchItem(_flow_visitor, switchValue) == FLOW_FAILURE ) {
            retval = FLOW_FAILURE;
            goto out;
         }
      }

      if ( _flow_visitor->currentNodeType == Module ){
         if( Flow_changeModule(_flow_visitor,pathToken) == FLOW_FAILURE ){
            retval = FLOW_FAILURE;
            goto out;
         }
      }
      count++;
   }

out:
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_walkPath() end\n");
   return retval;
}


/********************************************************************************
 * Sets the XML XPath context's current node to the child of the current node
 * whose name attribute matches the nodeName parameter.
********************************************************************************/
int Flow_doNodeQuery(FlowVisitorPtr _flow_visitor, const char * nodeName,
                                                               int isFirst)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_doNodeQuery() begin\n");
   xmlXPathObjectPtr result = NULL;
   int retval = FLOW_SUCCESS;
   char query[SEQ_MAXFIELD] = {'\0'};

   if ( isFirst )
      sprintf ( query, "(/*[@name='%s'])", nodeName );
   else
      sprintf ( query, "(child::*[@name='%s'])", nodeName );

   /* run the normal query */
   SeqUtil_TRACE(TL_FULL_TRACE,"Flow_doNodeQuery(): run the normal query:%s\n",query);
   if( (result = XmlUtils_getnodeset (query, _flow_visitor->context)) == NULL ) {
      retval = FLOW_FAILURE;
      goto out;
   }

   _flow_visitor->context->node = result->nodesetval->nodeTab[0];
   _flow_visitor->currentNodeType = getNodeType(_flow_visitor->context->node->name);

out_free:
   xmlXPathFreeObject(result);
out:
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_doNodeQuery() end\n");
   return retval;
}


/********************************************************************************
 * Changes the current module of the flow visitor.
 * returns FLOW_FAILURE if either the XML file cannot be parsed or an initial
 *                      "MODULE" query fails to return a result.
 * returns FLOW_SUCCESS oterwise.
********************************************************************************/
int Flow_changeModule(FlowVisitorPtr _flow_visitor, const char * module)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_changeModule(): begin : module=%s\n",module);
   int retval = FLOW_SUCCESS;
   xmlXPathObjectPtr result = NULL;


   char * infix = "/modules/";
   char * postfix = "/flow.xml";
   char xmlFilename[strlen(_flow_visitor->expHome) + strlen(infix)
                                  + strlen(module) + strlen(postfix) + 1];
   sprintf( xmlFilename, "%s%s%s%s", _flow_visitor->expHome, infix, module, postfix);

   if (Flow_changeXmlFile(_flow_visitor,  xmlFilename ) == FLOW_FAILURE){
      retval = FLOW_FAILURE;
      goto out;
   }

   if( (result = XmlUtils_getnodeset( "(/MODULE)", _flow_visitor->context )) == NULL ){
      retval = FLOW_FAILURE;
      goto out;
   } else {
      _flow_visitor->context->node = result->nodesetval->nodeTab[0];
      _flow_visitor->currentNodeType = Module;
      retval = FLOW_SUCCESS;
      goto out_free;
   }

out_free:
   xmlXPathFreeObject(result);
out:
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_changeModule() end\n");
   return retval;
}

int _pushContext(FlowVisitorPtr fv, xmlXPathContextPtr context)
{
   if( fv->_stackSize < MAX_CONTEXT_STACK_SIZE ){
      fv->_context_stack[(fv->_stackSize)++] = context;
      return FLOW_SUCCESS;
   } else {
      return FLOW_FAILURE;
   }
}

int Flow_saveContext(FlowVisitorPtr fv)
{
   return _pushContext(fv, fv->context);
}

xmlXPathContextPtr _popContext(FlowVisitorPtr fv)
{
   if ( fv->_stackSize > 0)
      return fv->_context_stack[--(fv->_stackSize)];
   else
      return NULL;
}

int Flow_restoreContext(FlowVisitorPtr fv)
{
   if (fv->context != NULL) {
      xmlFreeDoc(fv->context->doc);
      xmlXPathFreeContext(fv->context);
   }
   if( (fv->context = _popContext(fv)) == NULL ){
      return FLOW_FAILURE;
   } else {
      return FLOW_SUCCESS;
   }
}

xmlXPathContextPtr Flow_previousContext(FlowVisitorPtr fv)
{
   if( fv->_stackSize > 0)
      return fv->_context_stack[fv->_stackSize - 1];
   else
      return NULL;
}


/********************************************************************************
 * Changes the current xml document and context to a new one specified by
 * xmlFilename and saves the previous document and context.
 * Returns FLOW_SUCCESS if the XML file can be successfuly parsed and
 * Returns FLOW_FAILURE otherwise.
********************************************************************************/
int Flow_changeXmlFile(FlowVisitorPtr _flow_visitor, const char * xmlFilename)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_changeXmlFile(): begin : xmlFilename=%s\n", xmlFilename);
   xmlDocPtr doc = NULL;

   Flow_saveContext(_flow_visitor);

   if( (doc = XmlUtils_getdoc(xmlFilename)) == NULL ){
      return FLOW_FAILURE;
   }
   _flow_visitor->context = xmlXPathNewContext(doc);

   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_changeXmlFile(): end\n");
   return FLOW_SUCCESS;
}

/********************************************************************************
 * Appends things to the various paths we are constructing in getFlowInfo.
********************************************************************************/
int Flow_updatePaths(FlowVisitorPtr _flow_visitor, const char * pathToken, const int container)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_updatePaths() begin\n");
   int nodeType = _flow_visitor->currentNodeType;

   SeqUtil_stringAppend(&(_flow_visitor->currentFlowNode),pathToken);

   if ( _flow_visitor->currentNodeType == Module ){
      free( _flow_visitor->module);
      _flow_visitor->module = strdup(pathToken);
   }

   if (container) {
      SeqUtil_stringAppend( &(_flow_visitor->currentFlowNode), "/");
      /* Case and case_item are not part of the task_depot */
      /* they are however part of the container path */
      switch( nodeType ){
         case Module:
            free(_flow_visitor->taskPath);
            _flow_visitor->taskPath = NULL;
         case Family:
         case Loop:
         case Switch:
         case ForEach:
            SeqUtil_stringAppend( &(_flow_visitor->taskPath), "/" );
            SeqUtil_stringAppend( &(_flow_visitor->taskPath), pathToken );
            free(_flow_visitor->intramodulePath);
            _flow_visitor->intramodulePath = NULL;
            SeqUtil_stringAppend( &(_flow_visitor->intramodulePath), _flow_visitor->taskPath );
            break;
         default:
            break;
      }
   } else {
      switch(nodeType){
         case Task:
         case NpassTask:
            SeqUtil_stringAppend( &(_flow_visitor->taskPath), "/" );
            SeqUtil_stringAppend( &(_flow_visitor->taskPath), pathToken );
            break;
         default:
            break;
      }
   }
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_updatePaths() end\n");
   return FLOW_SUCCESS;
}

char *Flow_findSwitchArg(FlowVisitorPtr fv)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_findSwitchArg() begin,fv->nodePath:%s fv->switch_args=%s\n",fv->nodePath,fv->switch_args);
   char *retval = NULL;

   char *switchName = SeqUtil_getPathLeaf(fv->currentFlowNode);
   for_tokens(token_pair,fv->switch_args,",",sp1){
      SeqUtil_TRACE(TL_FULL_TRACE,"token_pair:%s\n",token_pair);
      char *tok_name = token_pair;
      char *tok_value;
      tok_value = strstr(token_pair,"=");
      *tok_value++ = '\0';
      SeqUtil_TRACE(TL_FULL_TRACE,"tok_name=%s, tok_value=%s\n",tok_name, tok_value);
      /* getchar(); */
      if(strcmp(tok_name,switchName)==0){
         retval = strdup(tok_value);
         goto out_free;
      }
   }

out_free:
   free(switchName);
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_findSwitchArg() end\n");
   return retval;
}
/********************************************************************************
 * Parses the attributes of a switch node into the nodeDataPtr.
 * Returns FLOW_SUCCESS if the right switch item (or default) is found.
 * Returns FLOW_FAILURE otherwise.
********************************************************************************/
int Flow_parseSwitchAttributes(FlowVisitorPtr fv,
                                 SeqNodeDataPtr _nodeDataPtr, int isLast )
{

   const char * switchType = NULL;
   char * switchValue = NULL;
   int retval = FLOW_SUCCESS;

   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_parseSwitchAttributes(): begin\n");

   if( (switchType = Flow_findSwitchType(fv)) == NULL )
      raiseError("Flow_parseSwitchAttributes(): switchType not found\n");

   /*
    * Switch args are here to allow us to select a SWITCH_ITEM by simply
    * specifying it in the special switch_args string instead of relying on the
    * context
    */
   if( fv->switch_args == NULL || isLast){
      switchValue = switchReturn(_nodeDataPtr, switchType);
   } else {
      switchValue = Flow_findSwitchArg(fv);
   }

   /*
    * Insert the switch_path=switchValue key-value pair into the node's switch
    * answers list.
    */
   char * fixedSwitchPath = SeqUtil_fixPath( fv->currentFlowNode );
   SeqNameValues_insertItem(&(_nodeDataPtr->switchAnswers), fixedSwitchPath , switchValue );

   /*
    * Enter the correct switch item
    */
   if( Flow_findSwitchItem(fv, switchValue) == FLOW_FAILURE ){
      SeqUtil_TRACE(TL_FULL_TRACE,"Flow_parseSwitchAttributes(): no SWITCH_ITEM found containing value=%s and no SWITCH_ITEM found containing value=%s\n", switchValue);
      retval = FLOW_FAILURE;
      goto out_free;
   }

   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_parseSwitchAttributes(): end\n");

   if(isLast){
      SeqNode_addSpecificData(_nodeDataPtr, "VALUE", switchValue);
      /* PHIL: do this outside of the while instead of using isLast */
      _nodeDataPtr->type = Switch;
   } else {
      SeqNode_addSpecificData( _nodeDataPtr, "SWITCH_TYPE", switchType );
      SeqNode_addSwitch(_nodeDataPtr,fixedSwitchPath , switchType, switchValue);
   }

out_free:
   free(switchValue);
   free(fixedSwitchPath);
   return retval;
}

/********************************************************************************
 * This function returns the switch type of the current node in the XML XPath
 * context
********************************************************************************/
const char * Flow_findSwitchType(const FlowVisitorPtr fv ){
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_findSwitchType() begin\n");
   const char * switchType = xmlGetProp(fv->context->node,"type");
out:
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_findSwitchType() end, returning %s\n",switchType);
   return switchType;
}

/********************************************************************************
 * Moves the flow visitor to the switch item of the current node whose name
 * contains switchValue or to the default if no switch item matches switchValue.
 * Returns FLOW_SUCCESS if a valid switch item is found.
 * Returns FLOW_FAILURE if no switch item is found.
********************************************************************************/
int Flow_findSwitchItem( FlowVisitorPtr _flow_visitor,const char *switchValue )
{
   SeqUtil_TRACE(TL_FULL_TRACE,"Flow_findSwitchItem(): begin\n");
   int retval = FLOW_SUCCESS;
   /* Look for SWITCH_ITEMs whose name attribute contains switchValue as one of
    * it's comma separated tokens */
   if ( Flow_findSwitchItemWithValue(_flow_visitor, switchValue ) == FLOW_SUCCESS ){
      retval = FLOW_SUCCESS;
      goto out;
   } else if ( Flow_findDefaultSwitchItem(_flow_visitor) == FLOW_SUCCESS ) {
      retval = FLOW_SUCCESS;
      goto out;
   } else {
      retval = FLOW_FAILURE;
      goto out;
   }

out_free:
out:
   SeqUtil_TRACE(TL_FULL_TRACE,"Flow_findSwitchItem(): end\n");
   return retval;
}

/********************************************************************************
 * Moves visitor to switch item child that has switchValue if it exists
 * Returns FLOW_FAILURE on failure
 * Returns FLOW_SUCCESS on success
********************************************************************************/
int Flow_findSwitchItemWithValue( FlowVisitorPtr _flow_visitor, const char * switchValue)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_findSwitchItemWithValue() begin\n");
   int retval = FLOW_FAILURE;
   xmlXPathObjectPtr switchItemResult;

   if( (switchItemResult = XmlUtils_getnodeset( "(child::SWITCH_ITEM[not(@name='default')])", _flow_visitor->context)) == NULL){
      retval = FLOW_FAILURE;
      goto out;
   }

   for_results( switchItem, switchItemResult){
      if( Flow_switchItemHasValue(_flow_visitor, switchItem, switchValue)){
         _flow_visitor->context->node = switchItem;
         retval = FLOW_SUCCESS;
         goto out_free;
      }
   }

out_free:
   xmlXPathFreeObject(switchItemResult);
out:
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_findSwitchItemWithValue() end\n");
   return retval;
}

/********************************************************************************
 * Moves visitor to default switch item child of current node if it exists.
 * Returns FLOW_FAILURE on failure
 * Returns FLOW_SUCCESS on success
********************************************************************************/
int Flow_findDefaultSwitchItem( FlowVisitorPtr _flow_visitor)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_findDefaultSwitchItem() begin\n");
   int retval = FLOW_SUCCESS;
   xmlXPathObjectPtr switchItemResult = NULL;

   if ( (switchItemResult = XmlUtils_getnodeset( "(child::SWITCH_ITEM[@name='default'])", _flow_visitor->context)) == NULL ){
      retval = FLOW_FAILURE;
      goto out;
   } else {
      _flow_visitor->context->node = switchItemResult->nodesetval->nodeTab[0];
      retval = FLOW_SUCCESS;
      goto out_free;
   }

out_free:
   xmlXPathFreeObject(switchItemResult);
out:
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_findDefaultSwitchItem() end\n");
   return retval;
}

/********************************************************************************
 * Returns FLOW_TRUE if the SWITCH_ITEM currentNodePtr has the argument switchValue as
 * one of the tokens in it's name attribute.
 * Note that we use the XML XPath context to perform the attribute search.  For
 * this, we need to change the context's current node temporarily and restore it
 * upon exiting the function.
********************************************************************************/
int Flow_switchItemHasValue(const FlowVisitorPtr _flow_visitor,
                            xmlNodePtr currentNodePtr, const char*  switchValue)
{
   /* Save xml context->node and restore it at the end  */
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_switchItemHasValue() begin\n");
   xmlNodePtr previousNodePtr = _flow_visitor->context->node;
   _flow_visitor->context->node = currentNodePtr;
   int retval = FLOW_FALSE;

   xmlXPathObjectPtr attributesResult = NULL;
   if( (attributesResult = XmlUtils_getnodeset( "(@name)" , _flow_visitor->context)) == NULL )
      raiseError("Flow_switchItemHasValue(): SWITCH_ITEM with no name attribute\n");

   retval = switchNameContains(attributesResult->nodesetval->nodeTab[0]->children->content, switchValue);

out:
   xmlXPathFreeObject(attributesResult);
   _flow_visitor->context->node = previousNodePtr;
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_switchItemHasValue() end\n");
   return retval;
}

/********************************************************************************
 * Parses the string name as a comma separated list of values and returns 1 if
 * the argument value is one of those values.
 * Returns FLOW_TRUE if the name does contain the switchValue and
 * returns FLOW_FALSE otherwise.
********************************************************************************/
int switchNameContains(const char * name, const char * switchValue)
{
   int retval = FLOW_FALSE;
   for_tokens( sv_token, name , ",)",sp){
      if ( strcmp(sv_token, switchValue) == 0){
         retval = FLOW_TRUE;
         goto out;
      }
   }
out:
   return retval;
}


/********************************************************************************
 * Parses worker path for current node if said node has a work_unit attribute.
********************************************************************************/
int Flow_checkWorkUnit(FlowVisitorPtr _flow_visitor, SeqNodeDataPtr _nodeDataPtr)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_checkWorkUnit begin\n");
   int retval = FLOW_SUCCESS;
   xmlXPathObjectPtr attributesResult = NULL;

   if ( (attributesResult = XmlUtils_getnodeset( "(@work_unit)", _flow_visitor->context) ) == NULL ){
      goto out;
   } else {
      SeqUtil_TRACE(TL_FULL_TRACE, "Attribute 'work_unit' found\n");
   }

   if( _flow_visitor->currentNodeType == Task || _flow_visitor->currentNodeType == NpassTask) {
      SeqUtil_TRACE(TL_CRITICAL, "Work unit mode is only for containers (single_reserv=1)");
      retval = FLOW_FAILURE;
      goto out_free;
   }
   if ( _flow_visitor->currentFlowNode == NULL ){
      SeqUtil_TRACE(TL_CRITICAL, "Work unit mode cannot be on the root node (single_reserv=1)");
      retval = FLOW_FAILURE;
      goto out_free;
   }

   Resource_parseWorkerPath(_flow_visitor->currentFlowNode, _flow_visitor->expHome, _nodeDataPtr);

out_free:
   xmlXPathFreeObject(attributesResult);
out:
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_checkWorkUnit end\n");
   return retval;
}

/********************************************************************************
 * Sets the nodeDataPtr's pathToModule, taskPath, suiteName, module and
 * intramodulePath from info gathered while parsing the xml files.
********************************************************************************/
int Flow_setPathData(FlowVisitorPtr _flow_visitor, SeqNodeDataPtr _nodeDataPtr)
{
   SeqUtil_TRACE(TL_FULL_TRACE,"Flow_setPathData() begin\n");

   Flow_setPathToModule(_flow_visitor,_nodeDataPtr);
   SeqNode_setInternalPath( _nodeDataPtr, _flow_visitor->taskPath );
   Flow_setSuiteName(_flow_visitor, _nodeDataPtr);
   SeqNode_setModule( _nodeDataPtr, _flow_visitor->module );
   SeqNode_setIntramoduleContainer( _nodeDataPtr, _flow_visitor->intramodulePath );
   _nodeDataPtr->type = _flow_visitor->currentNodeType;

   SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.getFlowInfo() *********** Node Information **********\n" );
   SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.getFlowInfo() node path:%s\n", _flow_visitor->currentFlowNode );
   SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.getFlowInfo() container=%s\n", _nodeDataPtr->container );
   SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.getFlowInfo() intramodule_container=%s\n", _flow_visitor->intramodulePath );
   SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.getFlowInfo() pathToModule=%s\n",_nodeDataPtr->pathToModule );
   SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.getFlowInfo() taskPath=%s\n", _flow_visitor->taskPath );

   SeqUtil_TRACE(TL_FULL_TRACE,"Flow_setPathData() end\n");
   return FLOW_SUCCESS;
}

/********************************************************************************
 * Sets the pathToModule of the node by subtracting the intramodulePath from
 * _nodeDataPtr->container.
********************************************************************************/
int Flow_setPathToModule(FlowVisitorPtr _flow_visitor, SeqNodeDataPtr _nodeDataPtr)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_setPathToModule() begin\n");
   int entryModule = (_flow_visitor->_stackSize == 0);
   char pathToModule[SEQ_MAXFIELD] = {'\0'};

   if( _flow_visitor->intramodulePath != NULL && ! entryModule ){
      int lengthDiff = strlen(_nodeDataPtr->container) - strlen( _flow_visitor->intramodulePath);
      strncpy(pathToModule, _nodeDataPtr->container, lengthDiff);
   } else {
      strcpy(pathToModule, _nodeDataPtr->container );
   }
   sprintf(pathToModule,"%s/%s",pathToModule,_flow_visitor->module);

   SeqNode_setPathToModule(_nodeDataPtr,pathToModule);

   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_setPathToModule() end\n");
   return FLOW_SUCCESS;
}

/********************************************************************************
 * Sets the suiteName based on the experiment home.
********************************************************************************/
int Flow_setSuiteName(FlowVisitorPtr _flow_visitor, SeqNodeDataPtr _nodeDataPtr)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_setSuiteName() begin\n");
   char *suiteName = (char*) SeqUtil_getPathLeaf(_flow_visitor->expHome);
   SeqNode_setSuiteName( _nodeDataPtr, suiteName );
   free(suiteName);
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_setSuiteName() end\n");
   return FLOW_SUCCESS;
}

/********************************************************************************
 * Adds flow dependencies to _nodeDataPtr
 * Returns FLOW_SUCCESS if there are dependencies,
 * Returns FLOW_FAILURE if there are none.  It should probably still return
 * FLOW_SUCCESS even if there are no dependencies.
********************************************************************************/
int Flow_parseDependencies(FlowVisitorPtr _flow_visitor, SeqNodeDataPtr _nodeDataPtr)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_parseDependencies begin\n");
   int retval = FLOW_SUCCESS;
   xmlXPathObjectPtr result = NULL;
   xmlXPathContextPtr context = NULL;

   if (_flow_visitor->currentNodeType == Module){
      context = Flow_previousContext(_flow_visitor);
   } else {
      context = _flow_visitor->context;
   }

   if( (result = XmlUtils_getnodeset ("(child::DEPENDS_ON)", context )) == NULL ) {
      retval = FLOW_FAILURE;
      goto out;
   }

   parseDepends( result, _nodeDataPtr, 1 );

out_free:
   xmlXPathFreeObject (result);
out:
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_parseDependencies end\n");
   return retval;
}

/********************************************************************************
 * Parses submits into _nodeDataPtr
********************************************************************************/
int Flow_parseSubmits(FlowVisitorPtr _flow_visitor, SeqNodeDataPtr _nodeDataPtr)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_parseSubmits begin\n");
   SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.getFlowInfo() *********** submits **********\n");

   xmlXPathObjectPtr result = XmlUtils_getnodeset ("(child::SUBMITS)", _flow_visitor->context);
   parseSubmits( result, _nodeDataPtr );

out_free:
   xmlXPathFreeObject (result);
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_parseSubmits return FLOW_SUCCESS\n");
   return FLOW_SUCCESS;
}

/********************************************************************************
 * Parses the preceding sibings and following siblings into _nodeDataPtr
 * We should get this tracinf info out of here, it clutters the code and nobody
 * is going to see it.  There should be a function to print this stuff.
********************************************************************************/
int Flow_parseSiblings(FlowVisitorPtr _flow_visitor, SeqNodeDataPtr _nodeDataPtr)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_parseSiblings begin\n");
   SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.getFlowInfo() *********** node siblings **********\n");
   int switchItemFound = 0;
   switchItemFound = (strcmp(_flow_visitor->context->node->name, "SWITCH_ITEM") == 0);
   xmlXPathObjectPtr result = NULL;
   xmlXPathContextPtr context = NULL;
   char query[SEQ_MAXFIELD] = {'\0'};
   char * switchPrefix = NULL;

   if (switchItemFound)
      switchPrefix = strdup("../");
   else
      switchPrefix = strdup("");

   if (_nodeDataPtr->type == Module)
      context = Flow_previousContext(_flow_visitor);
   else
      context = _flow_visitor->context;

   sprintf( query, "(%spreceding-sibling::*[@name])", switchPrefix);
   result =  XmlUtils_getnodeset (query, context);
   if (result) {
      SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.getFlowInfo() *********** preceding siblings found**********\n");
   }
   parseNodeSiblings( result, _nodeDataPtr);
   xmlXPathFreeObject (result);

   sprintf( query, "(%sfollowing-sibling::*[@name])", switchPrefix);
   result =  XmlUtils_getnodeset (query, context);
   if (result) {
      SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.getFlowInfo() *********** following siblings found**********\n");
   }
   parseNodeSiblings( result, _nodeDataPtr);

out_free:
   free(switchPrefix);
   xmlXPathFreeObject (result);
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_parseSiblings return FLOW_SUCCESS\n");
   return FLOW_SUCCESS;
}

/********************************************************************************
 * Calls parseNodeSpecifics with the attributes of the current node of the flow
 * visitor.
********************************************************************************/
int Flow_parseSpecifics(FlowVisitorPtr _flow_visitor, SeqNodeDataPtr _nodeDataPtr)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_parseSpecifics() begin\n");
   int retval = FLOW_SUCCESS;
   xmlXPathObjectPtr result = NULL;

   if ( (result = XmlUtils_getnodeset("(@*)", _flow_visitor->context)) == NULL){
      retval = FLOW_FAILURE;
      goto out;
   }

   parseNodeSpecifics(_nodeDataPtr->type,result,_nodeDataPtr);

out_free:
   xmlXPathFreeObject(result);
out:
   SeqUtil_TRACE(TL_FULL_TRACE, "Flow_parseSpecifics() end\n");
   return retval;
}

/********************************************************************************
 * Prints the current information of the visitor
********************************************************************************/
void Flow_print_state(FlowVisitorPtr _flow_visitor, int trace_level)
{
   SeqUtil_TRACE( trace_level , "Current Node Name: %s\n", _flow_visitor->context->node->name);
   SeqUtil_TRACE( trace_level , "Current currentFlowNode: %s\n", _flow_visitor->currentFlowNode);
}














