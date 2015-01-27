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
#include "tictac.h"
#include "SeqUtil.h"
#include "XmlUtils.h"
#include "SeqLoopsUtil.h"


int SHOW_ALL = 0;
int SHOW_CFGPATH = 0;
int SHOW_TASKPATH = 0;
int SHOW_RESSOURCE = 0;
int SHOW_ROOT_ONLY = 0;
int SHOW_DEP = 0;
int SHOW_RESPATH = 0;

/* root node of xml resource file */
const char* NODE_RES_XML_ROOT = "/NODE_RESOURCES";

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
   } else if ( strcmp( _node_name, "SWITCH" ) == 0 ) {
      nodeType = Switch;
   } else if ( strcmp( _node_name, "CASE" ) == 0 ) {
      nodeType = Case;
   } else if ( strcmp( _node_name, "CASE_ITEM" ) == 0 ) {
      nodeType = CaseItem;
   } else {
      raiseError("ERROR: nodeinfo.getNodeType()  unprocessed xml node name:%s\n", _node_name);
   }
   SeqUtil_TRACE( "nodeinfo.getNodeType() type=%d\n", nodeType );
   return nodeType;
}

void parseBatchResources (xmlXPathObjectPtr _result, SeqNodeDataPtr _nodeDataPtr) {
   xmlNodeSetPtr nodeset = NULL;
   xmlNodePtr nodePtr = NULL;
   const xmlChar *nodeName = NULL;
   char *tmpString = NULL; 

   int i=0;
   SeqUtil_TRACE( "nodeinfo.parseBatchResources() called\n" );
   if (_result) {
      nodeset = _result->nodesetval;
      SeqUtil_TRACE( "nodeinfo.parseBatchResources() nodeset->nodeNr=%d\n", nodeset->nodeNr);
      for (i=0; i < nodeset->nodeNr; i++) {
         nodePtr = nodeset->nodeTab[i];
         nodeName = nodePtr->name;
         SeqUtil_TRACE( "nodeinfo.parseBatchResources() nodePtr->name=%s\n", nodePtr->name);
         SeqUtil_TRACE( "nodeinfo.parseBatchResources() value=%s\n", nodePtr->children->content );
	 if ( strcmp( nodeName, "cpu" ) == 0 ) {
            SeqNode_setCpu( _nodeDataPtr, nodePtr->children->content );
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
         } else if ( strcmp( nodeName, "soumet_args" ) == 0 ) {
              /* add soumet args in the following order: 1) resource file 2) args sent by command line, who will override 1*/
             SeqUtil_stringAppend( &tmpString, nodePtr->children->content);
             SeqUtil_stringAppend( &tmpString, " ");
             SeqUtil_stringAppend( &tmpString, _nodeDataPtr->soumetArgs );
             SeqNode_setSoumetArgs( _nodeDataPtr, tmpString);
             free(tmpString);
         } else if ( strcmp( nodeName, "wallclock" ) == 0 ) {
             _nodeDataPtr->wallclock = atoi( nodePtr->children->content );
         } else if ( strcmp( nodeName, "catchup" ) == 0 ) {
             _nodeDataPtr->catchup = atoi( nodePtr->children->content );
         } else {
             raiseError("nodeinfo.parseBatchResources() ERROR: Unprocessed attribute=%s\n", nodeName);
         }
      }
   }

}

void parseDepends (xmlXPathObjectPtr _result, SeqNodeDataPtr _nodeDataPtr, int isIntraDep ) {
   xmlNodeSetPtr nodeset;
   xmlNodePtr nodePtr;
   const char *nodeName = NULL;
   char *depType = NULL, *depExp=NULL, *depName = NULL,  *depPath = NULL, *depProt=NULL, *depHour = NULL, *depStatus = NULL, *depIndex = NULL, *depLocalIndex = NULL, *depValidHour=NULL, *depValidDOW=NULL;
   char* fullDepIndex = NULL, *fullDepLocalIndex=NULL, *tmpstrtok=NULL, *parsedDepName=NULL, *tmpLoopName=NULL; 
   SeqNameValuesPtr depArgs = NULL, localArgs = NULL, tmpIterator = NULL;
   SeqLoopsPtr loopsPtr = NULL;
   int i=0;
   
   char *tmpsubstr = NULL, *sepLocal = NULL, *sepIndex = NULL, *tmpLocalIndexValue = NULL;
   char *resourceFile, *_seq_exp_home = _nodeDataPtr->expHome;
   char *tmpSavePtr1 = NULL, *tmpSavePtr2 = NULL, *tmpTokenLine, *indexToken = NULL;
   FILE *fp;
   int find_index_token = 0;
   char temp[512], tokenLine[512], tmpIndexToken[48];
   int alreadySet = 0, tmpCompare = 0;
   
   if (_result) {
      nodeset = _result->nodesetval;

      if( _nodeDataPtr->type == Task || _nodeDataPtr->type == NpassTask ) {
          resourceFile = malloc ( strlen ( _seq_exp_home ) + strlen("/resources") + strlen (_nodeDataPtr->name) + strlen(".xml") + 1 );
          sprintf( resourceFile, "%s/resources%s.xml", _seq_exp_home, _nodeDataPtr->name );
      } else {
          resourceFile = malloc ( strlen ( _seq_exp_home ) + strlen("/resources") + strlen (_nodeDataPtr->name) + strlen("/container.xml") + 1 );
          sprintf( resourceFile, "%s/resources%s/container.xml", _seq_exp_home, _nodeDataPtr->name );
      }
      
      SeqUtil_TRACE( "nodeinfo.parseDepends() nodeset->nodeNr=%d\n", nodeset->nodeNr);
      for (i=0; i < nodeset->nodeNr; i++) {
	 /* reset variables to null after being freed at the end of the loop for reuse*/
	 fullDepIndex=NULL;
	 fullDepLocalIndex=NULL;
	 nodePtr = nodeset->nodeTab[i];
	 nodeName = nodePtr->name;
	 find_index_token = 0;
	 indexToken = NULL;
	 tmpCompare = 0;
	 alreadySet = 0;
	 SeqUtil_TRACE( "nodeinfo.parseDepends()   *** depends_item=%s ***\n", nodeName);
	 depType = (char *) xmlGetProp( nodePtr, "type" );
	 SeqUtil_TRACE( "nodeinfo.parseDepends() Parsing Dependency Type:%s\n", depType);
	 if ( depType == NULL ) depType=strdup("node");
	 
	 if ( strcmp( depType, "node" ) == 0 ) {
	    depExp = (char *) xmlGetProp( nodePtr, "exp" );
	    depName = (char *) xmlGetProp( nodePtr, "dep_name" );
	    parsedDepName=SeqUtil_relativePathEvaluation(depName,_nodeDataPtr);
	    
	    depProt  = (char * ) xmlGetProp( nodePtr, "protocol" ); 
	    /* default interuser protocol if not defined */
	    if (depProt == NULL) depProt=strdup("polling"); 
	    depIndex = (char *) xmlGetProp( nodePtr, "index" );
	    depLocalIndex = (char *) xmlGetProp( nodePtr, "local_index" );
	    /* look for keywords in index fields */
	    
	    /* add loop context in case of intra dep */
	    if (isIntraDep) {
	       loopsPtr =  _nodeDataPtr->loops;
	       while( loopsPtr != NULL ) {
		  if( strstr(  _nodeDataPtr->pathToModule ,loopsPtr->loop_name ) != NULL ) {
		     /* add loop arg to full dep index */
		     tmpLoopName=(char*) SeqUtil_getPathLeaf( (const char*) loopsPtr->loop_name );
		     SeqUtil_TRACE( "Nodeinfo_parseDepends() adding loop argument to dependency for name = %s\n", tmpLoopName );
		     if (SeqNameValues_getValue(_nodeDataPtr->loop_args, tmpLoopName) != NULL) {
			SeqNameValues_insertItem( &depArgs, tmpLoopName, SeqNameValues_getValue(_nodeDataPtr->loop_args, tmpLoopName));
			SeqNameValues_insertItem( &localArgs, tmpLoopName, SeqNameValues_getValue(_nodeDataPtr->loop_args, tmpLoopName));
		     }
		     free(tmpLoopName);
		  }
		  loopsPtr  = loopsPtr->nextPtr;
	       }
	    }
	    
	    /*file stream method because the $ token is ignored with available function */
	    if( access( resourceFile, F_OK ) != -1 ) {
	       /* parse node resource file to find associative index token, first find the line where the token is ...*/
	       SeqUtil_TRACE("Nodeinfo_parseDepends() node resource file : %s\n", resourceFile);
	       if((fp = fopen(resourceFile, "r")) == NULL) {
		  SeqUtil_TRACE("Nodeinfo_parseDepends() cannot open resource xml file %s for index token parsing\n", resourceFile );
	       } else {
		  memset(tokenLine,'\0',sizeof tokenLine);
		  while(fgets(temp, 512, fp) != NULL) {
		     if (find_index_token == 0) {
			if((strstr(temp, "$((")) != NULL) {
			SeqUtil_TRACE("Nodeinfo_parseDepends() found associative index token, checking dependency\n");
			strcpy(tokenLine, temp);
			find_index_token = 1;
			}
		     }  
		  }
		  if(find_index_token == 0) {
		     SeqUtil_TRACE("Nodeinfo_parseDepends() did not find any associative index token\n");
		  }
		  if(fp) {
		     fclose(fp);
		  }
		  
		  /* ... second retrieve the token within the line found */
		  if (tokenLine != NULL && strstr(tokenLine, "$((") != NULL) {
		     tmpTokenLine = (char *) malloc( strlen(tokenLine) + 1 );
		     sprintf( tmpTokenLine, tokenLine);
		     tmpsubstr = strtok_r(tmpTokenLine,"$((",&tmpSavePtr1);
		     //SeqUtil_TRACE("tmpsubstr : %s\n", tmpsubstr);
		     while (tmpsubstr != NULL) {
			indexToken = strtok_r(tmpsubstr,")",&tmpSavePtr2);
			tmpsubstr = strtok_r(NULL, "$((", &tmpSavePtr1);
		     }
		     if (indexToken != NULL) {
			strcpy(tmpIndexToken, indexToken);
			sprintf(indexToken, "((%s))", tmpIndexToken);
			SeqUtil_TRACE("Nodeinfo_parseDepends() found associative index token: %s\n", indexToken);
		     }
		  }
	       }
	    }
	    
	    if( depLocalIndex != NULL ) {
	       /*validate local dependency args and create a namevalue list*/
	       if( SeqLoops_parseArgs( &localArgs, depLocalIndex ) != -1 ) {
		  tmpIterator = localArgs; 
		  while (tmpIterator != NULL) {
		     /*checks for current index keyword*/
		     if (strcmp(tmpIterator->value,"CURRENT_INDEX")==0) {
			if (SeqNameValues_getValue(_nodeDataPtr->loop_args, tmpIterator->name) != NULL) {
			   SeqNameValues_setValue( &localArgs, tmpIterator->name, SeqNameValues_getValue(_nodeDataPtr->loop_args, tmpIterator->name));
			   /* raiseError( "parseDepends(): Error -- CURRENT_INDEX keyword used in a non-loop context, or does not match current loop arguments. \n" ); */
			}   
		     } else if (indexToken != NULL && strcmp(tmpIterator->value, indexToken)==0) /*checks if token association is satisfied */ {
			if (SeqNameValues_getValue(_nodeDataPtr->loop_args, tmpIterator->name) != NULL) {
			   SeqNameValues_setValue( &localArgs, tmpIterator->name, SeqNameValues_getValue(_nodeDataPtr->loop_args, tmpIterator->name));
			   tmpLocalIndexValue = SeqNameValues_getValue(_nodeDataPtr->loop_args, tmpIterator->name);
			}
		     }
		     tmpIterator=tmpIterator->nextPtr;
		  }
	       } else {
		  raiseError( "parseDepends(): local dependency index format error\n" );
	       }
	    }
	    
	    if( depIndex != NULL ) {
	       /*validate dependency args and create a namevalue list*/
	       if( SeqLoops_parseArgs( &depArgs, depIndex ) != -1 ) {
		  tmpIterator = depArgs; 
		  while (tmpIterator != NULL) {
		     /*checks for current index keyword*/
		     if (strcmp(tmpIterator->value,"CURRENT_INDEX")==0) {
			if (SeqNameValues_getValue(_nodeDataPtr->loop_args, tmpIterator->name) != NULL) {
			   SeqNameValues_setValue( &depArgs, tmpIterator->name, SeqNameValues_getValue(_nodeDataPtr->loop_args, tmpIterator->name));
			   /* raiseError( "parseDepends(): Error -- CURRENT_INDEX keyword used in a non-loop context, or does not match current loop arguments. \n" ); */
			}
		     } else if(indexToken != NULL && strcmp(tmpIterator->value, indexToken)==0) /*checks if token association is satisfied */ {
			if (tmpIterator->name != NULL && tmpLocalIndexValue != NULL) {
			   SeqNameValues_setValue( &depArgs, tmpIterator->name, tmpLocalIndexValue);
			}
		     }
		     tmpIterator=tmpIterator->nextPtr;
		  }
	       } else {
		   raiseError( "parseDepends(): dependency index format error\n" );
		}
	     }
	   
	    if ( depArgs != NULL ) fullDepIndex=strdup((char *)SeqLoops_getLoopArgs(depArgs));
	    if( localArgs != NULL ) fullDepLocalIndex=strdup((char *)SeqLoops_getLoopArgs(localArgs));

            depPath = (char *) xmlGetProp( nodePtr, "path" );
            depHour = (char *) xmlGetProp( nodePtr, "hour" );
            depValidHour = (char *) xmlGetProp( nodePtr, "valid_hour" );
            depValidDOW = (char *) xmlGetProp( nodePtr, "valid_dow" );
            depStatus = (char *) xmlGetProp( nodePtr, "status" );
            SeqUtil_TRACE( "nodeinfo.parseDepends() dep depName: %s\n", depName );
            SeqUtil_TRACE( "nodeinfo.parseDepends() dep parsedDepName: %s\n", parsedDepName );
            SeqUtil_TRACE( "nodeinfo.parseDepends() dep depIndex: %s\n", depIndex );
            SeqUtil_TRACE( "nodeinfo.parseDepends() fullDepIndex: %s \n", fullDepIndex);
            SeqUtil_TRACE( "nodeinfo.parseDepends() fullDepLocalIndex: %s\n", fullDepLocalIndex );
            SeqUtil_TRACE( "nodeinfo.parseDepends() dep depPath: %s\n", depPath );
            SeqUtil_TRACE( "nodeinfo.parseDepends() dep depExp: %s\n", depExp );
            SeqUtil_TRACE( "nodeinfo.parseDepends() dep depHour: %s\n", depHour );
            SeqUtil_TRACE( "nodeinfo.parseDepends() dep depValidHour: %s\n", depValidHour );
            SeqUtil_TRACE( "nodeinfo.parseDepends() dep depValidDOW: %s\n", depValidDOW );
            SeqUtil_TRACE( "nodeinfo.parseDepends() depStatus: %s\n", depStatus );
            SeqUtil_TRACE( "nodeinfo.parseDepends() depProt: %s\n", depProt ); /* added by Rochdi */
            SeqNode_addNodeDependency ( _nodeDataPtr, NodeDependancy, parsedDepName, depPath, depExp, depStatus, fullDepIndex, fullDepLocalIndex, depHour, depProt, depValidHour, depValidDOW );
	    SeqUtil_TRACE( "nodeinfo.parseDepends() done\n" );
            free( depName );
            free( depIndex );
            free( depPath );
            free( depStatus );
            free( depProt );
            free( depExp );
            free( depHour );
            free( depValidHour );
            free( depValidDOW );
            free( parsedDepName );
	    free( fullDepIndex );
	    free( fullDepLocalIndex );
	    free( tmpstrtok );
	    SeqNameValues_deleteWholeList( &localArgs );
	    SeqNameValues_deleteWholeList( &depArgs );
         } else {
            SeqUtil_TRACE( "nodeinfo.parseDepends() no dependency found.\n" );
         }
      }
      free(resourceFile);
   }
}

void parseLoopAttributes (xmlXPathObjectPtr _result, const char* _loop_node_path, SeqNodeDataPtr _nodeDataPtr) {
   xmlNodeSetPtr nodeset;
   xmlNodePtr nodePtr;
   const xmlChar *nodeName = NULL;
   xmlChar *loopStart = NULL, *loopStep = strdup("1"), *loopEnd = NULL, *loopSet = strdup("1");
   int i=0;
   if (_result) {
      nodeset = _result->nodesetval;
      SeqUtil_TRACE( "nodeinfo.parseLoopAttributes()   *** loop info***\n");
      for (i=0; i < nodeset->nodeNr; i++) {
         nodePtr = nodeset->nodeTab[i];
         nodeName = nodePtr->name;
         SeqUtil_TRACE( "nodeinfo.parseLoopAttributes() nodeName=%s, value:%s\n", nodeName, nodePtr->children->content);
         if ( nodePtr->children != NULL ) {
            if( strcmp( nodeName, "start" ) == 0 ) {
               loopStart = strdup( nodePtr->children->content );
            } else if( strcmp( nodeName, "step" ) == 0 ) {
	       free( loopStep );
               loopStep = strdup( nodePtr->children->content );
            } else if( strcmp( nodeName, "set" ) == 0 ) {
	       free( loopSet );
               loopSet = strdup( nodePtr->children->content );
            } else if( strcmp( nodeName, "end" ) == 0 ) {
               loopEnd = strdup( nodePtr->children->content );
            }
         }
      }

      if( loopStep != NULL || loopSet != NULL ) {
         SeqNode_addNumLoop ( _nodeDataPtr, _loop_node_path, 
            loopStart, loopStep, loopSet, loopEnd );
      }
   }

   free( loopStart );
   free( loopStep );
   free( loopSet );
   free( loopEnd );
   
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

/* set the node's worker path */
void parseWorkerPath (char * pathToNode, const char * _seq_exp_home, SeqNodeDataPtr _nodeDataPtr ) {
   xmlDocPtr doc = NULL;
   xmlXPathObjectPtr result = NULL;
   xmlXPathContextPtr context = NULL;
   xmlNodeSetPtr nodeset = NULL;
   xmlNodePtr nodePtr = NULL;
   const xmlChar *nodeName = NULL;
   char query[256], *xmlFile=NULL ;
   int foundPath=0, i=0;

   memset(query,'\0',sizeof query);

   xmlFile = malloc( strlen(_seq_exp_home) + strlen("/resources/") + strlen(pathToNode) + strlen("/container.xml") + 1);

   /* build the xmlfile path */
   sprintf( xmlFile, "%s/resources/%s/container.xml", _seq_exp_home, pathToNode);

   /* parse the xml file */
   doc = XmlUtils_getdoc(xmlFile);

   if (doc==NULL) raiseError("File %s does not exist, but should contain necessary WORKER tag with path attribute for a work_unit container \n", xmlFile); 

   /* the context is used to walk trough the nodes */
   context = xmlXPathNewContext(doc);

  /* get the batch system resources */
   sprintf ( query, "(%s/WORKER/@*)", NODE_RES_XML_ROOT );
   SeqUtil_TRACE ( "nodeinfo.parseWorkerPath query: %s\n", query );
   if( (result = XmlUtils_getnodeset (query, context)) != NULL ) {
         nodeset = result->nodesetval;
	 for (i=0; i < nodeset->nodeNr; i++) {
            nodePtr = nodeset->nodeTab[i];
            nodeName = nodePtr->name;
            SeqUtil_TRACE( "nodeinfo.parseWorkerPath() nodePtr->name=%s\n", nodePtr->name);
            SeqUtil_TRACE( "nodeinfo.parseWorkerPath() value=%s\n", nodePtr->children->content );
   	    if ( strcmp( nodeName, "path" ) == 0 ) {
               SeqNode_setWorkerPath( _nodeDataPtr, nodePtr->children->content );
	       foundPath=1;
            }
      }
   }

   if (!foundPath) raiseError("File %s does not contain necessary WORKER tag with path attribute for a work_unit container \n", xmlFile); 

   xmlXPathFreeObject (result);
   free(xmlFile);
   xmlXPathFreeContext(context);
   xmlFreeDoc(doc);
   xmlCleanupParser();


}

void parseAbortActions (xmlXPathObjectPtr _result, SeqNodeDataPtr _nodeDataPtr) {
   xmlNodeSetPtr nodeset = NULL;
   xmlNodePtr nodePtr = NULL;
   const xmlChar *nodeName = NULL;

   int i=0;
   SeqUtil_TRACE( "nodeinfo.parseAbortActions() called\n" );
   if (_result) {
      nodeset = _result->nodesetval;
      SeqUtil_TRACE( "nodeinfo.parseAbortActions() nodeset->nodeNr=%d\n", nodeset->nodeNr);
      for (i=0; i < nodeset->nodeNr; i++) {
         nodePtr = nodeset->nodeTab[i];
         nodeName = nodePtr->name;
         SeqUtil_TRACE( "nodeinfo.parseAbortActions() nodePtr->name=%s\n", nodePtr->name);
         SeqUtil_TRACE( "nodeinfo.parseAbortActions() value=%s\n", nodePtr->children->content );
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
   SeqUtil_TRACE( "nodeinfo.getRootNode() nodeName=%s\n", nodeName );
   if ( nodePtr->children != NULL ) {
      SeqUtil_TRACE( "nodeinfo.getRootNode() %s=%s\n", nodeName, nodePtr->children->content );
      SeqNode_setName( _nodeDataPtr, SeqUtil_fixPath( nodePtr->children->content ) );

   }
   xmlXPathFreeObject (result);
   xmlXPathFreeContext (context);
   xmlFreeDoc(doc);
   free( xmlFile );
}

/* This function is called from getFlowInfo for a node that is part of a loop container(s); 
 * It retrieves the loop attributes of a parent loop container from the parent's xml resource file.
 * Now that the loop attributes are stored in each loop xml file, any child nodes will have
 * to make a call to parse the respective xml file of each parent loop.
 */
void getNodeLoopContainersAttr (  SeqNodeDataPtr _nodeDataPtr, const char *_loop_node_path, const char *_seq_exp_home ) {
   char query[512];
   char *fixedNodePath = (char*) SeqUtil_fixPath( _loop_node_path );
   int extraSpace = 0;
   char *xmlFile = NULL;
   
   FILE *pxml = NULL;
   int xmlSize = 0;

   xmlDocPtr doc = NULL;
   xmlXPathObjectPtr result = NULL;
   xmlXPathContextPtr context = NULL;

   /* dynamic allocation space */
   extraSpace = strlen( "/resources//container.xml" );
   xmlFile = malloc( strlen( _seq_exp_home ) + strlen( fixedNodePath ) + extraSpace + 1 );
   sprintf( xmlFile, "%s/resources/%s/container.xml", _seq_exp_home, fixedNodePath );
   SeqUtil_TRACE ( "getNodeLoopContainersAttr xmlFile:%s\n", xmlFile );

   /* loop xml file must exist */
   if ( access(xmlFile, R_OK) != 0 ) {
      raiseError("Cannot access mandatory resource file: %s\n", xmlFile );
   }

   /* parse the xml file */
   if ((doc = XmlUtils_getdoc(xmlFile)) == NULL) {
      raiseError("Unable to parse file, or file not found: %s\n", xmlFile);
   }

   /* validate xmlFile (container.xml) parsing */
   if (doc == NULL) {
	  SeqUtil_TRACE ( "File %s/resources/%s/container.xml not parsed successfully, opening file\n", _seq_exp_home, fixedNodePath);
	  pxml = fopen (xmlFile, "a+");
	  if(!pxml)
	     raiseError("Permission to write in %s/resources/%s/container.xml denied\n", _seq_exp_home, fixedNodePath);
	
	  fseek (pxml , 0 , SEEK_END);
	  xmlSize = ftell (pxml);
	
	  if ( xmlSize==0 ) {
	    SeqUtil_TRACE ( "File %s/resources/%s/container.xml is empty, writing mandatory tags\n", _seq_exp_home, fixedNodePath);
	    if (  _nodeDataPtr->type == Loop) {
	      if(fprintf(pxml, "<NODE_RESOURCES>\n\t<LOOP start=\"0\" set=\"1\" end=\"1\" step=\"1\"/>\n</NODE_RESOURCES>"));
	      else
	        raiseError("Permission to write in %s/resources/%s/container.xml denied\n", _seq_exp_home, fixedNodePath);
	      }
	    else {
	       if (fprintf(pxml, "<NODE_RESOURCES/>"));
	       else
	         raiseError("Cannot write in %s/resources/%s/container.xml\n", _seq_exp_home, fixedNodePath);
	    }
	  }
	else {
	  raiseError("File %s/resources/%s/container.xml not respecting xml syntax\n", _seq_exp_home, fixedNodePath);
	}
	fclose (pxml);
	doc = XmlUtils_getdoc(xmlFile);
   }

   /* the context is used to walk trough the nodes */
   context = xmlXPathNewContext(doc);

   /* validate NODE_RESOURCES node */
   sprintf ( query, "(%s)", NODE_RES_XML_ROOT );
   SeqUtil_TRACE ( "getNodeLoopContainersAttr query: %s\n", query );
   if ( (result = XmlUtils_getnodeset (query, context)) == NULL ) {
      xmlXPathFreeObject (result);
      raiseError("NODE_RESOURCES element not found in XML master file! (getNodeLoopContainersAttr)\n");
   }
   xmlXPathFreeObject (result);
 
   /* build and run loop query */
   sprintf ( query, "(%s/LOOP/@*)", NODE_RES_XML_ROOT );
   SeqUtil_TRACE ( "getNodeLoopContainersAttr query: %s\n", query );
   if( (result = XmlUtils_getnodeset (query, context)) != NULL ) {
      parseLoopAttributes( result, fixedNodePath, _nodeDataPtr );
   }
   xmlXPathFreeObject (result);
   xmlXPathFreeContext ( context );
   xmlFreeDoc( doc );
   free( xmlFile );
   free( fixedNodePath );
   free(pxml);
}

/* this function returns the value of the switch statement */

char * switchReturn( SeqNodeDataPtr _nodeDataPtr, const char* switchType ) {

    char returnValue[SEQ_MAXFIELD]; 
    char year[5], month[3], day[3];

    memset(returnValue,'\0', sizeof(returnValue));
    if (strcmp(switchType, "datestamp_hour") == 0) {
	strncpy(returnValue, _nodeDataPtr->datestamp+8,2);   
        SeqUtil_TRACE( "switchReturn datestamp parser on datestamp = %s\n",  _nodeDataPtr->datestamp );
    }
    if (strcmp(switchType, "day_of_week") == 0) {
        strncpy(year, _nodeDataPtr->datestamp,4); 
        year[4]='\0';
        strncpy(month, _nodeDataPtr->datestamp+4,2); 
        month[2]='\0';
        strncpy(day, _nodeDataPtr->datestamp+6,2); 
        day[2]='\0';
        SeqUtil_TRACE( "switchReturn datestamp parser on day of the week for date: %s%s%s \n",year,month,day);
	sprintf(returnValue,"%d",SeqDatesUtil_dow(atoi(year), atoi(month), atoi(day)));   
    }
    SeqUtil_TRACE( "switchReturn returnValue = %s\n", returnValue );
    return strdup(returnValue); 
}

/* this function reads the node xml resource file
 * to retrive info such as dependencies, batch resource, abort actions
 * and loop information for loop nodes. The xml resource, if it exists,
 * is located under $SEQ_EXP_HOME/resources/ It follows the experiment node tree.
 */

void getNodeResources ( SeqNodeDataPtr _nodeDataPtr, const char *_nodePath, const char *_seq_exp_home) {
   char *xmlFile = NULL, *defFile = NULL, *value=NULL;
   char query[256];
   char *fixedNodePath = (char*) SeqUtil_fixPath( _nodePath );
   int i,extraSpace = 0;
   SeqNodeDataPtr  workerNodeDataPtr=NULL;

   FILE *pxml = NULL;
   int xmlSize = 0;
   
   xmlDocPtr doc = NULL;
   xmlNodeSetPtr nodeset = NULL;
   xmlXPathObjectPtr result = NULL;
   xmlXPathContextPtr context = NULL;

   SeqUtil_TRACE( "getNodeResources _nodePath=%s type=%d\n", _nodePath, _nodeDataPtr->type );

   /* build the xmlfile path */
   if( _nodeDataPtr->type == Task || _nodeDataPtr->type == NpassTask )  {
      /* read task xml file */
      extraSpace = strlen( "/resources//.xml" );
      xmlFile = malloc ( strlen( _seq_exp_home) + strlen( _nodePath ) + extraSpace + 1);
      sprintf( xmlFile, "%s/resources/%s.xml", _seq_exp_home, _nodePath);
   } else {
      /* read container xml file */
      extraSpace = strlen( "/resources//container.xml" );
      xmlFile = malloc ( strlen( _seq_exp_home) + strlen( _nodePath ) + extraSpace + 1);
      sprintf( xmlFile, "%s/resources/%s/container.xml", _seq_exp_home, fixedNodePath );
      if (  _nodeDataPtr->type == Loop && access(xmlFile, R_OK) != 0 ) {
         /* loop xml file must exist */
         raiseError("Cannot access mandatory resource file: %s\n", xmlFile );
      }
   }

   defFile = malloc ( strlen ( _seq_exp_home ) + strlen("/resources/resources.def") + 1 );
   sprintf( defFile, "%s/resources/resources.def", _seq_exp_home );

   SeqUtil_TRACE ( "getNodeResources looking for xml file %s\n", xmlFile );

   /* verify xml exist, else get out of here */
   if (  access(xmlFile, R_OK) == 0 ) {
   
      /* parse the xml file */
      doc = XmlUtils_getdoc(xmlFile);
      
   /* validate xmlFile (container.xml) parsing */
   if (doc == NULL) {
	SeqUtil_TRACE ( "File %s/resources/%s/container.xml not parsed successfully, opening file\n", _seq_exp_home, fixedNodePath);
	pxml = fopen (xmlFile, "a+");
	if(!pxml)
	  raiseError("Permission to write in %s/resources/%s/container.xml denied\n", _seq_exp_home, fixedNodePath);
	
	fseek (pxml , 0 , SEEK_END);
	xmlSize = ftell (pxml);
	
	if ( xmlSize==0 ) {
	  SeqUtil_TRACE ( "File %s/resources/%s/container.xml is empty, writing mandatory tags\n", _seq_exp_home, fixedNodePath);
	  if (  _nodeDataPtr->type == Loop) {
	    if(fprintf(pxml, "<NODE_RESOURCES>\n\t<LOOP start=\"0\" set=\"1\" end=\"1\" step=\"1\"/>\n</NODE_RESOURCES>"));
	    else
	      raiseError("Permission to write in %s/resources/%s/container.xml denied\n", _seq_exp_home, fixedNodePath);
	  }
	  else {
	    if (fprintf(pxml, "<NODE_RESOURCES/>"));
	    else
	      raiseError("Cannot write in %s/resources/%s/container.xml\n", _seq_exp_home, fixedNodePath);
	  }
	}
	else {
	  raiseError("File %s/resources/%s/container.xml not respecting xml syntax\n", _seq_exp_home, fixedNodePath);
	}
	fclose (pxml);
	doc = XmlUtils_getdoc(xmlFile);
   }

      /* the context is used to walk trough the nodes */
      context = xmlXPathNewContext(doc);

      /* resolve environment variables found in XML file */
      XmlUtils_resolve(xmlFile,context,defFile);

      /* validate NODE_RESOURCES node */
      sprintf ( query, "(%s)", NODE_RES_XML_ROOT );
      SeqUtil_TRACE ( "getNodeResources query: %s\n", query );
      if ( (result = XmlUtils_getnodeset (query, context)) == NULL ) {
         xmlXPathFreeObject (result);
         raiseError("NODE_RESOURCES element not found in XML master file! (getNodeResources)\n");
      }
      xmlXPathFreeObject (result);
  
      /* get loop attributes if node is loop type */
      if (  _nodeDataPtr->type == Loop ) {
         sprintf ( query, "(%s/LOOP/@*)", NODE_RES_XML_ROOT );
         SeqUtil_TRACE ( "getNodeResources query: %s\n", query );
         if( (result = XmlUtils_getnodeset (query, context)) != NULL ) {
            parseNodeSpecifics( _nodeDataPtr->type, result, _nodeDataPtr );
         }
      }

      /* get the batch system resources */
      sprintf ( query, "(%s/BATCH/@*)", NODE_RES_XML_ROOT );
      SeqUtil_TRACE ( "getNodeResources query: %s\n", query );
      if( (result = XmlUtils_getnodeset (query, context)) != NULL ) {
         parseBatchResources( result, _nodeDataPtr ); 
      }
      xmlXPathFreeObject (result);

      /* get the dependencies */
      /* there could be more than one DEPENDS_ON tag */
      sprintf ( query, "(%s/child::DEPENDS_ON)", NODE_RES_XML_ROOT );
      SeqUtil_TRACE ( "getNodeResources query: %s\n", query );
      if( (result = XmlUtils_getnodeset (query, context)) != NULL ) {
         parseDepends( result, _nodeDataPtr, 0); 
      }
      xmlXPathFreeObject (result);

      /* get the abort actions */
      sprintf ( query, "(%s/ABORT_ACTION/@*)", NODE_RES_XML_ROOT );
      SeqUtil_TRACE ( "getNodeResources query: %s\n", query );
      if( (result = XmlUtils_getnodeset (query, context)) != NULL ) {
         parseAbortActions( result, _nodeDataPtr ); 
      }

      xmlXPathFreeObject (result);
      xmlXPathFreeContext ( context );
      xmlFreeDoc( doc );
   } else {
      /* xml file does not exist*/
      SeqUtil_TRACE( "getNodeResources not found xml file %s\n", xmlFile );
   }
   /* if it is within a work unit, set resources to match supertask */
   if (strcmp(_nodeDataPtr->workerPath, "") != 0) {
       SeqUtil_TRACE ( "nodeinfo.parseBatchResources() Resetting resources to worker's values\n");
       workerNodeDataPtr = nodeinfo(_nodeDataPtr->workerPath, "all", NULL, _seq_exp_home, NULL, NULL );
       _nodeDataPtr->mpi=workerNodeDataPtr->mpi;
       _nodeDataPtr->catchup=workerNodeDataPtr->catchup;
       SeqNode_setCpu( _nodeDataPtr, workerNodeDataPtr->cpu );
       SeqNode_setCpuMultiplier( _nodeDataPtr, workerNodeDataPtr->cpu_multiplier);
       SeqNode_setQueue( _nodeDataPtr,  workerNodeDataPtr->queue );
       SeqNode_setMachine( _nodeDataPtr, workerNodeDataPtr->machine );
       SeqNode_setMemory( _nodeDataPtr,  workerNodeDataPtr->memory );
       SeqNode_setArgs( _nodeDataPtr,  workerNodeDataPtr->soumetArgs );
       SeqNode_freeNode( workerNodeDataPtr );
   }

   /* validate a machine has been provided */
   if ( strcmp(_nodeDataPtr->machine,"") == 0) { 
      /* get default machine*/
      if ( (value = SeqUtil_getdef( defFile, "SEQ_DEFAULT_MACHINE" )) != NULL ){
          SeqNode_setMachine( _nodeDataPtr, value );
      } else {
          raiseError("ERROR: Required machine attribute of BATCH tag in %s or default machine key SEQ_DEFAULT_MACHINE in %s not set.\n",xmlFile, defFile); 
      }
   }
   free(xmlFile);
   free(defFile);
   free(value);
}

/* Returns 1 if the node exists within known context*/
int doesNodeExist(const char* _nodePath, const char* _seq_exp_home, const char* datestamp) {
   char *xmlFile = NULL, *currentFlowNode = NULL, *newNode=NULL;
   char *tmpstrtok = NULL, *tmpJobPath = NULL , *tmpAnswer=NULL, *tmpSwitchType=NULL;
   char query[512];
   int i = 0, count=0;
   xmlDocPtr doc = NULL, previousDoc=NULL;
   xmlNodeSetPtr nodeset = NULL;
   xmlXPathObjectPtr result = NULL, attributesResult = NULL;
   xmlNodePtr currentNodePtr = NULL;
   xmlXPathContextPtr context = NULL, previousContext=NULL;
   const xmlChar *nodeName = NULL;
   SeqNodeDataPtr  nodeDataPtr = NULL;
   
   int j = 0, switchItemCount = 0, completeAnswerFound = 0, answerIndex = 0, switchResultFound = 0;
   char *tmpName = NULL, *tmpSwitchItemName = NULL, *savePtr = NULL;
   char tmpQuery[512];
   xmlXPathObjectPtr switchItem = NULL, tmpResult = NULL;
   xmlChar *switchItemName = NULL;

   newNode = (char*) SeqUtil_fixPath( _nodePath );
   nodeDataPtr = (SeqNodeDataPtr) SeqNode_createNode ( newNode );
   SeqNode_setDatestamp( nodeDataPtr, (const char *) tictac_getDate(_seq_exp_home,"",datestamp) );

   SeqUtil_TRACE( "nodeinfo.doesNodeExist() node:%s seq_exp_home:%s\n", _nodePath, _seq_exp_home );

   /* count is 0-based */
   /* build the xmlfile path */
   xmlFile = malloc( strlen( _seq_exp_home ) + strlen( "/EntryModule/flow.xml" ) + 1 );
   sprintf( xmlFile, "%s/EntryModule/flow.xml", _seq_exp_home);

   /* parse the xml file */
   if ((doc = XmlUtils_getdoc(xmlFile)) == NULL) {
      raiseError("Unable to parse file, or file not found: %s\n", xmlFile);
   }

   /* the context is used to walk trough the nodes */
   context = xmlXPathNewContext(doc);

   if (tmpJobPath = (char*) malloc( strlen( _nodePath ) + 1 )) {
       strcpy( tmpJobPath, _nodePath );
   } else {
       raiseError("OutOfMemory exception in nodeinfo.doesNodeExist()\n");
   }

   tmpstrtok = (char*) strtok( tmpJobPath, "/" );
   while ( tmpstrtok != NULL ) {
      /* build the query */      
      if ( count == 0 ){
         sprintf ( query, "(/*[@name='%s'])", tmpstrtok );
         SeqUtil_stringAppend( &currentFlowNode, tmpstrtok );
      } else {
         /* next queries relative to previous node context */
         sprintf ( query, "(child::*[@name='%s'])", tmpstrtok );
         SeqUtil_stringAppend( &currentFlowNode, tmpstrtok );
      }
      
      /* run the normal query */
      if( (result = XmlUtils_getnodeset (query, context)) == NULL ) {
         SeqUtil_TRACE("nodeinfo.doesNodeExist() Node %s not found in XML master file! \n", _nodePath);
         SeqNode_freeNode( nodeDataPtr );
         SeqUtil_TRACE("nodeinfo.doesNodeExist() return = 0 \n");
         if (previousContext != NULL){
            xmlXPathFreeContext(previousContext);
         }
         if (previousDoc != NULL){
            xmlFreeDoc(previousDoc); 
         }
         xmlXPathFreeContext(context);
         xmlFreeDoc(doc);
         xmlCleanupParser();
         free(tmpJobPath);
         free(tmpSwitchType); 
         free(tmpAnswer); 
         free(xmlFile);
         free(currentFlowNode);
         return(0);
      }
       
      /* At this point I should only have one node in the result set
      I'm getting the node to set it in the context so that
      I can retrieve other nodes relative to the current one
      i.e. depends/submits/etc
      */

      nodeset = result->nodesetval;
      currentNodePtr = nodeset->nodeTab[0];
      nodeDataPtr->type = getNodeType( currentNodePtr->name );
      context->node = currentNodePtr;

          /* retrieve node specific attributes */
      if ( (attributesResult = XmlUtils_getnodeset ("(@*)", context)) != NULL){
         nodeset=attributesResult->nodesetval;
         for (i=0; i < nodeset->nodeNr; i++) {
            currentNodePtr = nodeset->nodeTab[i];
            nodeName = currentNodePtr->name;
	    if ( strcmp (nodeName, "type") == 0 && nodeDataPtr->type == Switch) {
	        tmpSwitchType=strdup(currentNodePtr->children->content);
	        tmpAnswer=switchReturn(nodeDataPtr,tmpSwitchType);
	        sprintf ( query, "(child::SWITCH_ITEM[contains(@name,'%s')])", tmpAnswer);
                /* run the normal query */
                if( (result = XmlUtils_getnodeset (query, context)) == NULL ) {
		    /*No exact match found, search for default item*/
                    SeqUtil_TRACE("nodeinfo.doesNodeExist() Query %s did not find any corresponding SWITCH ITEM in XML flow file! \nSearching for default SWITCH ITEM value (doesNodeExist)\n", query );
		    sprintf ( query, "(child::SWITCH_ITEM[@name='default'])");
		    if( (result = XmlUtils_getnodeset (query, context)) == NULL ) {
		      SeqUtil_TRACE("nodeinfo.doesNodeExist() Query %s did not find any default value SWITCH ITEM in XML flow file! (doesNodeExist)\n", query );
		      SeqNode_freeNode( nodeDataPtr );
		      if (previousContext != NULL){
			xmlXPathFreeContext(previousContext);
		      }
		      if (previousDoc != NULL){
			xmlFreeDoc(previousDoc); 
		      }
		      xmlXPathFreeContext(context);
		      xmlFreeDoc(doc);
		      xmlCleanupParser();
		      free(tmpJobPath);
		      free(tmpSwitchType); 
		      free(tmpAnswer); 
		      free(xmlFile);
		      free(currentFlowNode);
		      SeqUtil_TRACE("nodeinfo.doesNodeExist() return = 0 \n");
		      return(0);
		    } else {
		      SeqUtil_TRACE("nodeinfo.doesNodeExist() Query %s found default value SWITCH ITEM in XML flow file! (doesNodeExist)\n", query );
		      /* query returned results */
		      switchResultFound = 1;
		      nodeset = result->nodesetval;
		      currentNodePtr = nodeset->nodeTab[0];
		      context->node = currentNodePtr;
		    }
		} /*getnodeset found switch item name containing tmpAnswer */ else {
		  completeAnswerFound = 0;
		  answerIndex = 0;
		  switchItem = XmlUtils_getnodeset ("child::SWITCH_ITEM/@name", context);
		  switchItemCount = switchItem->nodesetval->nodeNr;
		  /*for each switch item containing tmpAnswer, check if it contains tmpAnswer as a complete name*/
		  for (j=0; j < switchItemCount; j++) {
		    if (completeAnswerFound == 0) {
		      switchItemName = switchItem->nodesetval->nodeTab[j]->children->content;
		      tmpSwitchItemName = switchItemName;
		      tmpName = (char*) strtok_r(tmpSwitchItemName, " ,)", &savePtr);
		      while (tmpName != NULL) {
			if ( strcmp(tmpName, tmpAnswer) == 0 ) {
			  completeAnswerFound = 1;
			  answerIndex = j;
			  break;
			} else {
			    tmpName = (char*) strtok_r ( NULL, " ,)", &savePtr);
			}
		      }
		  }
		  if ( completeAnswerFound == 1) {
		    sprintf(tmpQuery, "child::SWITCH_ITEM");
		    tmpResult = XmlUtils_getnodeset (tmpQuery, context);
		    /* query returned results */
		    switchResultFound = 1;
		    nodeset = tmpResult->nodesetval;
                    currentNodePtr = nodeset->nodeTab[answerIndex];
                    context->node = currentNodePtr;
		    completeAnswerFound = 0;
		  } 
		 } 
		 if (switchResultFound == 0) {
		    /*No exact match found, search for default item*/
		    SeqUtil_TRACE("nodeinfo.doesNodeExist() Query %s did not find any corresponding SWITCH ITEM in XML flow file! \nSearching for default SWITCH ITEM value (doesNodeExist)\n", query );
		    sprintf ( query, "(child::SWITCH_ITEM[@name='default'])");
		    if( (result = XmlUtils_getnodeset (query, context)) == NULL ) {
		      SeqUtil_TRACE("nodeinfo.doesNodeExist() Query %s did not find any default value SWITCH ITEM in XML flow file! (doesNodeExist)\n", query );
		      SeqNode_freeNode( nodeDataPtr );
		      if (previousContext != NULL){
			xmlXPathFreeContext(previousContext);
		      }
		      if (previousDoc != NULL){
			xmlFreeDoc(previousDoc); 
		      }
		      xmlXPathFreeContext(context);
		      xmlFreeDoc(doc);
		      xmlCleanupParser();
		      free(tmpJobPath);
		      free(tmpSwitchType); 
		      free(tmpAnswer); 
		      free(xmlFile);
		      free(currentFlowNode);
		      SeqUtil_TRACE("nodeinfo.doesNodeExist() return = 0 \n");
		      return(0);
		    } else {
		      SeqUtil_TRACE("nodeinfo.doesNodeExist() Query %s found default value SWITCH ITEM in XML flow file! (doesNodeExist)\n", query );
		      /* query returned results */
		      nodeset = result->nodesetval;
		      currentNodePtr = nodeset->nodeTab[0];
		      context->node = currentNodePtr;
		    }
		} 
	      }
	    }
         }
      xmlXPathFreeObject (attributesResult);
      }
      
      /* read the new flow file described in the module */
      if ( nodeDataPtr->type == Module && SHOW_ROOT_ONLY == 0 ) { 
       
         /* read the flow file located in the module depot */
	 free(xmlFile);
	 xmlFile = malloc( strlen( _seq_exp_home ) + strlen( "/modules//flow.xml") + strlen( tmpstrtok ) + 1 ); 
         sprintf ( xmlFile, "%s/modules/%s/flow.xml",_seq_exp_home, tmpstrtok ); 

	 /* keep record of previous xml file for use in siblings query*/

         if (previousContext != NULL){
             xmlXPathFreeContext(previousContext);
	 }
	 if (previousDoc != NULL){
             xmlFreeDoc(previousDoc); 
	 }
         previousContext=context;
	      previousDoc=doc; 
         context = NULL;
         doc = NULL;

         /* parse the new xml file */
         if ((doc = XmlUtils_getdoc(xmlFile)) == NULL) {
             raiseError("Unable to parse file, or file not found: %s\n", xmlFile);
         }

         /* the context is used to walk trough the nodes */
         context = xmlXPathNewContext(doc);
         sprintf ( query, "(/MODULE)" );

         if( (result = XmlUtils_getnodeset (query, context)) == NULL ) {
             SeqUtil_TRACE("nodeinfo.doesNodeExist() Warning: Problem with result set, Node %s not found in XML master file\n", _nodePath );
             SeqNode_freeNode( nodeDataPtr );
             if (previousContext != NULL){
                 xmlXPathFreeContext(previousContext);
             }
             if (previousDoc != NULL){
                 xmlFreeDoc(previousDoc); 
             }
             xmlXPathFreeContext(context);
             xmlFreeDoc(doc);
             xmlCleanupParser();
             free(tmpJobPath);
             free(tmpSwitchType); 
             free(tmpAnswer); 
             free(xmlFile);
             free(currentFlowNode);
             SeqUtil_TRACE("nodeinfo.doesNodeExist() return = 0 \n");
             return(0);
         }
         nodeset = result->nodesetval;
         currentNodePtr = nodeset->nodeTab[0];
         context->node = currentNodePtr;

      }

      /* get the next token */
      tmpstrtok = (char*) strtok(NULL,"/");

      count++;

      xmlXPathFreeObject (result);
   }

   if (previousContext != NULL){
      xmlXPathFreeContext(previousContext);
   }
   if (previousDoc != NULL){
      xmlFreeDoc(previousDoc); 
   }
   xmlXPathFreeContext(context);
   xmlFreeDoc(doc);
   xmlCleanupParser();
   free(tmpJobPath);
   free(tmpSwitchType); 
   free(tmpAnswer); 
   free(xmlFile);
   free(currentFlowNode);
   SeqNode_freeNode( nodeDataPtr );
   SeqUtil_TRACE("nodeinfo.doesNodeExist() return = 1 \n");
   return(1);

}

   /*
   */
void getFlowInfo ( SeqNodeDataPtr _nodeDataPtr, const char *_nodePath, const char *_seq_exp_home ) {
   char *xmlFile = NULL, *currentFlowNode = NULL;
   char query[512],pathToModule[SEQ_MAXFIELD];
   char *tmpstrtok = NULL, *tmpJobPath = NULL, *suiteName = NULL, *module = NULL;
   char *taskPath = NULL, *intramodulePath = NULL, *tmpPTM=NULL, *tmpAnswer=NULL, *tmpSwitchType=NULL;

   int i = 0, count=0, totalCount = 0, switchResultFound=0;
   xmlDocPtr doc = NULL, previousDoc=NULL;
   xmlNodeSetPtr nodeset = NULL;
   xmlXPathObjectPtr result = NULL, attributesResult = NULL;
   xmlNodePtr currentNodePtr = NULL;
   xmlXPathContextPtr context = NULL, previousContext=NULL;
   const xmlChar *nodeName = NULL;
   
   int j = 0, switchItemCount = 0, completeAnswerFound = 0, answerIndex = 0;
   char *tmpName = NULL, *tmpSwitchItemName = NULL, *savePtr = NULL;
   char tmpQuery[512];
   xmlXPathObjectPtr switchItem = NULL, tmpResult = NULL;
   xmlChar *switchItemName = NULL;
   
   SeqUtil_TRACE( "nodeinfo.getFlowInfo() node:%s seq_exp_home:%s\n", _nodePath, _seq_exp_home );

   /* count is 0-based */
   totalCount = (int) SeqUtil_tokenCount( _nodePath, "/" ) -1  ;
   SeqUtil_TRACE( "nodeinfo.getFlowInfo() tokenCount:%i \n", totalCount );
   /* build the xmlfile path */
   suiteName = (char*) SeqUtil_getPathLeaf( _seq_exp_home );
   xmlFile = malloc( strlen( _seq_exp_home ) + strlen( "/EntryModule/flow.xml" ) + 1 );
   sprintf( xmlFile, "%s/EntryModule/flow.xml", _seq_exp_home);

   /* parse the xml file */
   if ((doc = XmlUtils_getdoc(xmlFile)) == NULL) {
      raiseError("Unable to parse file, or file not found: %s\n", xmlFile);
   }

   /* the context is used to walk trough the nodes */
   context = xmlXPathNewContext(doc);

   tmpJobPath = (char*) malloc( strlen( _nodePath ) + 1 );

   strcpy( tmpJobPath, _nodePath );

   tmpstrtok = (char*) strtok( tmpJobPath, "/" );
   while ( tmpstrtok != NULL ) {
      switchResultFound=0;
      /* build the query */      
      if ( count == 0 ) {
         /* initial query */
         if ( SHOW_ROOT_ONLY == 1 ) {
            sprintf ( query, "(/MODULE)");
         } else {
            sprintf ( query, "(/*[@name='%s'])", tmpstrtok );
            SeqUtil_stringAppend( &currentFlowNode, tmpstrtok );
         }
      } else {
         /* next queries relative to previous node context */
         sprintf ( query, "(child::*[@name='%s'])", tmpstrtok );
         SeqUtil_stringAppend( &currentFlowNode, tmpstrtok );
      }
      
      /* run the normal query */
      if( (result = XmlUtils_getnodeset (query, context)) == NULL ) {
         raiseError("Node %s not found in XML master file! (getFlowInfo)\n", _nodePath);
      }
       
      /* At this point I should only have one node in the result set
      I'm getting the node to set it in the context so that
      I can retrieve other nodes relative to the current one
      i.e. depends/submits/etc
      */

      nodeset = result->nodesetval;
      currentNodePtr = nodeset->nodeTab[0];
      _nodeDataPtr->type = getNodeType( currentNodePtr->name );
      context->node = currentNodePtr;

          /* retrieve node specific attributes */
      if ( (attributesResult = XmlUtils_getnodeset ("(@*)", context)) != NULL){
         nodeset=attributesResult->nodesetval;
         for (i=0; i < nodeset->nodeNr; i++) {
            currentNodePtr = nodeset->nodeTab[i];
            nodeName = currentNodePtr->name;
	    if ( strcmp( nodeName, "work_unit" ) == 0 ) {
               if( _nodeDataPtr->type == Task || _nodeDataPtr->type == NpassTask )  {
	          raiseError("Work unit mode is only for containers (single_reserv=1)");
	       } else {
	          if (currentFlowNode==NULL) raiseError("Work unit mode cannot be on the root node (single_reserv=1)");
	          parseWorkerPath(currentFlowNode, _seq_exp_home, _nodeDataPtr );
	       }
   	    }
	    if ( strcmp (nodeName, "type") == 0 && _nodeDataPtr->type == Switch) {
	        tmpSwitchType=strdup(currentNodePtr->children->content);
	        tmpAnswer=switchReturn(_nodeDataPtr,tmpSwitchType);
		sprintf ( query, "(child::SWITCH_ITEM[contains(@name,'%s')])", tmpAnswer);
                /* run the normal query */
                if( (result = XmlUtils_getnodeset (query, context)) == NULL ) {
		    /*No exact match found, search for default item*/
		    SeqUtil_TRACE("Query %s did not find any corresponding SWITCH ITEM in XML flow file! (getFlowInfo)\nSearching for default SWITCH_ITEM value (getFlowInfo) \n", query );
		    sprintf ( query, "(child::SWITCH_ITEM[@name='default'])");
		    if( (result = XmlUtils_getnodeset (query, context)) == NULL ) {
		      SeqUtil_TRACE("Query %s did not find any default value SWITCH ITEM in XML flow file! (getFlowInfo)\n", query );
		      switchResultFound = 0;
		    } else {
		      SeqUtil_TRACE("Query %s found default value SWITCH ITEM in XML flow file! (getFlowInfo)\n", query );
		      /* query returned results */ 
		      switchResultFound = 1;
		      nodeset = result->nodesetval;
		      currentNodePtr = nodeset->nodeTab[0];
		      context->node = currentNodePtr;
		    }
		} /*getnodeset found switch item name containing tmpAnswer */ else {
		  completeAnswerFound = 0;
		  switchResultFound = 0;
		  answerIndex = 0;
		  switchItem = XmlUtils_getnodeset ("child::SWITCH_ITEM/@name", context);
		  switchItemCount = switchItem->nodesetval->nodeNr;
		  /*for each switch item containing tmpAnswer, check if it contains tmpAnswer as a complete name*/
		  for (j=0; j < switchItemCount; j++) {
		    if (completeAnswerFound == 0) {
		      switchItemName = switchItem->nodesetval->nodeTab[j]->children->content;
		      tmpSwitchItemName = switchItemName;
		      tmpName = (char*) strtok_r(tmpSwitchItemName, " ,)", &savePtr);
		      while (tmpName != NULL) {
			if ( strcmp(tmpName, tmpAnswer) == 0 ) {
			  completeAnswerFound = 1;
			  answerIndex = j;
			  break;
			} else {
			    tmpName = (char*) strtok_r ( NULL, " ,)", &savePtr);
			}
		      }
		      savePtr = NULL;
		  }
		  if ( completeAnswerFound == 1) {
		    sprintf(tmpQuery, "child::SWITCH_ITEM");
		    tmpResult = XmlUtils_getnodeset (tmpQuery, context);
		    /* query returned results */ 
		    switchResultFound = 1;
		    nodeset = tmpResult->nodesetval;
                    currentNodePtr = nodeset->nodeTab[answerIndex];
                    context->node = currentNodePtr;
		    completeAnswerFound = 0;
		  } 
		 } 
		if (switchResultFound == 0) {
		    /*No exact match found, search for default item*/
		    SeqUtil_TRACE("Query %s did not find any corresponding SWITCH ITEM in XML flow file! (getFlowInfo)\nSearching for default SWITCH_ITEM value (getFlowInfo) \n", query );
		    sprintf ( query, "(child::SWITCH_ITEM[@name='default'])");
		    if( (result = XmlUtils_getnodeset (query, context)) == NULL ) {
		      SeqUtil_TRACE("Query %s did not find any default value SWITCH ITEM in XML flow file! (getFlowInfo)\n", query );
		    } else {
		      SeqUtil_TRACE("Query %s found default value SWITCH ITEM in XML flow file! (getFlowInfo)\n", query );
		      /* query returned results */ 
		      switchResultFound = 1;
		      nodeset = result->nodesetval;
		      currentNodePtr = nodeset->nodeTab[0];
		      context->node = currentNodePtr;
		    }
		} 
	      }
	      if ( switchResultFound == 0 ) {
		 SeqUtil_TRACE("Query %s did not find any corresponding SWITCH ITEM in XML flow file! (getFlowInfo)\n", query );
	      }
	      SeqNameValues_insertItem( &_nodeDataPtr->switchAnswers, (char*) SeqUtil_fixPath(currentFlowNode), tmpAnswer);
	     }
      }
      xmlXPathFreeObject (attributesResult);
    }

      /* read the new flow file described in the module */
      if ( _nodeDataPtr->type == Module && SHOW_ROOT_ONLY == 0 ) { 
       
         /* reset the intramodule path */
         free(intramodulePath);
         intramodulePath = NULL;

         /* read the flow file located in the module depot */
	      free(xmlFile);
	      xmlFile = malloc( strlen( _seq_exp_home ) + strlen( "/modules//flow.xml") + strlen( tmpstrtok ) + 1 ); 
         sprintf ( xmlFile, "%s/modules/%s/flow.xml",_seq_exp_home, tmpstrtok ); 
         SeqUtil_TRACE( "nodeinfo Found new XML source from module:%s/%s path:%s\n", taskPath, tmpstrtok , xmlFile);

	 /* keep record of previous xml file for use in siblings query*/

         if (previousContext != NULL){
             xmlXPathFreeContext(previousContext);
	 }
	 if (previousDoc != NULL){
             xmlFreeDoc(previousDoc); 
	 }

         previousContext=context;
	 previousDoc=doc; 
         context = NULL;
         doc = NULL;

         /* parse the xml file */
         if ((doc = XmlUtils_getdoc(xmlFile)) == NULL) {
             raiseError("Unable to parse file, or file not found: %s\n", xmlFile);
         }

         /* the context is used to walk trough the nodes */
         context = xmlXPathNewContext(doc);
         sprintf ( query, "(/MODULE)" );

         if( (result = XmlUtils_getnodeset (query, context)) == NULL ) {
             raiseError ("ERROR: Problem with result set, Node %s not found in XML master file\n", _nodePath );
         }
         nodeset = result->nodesetval;
         currentNodePtr = nodeset->nodeTab[0];
         context->node = currentNodePtr;

      }

      /* we are building the internal path of the node and container*/
      if ( count != totalCount ) {
         SeqUtil_stringAppend( &currentFlowNode, "/");
         /* Case and case_item are not part of the task_depot */
         /* they are however part of the container path */
         if (_nodeDataPtr->type == Family ||_nodeDataPtr->type == Loop || _nodeDataPtr->type == Switch ) {
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
         /* get loop info from parent loop xml resource file */
	 getNodeLoopContainersAttr( _nodeDataPtr, currentFlowNode, _seq_exp_home );
      }
       /* adds switch containers except if last node is also loop */
      if ( _nodeDataPtr->type == Switch &&  tmpstrtok != NULL ) {
         SeqNode_addSpecificData( _nodeDataPtr, "SWITCH_TYPE", tmpSwitchType );
         SeqNode_addSwitch(_nodeDataPtr, currentFlowNode, tmpSwitchType, tmpAnswer); 
	 /*reset switch and answer*/ 
         free(tmpSwitchType); 
         free(tmpAnswer);
	 tmpSwitchType=NULL;
         tmpAnswer=NULL;
      }

      if ( SHOW_ROOT_ONLY == 1 ) {
         tmpstrtok = NULL;
      }

      xmlXPathFreeObject (result);
   }

  /* SeqUtil_stripSubstring(&pathToModule,intramodulePath,1); */
   memset(pathToModule, '\0', sizeof(pathToModule)); 
   if (intramodulePath != NULL) { 
       /* if container and intramodule path are equal, you're in the Entry module, so pathToModule is empty */
      if (strcmp(intramodulePath, _nodeDataPtr->container) != 0) {   
           tmpPTM=malloc(strlen(_nodeDataPtr->container) - strlen(intramodulePath)+1); 
           (void)strncpy(pathToModule,_nodeDataPtr->container,strlen(_nodeDataPtr->container) - strlen(intramodulePath)); 
      }
   } else {
     strcpy(pathToModule,_nodeDataPtr->container); 
   }
   sprintf(pathToModule,"%s/%s",pathToModule,module); 
   SeqNode_setPathToModule(_nodeDataPtr, pathToModule);

   SeqUtil_TRACE ( "nodeinfo.getFlowInfo() *********** Node Information **********\n" );
   SeqUtil_TRACE ( "nodeinfo.getFlowInfo() node path:%s\n", _nodePath );
   SeqUtil_TRACE ( "nodeinfo.getFlowInfo() container=%s\n", _nodeDataPtr->container );
   SeqUtil_TRACE ( "nodeinfo.getFlowInfo() intramodule_container=%s\n", intramodulePath );
   SeqUtil_TRACE ( "nodeinfo.getFlowInfo() pathToModule=%s\n", pathToModule );
   SeqUtil_TRACE ( "nodeinfo.getFlowInfo() taskPath=%s\n", taskPath );
   SeqNode_setInternalPath( _nodeDataPtr, taskPath );

   /* at this point we're done validating the nodes */
   /* point the context to the last node retrieved so that we can
      do relative queries */
  
   /* retrieve node specific attributes */
   strcpy ( query, "(@*)");
   if ( (result = XmlUtils_getnodeset (query, context)) != NULL){
      parseNodeSpecifics( _nodeDataPtr->type, result, _nodeDataPtr );
   }
   xmlXPathFreeObject (result);

   if ( SHOW_ALL || SHOW_DEP ) {
       /* retrieve depends node */
       SeqUtil_TRACE ( "nodeinfo.getFlowInfo() *********** internal depends **********\n");
       sprintf ( query, "(child::DEPENDS_ON)");
       if (_nodeDataPtr->type==Module){
           if( (result = XmlUtils_getnodeset (query, previousContext)) != NULL ) {
               parseDepends( result, _nodeDataPtr, 1 ); 
           }
        } else {
           if( (result = XmlUtils_getnodeset (query, context)) != NULL ) {
               parseDepends( result, _nodeDataPtr, 1 ); 
           }
        }
       xmlXPathFreeObject (result);
   }

   if( SHOW_ALL ) {
      /* retrieve submits node */
      SeqUtil_TRACE ( "nodeinfo.getFlowInfo() *********** submits **********\n");
      strcpy ( query, "(child::SUBMITS)");
      result = XmlUtils_getnodeset (query, context);
      parseSubmits( result, _nodeDataPtr ); 
      xmlXPathFreeObject (result);
    
      /* retrieve node's siblings */
      SeqUtil_TRACE ( "nodeinfo.getFlowInfo() *********** node siblings **********\n");
      
      if ( _nodeDataPtr->type == Switch && switchResultFound ) {
          strcpy (query, "(../preceding-sibling::*[@name])");
      } else {
          strcpy (query, "(preceding-sibling::*[@name])");
      }
      if ( _nodeDataPtr->type == Module) {
          result =  XmlUtils_getnodeset (query, previousContext);
      } else {
          result =  XmlUtils_getnodeset (query, context);
      }
      if (result) {
         SeqUtil_TRACE ( "nodeinfo.getFlowInfo() *********** preceding siblings found**********\n");
      }
      parseNodeSiblings( result, _nodeDataPtr); 
      xmlXPathFreeObject (result);
   
      if ( _nodeDataPtr->type == Switch && switchResultFound ) {
          strcpy (query, "(../following-sibling::*[@name])");
      } else {
          strcpy (query, "(following-sibling::*[@name])");
      }
      if ( _nodeDataPtr->type == Module) {
          result =  XmlUtils_getnodeset (query, previousContext);
      } else {
          result =  XmlUtils_getnodeset (query, context);
      }
      if (result) {
         SeqUtil_TRACE ( "nodeinfo.getFlowInfo() *********** following siblings found**********\n");
      }
      parseNodeSiblings( result, _nodeDataPtr); 
      xmlXPathFreeObject (result);
   }

   SeqNode_setSuiteName( _nodeDataPtr, suiteName ); 
   SeqNode_setModule( _nodeDataPtr, module );
   SeqNode_setIntramoduleContainer( _nodeDataPtr, intramodulePath );

   if (previousContext != NULL){
      xmlXPathFreeContext(previousContext);
   }
   if (previousDoc != NULL){
      xmlFreeDoc(previousDoc); 
   }
   xmlXPathFreeContext(context);
   xmlFreeDoc(doc);
   xmlCleanupParser();
   free(suiteName);
   free(tmpJobPath);
   free(tmpSwitchType); 
   free(tmpAnswer); 
   free(module);
   free(taskPath);
   free(tmpPTM);
   free(xmlFile);
   free(currentFlowNode);
}

SeqNodeDataPtr nodeinfo ( const char* node, const char* filters, SeqNameValuesPtr _loops, const char* _exp_home, char *extraArgs, char* datestamp ) {

   char *seq_exp_home = NULL, *newNode = NULL, *tmpstrtok = NULL, *tmpfilters = NULL;
   SeqNodeDataPtr  nodeDataPtr = NULL;

   if( _exp_home == NULL ) {
      seq_exp_home=getenv("SEQ_EXP_HOME");
      if ( seq_exp_home == NULL ) {
         raiseError("SEQ_EXP_HOME not set! (nodeinfo) \n");
      }
   } else {
      seq_exp_home = _exp_home;
   }

   tmpfilters = strdup( filters );
   tmpstrtok = (char*) strtok( tmpfilters, "," );
   while ( tmpstrtok != NULL ) {
      if ( strcmp( tmpfilters, "all" ) == 0 ) SHOW_ALL = 1;
      if ( strcmp( tmpfilters, "cfg" ) == 0 ) SHOW_CFGPATH = 1;
      if ( strcmp( tmpfilters, "task" ) == 0 ) SHOW_TASKPATH= 1;
      if ( strcmp( tmpfilters, "res" ) == 0 ) SHOW_RESSOURCE = 1;
      if ( strcmp( tmpfilters, "root" ) == 0 ) SHOW_ROOT_ONLY = 1;
      if ( strcmp( tmpfilters, "dep" ) == 0 ) SHOW_DEP = 1;
      if ( strcmp( tmpfilters, "res_path" ) == 0 ) SHOW_RESPATH = 1;
      tmpstrtok = (char*) strtok(NULL,",");
   }
   newNode = (char*) SeqUtil_fixPath( node );
   SeqUtil_TRACE ( "nodeinfo.nodefinfo() trying to create node %s\n", newNode );
   nodeDataPtr = (SeqNodeDataPtr) SeqNode_createNode ( newNode );
   SeqNode_setSeqExpHome(nodeDataPtr,seq_exp_home); 

   SeqUtil_TRACE ( "nodeinfo.nodefinfo() argument datestamp %s. If (null), will run tictac to find value.\n", datestamp );
   SeqNode_setDatestamp( nodeDataPtr, (const char *) tictac_getDate(seq_exp_home,"",datestamp) );
   /*pass the content of the command-line (or interface) extra soumet args to the node*/
   SeqNode_setSoumetArgs(nodeDataPtr,extraArgs);

   if ( SHOW_ROOT_ONLY ) {
      getRootNode ( nodeDataPtr, seq_exp_home );
   } else {
      /* add loop arg list to node */
      SeqNode_setLoopArgs( nodeDataPtr,_loops);
      getFlowInfo ( nodeDataPtr, (char*) newNode, seq_exp_home );
      getNodeResources (nodeDataPtr, (char*) newNode, seq_exp_home);

   }

   free( tmpfilters ); 
   return nodeDataPtr;
}
