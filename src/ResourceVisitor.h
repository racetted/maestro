/* ResourceVisitor.c - Parses data from resource XML file of a node into the
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

#ifndef _RESOURCE_VISITOR_H_
#define _RESOURCE_VISITOR_H_

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/tree.h>
#include <libxml/xpathInternals.h>
#include "SeqNode.h"

/* Uncomment this line to use the replacement functions for getting worker
 * information:
 * The body of Resource_setWorkerData() is changed for one that does no extra
 * work instead of calling nodeinfo,
 * ParseWorkerPath() is replaced by Resource_parseWorkerPath in
 * Flow_setWorkerPath() which do a DFS through the VALIDITY nodes of the xml
 * file.*/

#define RESOURCE_VISITOR_STACK_SIZE 30
#define RESOURCE_MAX_RECURSION_DEPTH ((RESOURCE_VISITOR_STACK_SIZE)-1)
#define RESOURCE_SUCCESS 0
#define RESOURCE_FAILURE -1
#define RESOURCE_TRUE 1
#define RESOURCE_FALSE 0

typedef struct _ValidityData {

   char * dow;
   char * hour;
   char * time_delta;
   char * valid_hour;
   char * valid_dow;
   char * local_index;
}ValidityData;
typedef ValidityData * ValidityDataPtr;

ValidityDataPtr newValidityData();
void deleteValidityData( ValidityDataPtr val );
void printValidityData(ValidityDataPtr val);
ValidityDataPtr getValidityData(xmlNodePtr validityNode);
int checkValidity(SeqNodeDataPtr _nodeDataPtr, ValidityDataPtr val );
int isValid(SeqNodeDataPtr _nodeDataPtr, xmlNodePtr validityNode);



typedef struct _ResourceVisitor {
   const char * nodePath; /* The path of the node for which we are getting
                             resources, which may not be the path of the
                             nodeDataPtr */
   xmlXPathContextPtr context;
   const char * xmlFile;
   const char * defFile;

   int loopResourcesFound;
   int forEachResourcesFound;
   int batchResourcesFound;
   int abortActionFound;
   int workerPathFound;

   xmlNodePtr _nodeStack[RESOURCE_VISITOR_STACK_SIZE];
   int _stackSize;
} ResourceVisitor;
typedef ResourceVisitor * ResourceVisitorPtr;


ResourceVisitorPtr newResourceVisitor(SeqNodeDataPtr _nodeDataPtr, const char * _seq_exp_home, const char * nodePath, SeqNodeType nodeType);
void deleteResourceVisitor(ResourceVisitorPtr rv);
int _pushNode(ResourceVisitorPtr rv, xmlNodePtr node);
int Resource_setNode(ResourceVisitorPtr rv, xmlNodePtr _node);
xmlNodePtr _popNode(ResourceVisitorPtr rv);
int Resource_unsetNode(ResourceVisitorPtr rv);

const char * xmlResourceFilename(const char * _seq_exp_home, const char * nodePath, SeqNodeType nodeType );
xmlXPathContextPtr Resource_createContext(SeqNodeDataPtr _nodeDataPtr, const char * xmlFile, const char * defFile, SeqNodeType nodeType);
xmlDocPtr xml_fallbackDoc(const char * xmlFile, SeqNodeType nodeType);

/* Node functions are passed to pareNodeDFS to be executed inside the right VALIDITY tags. */
typedef  int (*NodeFunction)(ResourceVisitorPtr rv, SeqNodeDataPtr _nodeDataPtr);

int Resource_parseNodeDFS(ResourceVisitorPtr rv, SeqNodeDataPtr _nodeDataPtr, NodeFunction nf);
int Resource_parseNodeDFS_internal(ResourceVisitorPtr rv, SeqNodeDataPtr _nodeDataPtr,
                                    xmlNodePtr node, NodeFunction nf, int depth);

int do_all(ResourceVisitorPtr rv, SeqNodeDataPtr _nodeDataPtr);
int Resource_getLoopAttributes(ResourceVisitorPtr rv, SeqNodeDataPtr _nodeDataPtr);
int Resource_getForEachAttributes(ResourceVisitorPtr rv, SeqNodeDataPtr _nodeDataPtr);
int Resource_getBatchAttributes(ResourceVisitorPtr rv,SeqNodeDataPtr _nodeDataPtr);
int Resource_getDependencies(ResourceVisitorPtr rv, SeqNodeDataPtr _nodeDataPtr);
int Resource_getAbortActions(ResourceVisitorPtr rv, SeqNodeDataPtr _nodeDataPtr);
int Resource_getContainerLoopAttributes(ResourceVisitorPtr rv, SeqNodeDataPtr _nodeDataPtr);
int Resource_getWorkerPath(ResourceVisitorPtr rv, SeqNodeDataPtr _nodeDataPtr);



int Resource_setWorkerData(ResourceVisitorPtr rv, SeqNodeDataPtr _nodeDataPtr);
int Resource_validateMachine(ResourceVisitorPtr rv, SeqNodeDataPtr _nodeDataPtr);
int Resource_setShell(ResourceVisitorPtr rv, SeqNodeDataPtr _nodeDataPtr);



/* "Public functions" */

int Resource_parseWorkerPath( const char * pathToNode, const char * _seq_exp_home, SeqNodeDataPtr _nodeDataPtr);
void getNodeLoopContainersAttr (  SeqNodeDataPtr _nodeDataPtr, const char *loopNodePath, const char *expHome );
int getNodeResources(SeqNodeDataPtr _nodeDataPtr, const char * expHome, const char * nodePath);


#endif
