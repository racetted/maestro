/* nodeinfo.c - Creator of the node construct used by the Maestro sequencer software package.
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
#include <stdarg.h>
#include <pwd.h>
#include <unistd.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/tree.h>
#include <libxml/xpathInternals.h>
#include "nodeinfo.h"
#include "SeqDepends.h"
#include "tictac.h"
#include "SeqUtil.h"
#include "SeqDatesUtil.h"
#include "XmlUtils.h"
#include "SeqLoopsUtil.h"
#include "FlowVisitor.h"
#include "ResourceVisitor.h"

/* root node of xml resource file */
const char* NODE_RES_XML_ROOT = "/NODE_RESOURCES";
const char* NODE_RES_XML_ROOT_NAME = "NODE_RESOURCES";
SeqNodeType getNodeType ( const xmlChar *_node_name ) {
   SeqNodeType nodeType = Task;
   SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.getNodeType() node name: %s\n", _node_name);
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
   } else if ( strcmp( _node_name, "SWITCH" ) == 0 ) {
      nodeType = Switch;
   } else if ( strcmp( _node_name, "FOR_EACH" ) == 0 ) {
      nodeType = ForEach;
   } else {
      raiseError("ERROR: nodeinfo.getNodeType()  unprocessed xml node name:%s\n", _node_name);
   }
   SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.getNodeType() type=%d\n", nodeType );
   return nodeType;
}

void parseBatchResources (xmlXPathObjectPtr _result, SeqNodeDataPtr _nodeDataPtr) {
   xmlNodeSetPtr nodeset = NULL;
   xmlNodePtr nodePtr = NULL;
   const xmlChar *nodeName = NULL;
   char *tmpString = NULL; 
   char *cpuString = NULL;

   int i=0;
   SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseBatchResources() called\n" );
   if (_result) {
      nodeset = _result->nodesetval;
      SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseBatchResources() nodeset->nodeNr=%d\n", nodeset->nodeNr);
      for (i=0; i < nodeset->nodeNr; i++) {
         nodePtr = nodeset->nodeTab[i];
         nodeName = nodePtr->name;
         SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseBatchResources() nodePtr->name=%s\n", nodePtr->name);
         SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseBatchResources() value=%s\n", nodePtr->children->content );
	 if ( strcmp( nodeName, "cpu" ) == 0 ) {
            SeqNode_setCpu( _nodeDataPtr, nodePtr->children->content );
            cpuString=strdup(nodePtr->children->content);
	 } else if ( strcmp( nodeName, "cpu_multiplier" ) == 0 ) {
	    SeqNode_setCpuMultiplier( _nodeDataPtr, nodePtr->children->content );
         } else if ( strcmp( nodeName, "machine" ) == 0 ) {
            SeqNode_setMachine( _nodeDataPtr, nodePtr->children->content );
         } else if ( strcmp( nodeName, "queue" ) == 0 ) {
            SeqNode_setQueue( _nodeDataPtr, nodePtr->children->content );
         } else if ( strcmp( nodeName, "memory" ) == 0 ) {
            SeqNode_setMemory( _nodeDataPtr, nodePtr->children->content );
         } else if ( strcmp( nodeName, "mpi" ) == 0 ) {
             _nodeDataPtr->mpi = atoi( nodePtr->children->content );
             /* if cpu has already been set, and the mpi flag is on, it will need to be recalculated depending on the format it may change for npex / omp */
             if (cpuString !=NULL && _nodeDataPtr->mpi ) SeqNode_setCpu( _nodeDataPtr, cpuString );
         } else if ( strcmp( nodeName, "soumet_args" ) == 0 ) {
              /* add soumet args in the following order: 1) resource file 2) args sent by command line, who will override 1*/
             SeqUtil_stringAppend( &tmpString, nodePtr->children->content);
             SeqUtil_stringAppend( &tmpString, " ");
             SeqUtil_stringAppend( &tmpString, _nodeDataPtr->soumetArgs );
             SeqNode_setSoumetArgs( _nodeDataPtr, tmpString);
             free(tmpString);
         } else if ( strcmp( nodeName, "wallclock" ) == 0 ) {
             _nodeDataPtr->wallclock = atoi( nodePtr->children->content );
         } else if ( strcmp( nodeName, "immediate" ) == 0 ) {
             _nodeDataPtr->immediateMode = atoi( nodePtr->children->content );
         } else if ( strcmp( nodeName, "catchup" ) == 0 ) {
             _nodeDataPtr->catchup = atoi( nodePtr->children->content );
	 } else if ( strcmp( nodeName, "shell" ) == 0 ) {
	    SeqNode_setShell( _nodeDataPtr, nodePtr->children->content );
	 } else {
             raiseError("nodeinfo.parseBatchResources() ERROR: Unprocessed attribute=%s\n", nodeName);
         }
      }
   }
   free(cpuString);
}

/********************************************************************************
 *
********************************************************************************/
void validateDepIndices(SeqNodeDataPtr _nodeDataPtr, SeqDepDataPtr dep, int isIntraDep)
{
   char *tmpLoopName=NULL, *tmpValue=NULL;
   char *tmpSavePtr1 = NULL, *tmpSavePtr2 = NULL, *tmpString=NULL, tmpTokenName[100];
   SeqUtil_TRACE(TL_FULL_TRACE,"Nodeinfo_parseDepends() dep->local_index = %s\n",dep->local_index );
   SeqNameValuesPtr depArgs = NULL, localArgs = NULL, tmpIterator = NULL, tokenValues=NULL;
   if (isIntraDep) {
      SeqLoopsPtr loopsPtr = NULL;
      loopsPtr =  _nodeDataPtr->loops;
      while( loopsPtr != NULL ) {
         if( strstr(  _nodeDataPtr->pathToModule ,loopsPtr->loop_name ) != NULL ) {
            /* add loop arg to full dep index */
            tmpLoopName=(char*) SeqUtil_getPathLeaf( (const char*) loopsPtr->loop_name );
            SeqUtil_TRACE(TL_FULL_TRACE, "Nodeinfo_parseDepends() adding loop argument to dependency for name = %s\n", tmpLoopName );
            if (SeqNameValues_getValue(_nodeDataPtr->loop_args, tmpLoopName) != NULL) {
               SeqNameValues_insertItem( &depArgs, tmpLoopName, SeqNameValues_getValue(_nodeDataPtr->loop_args, tmpLoopName));
               SeqNameValues_insertItem( &localArgs, tmpLoopName, SeqNameValues_getValue(_nodeDataPtr->loop_args, tmpLoopName));
            }
            free(tmpLoopName);
         }
         loopsPtr  = loopsPtr->nextPtr;
      }
   }

   if( dep->local_index != NULL ) {
      /*validate local dependency args and create a namevalue list*/
      if( SeqLoops_parseArgs( &localArgs, dep->local_index ) != -1 ) {

         tmpIterator = localArgs;
         while (tmpIterator != NULL) {
            SeqUtil_TRACE(TL_FULL_TRACE,"Nodeinfo_parseDepends() tmpIterator->value=%s \n", tmpIterator->value);
            /*checks for current index keyword*/
            if (strcmp(tmpIterator->value,"CURRENT_INDEX")==0) {
               if ((tmpValue = SeqNameValues_getValue(_nodeDataPtr->loop_args, tmpIterator->name)) != NULL) {
                  SeqNameValues_setValue( &localArgs, tmpIterator->name,tmpValue);
                  free(tmpValue);
                  /* raiseError( "parseDepends(): Error -- CURRENT_INDEX keyword used in a non-loop context, or does not match current loop arguments. \n" ); */
               }
            } else if ((tmpSavePtr1=strstr(tmpIterator->value, "$((")) != NULL) {
               /* associative token local loopA's index -> target loopB's index */
               tmpSavePtr2=strstr(tmpSavePtr1,"))");
               if (tmpSavePtr2 == NULL) {
                  raiseError("parseDepends(): local dependency index format error with associative token. Format should be: %s=\"$((token))\"\n",tmpIterator->name);
               }
               memset(tmpTokenName,'\0',sizeof tmpTokenName);
               snprintf(tmpTokenName,strlen(tmpSavePtr1)-strlen(tmpSavePtr2) - strlen("$((")+1,"%s", tmpSavePtr1+3);
               if ((tmpString=SeqNameValues_getValue(_nodeDataPtr->loop_args, tmpIterator->name)) != NULL) {
                  SeqNameValues_setValue( &localArgs, tmpIterator->name, tmpString);
                  SeqNameValues_insertItem( &tokenValues, tmpTokenName, tmpString);
                  SeqUtil_TRACE(TL_FULL_TRACE,"Nodeinfo_parseDepends() inserting token=%s value=%s\n", tmpTokenName, tmpString);
               }
            }
            tmpIterator=tmpIterator->nextPtr;
         }
      } else {
         raiseError( "parseDepends(): local dependency index format error\n" );
      }
   }

   if( dep->index != NULL ) {
      /*validate dependency args and create a namevalue list*/
      if( SeqLoops_parseArgs( &depArgs, dep->index ) != -1 ) {
         tmpIterator = depArgs;
         while (tmpIterator != NULL) {
            /*checks for current index keyword*/
            if (strcmp(tmpIterator->value,"CURRENT_INDEX")==0) {
               if ((tmpValue = SeqNameValues_getValue(_nodeDataPtr->loop_args, tmpIterator->name)) != NULL) {
                  SeqNameValues_setValue( &depArgs, tmpIterator->name, tmpValue);
                  free(tmpValue);
                  /* raiseError( "parseDepends(): Error -- CURRENT_INDEX keyword used in a non-loop context, or does not match current loop arguments. \n" ); */
               }
            } else if ((tmpSavePtr1=strstr(tmpIterator->value, "$((")) != NULL) { 
               tmpSavePtr2=strstr(tmpSavePtr1,"))");
               if (tmpSavePtr2 == NULL) {
                  raiseError("parseDepends(): target dependency index format error with associative token. Format should be: %s=\"$((token))\"\n",tmpIterator->name);
               }
               memset(tmpTokenName,'\0',sizeof tmpTokenName);
               snprintf(tmpTokenName,strlen(tmpSavePtr1)-strlen(tmpSavePtr2) - strlen("$((")+1,"%s", tmpSavePtr1+3);
               SeqUtil_TRACE(TL_FULL_TRACE,"Nodeinfo_parseDepends() looking for token=%s\n", tmpTokenName);

               if ((tmpString=SeqNameValues_getValue(tokenValues, tmpTokenName)) != NULL) {
                  SeqNameValues_setValue( &depArgs, tmpIterator->name, tmpString);
               }
            }
            tmpIterator=tmpIterator->nextPtr;
         }   
      } else {
         raiseError( "parseDepends(): dependency index format error\n" );
      }
   }

   if ( depArgs != NULL ){
      free(dep->index);
      dep->index = SeqLoops_getLoopArgs(depArgs);
   }
   if( localArgs != NULL ){
      free(dep->local_index);
      dep->local_index = SeqLoops_getLoopArgs(localArgs);
   }
   SeqNameValues_deleteWholeList( &localArgs );
   SeqNameValues_deleteWholeList( &depArgs );
   SeqNameValues_deleteWholeList( &tokenValues );
}
SeqDepDataPtr xmlDepNode_to_depDataPtr(SeqNodeDataPtr _nodeDataPtr, xmlNodePtr nodePtr, int isIntraDep)
{
   SeqDepDataPtr dep = SeqDep_newDep();
   /*
    * char * depType = xmlGetProp( nodePtr, "type" );
    * if (strcmp(depType, "node") == 0)
    *    depType = NodeDependancy;
    * else if (strcmp(depType, "date") == 0)
    *    depType = DateDependancy;
    * else
    *    depType = NodeDependancy;
    */
   dep->type = NodeDependancy; /* Since there is only one dependency type */
   dep->exp = (char *) xmlGetProp( nodePtr, "exp" );

   /*
    * Get dep_name from XML and do relative path eveluation
    */
   char *depName = NULL;
   depName = (char *) xmlGetProp( nodePtr, "dep_name" );
   dep->node_name = SeqUtil_relativePathEvaluation(depName,_nodeDataPtr);
   free(depName);depName = NULL;

   dep->index = (char *) xmlGetProp( nodePtr, "index" );
   dep->local_index = (char *) xmlGetProp( nodePtr, "local_index" );
   dep->node_path = (char *) xmlGetProp( nodePtr, "path" );
   dep->hour = (char *) xmlGetProp( nodePtr, "hour" );
   dep->valid_hour = (char *) xmlGetProp( nodePtr, "valid_hour" );
   dep->valid_dow = (char *) xmlGetProp( nodePtr, "valid_dow" );
   dep->time_delta = (char *) xmlGetProp( nodePtr, "time_delta" );
   dep->protocol  = (char * ) xmlGetProp( nodePtr, "protocol" ); 
   dep->status = (char *) xmlGetProp( nodePtr, "status" );

   /*
    * Set default values for protocol and status if not set
    */
   if (dep->protocol == NULL) dep->protocol=strdup("polling"); 
   if (dep->status == NULL) dep->status=strdup("end"); 

   /*
    * Do some special treatement on dep->index and dep->local_index
    */
   validateDepIndices(_nodeDataPtr, dep, isIntraDep);

   return dep;
}

void parseDepends (xmlXPathObjectPtr _result, SeqNodeDataPtr _nodeDataPtr, int isIntraDep )
{
   if (_result == NULL){
      goto out;
   }

   xmlNodeSetPtr nodeset = _result->nodesetval;

   SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseDepends() nodeset->nodeNr=%d\n", nodeset->nodeNr);
   int i=0;
   for (i=0; i < nodeset->nodeNr; i++) {
      xmlNodePtr nodePtr = nodeset->nodeTab[i];
      SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseDepends()   *** depends_item=%s ***\n", nodePtr->name);

      /*
       * Design comments: This following paragraph should be in
       * xmlDepNode_to_depDataPtr() and I have already put the basic idea there.
       * In the interest of being seamless, I'm leaving the control flow as is.
       *
       * Suggestion: there should be no If here, there should be only one
       * SeqNode_addDependency() with the struct SeqDependencyData serving for
       * both types of dependency with a sort of "lazy polymorphism": the struct
       * has fields for both sub-types of dependency, and functions that deal
       * with them treat the deps differently based on the dep->type field.
       *
       * It's what the cool cats call runtime-type-ID (RTTI).  It's not as good
       * as polymorphism but I'm not going to get carried away with
       * improvements.
       */
      char * depType = NULL;
      depType = (char *) xmlGetProp( nodePtr, "type" );
      SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseDepends() Parsing Dependency Type:%s\n", depType);
      if ( depType == NULL ) depType=strdup("node");

      if ( strcmp( depType, "node" ) == 0 ) {

         SeqDepDataPtr dep = xmlDepNode_to_depDataPtr(_nodeDataPtr, nodePtr, isIntraDep);

         SeqDep_printDep(TL_FULL_TRACE, dep);

         /*
          * Add the dependency to the nodeDataPtr's dependency list
          */
         SeqNode_addNodeDependency ( _nodeDataPtr, dep->type, dep->node_name,
                                    dep->node_path, dep->exp, dep->status,
                                    dep->index, dep->local_index, dep->hour,
                                    dep->time_delta, dep->protocol,
                                    dep->valid_hour, dep->valid_dow );
      } else {
         SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseDepends() no dependency found.\n" );
      }
   }
out:
   return;
}

void parseLoopAttributes (xmlXPathObjectPtr _result, const char* _loop_node_path, SeqNodeDataPtr _nodeDataPtr) {
   xmlNodeSetPtr nodeset;
   xmlNodePtr nodePtr;
   const xmlChar *nodeName = NULL;
   xmlChar *loopStart = DEFAULT_LOOP_START_STR,
           *loopStep = DEFAULT_LOOP_STEP_STR,
           *loopEnd = DEFAULT_LOOP_END_STR,
           *loopSet = DEFAULT_LOOP_SET_STR,
           *loopExpression = strdup("");
   int i=0;
   
   if (_result) {
      nodeset = _result->nodesetval;
      SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseLoopAttributes()   *** loop info***\n");
      
      for (i=0; i < nodeset->nodeNr; i++) {
         nodePtr = nodeset->nodeTab[i];
         nodeName = nodePtr->name;
         SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseLoopAttributes() nodeName=%s, value:%s\n", nodeName, nodePtr->children->content);
         if ( nodePtr->children != NULL ) {
            if( strcmp( nodeName, "start" ) == 0 ) {
               free(loopStart);
               loopStart = strdup( nodePtr->children->content );
            } else if( strcmp( nodeName, "step" ) == 0 ) {
               free( loopStep );
               loopStep = strdup( nodePtr->children->content );
            } else if( strcmp( nodeName, "set" ) == 0 ) {
               free( loopSet );
               loopSet = strdup( nodePtr->children->content );
            } else if( strcmp( nodeName, "end" ) == 0 ) {
               free( loopEnd );
               loopEnd = strdup( nodePtr->children->content );
            } else if( strcmp( nodeName, "expression" ) == 0 ) {
               free(loopExpression);
               loopExpression = strdup( nodePtr->children->content );
               break;
            }
         }
      }
      if( loopStep != NULL || loopSet != NULL || loopExpression != NULL) {
         SeqNode_addNumLoop ( _nodeDataPtr, _loop_node_path, 
            loopStart, loopStep, loopSet, loopEnd, loopExpression );
      }
   }
   free( loopStart );
   free( loopStep );
   free( loopSet );
   free( loopEnd );
   free( loopExpression );
   
}

void parseSubmits (xmlXPathObjectPtr _result, SeqNodeDataPtr _nodeDataPtr) {
   xmlNodeSetPtr nodeset;
   xmlNodePtr nodePtr;
   const xmlChar *propertyName;
   xmlAttrPtr propertiesPtr;
   int i=0, isSubmit=1;
   char* tmpstring;
   SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseSubmits() called\n" );
   if (_result) {
      nodeset = _result->nodesetval;
      for (i=0; i < nodeset->nodeNr; i++) {
         nodePtr = nodeset->nodeTab[i];
         propertiesPtr = nodePtr->properties;
         isSubmit = 0;
         while (propertiesPtr != NULL) {
            propertyName = propertiesPtr->name;
            SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseSubmits() submits:%s\n", propertiesPtr->children->content);
            SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseSubmits() property:%s\n",propertyName);
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
               SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseSubmits() got user submit node\n" );
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
   const xmlChar *nodeName = NULL;

   int i=0;
   SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseAbortActions() called\n" );
   if (_result) {
      nodeset = _result->nodesetval;
      SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseAbortActions() nodeset->nodeNr=%d\n", nodeset->nodeNr);
      for (i=0; i < nodeset->nodeNr; i++) {
         nodePtr = nodeset->nodeTab[i];
         nodeName = nodePtr->name;
         SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseAbortActions() nodePtr->name=%s\n", nodePtr->name);
         SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseAbortActions() value=%s\n", nodePtr->children->content );
         if ( strcmp( nodeName, "name" ) == 0 ) {
            SeqNode_addAbortAction( _nodeDataPtr, nodePtr->children->content );
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
   int i=0;
   SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseNodeSpecifics() called node_type=%s\n", SeqNode_getTypeString( _nodeType ) );
   nodeset = _result->nodesetval;
   SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseNodeSpecifics() nodeset cardinal=%d\n", nodeset->nodeNr );
   for (i=0; i < nodeset->nodeNr; i++) {
      nodePtr = nodeset->nodeTab[i];
      nodeName = nodePtr->name;

      /* name attribute is not node specific */
      if ( nodePtr->children != NULL && strcmp((char*)nodeName,"name") != 0 ) {
         SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseNodeSpecifics() %s=%s\n", nodeName, nodePtr->children->content );
         SeqNode_addSpecificData ( _nodeDataPtr, nodeName, nodePtr->children->content );
      }
   }
   if( _nodeType == Loop ) {
      SeqNode_addSpecificData( _nodeDataPtr, "TYPE", "Default" );
   }
}

/* To parse the siblings to generate a list. This list will be used to monitor container ends.  */
void parseNodeSiblings (xmlXPathObjectPtr _result, SeqNodeDataPtr _nodeDataPtr) {

   xmlNodeSetPtr nodeset;
   xmlNodePtr nodePtr;
   const xmlChar *propertyName = NULL;
   xmlAttrPtr propertiesPtr = NULL;

   int i=0;
   SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseNodeSiblings() called\n" );
   if (_result) {
      nodeset = _result->nodesetval;
      for (i=0; i < nodeset->nodeNr; i++) {
         nodePtr = nodeset->nodeTab[i];
         propertiesPtr = nodePtr->properties;
         while (propertiesPtr != NULL) {
            propertyName = propertiesPtr->name;

            if ( strcmp( (char*) propertyName, "name" ) == 0 ) {
               SeqNode_addSibling( _nodeDataPtr, propertiesPtr->children->content );
               SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseNodeSiblings() sibling:%s\n", propertiesPtr->children->content);
            }
            propertiesPtr = propertiesPtr->next;
         }
      }
   }
}

/* this function returns the root node of an experiment as
 * defined by the entry module flow.mxl file */
void getRootNode( SeqNodeDataPtr _nodeDataPtr, const char *_seq_exp_home ) {
   char *xmlFile = NULL;
   char query[256];

   xmlDocPtr doc = NULL;
   xmlNodeSetPtr nodeset = NULL;
   xmlXPathObjectPtr result = NULL;
   xmlXPathContextPtr context = NULL;
   const xmlChar *nodeName = NULL;
   xmlNodePtr nodePtr;

   xmlFile = malloc( strlen( _seq_exp_home ) + strlen( "/EntryModule/flow.xml" ) + 1 );

   /* build the xmlfile path */
   sprintf( xmlFile, "%s/EntryModule/flow.xml", _seq_exp_home);

   /* parse the xml file */
   if ((doc = XmlUtils_getdoc(xmlFile)) == NULL) {
      raiseError("Unable to parse file, or file not found: %s\n", xmlFile);
   }

   /* the context is used to walk trough the nodes */
   context = xmlXPathNewContext(doc);

   /* get the first MODULE name attribute */
   sprintf ( query, "(/MODULE/@name)");
   if( (result = XmlUtils_getnodeset (query, context)) == NULL ) {
      raiseError("MODULE not found in XML master file! (getRootNode)\n");
   }
   nodeset = result->nodesetval;
   nodePtr = nodeset->nodeTab[0];
   nodeName = nodePtr->name;
   SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.getRootNode() nodeName=%s\n", nodeName );
   if ( nodePtr->children != NULL ) {
      SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.getRootNode() %s=%s\n", nodeName, nodePtr->children->content );
      SeqNode_setName( _nodeDataPtr, SeqUtil_fixPath( nodePtr->children->content ) );

   }
   xmlXPathFreeObject (result);
   xmlXPathFreeContext (context);
   xmlFreeDoc(doc);
   free( xmlFile );
}


/* this function returns the value of the switch statement */

char * switchReturn( SeqNodeDataPtr _nodeDataPtr, const char* switchType ) {

    char returnValue[SEQ_MAXFIELD]; 
    char year[5], month[3], day[3];

    memset(returnValue,'\0', sizeof(returnValue));
    if (strcmp(switchType, "datestamp_hour") == 0) {
	strncpy(returnValue, _nodeDataPtr->datestamp+8,2);   
        SeqUtil_TRACE(TL_FULL_TRACE, "switchReturn datestamp parser on datestamp = %s\n",  _nodeDataPtr->datestamp );
    }
    if (strcmp(switchType, "day_of_week") == 0) {
        strncpy(year, _nodeDataPtr->datestamp,4); 
        year[4]='\0';
        strncpy(month, _nodeDataPtr->datestamp+4,2); 
        month[2]='\0';
        strncpy(day, _nodeDataPtr->datestamp+6,2); 
        day[2]='\0';
        SeqUtil_TRACE(TL_FULL_TRACE, "switchReturn datestamp parser on day of the week for date: %s%s%s \n",year,month,day);
	sprintf(returnValue,"%d",SeqDatesUtil_dow(atoi(year), atoi(month), atoi(day)));   
    }
    SeqUtil_TRACE(TL_FULL_TRACE, "switchReturn returnValue = %s\n", returnValue
          ); return strdup(returnValue); 
}

/*  parseForEachTarget
Parses the information from the ressources file to know the ForEach container target and adds it to the node 
input: 
_result = FOR_EACH xml tag and attributes
_nodeDataPtr = current node

output: 
_nodeDataPtr will be modified to contain the ForEach container's target

*/
void parseForEachTarget(xmlXPathObjectPtr _result, SeqNodeDataPtr _nodeDataPtr) {

   xmlNodeSetPtr nodeset = NULL;
   xmlNodePtr nodePtr = NULL;
   const xmlChar *nodeName = NULL;
   char * t_index = NULL, * t_exp = NULL, *t_hour = NULL, *t_node = NULL; 
   int i=0;
   SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseForEachTarget() called\n" );
   if (_result) {
      nodeset = _result->nodesetval;
      SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseForEachTarget() nodeset->nodeNr=%d\n", nodeset->nodeNr);
      for (i=0; i < nodeset->nodeNr; i++) {
         nodePtr = nodeset->nodeTab[i];
         nodeName = nodePtr->name;
         SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseForEachTarget() nodePtr->name=%s\n", nodePtr->name);
         SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.parseForEachTarget() value=%s\n", nodePtr->children->content );
         if ( strcmp( nodeName, "node" ) == 0 ) {
           t_node = strdup(nodePtr->children->content); 
         }
         if ( strcmp( nodeName, "index" ) == 0 ) {
           t_index = strdup(nodePtr->children->content); 
         }
         if ( strcmp( nodeName, "exp" ) == 0 ) {
           t_exp = strdup(nodePtr->children->content); 
         }
         if ( strcmp( nodeName, "hour" ) == 0 ) {
           t_hour = strdup(nodePtr->children->content); 
         }
      }
   if (t_exp == NULL) t_exp = strdup(_nodeDataPtr->expHome);
   if (t_hour == NULL) t_hour = strdup("0"); 
   if ((t_node == NULL) || (t_index == NULL)) {
      raiseError("Node's xml resource file does not contain mandatory node and/or index attributes.\n");
   }  
   SeqNode_setForEachTarget(_nodeDataPtr, t_node, t_index, t_exp, t_hour);
   free(t_node);
   free(t_exp);
   free(t_index);
   free(t_hour);
   }
}

/*********************************************************************************
 * Function getFlowInfo():
 * Parses ${SEQ_EXP_HOME}/EntryModule/flow.xml does one of two things:
 * Adds information to the SeqNodeData data structure about the the flow, such
 * as children, parent, siblings, submits and dependencies.
*********************************************************************************/
void getFlowInfo ( SeqNodeDataPtr _nodeDataPtr, const char *_seq_exp_home,
                     const char *_nodePath,const char *switch_args, unsigned int filters)
{
   if (_nodePath == NULL || strcmp(_nodePath, "") == 0)
      raiseError("Calling getFlowInfo() with an empty path'\n");
   FlowVisitorPtr flow_visitor = Flow_newVisitor(_nodePath,_seq_exp_home,switch_args);

   if ( Flow_parsePath(flow_visitor,_nodeDataPtr, _nodePath) == FLOW_FAILURE )
      raiseError("Unable to get to the specified node %s\n",_nodePath);

   Flow_setPathData(flow_visitor, _nodeDataPtr);

   Flow_parseSpecifics(flow_visitor,_nodeDataPtr);

   if(filters & (NI_SHOW_ALL | NI_SHOW_DEP) )
      Flow_parseDependencies(flow_visitor, _nodeDataPtr);

   if(filters & NI_SHOW_ALL ){
      Flow_parseSubmits(flow_visitor, _nodeDataPtr);
      Flow_parseSiblings(flow_visitor, _nodeDataPtr);
   }

   Flow_deleteVisitor(flow_visitor);
}
/*********************************************************************************
 * Assertains whether the given node exists in the current scope of the
 * experiment by attempting to walk the path through the flow.xml files using a
 * flow visitor.
*********************************************************************************/
int doesNodeExist (const char *_nodePath, const char *_seq_exp_home , const char * _datestamp) {

   SeqUtil_TRACE(TL_FULL_TRACE, "doesNodeExist() begin\n");
   FlowVisitorPtr flow_visitor = Flow_newVisitor(_nodePath, _seq_exp_home, NULL);
   int nodeExists = FLOW_FALSE;
   char * newNode = (char*) SeqUtil_fixPath( _nodePath );
   SeqNodeDataPtr tempNode = (SeqNodeDataPtr) SeqNode_createNode ( newNode );
   SeqNode_setDatestamp( tempNode, (const char *) tictac_getDate(_seq_exp_home,"",_datestamp) );

   if ( Flow_walkPath(flow_visitor,tempNode,_nodePath) == FLOW_SUCCESS )
      nodeExists = FLOW_TRUE;
   else
      nodeExists = FLOW_FALSE;

   Flow_deleteVisitor(flow_visitor);
   free(newNode);
   SeqNode_freeNode(tempNode);
   SeqUtil_TRACE(TL_FULL_TRACE, "doesNodeExist() end\n");
   return nodeExists;
}

SeqNodeDataPtr nodeinfo ( const char* node, unsigned int filters,
                              SeqNameValuesPtr _loops, const char* _exp_home,
                              char *extraArgs, char* datestamp, const char * switch_args )
{
   char *newNode = NULL,  *tmpfilters = NULL;
   char workdir[SEQ_MAXFIELD];
   SeqNodeDataPtr  nodeDataPtr = NULL;

   if( _exp_home == NULL ) {
      raiseError("SEQ_EXP_HOME not set! (nodeinfo) \n");
   }

   newNode = (const char*) SeqUtil_fixPath( node );
   SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.nodeinfo() trying to create node %s, exp %s\n", newNode, _exp_home );
   nodeDataPtr = (SeqNodeDataPtr) SeqNode_createNode ( newNode );
   SeqNode_setSeqExpHome(nodeDataPtr,_exp_home); 
   memset(workdir,'\0',sizeof workdir);
   sprintf(workdir,"%s/sequencing/status", _exp_home);
   SeqNode_setWorkdir( nodeDataPtr, workdir );

   SeqUtil_TRACE(TL_FULL_TRACE, "nodeinfo.nodefinfo() argument datestamp %s. If (null), will run tictac to find value.\n", datestamp );
   const char * newDatestamp = tictac_getDate(_exp_home,"",datestamp);
   SeqNode_setDatestamp( nodeDataPtr, newDatestamp );
   /*pass the content of the command-line (or interface) extra soumet args to the node*/
   SeqNode_setSoumetArgs(nodeDataPtr,extraArgs);

   if (filters & NI_SHOW_ROOT_ONLY ) {
      getRootNode ( nodeDataPtr, _exp_home );
   } else {
      /* add loop arg list to node */
      SeqNode_setLoopArgs( nodeDataPtr,_loops);
      getFlowInfo ( nodeDataPtr, _exp_home, newNode,switch_args,filters);
      getNodeResources(nodeDataPtr,_exp_home, newNode);

   }

   free((char*)newNode);
   free((char*) newDatestamp);
   free( tmpfilters );
   return nodeDataPtr;
}
