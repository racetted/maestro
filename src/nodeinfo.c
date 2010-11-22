#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <stdarg.h>
#include <pwd.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/tree.h>
#include <libxml/xpathInternals.h>
#include "nodeinfo.h"
#include "SeqUtil.h"


static int SHOW_ALL = 0;
static int SHOW_CFGPATH = 0;
static int SHOW_TASKPATH = 0;
static int SHOW_RESSOURCE = 0;
static int SHOW_ROOT_ONLY = 0;

xmlDocPtr
getdoc (char *_docname) {
   xmlDocPtr doc;
   doc = xmlParseFile(_docname);
   
   if (doc == NULL ) {
      fprintf(stderr,"Document not parsed successfully. \n");
      return NULL;
   }

   return doc;
}

xmlXPathObjectPtr
getnodeset (xmlDocPtr _doc, xmlChar *_xpathQuery, xmlXPathContextPtr _context){
   
   xmlXPathObjectPtr result;
    SeqUtil_TRACE ("nodeinfo.getnodeset() xpath query: %s\n", _xpathQuery );
   /* context = xmlXPathNewContext(_doc); */
   result = xmlXPathEvalExpression(_xpathQuery, _context);

   if(xmlXPathNodeSetIsEmpty(result->nodesetval)) {
      SeqUtil_TRACE("No result\n");
      return NULL;
   }
   /* xmlXPathFreeContext(context); */
   return result;
}


SeqNodeType getNodeType ( const xmlChar *_node_name ) {
   SeqNodeType nodeType = Task;
   SeqUtil_TRACE( "nodeinfo.getNodeType() node name: %s\n", _node_name);
   if ( strcmp( (char *) _node_name, "FAMILY" ) == 0 ) {
      nodeType = Family;
   } else if ( strcmp( (char *) _node_name, "MODULE" ) == 0 ) {
      nodeType = Module;
   } else if ( strcmp( _node_name, "TASK" ) == 0 ) {
      nodeType = Task;
   } else if ( strcmp( _node_name, "NPASS_TASK" ) == 0 ) {
      nodeType = NpassTask;
   } else if ( strcmp( _node_name, "LOOP" ) == 0 ) {
      nodeType = Loop;
   } else if ( strcmp( _node_name, "CASE" ) == 0 ) {
      nodeType = Case;
   } else if ( strcmp( _node_name, "CASE_ITEM" ) == 0 ) {
      nodeType = CaseItem;
   } else {
      printf( "ERROR: nodeinfo.getNodeType()  unprocessed xml node name:%s\n", _node_name);
   }
   SeqUtil_TRACE( "nodeinfo.getNodeType() type=%d\n", nodeType );
   return nodeType;
}

void parseDepends (xmlXPathObjectPtr _result, SeqNodeDataPtr _nodeDataPtr) {
   xmlNodeSetPtr nodeset;
   xmlNodePtr nodePtr;
   const xmlChar *nodeName = NULL;
   xmlChar *depType = NULL, *depUser = NULL, *depExp, *depName = NULL, 
      *depHour = NULL, *depStatus = NULL, *depIndex = NULL, *depLocalIndex = NULL;
   xmlChar *depPath = NULL;
   xmlAttrPtr propertiesPtr;
   int i=0;
   if (_result) {
      nodeset = _result->nodesetval;
      for (i=0; i < nodeset->nodeNr; i++) {
         SeqUtil_TRACE( "nodeinfo.parseDepends()   *** depends item ***\n");
         nodePtr = nodeset->nodeTab[i];
         nodeName = nodePtr->name;
         propertiesPtr = nodePtr->properties;
         depType = xmlGetProp( nodePtr, "type" );
         SeqUtil_TRACE( "nodeinfo.parseDepends() Parsing Dependency Type:%s\n", depType);
         if ( strcmp( depType, "task" ) == 0 || strcmp( depType, "npass_task" ) == 0 ) {
            depUser = xmlGetProp( nodePtr, "user" );
            depExp = xmlGetProp( nodePtr, "exp" );
            depName = xmlGetProp( nodePtr, "dep_name" );
            depIndex = xmlGetProp( nodePtr, "index" );
            depLocalIndex = xmlGetProp( nodePtr, "local_index" );
            depPath = xmlGetProp( nodePtr, "path" );
            depHour = xmlGetProp( nodePtr, "hour" );
            depStatus = xmlGetProp( nodePtr, "status" );
            SeqUtil_TRACE( "nodeinfo.parseDepends() dep depName: %s\n", depName );
            SeqUtil_TRACE( "nodeinfo.parseDepends() dep depIndex: %s\n", depIndex );
            SeqUtil_TRACE( "nodeinfo.parseDepends() dep depPath: %s\n", depPath );
            SeqUtil_TRACE( "nodeinfo.parseDepends() dep user: %s\n", depUser );
            SeqUtil_TRACE( "nodeinfo.parseDepends() dep depExp: %s\n", depExp );
            SeqUtil_TRACE( "nodeinfo.parseDepends() dep depHour: %s\n", depHour );
            SeqUtil_TRACE( "nodeinfo.parseDepends() depStatus: %s\n", depStatus );
            SeqUtil_TRACE( "nodeinfo.parseDepends() depLocalIndex: %s\n", depLocalIndex );
	    if ( strcmp( depType, "npass_task" ) == 0 ) { 
               SeqNode_addNodeDependency ( _nodeDataPtr, NpassDependancy, depName, depPath, depUser, depExp, depStatus, depIndex, depLocalIndex, depHour );
            } else {
               SeqNode_addNodeDependency ( _nodeDataPtr, NodeDependancy, depName, \
	                depPath, depUser, depExp, depStatus, depIndex, depLocalIndex, depHour );
	    }
            SeqUtil_TRACE( "nodeinfo.parseDepends() done\n" );
            xmlFree( depName );
            xmlFree( depIndex );
            xmlFree( depPath );
            xmlFree( depStatus );
            xmlFree( depUser );
            xmlFree( depExp );
            xmlFree( depHour );
         } else {
            SeqUtil_TRACE( "nodeinfo.parseDepends() unprocessed dependency type:%s\n", depType);
         }
         xmlFree( depType );
      }
   }
}

void parseLoopContainer (xmlXPathObjectPtr _result, SeqNodeDataPtr _nodeDataPtr,
   const char* loop_node_path ) {
   char* newLoopNodePath = NULL;
   xmlNodeSetPtr nodeset;
   xmlNodePtr nodePtr;
   const xmlChar *nodeName = NULL;
   xmlChar *loopStart = NULL, *loopStep = NULL, *loopEnd = NULL, *loopSet = NULL;
   xmlAttrPtr propertiesPtr;
   int i=0;
   if (_result) {
      nodeset = _result->nodesetval;
      SeqUtil_TRACE( "nodeinfo.parseLoopContainer()   *** loop info***\n");
      for (i=0; i < nodeset->nodeNr; i++) {
         nodePtr = nodeset->nodeTab[i];
         nodeName = nodePtr->name;
         if ( nodePtr->children != NULL ) {
            if( strcmp( nodeName, "start" ) == 0 ) {
               loopStart = strdup( nodePtr->children->content );
            } else if( strcmp( nodeName, "step" ) == 0 ) {
               loopStep = strdup( nodePtr->children->content );
            } else if( strcmp( nodeName, "set" ) == 0 ) {
               loopSet = strdup( nodePtr->children->content );
            } else if( strcmp( nodeName, "end" ) == 0 ) {
               loopEnd = strdup( nodePtr->children->content );
            }
         }
      }

      newLoopNodePath = (char *) SeqUtil_fixPath( loop_node_path );

      if( loopStep != NULL && loopSet != NULL ) {
         raiseError( "Loop Definition Error for node %s.\nLoop step and set attribute are mutually exclusive!\n", newLoopNodePath );
      }
      if( loopSet != NULL ) {
         SeqNode_addNumSetLoop ( _nodeDataPtr, newLoopNodePath, 
            loopStart, loopSet, loopEnd );
      } else {
         SeqNode_addNumLoop ( _nodeDataPtr, newLoopNodePath, 
            loopStart, loopStep, loopEnd );
      }

      SeqUtil_TRACE( "nodeinfo.parseLoopContainer() loopNode: %s\n", newLoopNodePath );
      SeqUtil_TRACE( "nodeinfo.parseLoopContainer() loopStart: %s\n", loopStart );
      SeqUtil_TRACE( "nodeinfo.parseLoopContainer() loopStep: %s\n", loopStep );
      SeqUtil_TRACE( "nodeinfo.parseLoopContainer() loopSet: %s\n", loopSet );
      SeqUtil_TRACE( "nodeinfo.parseLoopContainer() loopEnd: %s\n", loopEnd );
      free( newLoopNodePath );
      free( loopStart );
      free( loopStep );
      free( loopSet );
      free( loopEnd );
   }
}

void parseSubmits (xmlXPathObjectPtr _result, SeqNodeDataPtr _nodeDataPtr) {
   xmlNodeSetPtr nodeset;
   xmlNodePtr nodePtr;
   const xmlChar *nodeName, *propertyName;
   xmlAttrPtr propertiesPtr;
   int i=0, isSubmit=1;
   char* tmpstring;
   SeqUtil_TRACE( "nodeinfo.parseSubmits() called\n" );
   if (_result) {
      nodeset = _result->nodesetval;
      for (i=0; i < nodeset->nodeNr; i++) {
         nodePtr = nodeset->nodeTab[i];
         nodeName = nodePtr->name;
         propertiesPtr = nodePtr->properties;
	 isSubmit = 0;
         while (propertiesPtr != NULL) {
            propertyName = propertiesPtr->name;

            SeqUtil_TRACE( "nodeinfo.parseSubmits() submits:%s\n", propertiesPtr->children->content);
            SeqUtil_TRACE( "nodeinfo.parseSubmits() property:%s\n",propertyName);
            if ( strcmp( propertyName, "sub_name" ) == 0 ) {
	       isSubmit = 1;
               if (_nodeDataPtr->type == Task ) {
                   tmpstring=strdup(_nodeDataPtr->container);
                   SeqUtil_stringAppend(&tmpstring,"/");
                   SeqUtil_stringAppend(&tmpstring,propertiesPtr->children->content);
               }
               else {
                   tmpstring=strdup(_nodeDataPtr->name);
                   SeqUtil_stringAppend(&tmpstring,"/");
                   SeqUtil_stringAppend(&tmpstring,propertiesPtr->children->content);
               }
            } else if ( strcmp( propertyName, "type" ) == 0 && strcmp( propertiesPtr->children->content, "user" ) == 0  ) {
	       isSubmit = 0;
	       SeqUtil_TRACE( "nodeinfo.parseSubmits() got user submit node\n" );
	    }

            propertiesPtr = propertiesPtr->next;
         }
         if( isSubmit == 1 ) {
	    SeqNode_addSubmit(_nodeDataPtr, tmpstring);
         }
         free(tmpstring);
      }
   }
}

void parseAbortActions (xmlXPathObjectPtr _result, SeqNodeDataPtr _nodeDataPtr) {
   xmlNodeSetPtr nodeset = NULL;
   xmlNodePtr nodePtr = NULL;
   const xmlChar *nodeName = NULL, *propertyName = NULL;
   xmlAttrPtr propertiesPtr = NULL;
   int i=0;
   SeqUtil_TRACE( "nodeinfo.parseAbortActions() called\n" );
   if (_result) {
      nodeset = _result->nodesetval;
      for (i=0; i < nodeset->nodeNr; i++) {
         nodePtr = nodeset->nodeTab[i];
         nodeName = nodePtr->name;
         propertiesPtr = nodePtr->properties;
         while (propertiesPtr != NULL) {
            propertyName = propertiesPtr->name;
            SeqUtil_TRACE( "nodeinfo.parseAbortActions() propertyName:%s action:%s\n", propertyName, propertiesPtr->children->content);
            if ( strcmp( propertyName, "name" ) == 0 ) {
               SeqNode_addAbortAction( _nodeDataPtr, propertiesPtr->children->content );
            }
            propertiesPtr = propertiesPtr->next;
         }
      }
   }
}

/* to parse attributes that are specific to nodes for example loops(start-step-end), case (exec_script) etc
   there should only be one node in the result set */
void parseNodeSpecifics (SeqNodeType _nodeType, xmlXPathObjectPtr _result, SeqNodeDataPtr _nodeDataPtr) {
   xmlNodeSetPtr nodeset = NULL;
   xmlNodePtr nodePtr = NULL;
   const xmlChar *nodeName = NULL;
   xmlAttrPtr propertiesPtr = NULL;
   int i=0, isLoopSet = 0;
   SeqUtil_TRACE( "nodeinfo.parseNodeSpecifics() called node_type=%s\n", SeqNode_getTypeString( _nodeType ) );
   nodeset = _result->nodesetval;
   SeqUtil_TRACE( "nodeinfo.parseNodeSpecifics() nodeset cardinal=%d\n", nodeset->nodeNr );
   for (i=0; i < nodeset->nodeNr; i++) {
      nodePtr = nodeset->nodeTab[i];
      nodeName = nodePtr->name;
      /* name attribute is not node specific */
      if ( nodePtr->children != NULL && strcmp((char*)nodeName,"name") != 0 ) {
         SeqUtil_TRACE( "nodeinfo.parseNodeSpecifics() %s=%s\n", nodeName, nodePtr->children->content );
         SeqNode_addSpecificData ( _nodeDataPtr, nodeName, nodePtr->children->content );
	 if( _nodeType == Loop && strcmp( nodeName, "set" ) == 0 ) {
	    isLoopSet = 1;
	 }
      }
   }
   if( _nodeType == Loop ) {
      isLoopSet == 1 ? SeqNode_addSpecificData( _nodeDataPtr, "TYPE", "LoopSet" ) : SeqNode_addSpecificData( _nodeDataPtr, "TYPE", "Default" );
   }
}

/* To parse the siblings to generate a list. This list will be used to monitor container ends.  */

void parseNodeSiblings (xmlXPathObjectPtr _result, SeqNodeDataPtr _nodeDataPtr) {

   xmlNodeSetPtr nodeset;
   xmlNodePtr nodePtr;
   const xmlChar *nodeName = NULL, *propertyName = NULL;
   xmlAttrPtr propertiesPtr = NULL;

   int i=0;
   SeqUtil_TRACE( "nodeinfo.parseNodeSiblings() called\n" );
   if (_result) {
      nodeset = _result->nodesetval;
      for (i=0; i < nodeset->nodeNr; i++) {
         nodePtr = nodeset->nodeTab[i];
         nodeName = nodePtr->name;
         propertiesPtr = nodePtr->properties;
         while (propertiesPtr != NULL) {
            propertyName = propertiesPtr->name;

            if ( strcmp( (char*) propertyName, "name" ) == 0 ) {
               SeqNode_addSibling( _nodeDataPtr, propertiesPtr->children->content );
               SeqUtil_TRACE( "nodeinfo.parseNodeSiblings() sibling:%s\n", propertiesPtr->children->content);
            }
            propertiesPtr = propertiesPtr->next;
         }
      }
   }
}


void getRootNode( SeqNodeDataPtr _nodeDataPtr, const char *_seq_exp_home ) {
   char xml_file[256];
   char query[256];

   xmlDocPtr doc = NULL;
   xmlNodeSetPtr nodeset = NULL;
   xmlXPathObjectPtr result = NULL;
   xmlXPathContextPtr context = NULL;
   const xmlChar *nodeName = NULL;
   xmlNodePtr nodePtr;

   /* build the xmlfile path */
   sprintf( xml_file, "%s/EntryModule/flow.xml", _seq_exp_home);

   /* parse the xml file */
   doc = getdoc(xml_file);

   /* the context is used to walk trough the nodes */
   context = xmlXPathNewContext(doc);

   /* get the first MODULE name attribute */
   sprintf ( query, "(/MODULE/@name)");
   if( (result = getnodeset (doc, query, context)) == NULL ) {
      raiseError("MODULE not found in XML master file! (getRootNode)\n");
   }
   nodeset = result->nodesetval;

  nodePtr = nodeset->nodeTab[0];
  nodeName = nodePtr->name;
  SeqUtil_TRACE( "nodeinfo.getRootNode() nodeName=%s\n", nodeName );
  if ( nodePtr->children != NULL ) {
     SeqUtil_TRACE( "nodeinfo.getRootNode() %s=%s\n", nodeName, nodePtr->children->content );
     SeqNode_setName( _nodeDataPtr, SeqUtil_fixPath( nodePtr->children->content ) );

  }
}

   /*
   */
void getFlowInfo ( SeqNodeDataPtr _nodeDataPtr, const char *_jobPath, const char *_seq_exp_home ) {
   char xml_file[256], currentFlowNode[256];
   char query[256],submitsQuery[256];

   char *tmpstrtok = NULL, *tmpJobPath = NULL, *suiteName = NULL, *module = NULL;
   char *taskPath = NULL, *intramodulePath = NULL;
   int i = 0, count=0, totalCount = 0;
   xmlDocPtr doc = NULL;
   xmlAttrPtr propertiesPtr = NULL;
   xmlNodeSetPtr nodeset = NULL;
   xmlXPathObjectPtr result = NULL, submitsResult = NULL;
   xmlNodePtr currentNodePtr = NULL;
   xmlXPathContextPtr context = NULL;
 
   SeqUtil_TRACE( "nodeinfo.getFlowInfo() task:%s seq_exp_home:%s\n", _jobPath, _seq_exp_home );

   /* count is 0-based */
   totalCount = SeqUtil_tokenCount( _jobPath, "/" ) -1  ;
   SeqUtil_TRACE( "nodeinfo.getFlowInfo() tokenCount:%i \n", totalCount );
   /* build the xmlfile path */
   suiteName = (char*) SeqUtil_getPathLeaf( _seq_exp_home );
   sprintf( xml_file, "%s/EntryModule/flow.xml", _seq_exp_home);

   /* parse the xml file */
   doc = getdoc(xml_file);

   /* the context is used to walk trough the nodes */
   context = xmlXPathNewContext(doc);

   tmpJobPath = (char*) malloc( strlen( _jobPath ) + 1 );

   strcpy( tmpJobPath, _jobPath );
   tmpstrtok = (char*) strtok( tmpJobPath, "/" );
   while ( tmpstrtok != NULL ) {
      /* build the query */      
      if ( count == 0 ) {
         /* initial query */
         if ( SHOW_ROOT_ONLY == 1 ) {
            sprintf ( query, "(/MODULE)");
         } else {
            sprintf ( query, "(/*[@name='%s'])", tmpstrtok );
            strcpy( currentFlowNode, tmpstrtok );
         }
      } else {
         /* next queries relative to previous node context */
         sprintf ( query, "(child::*[@name='%s'])", tmpstrtok );
         sprintf ( submitsQuery, "(child::SUBMITS[@sub_name='%s'])", tmpstrtok );
         strcat( currentFlowNode, tmpstrtok );
      }
      /* run the normal query */
      if( (result = getnodeset (doc, query, context)) == NULL ) {
         raiseError("Node %s not found in XML master file! (getFlowInfo)\n", _jobPath);
      }

      /* At this point I should only have one node in the result set
      I'm getting the node to set it in the context so that
      I can retrieve other nodes relative to the current one
      i.e. depends/submits/etc
      */
      nodeset = result->nodesetval;
      currentNodePtr = nodeset->nodeTab[0];
      /* set the current node for the context
         change only if the current is of a container type family/case/case_item */
      _nodeDataPtr->type = getNodeType( currentNodePtr->name );
       context->node = currentNodePtr;

      /* read the new flow file described in the module */

      if ( _nodeDataPtr->type == Module && SHOW_ROOT_ONLY == 0 ) { 
       
         /* reset the intramodule path */
         free(intramodulePath);
         intramodulePath = NULL;
	 SeqUtil_stringAppend( &intramodulePath, "/" );

         /* read the flow file located in the module depot */
         memset( xml_file,'\0',sizeof xml_file);
         sprintf ( xml_file, "%s/modules/%s/flow.xml",_seq_exp_home, tmpstrtok ); 
         SeqUtil_TRACE( "nodeinfo Found new XML source from module:%s/%s path:%s\n", taskPath, tmpstrtok , xml_file);

         /* check if we're targetting the leaf node */
         if (count == totalCount) {
               xmlXPathFreeObject (result);
               context->node = currentNodePtr;
               /* retrieve node's siblings */
               SeqUtil_TRACE ( "nodeinfo.getFlowInfo() *********** node siblings **********\n");
               strcpy (query, "(preceding-sibling::*[@name])");
               result =  getnodeset (doc, query, context);
               if (result) {
               SeqUtil_TRACE ( "nodeinfo.getFlowInfo() *********** preceding siblings found**********\n");
               }
               parseNodeSiblings( result, _nodeDataPtr); 
               xmlXPathFreeObject (result);

               strcpy (query, "(following-sibling::*[@name])");
               result =  getnodeset (doc, query, context);
               if (result) {
               SeqUtil_TRACE ( "nodeinfo.getFlowInfo() *********** following siblings found**********\n");
               }
               parseNodeSiblings( result, _nodeDataPtr); 
               xmlXPathFreeObject (result);
         }

         xmlXPathFreeContext(context);
         context = NULL;
         xmlFreeDoc(doc); 
         doc = NULL;

         /* parse the new xml file */
         doc = getdoc(xml_file);

         /* the context is used to walk trough the nodes */
         context = xmlXPathNewContext(doc);
         sprintf ( query, "(/*[@name='%s'])", tmpstrtok );

         if( (result = getnodeset (doc, query, context)) == NULL ) {
             printf ( "ERROR: Problem with result set, Node %s not found in XML master file\n", _jobPath );
             exit(1);
         }
         nodeset = result->nodesetval;
         currentNodePtr = nodeset->nodeTab[0];
         context->node = currentNodePtr;

      }

      /* we are building the internal path of the node and container*/
      if ( count != totalCount ) {
         strcat( currentFlowNode, "/" );
         /* Case and case_item are not part of the task_depot */
         /* they are however part of the container path */
         if (_nodeDataPtr->type == Family ||_nodeDataPtr->type == Loop ) {
            SeqUtil_stringAppend( &taskPath, "/" );
            SeqUtil_stringAppend( &taskPath, tmpstrtok );
	    free(intramodulePath);
            intramodulePath = NULL;
	    SeqUtil_stringAppend( &intramodulePath, taskPath );
 


         }
         if (_nodeDataPtr->type == Module) {
            free(taskPath);
            taskPath = NULL;
            SeqUtil_stringAppend( &taskPath, "/" );
            SeqUtil_stringAppend( &taskPath, tmpstrtok );
	    free(intramodulePath);
            intramodulePath = NULL;
	    SeqUtil_stringAppend( &intramodulePath, taskPath );
          }

      } else {
         if( _nodeDataPtr->type == Task || _nodeDataPtr->type == NpassTask )  {
             SeqUtil_stringAppend( &taskPath, "/" );
             SeqUtil_stringAppend( &taskPath, tmpstrtok );
         }
      } 

      /* the containing module is set if the current node
      is a module and is not the last node  */
      if (  _nodeDataPtr->type == Module && tmpstrtok != NULL ) {
         free( module );
         module = malloc ( strlen(tmpstrtok) +1);
         strcpy( module, tmpstrtok );
      }
      /* get the next token */
      tmpstrtok = (char*) strtok(NULL,"/");

      count++;

      /* adds loop containers except if last node is also loop */
      if ( _nodeDataPtr->type == Loop && tmpstrtok != NULL ) {
         SeqUtil_TRACE( "nodeinfo.getFlowInfo() Found Loop Node=%s\n", currentFlowNode );
         /* SeqNode_addLoop( _nodeDataPtr, currentFlowNode ); */
         xmlXPathFreeObject (result);
         strcpy ( query, "(@*)");
         result = getnodeset (doc, query, context);
         parseLoopContainer( result, _nodeDataPtr, currentFlowNode );
      }

      if ( SHOW_ROOT_ONLY == 1 ) {
         tmpstrtok = NULL;
      }

      xmlXPathFreeObject (result);
      xmlXPathFreeObject (submitsResult);
   }

   /* at this point we're done validating the nodes */
   /* point the context to the last node retrieved so that we can
      do relative queries */
   SeqUtil_TRACE ( "nodeinfo.getFlowInfo() *********** Node Information **********\n" );
   SeqUtil_TRACE ( "nodeinfo.getFlowInfo() node path:%s\n", _jobPath );
   SeqUtil_TRACE ( "nodeinfo.getFlowInfo() container=%s\n", _nodeDataPtr->container );
   SeqUtil_TRACE ( "nodeinfo.getFlowInfo() intramodule_container=%s\n", intramodulePath );
   SeqUtil_TRACE ( "nodeinfo.getFlowInfo() taskPath=%s\n", taskPath );
   SeqNode_setInternalPath( _nodeDataPtr, taskPath );
   strcpy ( query, "(@*)");
   result = getnodeset (doc, query, context);
   /* parseNodeSpecifics( currentNodePtr->type, result, _nodeDataPtr ); */
   parseNodeSpecifics( _nodeDataPtr->type, result, _nodeDataPtr );
   xmlXPathFreeObject (result);

   if( SHOW_ALL ) {
      /* retrieve depends node */
      SeqUtil_TRACE ( "nodeinfo.getFlowInfo() *********** depends **********\n");
      strcpy ( query, "(child::DEPENDS_ON)");
   
      result = getnodeset (doc, query, context);
      parseDepends( result, _nodeDataPtr ); 
      xmlXPathFreeObject (result);
   
      /* retrieve submits node */
      SeqUtil_TRACE ( "nodeinfo.getFlowInfo() *********** submits **********\n");
      strcpy ( query, "(child::SUBMITS)");
      result = getnodeset (doc, query, context);
      parseSubmits( result, _nodeDataPtr ); 
      xmlXPathFreeObject (result);
   
      /* retrieve abort actions */
      SeqUtil_TRACE ( "nodeinfo.getFlowInfo() *********** abort actions **********\n");
      strcpy ( query, "(child::ABORT_ACTION)");
      result = getnodeset (doc, query, context);
      parseAbortActions( result, _nodeDataPtr ); 
      xmlXPathFreeObject (result);
   
      /* retrieve node's siblings */
      SeqUtil_TRACE ( "nodeinfo.getFlowInfo() *********** node siblings **********\n");
      strcpy (query, "(preceding-sibling::*[@name])");
      result =  getnodeset (doc, query, context);
      if (result) {
         SeqUtil_TRACE ( "nodeinfo.getFlowInfo() *********** preceding siblings found**********\n");
      }
      parseNodeSiblings( result, _nodeDataPtr); 
      xmlXPathFreeObject (result);
   
      strcpy (query, "(following-sibling::*[@name])");
      result =  getnodeset (doc, query, context);
      if (result) {
         SeqUtil_TRACE ( "nodeinfo.getFlowInfo() *********** following siblings found**********\n");
      }
      parseNodeSiblings( result, _nodeDataPtr); 
      xmlXPathFreeObject (result);
   }

   SeqNode_setSuiteName( _nodeDataPtr, suiteName ); 
   SeqNode_setModule( _nodeDataPtr, module );
   SeqNode_setIntramoduleContainer( _nodeDataPtr, intramodulePath );
   /* SeqNode_setMasterfile( _nodeDataPtr, xml_file ); */
   
   xmlXPathFreeContext(context);
   xmlFreeDoc(doc);
   xmlCleanupParser();
   free(suiteName);
   free(tmpJobPath);
   free(module);
   free(taskPath);
}

void getSchedulerInfo ( SeqNodeDataPtr  _nodeDataPtr, char* _jobPath, char* _seq_exp_home ) {
   FILE *cfgFilePtr = NULL;
   int startTagFound = 0;
   char* fullpath_cfg_file = NULL;
   char line[256], attrName[50], attrValue[50];
   if ( _nodeDataPtr->type == Task || _nodeDataPtr->type == NpassTask ) {
      fullpath_cfg_file = malloc ( strlen( _nodeDataPtr->taskPath ) + strlen( _seq_exp_home ) + 18 );
      sprintf( fullpath_cfg_file, "%s/modules%s.cfg", _seq_exp_home, _nodeDataPtr->taskPath );
   } else {
         fullpath_cfg_file = malloc ( strlen( _seq_exp_home) + strlen( _nodeDataPtr->intramodule_container) + strlen( _nodeDataPtr->nodeName ) + 25 );
         sprintf( fullpath_cfg_file, "%s/modules%s/%s/container.cfg", _seq_exp_home, _nodeDataPtr->intramodule_container, _nodeDataPtr->nodeName);
   /*
      if ( _nodeDataPtr->intramodule_container != NULL ) {
         fullpath_cfg_file = malloc ( strlen( _seq_exp_home) + strlen( _nodeDataPtr->intramodule_container) + strlen( _nodeDataPtr->nodeName ) + 25 );
         sprintf( fullpath_cfg_file, "%s/modules%s/%s/container.cfg", _seq_exp_home, _nodeDataPtr->intramodule_container, _nodeDataPtr->nodeName);
      } else {
         fullpath_cfg_file = malloc ( strlen( _seq_exp_home) + strlen( _nodeDataPtr->nodeName) + strlen( _nodeDataPtr->nodeName ) + 25 );
         sprintf( fullpath_cfg_file, "%s/modules%s/container.cfg", _seq_exp_home, _nodeDataPtr->nodeName);
      } 	
      */
   }
   SeqUtil_TRACE ( "nodeinfo.getSchedulerInfo() cfg_file=%s\n", fullpath_cfg_file );

   if ( (cfgFilePtr = fopen( fullpath_cfg_file, "r" )) == NULL ) {
      SeqUtil_TRACE ( "nodeinfo.getSchedulerInfo() unable to open cfg_file=%s\n", fullpath_cfg_file );
   } else {
      /* the seq scheduler info for the node starts and ends with the <seq_scheduler> tag in
         the config file */
      while ( fgets( line, 256, cfgFilePtr ) != NULL && startTagFound == 0) {
         if( strstr( line, "<seq_scheduler>" ) != NULL ) {
            startTagFound = 1;
         }
      }
      if ( startTagFound == 1 ) {
         while ( fgets( line, 256, cfgFilePtr ) != NULL && strstr( line, "</seq_scheduler>" ) == NULL ) {
            if ( sscanf( line, "%*s SEQ_ATTR %s %s", attrName, attrValue ) == 2 ) {
               SeqUtil_TRACE ( "nodeinfo.getSchedulerInfo() attrName=%s attrValue=%s\n", attrName, attrValue );
               if ( strcmp( attrName, "cpu" ) == 0 ) {
                  SeqNode_setCpu( _nodeDataPtr, attrValue );
               } else if ( strcmp( attrName, "machine" ) == 0 ) {
                  SeqNode_setMachine( _nodeDataPtr, attrValue );
               } else if ( strcmp( attrName, "queue" ) == 0 ) {
                  SeqNode_setQueue( _nodeDataPtr, attrValue );
               } else if ( strcmp( attrName, "memory" ) == 0 ) {
                  SeqNode_setMemory( _nodeDataPtr, attrValue );
               } else if ( strcmp( attrName, "mpi" ) == 0 ) {
                  _nodeDataPtr->mpi = atoi( attrValue );
               } else if ( strcmp( attrName, "wallclock" ) == 0 ) {
                  _nodeDataPtr->wallclock = atoi( attrValue );
               } else if ( strcmp( attrName, "catchup" ) == 0 ) {
                  _nodeDataPtr->catchup = atoi( attrValue );
               } else {
                  printf ( "nodeinfo.getSchedulerInfo() WARNING: Unprocessed attribute=%s \nfile=%s\nline=%s\n", attrName, fullpath_cfg_file, line );
               }
            } else {
               printf ( "nodeinfo.getSchedulerInfo() ERROR: Invalid syntax\nfile=%s\nline=%s\n", fullpath_cfg_file, line );
            }
         }
      }
      fclose( cfgFilePtr );
   }
   free( fullpath_cfg_file );
}

void test() {
   char* testPtr = NULL;
   //testPtr = malloc( strlen( "initial_value" ) + 1 );
   //strcpy( testPtr, "initial_value" );
   SeqUtil_stringAppend( &testPtr, "value0" );
   SeqUtil_stringAppend( &testPtr, "value1" );
   printf( "value=%s\n", testPtr );

   printf( "Count of / source=%s value=%d\n", "test/node/value", SeqUtil_tokenCount( "test/node/value","/" ));
}

SeqNodeDataPtr nodeinfo ( const char* node, const char* filters ) {

   char* seq_exp_home = NULL, *newNode = NULL;
   SeqNodeDataPtr  nodeDataPtr = NULL;

   seq_exp_home=getenv("SEQ_EXP_HOME");
   if ( seq_exp_home == NULL ) {
      raiseError("SEQ_EXP_HOME not set! (nodeinfo) \n");
   }
   if( strstr( filters, "all" ) != NULL ) SHOW_ALL = 1;
   if( strstr( filters, "cfg" ) != NULL ) SHOW_CFGPATH = 1;
   if( strstr( filters, "task" ) != NULL ) SHOW_TASKPATH = 1;
   if( strstr( filters, "res" ) != NULL ) SHOW_RESSOURCE = 1;
   if( strstr( filters, "root" ) != NULL ) SHOW_ROOT_ONLY = 1;

   newNode = (char*) SeqUtil_fixPath( node );
   SeqUtil_TRACE ( "nodeinfo.nodefinfo() trying to create node %s\n", newNode );
   nodeDataPtr = (SeqNodeDataPtr) SeqNode_createNode ( newNode );
   if ( SHOW_ROOT_ONLY ) {
      getRootNode ( nodeDataPtr, seq_exp_home );
   } else {
      getFlowInfo ( nodeDataPtr, (char*) newNode, seq_exp_home );
   }
   if( SHOW_ALL || SHOW_RESSOURCE ) {
      getSchedulerInfo( nodeDataPtr, (char*) newNode, seq_exp_home );
   }
   return nodeDataPtr;
}
