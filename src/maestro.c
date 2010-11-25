#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include "SeqUtil.h"
#include "tictac.h"
#include "nodeinfo.h"
#include "SeqLoopsUtil.h"
#include "SeqDatesUtil.h"

#define SEQ_MAXFIELD 1000

/*
# level 8 is reserved for normal everyday runs
# level 9 includes normal PLUS discretionary jobs
*/
#define SEQ_CATCHUP_NORMAL 8
#define SEQ_CATCHUP_DISCR 9
static const char* CATCHUP_DISCR_MSG = "DISCRETIONARY: this job is not scheduled to run";
static const char* CATCHUP_UNSUBMIT_MSG = "CATCHUP mode: this job will not be submitted";

static char OCSUB[256];
static char SEQ_EXP_HOME[256];
static SeqNameValuesPtr LOOP_ARGS;

/* static char DATESTAMP[SEQ_MAXFIELD]; */
static char USERNAME[8];
static char EXPNAME[128];

/* Function declarations */
static int go_abort(char *_signal, char *_flow, const SeqNodeDataPtr _nodeDataPtr);
static int go_initialize(char *_signal, char *_flow ,const SeqNodeDataPtr _nodeDataPtr);
static int go_begin(char *_signal, char *_flow ,const SeqNodeDataPtr nodeDataPtr);
static int go_end(char *_signal, char *_flow ,const SeqNodeDataPtr _nodeDataPtr);
static int go_submit(const char *signal, char *_flow ,const SeqNodeDataPtr nodeDataPtr, int ignoreAllDeps );

/* deal with containers states */
static void processContainerEnd ( const SeqNodeDataPtr _nodeDataPtr, char *_flow );
static void processContainerBegin ( const SeqNodeDataPtr _nodeDataPtr);
static void processLoopContainerEnd( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr loop_args_ptr , char *_flow);
static void processLoopContainerBegin( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr loop_args_ptr);
static int isLoopComplete ( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args );

/* State functions: these deal with the status files */
static void setBeginState(char *_signal, const SeqNodeDataPtr _nodeDataPtr);
static void setSubmitState(const SeqNodeDataPtr _nodeDataPtr);
static void setInitState(const SeqNodeDataPtr _nodeDataPtr);
static void setEndState(const char *_signal, const SeqNodeDataPtr _nodeDataPtr);
static void setAbortState(const SeqNodeDataPtr _nodeDataPtr, char * current_action);
static void setWaitingState(const SeqNodeDataPtr _nodeDataPtr, const char* waited_one, const char* waited_status);
static void clearAllFinalStates (SeqNodeDataPtr _nodeDataPtr, char * fullNodeName, char * originator );

/* submission utilities */
static void submitDependencies ( const SeqNodeDataPtr _nodeDataPtr, const char* signal );
static void submitNodeList ( const LISTNODEPTR listToSubmit, SeqNameValuesPtr loop_args_ptr); 
static void submitLoopSetNodeList ( const LISTNODEPTR listToSubmit, 
                                SeqNameValuesPtr container_args_ptr, SeqNameValuesPtr set_args_ptr); 

/* dependancy related */
static int writeNodeWaitedFile( const SeqNodeDataPtr _nodeDataPtr, char* dep_exp_path, char* dep_node, 
                                char* dep_status, char* dep_index, char* dep_datestamp );
static int validateDependencies (const SeqNodeDataPtr _nodeDataPtr);

/* ord_soumet related */
char* generateConfig (const SeqNodeDataPtr _nodeDataPtr, const char* flow);

/* 
go_abort

This function is used to cause the node to enter an abort state, reacting to the abort actions defined
and handling the container behaviour.

Inputs:
  signal - pointer to the value of the signal given to the binary (-s option)
  flow - pointer to the value of the flow given to the binary ( -f option)
  _nodeDataPtr - pointer to the node targetted by the execution

*/

static int go_abort(char *_signal, char *_flow ,const SeqNodeDataPtr _nodeDataPtr) {
   char *extName = NULL, *extensionBase = NULL;
   LISTNODEPTR tempPtr=NULL;
   SeqNameValuesPtr newArgs = NULL;
   char *current_action=NULL;
   char filename[SEQ_MAXFIELD];

   SeqUtil_TRACE( "maestro.go_abort() node=%s signal=%s flow=%s\n", _nodeDataPtr->name, _signal, _flow );
   actions( _signal, _flow, _nodeDataPtr->name );
   SeqUtil_stringAppend( &extName, _nodeDataPtr->name );
   if( strlen( _nodeDataPtr->extension ) > 0 ) {
      SeqUtil_stringAppend( &extName, "." );
      SeqUtil_stringAppend( &extName, _nodeDataPtr->extension );
   }

   /*
   Go through the list of _nodeDataPtr->abort_actions to find the current status.
   A typical _nodeDataPtr->abort will have two or more elements "rerun stop or cont".
   A lockfile is created for the current action.....
   Using the above example the first time the job aborts it'll create
   a lockfile with an extension "rerun". If it bombs again, the extension
   will be "stop". 
   */

   tempPtr = _nodeDataPtr->abort_actions;
   while ( tempPtr != NULL ) {
      current_action = (char *) malloc(strlen(tempPtr->data)+1);
      strcpy(current_action,tempPtr->data);
      SeqUtil_TRACE( "maestro.go_abort() checking for action %s on node %s \n", current_action, _nodeDataPtr->name); 
      sprintf(filename,"%s/%s.%s.abort.%s",_nodeDataPtr->workdir, extName, _nodeDataPtr->datestamp, current_action);
      if ( access(filename, R_OK) == 0 ) {
      /* We've done this action the last time, so we're up to the next one */
         tempPtr = tempPtr->nextPtr;
         free(current_action);
         current_action = NULL;
         if ( tempPtr != NULL ) {
             current_action = (char *) malloc(strlen(tempPtr->data)+1);
             strcpy(current_action,tempPtr->data);
             SeqUtil_TRACE( "maestro.go_abort() doing action %s on node %s \n", current_action, _nodeDataPtr->name); 
         }
         break;
      } else if ( tempPtr->nextPtr == NULL ) {
         /* no file was found, and we've reached the end of the list, so we must do the first action */
         tempPtr = _nodeDataPtr->abort_actions;
         current_action = (char *) malloc(strlen(tempPtr->data)+1);
         strcpy(current_action,tempPtr->data);
         SeqUtil_TRACE( "maestro.go_abort() doing action %s on node %s \n", current_action, _nodeDataPtr->name); 
         break; 
      } else {
         /* next action, we're not at the end of the list yet */
         tempPtr = tempPtr->nextPtr;
      }
   }

   /*
   if flow is "stop", set abort_action to stop so as not to continue {icrrun}
   */
   
   if ( strcmp( _flow, "stop" ) == 0 ) {
      current_action = strdup("stop");
   }

   if ( current_action == NULL) {
      if( _nodeDataPtr->abort_actions != NULL ) {
         printf( "nodelogger: %s i \"maestro info: abort action list finished. Aborting with stop.\" \n", _nodeDataPtr->name );
         nodelogger( _nodeDataPtr->name, "info", _nodeDataPtr->extension, "maestro info: abort action list finished. Aborting with stop.",_nodeDataPtr->datestamp);
      }
      current_action = strdup("stop");
   }

   /* create status file for current action */

   setAbortState ( _nodeDataPtr, current_action );

   if ( strcmp( current_action, "rerun" ) == 0 ) {
      /* issue an appropriate message, then rerun the node */
      printf( "nodelogger: %s X \"BOMBED: it has been resubmitted\"\n", _nodeDataPtr->name );
      nodelogger(_nodeDataPtr->name,"info",_nodeDataPtr->extension,"BOMBED: it has been resubmitted",_nodeDataPtr->datestamp);
      go_submit( "submit", _flow , _nodeDataPtr, 1 );
   } else if (strcmp(current_action,"cont") == 0) {
         /*  issue a log message, then submit the jobs in node->submits */
         printf( "nodeabort: %s %s %s\n", _nodeDataPtr->name, _signal, "cont" );
         nodeabort( _signal, _nodeDataPtr, "cont", _nodeDataPtr->datestamp);
         /* submit the rest of the jobs it's supposed to submit (still missing dependency submissions)*/
         SeqUtil_TRACE( "maestro.go_abort() doing submissions\n", _nodeDataPtr->name, _signal );
         submitNodeList(_nodeDataPtr->submits, LOOP_ARGS);
   } else if (strcmp(current_action,"stop") == 0) {
      /* issue a message that the job has bombed */
         printf("nodeabort: %s %s %s\n", _nodeDataPtr->name, _signal, current_action);
         nodeabort(_signal, _nodeDataPtr,current_action, _nodeDataPtr->datestamp);
   } else {
      /*
      if abort_flag is unrecogized, aborts
      */
      printf("invalid abort action flag %s... aborting...nodeabort: %s stop\n", current_action, _nodeDataPtr->name);
      nodeabort(_signal, _nodeDataPtr,"stop", _nodeDataPtr->datestamp);

   }
      /* make containers "abort" except for discreet nodes*/
   if ( strcmp(_nodeDataPtr->container, "") != 0 && _nodeDataPtr->catchup != SEQ_CATCHUP_DISCR ) {
      SeqUtil_TRACE( "maestro.go_abort() causing container %s to abort\n", _nodeDataPtr->container );
      maestro ( _nodeDataPtr->container, "abortx", "stop", LOOP_ARGS, 0 );
   }

   if( _nodeDataPtr->type == Loop ) {
      if( ((char*) SeqLoops_getLoopAttribute( LOOP_ARGS, _nodeDataPtr->nodeName )) != NULL ) {
         extensionBase = (char*) SeqLoops_getExtensionBase( _nodeDataPtr );
         newArgs = (SeqNameValuesPtr) SeqLoops_convertExtension( _nodeDataPtr->loops, extensionBase );
         printf( "********** calling maestro -s abort -f stop -n %s with loop args=%s\n", _nodeDataPtr->name, SeqLoops_getLoopArgs(LOOP_ARGS) );
         maestro ( _nodeDataPtr->name, "abort", "stop", newArgs, 0 );
      }
   }

   free( extName );
   free( extensionBase );
   SeqNameValues_deleteWholeList( &newArgs );
   actionsEnd( _signal, _flow, _nodeDataPtr->name );
   return 0;
}

/* 
setAbortState

Deals with the status files associated with the abort state by clearing the other states then putting the node in abort state.

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution
  current_action - pointer to the string declaring the abort action to do (usually cont, stop, rerun).

*/

static void setAbortState(const SeqNodeDataPtr _nodeDataPtr, char * current_action) {

   char *extName = NULL ;
   char filename[SEQ_MAXFIELD];

   SeqUtil_stringAppend( &extName, _nodeDataPtr->name );
   if( strlen( _nodeDataPtr->extension ) > 0 ) {
      SeqUtil_stringAppend( &extName, "." );
      SeqUtil_stringAppend( &extName, _nodeDataPtr->extension );
   }      

   /* clear any other state */
   clearAllFinalStates( _nodeDataPtr, extName, "initialize" ); 

   /* clear intermidiary states */
   /* remove the node abort.cont lock file */
   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s.%s.abort.cont",_nodeDataPtr->workdir, extName , _nodeDataPtr->datestamp); 
   if ( access(filename, R_OK) == 0 ) {
      SeqUtil_TRACE( "maestro.setAbortState() removed lockfile %s\n", filename);
      removeFile(filename);
   }

   /* remove the node abort.rerun lock file */
   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s.%s.abort.rerun",_nodeDataPtr->workdir, extName, _nodeDataPtr->datestamp); 
   if ( access(filename, R_OK) == 0 ) {
      SeqUtil_TRACE( "maestro.setAbortState() removed lockfile %s\n", filename);
      removeFile(filename);
   }

   /* create the node status file */
   memset(filename,'\0',sizeof filename);

   sprintf(filename,"%s/%s.%s.abort.%s",_nodeDataPtr->workdir,extName, _nodeDataPtr->datestamp, current_action); 
   SeqUtil_TRACE( "maestro.setAbortState() created lockfile %s\n", filename);
   touch(filename);

   /* for npasstask, we need to create a lock file without the extension
    * so that its container gets the same state */
   if( _nodeDataPtr->type == NpassTask) {
      sprintf(filename,"%s/%s.%s.abort",_nodeDataPtr->workdir, _nodeDataPtr->name, _nodeDataPtr->datestamp); 
      touch(filename);
   }

   free( extName );

}

/* 
go_initialize

This function is used to cause the node to enter an initialized state, clearing the node's state, and possibly any child nodes. 

Inputs:
  signal - pointer to the value of the signal given to the binary (-s option)
  flow - pointer to the value of the flow given to the binary ( -f option)
  _nodeDataPtr - pointer to the node targetted by the execution

*/


static int go_initialize(char *_signal, char *_flow ,const SeqNodeDataPtr _nodeDataPtr) {
   
    /* clears all the datestamped status files starting from the current node, if node was submitted with xfer, else just the current node */
   char *extName = NULL ;
   char cmd[SEQ_MAXFIELD];
   actions( _signal, _flow , _nodeDataPtr->name );
   SeqUtil_TRACE( "maestro.go_initialize() node=%s signal=%s flow=%s\n", _nodeDataPtr->name, _signal, _flow );

   if ((strcmp (_signal,"initbranch" ) == 0) && (_nodeDataPtr->type == Task || _nodeDataPtr->type == NpassTask)) {
      raiseError( "maestro -s initbranch cannot be called on task nodes. Exiting. \n" );
   }
   setInitState( _nodeDataPtr ); 

   SeqUtil_stringAppend( &extName, "" );
   if( strlen( _nodeDataPtr->extension ) > 0 ) {
      SeqUtil_stringAppend( &extName, "." );
      SeqUtil_stringAppend( &extName, _nodeDataPtr->extension );
   }      

   /* delete lockfiles in branches under current node */
   if (  strcmp (_signal,"initbranch" ) == 0 ) {
       memset( cmd, '\0' , sizeof cmd);
       printf("Following lockfiles are being deleted: \n");
       SeqUtil_TRACE( "maestro.go_initialize() deleting end lockfiles starting at node=%s\n", _nodeDataPtr->name);
       sprintf(cmd, "find %s/%s/%s -name \"*%s*%s.end\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir,_nodeDataPtr->container, _nodeDataPtr->nodeName, extName, _nodeDataPtr->datestamp );
       system(cmd);
       SeqUtil_TRACE( "maestro.go_initialize() deleting begin lockfiles starting at node=%s\n", _nodeDataPtr->name); 
       sprintf(cmd, "find %s/%s/%s -name \"*%s*%s.begin\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir,_nodeDataPtr->container, _nodeDataPtr->nodeName, extName, _nodeDataPtr->datestamp );
       system(cmd);
       SeqUtil_TRACE( "maestro.go_initialize() deleting abort lockfiles starting at node=%s\n", _nodeDataPtr->name); 
       sprintf(cmd, "find %s/%s/%s -name \"*%s*%s.abort.*\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir,_nodeDataPtr->container, _nodeDataPtr->nodeName, extName, _nodeDataPtr->datestamp );
       system(cmd);
       SeqUtil_TRACE( "maestro.go_initialize() deleting submit lockfiles starting at node=%s\n", _nodeDataPtr->name); 
       sprintf(cmd, "find %s/%s/%s -name \"*%s*%s.submit\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir,_nodeDataPtr->container, _nodeDataPtr->nodeName, extName, _nodeDataPtr->datestamp );
       system(cmd);
       SeqUtil_TRACE( "maestro.go_initialize() deleting waiting lockfiles starting at node=%s\n", _nodeDataPtr->name); 
       sprintf(cmd, "find %s/%s/%s -name \"*%s*%s.waiting\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir,_nodeDataPtr->container, _nodeDataPtr->nodeName, extName, _nodeDataPtr->datestamp  );
       system(cmd);
   }

   /* delete every iterations if no extension specified for npasstask */
   if ( _nodeDataPtr->type == NpassTask && strlen( _nodeDataPtr->extension ) == 0 ) {
      sprintf(cmd, "find %s/%s/ -name \"%s.*.%s.*\" -type f -print",_nodeDataPtr->workdir,_nodeDataPtr->container, _nodeDataPtr->nodeName, _nodeDataPtr->datestamp );
      system(cmd);
   }
   nodelogger(_nodeDataPtr->name,"init",_nodeDataPtr->extension,"",_nodeDataPtr->datestamp);
   actionsEnd( _signal, _flow, _nodeDataPtr->name );
   free( extName );
   return 0; 
}

/* 
setInitState

Deals with the status files associated with the initial state by clearing the other states.

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution
  
*/

static void setInitState(const SeqNodeDataPtr _nodeDataPtr) {

   char *extName = NULL;
   char filename[SEQ_MAXFIELD];
   LISTNODEPTR nodeList = NULL;

   /* delete the lock files for the current loop, it is stored in it's container */
   if( _nodeDataPtr->type == Loop ) {
      /* this will build the list for the loop iterations */
      if( ((char*) SeqLoops_getLoopAttribute( LOOP_ARGS, _nodeDataPtr->nodeName )) == NULL ) {
         /* user has not selected an extension for the current loop, clear them all */
         /* get the list of child leaf extensions for the node */
         nodeList = (LISTNODEPTR) SeqLoops_childExtensions( _nodeDataPtr, LOOP_ARGS );
      }
   }
   /* this line will take care of task, normal containers (including the loop node) */
   SeqListNode_insertItem( &nodeList, "" );

   while( nodeList != NULL ) {
      SeqUtil_stringAppend( &extName, _nodeDataPtr->name );
      if( strlen( _nodeDataPtr->extension ) > 0 || strlen( nodeList->data ) > 0 ) {
         SeqUtil_stringAppend( &extName, "." );
         SeqUtil_stringAppend( &extName, _nodeDataPtr->extension );
         SeqUtil_stringAppend( &extName, nodeList->data );
      }
      SeqUtil_TRACE("maestro.go_initialize()() Looking for status files: %s\n", extName);

      /* clear any other state */
      clearAllFinalStates( _nodeDataPtr, extName, "initialize" ); 

      /* clear intermidiary states */
      /* remove the node abort.cont lock file */
      memset(filename,'\0',sizeof filename);
      sprintf(filename,"%s/%s.%s.abort.cont",_nodeDataPtr->workdir, extName , _nodeDataPtr->datestamp); 
      if ( access(filename, R_OK) == 0 ) {
         SeqUtil_TRACE( "maestro.go_initialize() removed lockfile %s\n", filename);
         removeFile(filename);
      }
   
      /* remove the node abort.cont lock file */
      memset(filename,'\0',sizeof filename);
      sprintf(filename,"%s/%s.%s.abort.rerun",_nodeDataPtr->workdir, extName, _nodeDataPtr->datestamp); 
      if ( access(filename, R_OK) == 0 ) {
         SeqUtil_TRACE( "maestro.go_initialize() removed lockfile %s\n", filename);
         removeFile(filename);
      }
      free( extName );
      extName = NULL;
      nodeList = nodeList->nextPtr;
   }

   SeqListNode_deleteWholeList(&nodeList);
}

/* 
go_begin

This function is used to cause the node to enter the begin state. Containers submit their implicit dependencies in this state.
The function also deals with the parent containers' begin state if conditions are met.  

Inputs:
  signal - pointer to the value of the signal given to the binary (-s option)
  flow - pointer to the value of the flow given to the binary ( -f option)
  _nodeDataPtr - pointer to the node targetted by the execution

*/

static int go_begin(char *_signal, char *_flow, const SeqNodeDataPtr _nodeDataPtr) {
   char *extensionBase = NULL, *nodeBase = NULL;
   SeqNameValuesPtr newArgs = NULL, loopArgs = NULL, loopSetArgs = NULL;
   SeqUtil_TRACE( "maestro.go_begin() node=%s signal=%s flow=%s\n", _nodeDataPtr->name, _signal, _flow );
   nodeBase = (char*) SeqUtil_getPathBase( (const char*) _nodeDataPtr->name );

   actions( _signal, _flow ,_nodeDataPtr->name );

   /* clear status files of nodes underneath containers when they begin */
   if ( _nodeDataPtr->type != Task && _nodeDataPtr->type != NpassTask && strcmp(_flow, "continue") == 0 ) {
       go_initialize("initbranch", _flow, _nodeDataPtr); 
   } 

   /* create begin lock file and other lock file */
   setBeginState ( _signal, _nodeDataPtr );

   /* containers will submit their direct submits in begin */
   if( _nodeDataPtr->type == Loop && (strcmp(_flow, "continue") == 0) ) {
      if( strcmp(SeqLoops_getLoopAttribute( _nodeDataPtr->data, "TYPE" ), "LoopSet") == 0 ) {
         /* we might have to submit a set of iterations instead of only one */
	 /* get the list of iterations to submit */
         loopSetArgs = (SeqNameValuesPtr) SeqLoops_getLoopSetArgs( _nodeDataPtr, LOOP_ARGS );
	 loopArgs =  (SeqNameValuesPtr) SeqLoops_getContainerArgs( _nodeDataPtr, LOOP_ARGS );
	 /* submit the set iterations */
         submitLoopSetNodeList(_nodeDataPtr->submits, loopArgs, loopSetArgs);
      } else {
         loopArgs = (SeqNameValuesPtr) SeqLoops_submitLoopArgs( _nodeDataPtr, LOOP_ARGS );
         SeqUtil_TRACE( "maestro.go_begin() doing loop submissions\n", _nodeDataPtr->name, _signal );
         submitNodeList(_nodeDataPtr->submits, loopArgs);
     }
   } else if (_nodeDataPtr->type != Task && _nodeDataPtr->type != NpassTask 
             && _nodeDataPtr->type != Case && (strcmp(_flow, "continue") == 0) ) {
        SeqUtil_TRACE( "maestro.go_begin() doing submissions\n", _nodeDataPtr->name, _signal );
        submitNodeList(_nodeDataPtr->submits, LOOP_ARGS);
   } 

   if (_nodeDataPtr->type == Case && strcmp(_flow, "continue") == 0 ) {
   /* run the script that will handle the case decision and submit that node 
       submitNodeList(_nodeDataPtr->whateverwecomeupwith)
   */
   }

   if ( strcmp(_nodeDataPtr->container, "") != 0 ) {
      processContainerBegin(_nodeDataPtr); 
   }

   /* this will set the loop into begin state */
   if( _nodeDataPtr->type == Loop ) {
      if( ((char*) SeqLoops_getLoopAttribute( LOOP_ARGS, _nodeDataPtr->nodeName )) != NULL ) {
         extensionBase = (char*) SeqLoops_getExtensionBase( _nodeDataPtr );
         newArgs = (SeqNameValuesPtr) SeqLoops_convertExtension( _nodeDataPtr->loops, extensionBase );
         printf( "********** calling maestro -s begin -n %s with loop args=%s\n", _nodeDataPtr->name, SeqLoops_getLoopArgs(newArgs) );
         maestro ( _nodeDataPtr->name, "begin", "stop", newArgs, 0 );
      }
   }

   SeqNameValues_deleteWholeList( &newArgs );
   SeqNameValues_deleteWholeList( &loopArgs );
   actionsEnd( _signal, _flow ,_nodeDataPtr->name );
   return(0);
}

/* 
setBeginState

Deals with the status files associated with the begin state by clearing the other states then putting the node in begin state.

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution

*/

static void setBeginState(char *_signal, const SeqNodeDataPtr _nodeDataPtr) {

   char *extName = NULL ;
   char filename[SEQ_MAXFIELD];

   SeqUtil_stringAppend( &extName, _nodeDataPtr->name );
   if( strlen( _nodeDataPtr->extension ) > 0 ) {
      SeqUtil_stringAppend( &extName, "." );
      SeqUtil_stringAppend( &extName, _nodeDataPtr->extension );
   }
   /* create the node begin lock file name */
   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s.%s.begin",_nodeDataPtr->workdir,extName, _nodeDataPtr->datestamp); 

   /* For a container, we don't send the log file entry again if the
      status file already exists and if the signal is beginx */
   if( _nodeDataPtr->type != Task && _nodeDataPtr->type != NpassTask) {
      if( ( strcmp( _signal, "begin" ) == 0 ) ||
          ( strcmp( _signal, "beginx" ) == 0 && !isFileExists( filename, "setBeginState()") ) ) {

         nodebegin( _signal, _nodeDataPtr, _nodeDataPtr->datestamp );
      } 	
   } else {
      nodebegin( _signal, _nodeDataPtr, _nodeDataPtr->datestamp );
   }

   /* clear any other state */
   clearAllFinalStates( _nodeDataPtr, extName, "begin" ); 

   SeqUtil_TRACE( "maestro.go_begin() created lockfile %s\n", filename);
   touch(filename);

   /* for npasstask, we need to create a lock file without the extension
    * so that its container gets the same state */
   if( _nodeDataPtr->type == NpassTask) {
      sprintf(filename,"%s/%s.%s.begin",_nodeDataPtr->workdir, _nodeDataPtr->name, _nodeDataPtr->datestamp); 
      touch(filename);
   }

   free( extName );

}

/* 
processContainerBegin

 Treats the begin state of a container by checking the status of the siblings nodes around the targetted node.

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution

*/

static void processContainerBegin ( const SeqNodeDataPtr _nodeDataPtr ) {

   char filename[SEQ_MAXFIELD];
   LISTNODEPTR siblingIteratorPtr = NULL;
   SeqNameValuesPtr loopArgs = LOOP_ARGS;
   int abortedSibling = 0;
   char *nodeBase = NULL;
   if ( _nodeDataPtr->catchup == SEQ_CATCHUP_DISCR ) {   
      SeqUtil_TRACE( "maestro.processContainerBegin() bypassing discreet node:%s\n", _nodeDataPtr->name );
      return;
   }

   nodeBase = (char*) SeqUtil_getPathBase( (const char*) _nodeDataPtr->name );
   if ( _nodeDataPtr->type != CaseItem ) {
      if( _nodeDataPtr->loops != NULL ) {
         /* process loops containers */
         processLoopContainerBegin(_nodeDataPtr, loopArgs);
      } else {
         siblingIteratorPtr = _nodeDataPtr->siblings;
         /* process the siblings */
         while( siblingIteratorPtr != NULL && abortedSibling == 0 ) {
            sprintf(filename,"%s/%s/%s.%s.abort.stop", _nodeDataPtr->workdir, _nodeDataPtr->container, siblingIteratorPtr->data, _nodeDataPtr->datestamp);
            if ( access(filename, R_OK) == 0 ) {
               SeqUtil_TRACE( "maestro.processContainerBegin() found abort.stop file=%s\n", filename );
               abortedSibling = 1;
            }
            siblingIteratorPtr = siblingIteratorPtr->nextPtr;
         }
         if( abortedSibling == 0 ) {
            printf( "********** calling maestro -s beginx -n %s -f stop \n", nodeBase);
            maestro (nodeBase, "beginx", "stop" ,NULL, 0); 
         }
      }
   }
   else {
       /* caseItem always finish their container since only one should execute */
      maestro (nodeBase, "beginx", "stop", NULL, 0); 
   }
   free( nodeBase );
   SeqUtil_TRACE( "maestro.processContainerBegin() return value=%d\n", (abortedSibling == 1 ? 0 : 1) );
}

/* 
processLoopContainerBegin

 Treats the begin state of a container by checking the status of the siblings loop members around the targetted node.

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution
  _loop_args_ptr - pointer to the loop arguments being checked

*/

static void processLoopContainerBegin( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr loop_args_ptr) {
   char filename[SEQ_MAXFIELD];
   char *extension = NULL, *extWrite = NULL;
   int abortedChild = 0;
   LISTNODEPTR siblingIteratorPtr = NULL;
   int isParentLoop = SeqLoops_isParentLoopContainer( _nodeDataPtr );

   char* nodeBase = (char*) SeqUtil_getPathBase( (const char*) _nodeDataPtr->name );
   siblingIteratorPtr = _nodeDataPtr->siblings;
   memset( filename, '\0', sizeof filename );
   
   /* do we need to modify the loop extension */
   extension = (char*) SeqLoops_getExtensionBase( _nodeDataPtr );
   SeqUtil_TRACE( "processLoopContainerBegin() nodeBase=%s extension=%s\n", nodeBase, extension );
   if( strlen( extension ) > 0 ) {
      SeqUtil_stringAppend( &extWrite, "." );
      SeqUtil_stringAppend( &extWrite, extension );
   } else {
      SeqUtil_stringAppend( &extWrite, "" );
   }
   sprintf(filename,"%s/%s%s.%s.abort.stop", _nodeDataPtr->workdir, _nodeDataPtr->name, extWrite, _nodeDataPtr->datestamp);
   abortedChild = isFileExists( filename, "processLoopContainerBegin()" ) ;

   if( siblingIteratorPtr != NULL && abortedChild == 0 ) {
      /* need to process multile childs within loop context */
      while(  siblingIteratorPtr != NULL && abortedChild == 0 ) {
         sprintf(filename,"%s/%s/%s%s.%s.abort.stop", _nodeDataPtr->workdir, _nodeDataPtr->container, siblingIteratorPtr->data, extWrite, _nodeDataPtr->datestamp);
         abortedChild = isFileExists( filename, "processLoopContainerBegin()");
         siblingIteratorPtr = siblingIteratorPtr->nextPtr;
      }
   }

   if( abortedChild == 0 ) {
      //printf( "processLoopContainerEnd() sending \"maestro end %s%s\n", nodeBase, extension );
      if( _nodeDataPtr->type == Loop ) {
         /* remove the loop argument for the current loop first */
         SeqNameValues_deleteItem(&loop_args_ptr, _nodeDataPtr->nodeName );
      }
      printf( "********** calling maestro -s begin -n %s with loop args=%s\n", nodeBase, SeqLoops_getLoopArgs(loop_args_ptr)  );
      maestro ( nodeBase, "beginx", "stop", loop_args_ptr, 0 );
   }
   free( nodeBase );
   free( extWrite );
}

/*
go_end

This function is used to cause the node to enter the end state. Tasks submit their implicit dependencies in this state.
The function also deals with the parent containers' end state if conditions are met.

Inputs:
  signal - pointer to the value of the signal given to the binary (-s option)
  flow - pointer to the value of the flow given to the binary ( -f option)
  _nodeDataPtr - pointer to the node targetted by the execution

*/

static int go_end(char *_signal,char *_flow , const SeqNodeDataPtr _nodeDataPtr) {
   char *extensionBase = NULL; 
   int checkContainerEnd = 1;
   SeqNameValuesPtr newArgs = NULL;
   SeqUtil_TRACE( "maestro.go_end() node=%s signal=%s\n", _nodeDataPtr->name, _signal );
   
   actions( _signal, _flow, _nodeDataPtr->name );
   setEndState( _signal, _nodeDataPtr );

   if ( (_nodeDataPtr->type == Task || _nodeDataPtr->type == NpassTask) && (strcmp(_flow, "continue") == 0)) {
        submitNodeList(_nodeDataPtr->submits, LOOP_ARGS);
   } else if (_nodeDataPtr->type == Loop ) {
      if( isLoopComplete ( _nodeDataPtr, LOOP_ARGS ) ) {
         /* no loop attribute for the current node means we're done,
            no need to send maestro end again or else it's an infinite loop */
         if( ((char*) SeqLoops_getLoopAttribute( LOOP_ARGS, _nodeDataPtr->nodeName )) != NULL ) {
            extensionBase = (char*) SeqLoops_getExtensionBase( _nodeDataPtr );
            newArgs = (SeqNameValuesPtr) SeqLoops_convertExtension( _nodeDataPtr->loops, extensionBase );
            SeqNameValues_printList( newArgs );
            maestro ( _nodeDataPtr->name, _signal, _flow, newArgs, 0 );
         }
      } else {
	 /* if loop is not done, no need to check parent end */
         checkContainerEnd  = 0;
         /* get next loop extension to submit only if we have an extension for the current loop and it
            has not reached the end */
         if( ((char*) SeqLoops_getLoopAttribute( LOOP_ARGS, _nodeDataPtr->nodeName )) != NULL && 
             !SeqLoops_isLastIteration( _nodeDataPtr, LOOP_ARGS ) ) {
            if( (newArgs = (SeqNameValuesPtr) SeqLoops_nextLoopArgs( _nodeDataPtr, LOOP_ARGS )) != NULL && (strcmp(_flow, "continue") == 0)) {
               maestro (_nodeDataPtr->name, "submit", _flow, newArgs, 0);
            }
         }
      }
   }

   /* check if the container has been completed by the end of this */
   if ( strcmp( _nodeDataPtr->container, "" ) != 0 && checkContainerEnd ) {
      processContainerEnd( _nodeDataPtr, _flow );
   }

   /* submit nodes waiting for this one to end */
   submitDependencies( _nodeDataPtr, "end" );

   free( extensionBase );
   SeqNameValues_deleteWholeList( &newArgs );
   actionsEnd( _signal, _flow, _nodeDataPtr->name );
   return 0;
}

/* 
setEndState

Deals with the status files associated with the end state by clearing the other states then putting the node in end state.

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution

*/

static void setEndState(const char* _signal, const SeqNodeDataPtr _nodeDataPtr) {

   char filename[SEQ_MAXFIELD];
   char *extName = NULL ;

   SeqUtil_stringAppend( &extName, _nodeDataPtr->name );
   if( strlen( _nodeDataPtr->extension ) > 0 ) {
      SeqUtil_stringAppend( &extName, "." );
      SeqUtil_stringAppend( &extName, _nodeDataPtr->extension );
   }      

   /* create the node end lock file name*/
   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s.%s.end",_nodeDataPtr->workdir,extName, _nodeDataPtr->datestamp); 

   /* For a container, we don't send the log file entry again if the
      status file already exists and the signal is endx */
   if( _nodeDataPtr->type != Task && _nodeDataPtr->type != NpassTask) {
      if( ( strcmp( _signal, "end" ) == 0 ) ||
          ( strcmp( _signal, "endx" ) == 0 && !isFileExists( filename, "setEndState()") ) ) {

         nodeend( _signal, _nodeDataPtr, _nodeDataPtr->datestamp );
      }
   } else {
      nodeend( _signal, _nodeDataPtr, _nodeDataPtr->datestamp );
   }

   /* clear any other state */
   clearAllFinalStates( _nodeDataPtr, extName, "end" ); 

   SeqUtil_TRACE( "maestro.go_end() created lockfile %s\n", filename);
   touch(filename);

   /* for npasstask, we need to create a lock file without the extension
    * so that its container gets the same state */
   if( _nodeDataPtr->type == NpassTask) {
      SeqUtil_TRACE( "maestro.go_end() entering npass lockfile logic, args: workdir=%s name=%s loop_ext=%s datestamp=%s\n",_nodeDataPtr->workdir, _nodeDataPtr->name, (char *) SeqLoops_getExtensionBase(_nodeDataPtr), _nodeDataPtr->datestamp );
      sprintf(filename,"%s/%s.%s.%s.end",_nodeDataPtr->workdir, _nodeDataPtr->name, (char *) SeqLoops_getExtensionBase(_nodeDataPtr), _nodeDataPtr->datestamp); 
      SeqUtil_TRACE( "maestro.go_end() creating npass lockfile=%s\n", filename);
      touch(filename);
   }
   free( extName );

}

/* 
clearAllFinalStates

Clears all the terminal states files.

Inputs:
  fullNodeName - pointer to the node's full name (i.e. /testsuite/assimilation/00/randomjob )
  originator - pointer to the name of the originating function that called this

*/


static void clearAllFinalStates (const SeqNodeDataPtr _nodeDataPtr, char * fullNodeName, char * originator ) {

   char filename[SEQ_MAXFIELD];
   memset(filename,'\0',sizeof filename);

   /* remove the node begin lock file */
   sprintf(filename,"%s/%s.%s.begin",_nodeDataPtr->workdir,fullNodeName, _nodeDataPtr->datestamp); 
   if ( access(filename, R_OK) == 0 ) {
      SeqUtil_TRACE( "maestro.go_%s() removed lockfile %s\n", originator, filename);
      removeFile(filename);
   }
   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s.%s.end",_nodeDataPtr->workdir,fullNodeName, _nodeDataPtr->datestamp); 
   if ( access(filename, R_OK) == 0 ) {
      SeqUtil_TRACE( "maestro.go_%s() removed lockfile %s\n", originator, filename);
      removeFile(filename);
   }

   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s.%s.abort.stop",_nodeDataPtr->workdir,fullNodeName, _nodeDataPtr->datestamp); 
   if ( access(filename, R_OK) == 0 ) {
      SeqUtil_TRACE( "maestro.go_%s() removed lockfile %s\n", originator, filename);
      removeFile(filename);
   }

   memset(filename,'\0',sizeof filename);
/* Notice that clearing submit will cause a concurrency vs NFS problem when we add dependency */
   sprintf(filename,"%s/%s.%s.submit",_nodeDataPtr->workdir,fullNodeName, _nodeDataPtr->datestamp); 
   if ( access(filename, R_OK) == 0 ) {
      SeqUtil_TRACE( "maestro.go_%s() removed lockfile %s\n", originator, filename);
      removeFile(filename);
   }

   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s.%s.waiting",_nodeDataPtr->workdir,fullNodeName, _nodeDataPtr->datestamp); 
   if ( access(filename, R_OK) == 0 ) {
      SeqUtil_TRACE( "maestro.go_%s() removed lockfile %s\n", originator, filename);
      removeFile(filename);
   }
}

/* 
isLoopComplete

 returns 1 if all iteration of the loop is complete in terms of iteration status file
 returns 0 otherwise

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution
  _loop_args - pointer to the loop arguments being checked
  
*/

static int isLoopComplete ( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args ) {
   char endfile[SEQ_MAXFIELD];
   char continuefile[SEQ_MAXFIELD];
   LISTNODEPTR extensions = NULL;
   int undoneIteration = 0;

   memset( endfile, '\0', sizeof endfile );
   memset( continuefile, '\0', sizeof continuefile );

   /* check if the loop is completed */
   extensions = (LISTNODEPTR) SeqLoops_childExtensions( _nodeDataPtr, _loop_args );
   if( extensions != NULL ) {
      while( extensions != NULL && undoneIteration == 0 ) {
         sprintf(endfile,"%s/%s.%s.%s.end", _nodeDataPtr->workdir, _nodeDataPtr->name, extensions->data, _nodeDataPtr->datestamp);
         sprintf(continuefile,"%s/%s.%s.%s.abort.cont", _nodeDataPtr->workdir, _nodeDataPtr->name, extensions->data, _nodeDataPtr->datestamp);
         SeqUtil_TRACE( "maestro.isLoopComplete() loop done? checking for:%s or %s\n", endfile, continuefile);
         undoneIteration = ! ( isFileExists( endfile, "isLoopComplete()" ) || isFileExists( continuefile, "isLoopComplete()" ) ) ;
         extensions = extensions->nextPtr;
      }
   }

   SeqUtil_TRACE( "maestro.isLoopComplete() return value=%d\n", (! undoneIteration) );
   return ! undoneIteration;
}

/* 
processContainerEnd

 Treats the end of a container by checking the status of the siblings around the targetted node.

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution
  _flow - pointer to the flow input argument (continue or stop)
  
*/

static void processContainerEnd ( const SeqNodeDataPtr _nodeDataPtr, char *_flow ) {

   char endfile[SEQ_MAXFIELD];
   char continuefile[SEQ_MAXFIELD];
   char tmp[SEQ_MAXFIELD];
   LISTNODEPTR siblingIteratorPtr = NULL;
   SeqNodeDataPtr siblingDataPtr = NULL;
   SeqNameValuesPtr loopArgs = LOOP_ARGS;
   int undoneChild = 0;
   char* nodeBase = (char*) SeqUtil_getPathBase( (const char*) _nodeDataPtr->name );
   memset( endfile, '\0', sizeof endfile );
   memset( continuefile, '\0', sizeof continuefile );
   memset( tmp, '\0', sizeof tmp);

   if ( _nodeDataPtr->type != CaseItem ) {
      if( _nodeDataPtr->loops != NULL ) {
         /* process loops containers */
         processLoopContainerEnd( _nodeDataPtr, loopArgs, _flow );
      } else {
         siblingIteratorPtr = _nodeDataPtr->siblings;
         /* process the siblings */
         while( siblingIteratorPtr != NULL && undoneChild == 0 ) {
            sprintf(endfile,"%s/%s/%s.%s.end", _nodeDataPtr->workdir, _nodeDataPtr->container, siblingIteratorPtr->data, _nodeDataPtr->datestamp);
            sprintf(continuefile,"%s/%s/%s.%s.abort.cont", _nodeDataPtr->workdir, _nodeDataPtr->container, siblingIteratorPtr->data, _nodeDataPtr->datestamp);
            if (( access(endfile, R_OK) == 0 ) || ( access(continuefile, R_OK) == 0 ) )  {
               SeqUtil_TRACE( "maestro.processContainerEnd() found end file=%s or abort.continue file=%s \n", endfile, continuefile );
               siblingIteratorPtr = siblingIteratorPtr->nextPtr;
            } else {
	       /* check if it's a discretionary job, bypass if yes */ 
	       sprintf(tmp, "%s/%s", _nodeDataPtr->container, siblingIteratorPtr->data); 
               SeqUtil_TRACE( "maestro.processContainerEnd() getting sibling info: %s\n", tmp );
	       siblingDataPtr = nodeinfo( tmp, "type,res" );
	       if ( siblingDataPtr->catchup == SEQ_CATCHUP_DISCR ) {
                  SeqUtil_TRACE( "maestro.processContainerEnd() bypassing discretionary task: %s\n", siblingIteratorPtr->data );
                  siblingIteratorPtr = siblingIteratorPtr->nextPtr;
	       } else {
                  SeqUtil_TRACE( "maestro.processContainerEnd() missing end file=%s or abort.continue file=%s\n", endfile, continuefile );
                  undoneChild = 1;
	       }
            }
         }
         if( undoneChild == 0 ) {
            printf( "********** calling maestro -s endx -n %s -f %s\n", nodeBase, _flow );
            maestro (nodeBase, "endx", _flow ,NULL, 0); 
         }
      }
   }
   else {
       /* caseItem always finish their container since only one should execute */
      maestro (nodeBase, "end", _flow, NULL, 0); 
   }
   free( nodeBase );
   SeqUtil_TRACE( "maestro.processContainerEnd() return value=%d\n", (undoneChild == 1 ? 0 : 1) );
}

/* 
processLoopContainerEnd

 Treats the end of a loop container by checking the status of the loop members.

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution
  _loop_args_ptr - pointer to the loop arguments being checked
  _flow - pointer to the flow input argument (continue or stop)
  
*/

static void processLoopContainerEnd( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr loop_args_ptr, char *_flow) {
   char endfile[SEQ_MAXFIELD];
   char continuefile[SEQ_MAXFIELD];
   char tmp[SEQ_MAXFIELD];
   char *extension = NULL, *extWrite = NULL;
   int undoneChild = 0;
   LISTNODEPTR siblingIteratorPtr = NULL;
   SeqNodeDataPtr siblingDataPtr = NULL;
   int isParentLoop = SeqLoops_isParentLoopContainer( _nodeDataPtr );

   char* nodeBase = (char*) SeqUtil_getPathBase( (const char*) _nodeDataPtr->name );
   siblingIteratorPtr = _nodeDataPtr->siblings;
   memset( endfile, '\0', sizeof endfile );
   memset( continuefile, '\0', sizeof continuefile );
   memset( tmp, '\0', sizeof tmp );

   /* do we need to modify the loop extension */
   extension = (char*) SeqLoops_getExtensionBase( _nodeDataPtr );
   SeqUtil_TRACE( "processLoopContainerEnd() nodeBase=%s extension=%s\n", nodeBase, extension );
   if( strlen( extension ) > 0 ) {
      SeqUtil_stringAppend( &extWrite, "." );
      SeqUtil_stringAppend( &extWrite, extension );
   } else {
      SeqUtil_stringAppend( &extWrite, "" );
   }

   sprintf(endfile,"%s/%s%s.%s.end", _nodeDataPtr->workdir, _nodeDataPtr->name, extWrite, _nodeDataPtr->datestamp);
   sprintf(continuefile,"%s/%s%s.%s.abort.cont", _nodeDataPtr->workdir, _nodeDataPtr->name, extWrite, _nodeDataPtr->datestamp);
   undoneChild = ! (isFileExists( endfile, "processLoopContainerEnd()" ) || isFileExists( continuefile, "processLoopContainerEnd()") );
   if( siblingIteratorPtr != NULL && undoneChild == 0 ) {
      /* need to process multile childs within loop context */
      while(  siblingIteratorPtr != NULL && undoneChild == 0 ) {
         sprintf(endfile,"%s/%s/%s%s.%s.end", _nodeDataPtr->workdir, _nodeDataPtr->container, siblingIteratorPtr->data, extWrite, _nodeDataPtr->datestamp);
         sprintf(continuefile,"%s/%s/%s%s.%s.abort.cont", _nodeDataPtr->workdir, _nodeDataPtr->container, siblingIteratorPtr->data, extWrite, _nodeDataPtr->datestamp);
         undoneChild = ! (isFileExists( endfile, "processLoopContainerEnd()") || isFileExists( continuefile, "processLoopContainerEnd()") );
	 if ( undoneChild  ) {
            /* check if it's a discretionary job, bypass if yes */
            sprintf(tmp, "%s/%s", _nodeDataPtr->container, siblingIteratorPtr->data);
            SeqUtil_TRACE( "maestro.processLoopContainerEnd() getting sibling info: %s\n", tmp );
            siblingDataPtr = nodeinfo( tmp, "type,res" );
            if ( siblingDataPtr->catchup == SEQ_CATCHUP_DISCR ) {
	       undoneChild = 0;
               SeqUtil_TRACE( "maestro.processLoopContainerEnd() bypassing discretionary task: %s\n", siblingIteratorPtr->data );
               siblingIteratorPtr = siblingIteratorPtr->nextPtr;
            }
         } else {
            siblingIteratorPtr = siblingIteratorPtr->nextPtr;
         }
      }
   }

   if( undoneChild == 0 ) {
      //printf( "processLoopContainerEnd() sending \"maestro end %s%s\n", nodeBase, extension );
      if( _nodeDataPtr->type == Loop ) {
         /* remove the loop argument for the current loop first */
         SeqNameValues_deleteItem(&loop_args_ptr, _nodeDataPtr->nodeName );
      }
      printf( "********** calling maestro -s endx -n %s with loop args=%s\n", nodeBase, SeqLoops_getLoopArgs(loop_args_ptr) );
      maestro ( nodeBase, "endx", _flow, loop_args_ptr, 0 );
   }
   free( nodeBase );
   free( extWrite );
}

/*
go_submit

This function is used to cause the node to enter the submit state. Checks dependencies of the node then submits to batch system.

Inputs:
  signal - pointer to the value of the signal given to the binary (-s option)
  flow - pointer to the value of the flow given to the binary ( -f option)
  _nodeDataPtr - pointer to the node targetted by the execution
  ignoreAllDeps - integer representing True or False whether dependencies are ignored

Returns the error status of the ord_soumet call.

*/

static int go_submit(const char *_signal, char *_flow , const SeqNodeDataPtr _nodeDataPtr, int ignoreAllDeps) {
   char tmpfile[SEQ_MAXFIELD], noendwrap[12], nodeFullPath[SEQ_MAXFIELD];
   char listingDir[SEQ_MAXFIELD];
   char *tmpCfgFile = NULL;
   char cmd[1000];
   char *loopArgs = NULL, *extName = NULL;
   int catchup = SEQ_CATCHUP_NORMAL;
   int status = 0;
   SeqUtil_TRACE( "maestro.go_submit() node=%s signal=%s flow=%s ignoreAllDeps=%d\n ", _nodeDataPtr->name, _signal, _flow, ignoreAllDeps );
   actions( (char*) _signal, _flow, _nodeDataPtr->name );
   memset(nodeFullPath,'\0',sizeof nodeFullPath);
   memset(listingDir,'\0',sizeof listingDir);
   sprintf( nodeFullPath, "%s/modules/%s.tsk", SEQ_EXP_HOME, _nodeDataPtr->taskPath );
   sprintf( listingDir, "%s/sequencing/output/%s", SEQ_EXP_HOME, _nodeDataPtr->container );
   /* check catchup value of the node */
   printf("node catchup= %d , normal catchup = %d , discretionary catchup = %d  \n",_nodeDataPtr->catchup, catchup, SEQ_CATCHUP_DISCR );
   if (_nodeDataPtr->catchup > catchup && !ignoreAllDeps ) {
      if (_nodeDataPtr->catchup == SEQ_CATCHUP_DISCR ) {
         printf("nodelogger: -n %s -l %s \"%s\"\n", _nodeDataPtr->name, _nodeDataPtr->extension, CATCHUP_DISCR_MSG );
         nodelogger( _nodeDataPtr->name ,"catchup", _nodeDataPtr->extension, CATCHUP_DISCR_MSG,_nodeDataPtr->datestamp);
      } else {
         printf("nodelogger: -n %s -l %s \"%s\"\n", _nodeDataPtr->name, _nodeDataPtr->extension, CATCHUP_UNSUBMIT_MSG );
         nodelogger( _nodeDataPtr->name ,"info", _nodeDataPtr->extension, CATCHUP_UNSUBMIT_MSG,_nodeDataPtr->datestamp);
      }
      return(0);
   }
   if( validateDependencies( _nodeDataPtr ) == 0 || ignoreAllDeps == 1 ) {
      /* dependencies are satisfied */
      loopArgs = (char*) SeqLoops_getLoopArgs( LOOP_ARGS );
   
      setSubmitState( _nodeDataPtr ) ;
      tmpCfgFile = generateConfig( _nodeDataPtr, _flow );

      /* go and submit the job */

      SeqUtil_stringAppend( &extName, _nodeDataPtr->nodeName );
      if( strlen( _nodeDataPtr->extension ) > 0 ) {
           SeqUtil_stringAppend( &extName, "." );
           SeqUtil_stringAppend( &extName, _nodeDataPtr->extension );
      }

      if ( _nodeDataPtr->type == Task || _nodeDataPtr->type == NpassTask ) {
         sprintf(cmd,"%s -sys maestro -jobfile %s -node %s -jn %s -d %s -q %s -p %d -c %s -m %s -w %d -v -listing %s -wrapdir %s/sequencing -jobcfg %s -args \"%s\"",OCSUB, nodeFullPath, _nodeDataPtr->name, extName,_nodeDataPtr->machine,_nodeDataPtr->queue,      _nodeDataPtr->mpi,_nodeDataPtr->cpu,_nodeDataPtr->memory,_nodeDataPtr->wallclock, listingDir, SEQ_EXP_HOME, tmpCfgFile, _nodeDataPtr->args);
         printf( "%s\n", cmd );
         SeqUtil_TRACE("maestro.go_submit() cmd_length=%d %s\n",strlen(cmd), cmd);
         status = system(cmd);
         nodesubmit(_nodeDataPtr, _nodeDataPtr->datestamp);
      } else {
         memset( noendwrap, '\0', sizeof( noendwrap ) );
         memset(tmpfile,'\0',sizeof tmpfile);
         sprintf(tmpfile,"%s/sequencing/tmpfile/container.tsk",SEQ_EXP_HOME);
         touch(tmpfile);         
         _nodeDataPtr->submits == NULL ? strcpy( noendwrap, "" ) : strcpy( noendwrap, "-noendwrap" ) ;
         sprintf(cmd,"%s -sys maestro -jobfile %s -node %s -jn %s -d %s -q %s -p %d -c %s -m %s -w %d -v -listing %s -wrapdir %s/sequencing -immediate %s -jobcfg %s -args \"%s\"",OCSUB,tmpfile,_nodeDataPtr->name, extName, _nodeDataPtr->machine,_nodeDataPtr->queue,_nodeDataPtr->mpi,_nodeDataPtr->cpu,_nodeDataPtr->memory,_nodeDataPtr->wallclock, listingDir, SEQ_EXP_HOME, noendwrap, tmpCfgFile, _nodeDataPtr->args);
         printf( "%s\n", cmd );
         SeqUtil_TRACE("maestro.go_submit() cmd_length=%d %s\n",strlen(cmd), cmd);
         status=system(cmd);
         nodesubmit(_nodeDataPtr, _nodeDataPtr->datestamp);
      }
      /* don't forget to remove config file later on */
   }
   actionsEnd( (char*) _signal, _flow, _nodeDataPtr->name );
   free( tmpCfgFile );
   free( extName );
   return(status);
}

/*
setSubmitState

Deals with the status files associated with the submit state by clearing the other states then putting the node in submit state.

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution

*/

static void setSubmitState(const SeqNodeDataPtr _nodeDataPtr) {

   /* Notice that clearing submit will cause a concurrency vs NFS problem when we add dependency */

   char *extName = NULL ;
   char filename[SEQ_MAXFIELD];

   SeqUtil_stringAppend( &extName, _nodeDataPtr->name );
   if( strlen( _nodeDataPtr->extension ) > 0 ) {
      SeqUtil_stringAppend( &extName, "." );
      SeqUtil_stringAppend( &extName, _nodeDataPtr->extension );
   }      

   /* clear any other state */
   clearAllFinalStates( _nodeDataPtr, extName, "submit" ); 

   /* create the node end lock file */
   memset(filename,'\0',sizeof filename);

   sprintf(filename,"%s/%s.%s.submit",_nodeDataPtr->workdir,extName, _nodeDataPtr->datestamp); 
   SeqUtil_TRACE( "maestro.go_submit() created lockfile %s\n", filename);
   touch(filename);

   free( extName );

}

/*
submitNodeList

Submits a list of nodes.

Inputs:
  listToSubmit - pointer to a list of nodes to be executed
  loop_args_ptr - pointer to loop arguments of the nodes to be submitted
                  if the pointer contains more than one argument, it is considered
		  as the indexes of an initial loop set submits

*/

static void submitNodeList ( const LISTNODEPTR listToSubmit, SeqNameValuesPtr loop_args_ptr ) {
   LISTNODEPTR listIteratorPtr = listToSubmit;
   while (listIteratorPtr != NULL) {
      SeqUtil_TRACE( "maestro.submitNodeList() submits node %s\n", listIteratorPtr->data );
      maestro ( listIteratorPtr->data, "submit", "continue" , loop_args_ptr, 0 );
      listIteratorPtr =  listIteratorPtr->nextPtr;
   }
}

/*
submitNodeList

Submits a list of nodes.

Inputs:
  listToSubmit - pointer to a list of nodes to be executed
  container_args_ptr -  pointer to container loops
  loopset_index - list of index for the set iterations
*/
static void submitLoopSetNodeList ( const LISTNODEPTR listToSubmit, 
                                    SeqNameValuesPtr container_args_ptr, SeqNameValuesPtr loopset_index ) {
   LISTNODEPTR listIteratorPtr = NULL;
   SeqNameValuesPtr myLoopArgsPtr = loopset_index;
   SeqNameValuesPtr cmdLoopArg = NULL;
   SeqUtil_TRACE( "maestro.submitLoopSetNodeList() container_args_ptr=%s loopset_index=%s\n", SeqLoops_getLoopArgs(container_args_ptr), SeqLoops_getLoopArgs( loopset_index ) );
   while ( myLoopArgsPtr != NULL) {
      /* first get the parent container loop arguments */
      if( container_args_ptr != NULL ) {
         cmdLoopArg = SeqNameValues_clone( container_args_ptr );
      }
      /* then add the current loop argument */
      SeqNameValues_insertItem( &cmdLoopArg, myLoopArgsPtr->name, myLoopArgsPtr->value );
      /*now submit the child nodes */
      listIteratorPtr = listToSubmit;
      while (listIteratorPtr != NULL) {
         maestro ( listIteratorPtr->data, "submit", "continue" , cmdLoopArg, 0 );
	 SeqUtil_TRACE( "submitLoopSetNodeList calling maestro -n %s -s submit -l %s -f continue\n", listIteratorPtr->data, SeqLoops_getLoopArgs( cmdLoopArg));
         listIteratorPtr =  listIteratorPtr->nextPtr;
      }
      cmdLoopArg = NULL;
      myLoopArgsPtr = myLoopArgsPtr->nextPtr;
   }
}

/*
/*
setWaitingState

Deals with the status files associated with the waiting state by clearing the other states then putting the node in waiting state.

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution

*/
static void setWaitingState(const SeqNodeDataPtr _nodeDataPtr, const char* waited_one, const char* waited_status) {

   char *extName = NULL, *waitMsg = NULL;
   char filename[SEQ_MAXFIELD];

   waitMsg = malloc ( strlen( waited_one ) + strlen( waited_status ) + 2);

   SeqUtil_stringAppend( &extName, _nodeDataPtr->name );
   if( strlen( _nodeDataPtr->extension ) > 0 ) {
      SeqUtil_stringAppend( &extName, "." );
      SeqUtil_stringAppend( &extName, _nodeDataPtr->extension );
   }      

   /* create the node waiting lock file name*/
   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s.%s.waiting",_nodeDataPtr->workdir,extName, _nodeDataPtr->datestamp); 

   sprintf( waitMsg, "%s %s", waited_status, waited_one );
   nodewait( _nodeDataPtr, waitMsg, _nodeDataPtr->datestamp);  

   /* clear any other state */
   clearAllFinalStates( _nodeDataPtr, extName, "waiting" ); 

   SeqUtil_TRACE( "maestro.setWaitingState() created lockfile %s\n", filename);
   touch(filename);

   free( extName );
   free( waitMsg );
}

/*
submitDependencies

 Checks nodes that are dependent on this node and the nodes for which the dependencies have been satisfied

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution
  _signal - pointer to the signal being checked

*/
static void submitDependencies ( const SeqNodeDataPtr _nodeDataPtr, const char* _signal ) {
   char line[512];
   FILE* waitedFile = NULL;
   char depUser[12], depExp[256], depNode[256], depArgs[SEQ_MAXFIELD], depDatestamp[20];
   char filename[SEQ_MAXFIELD], submitCmd[SEQ_MAXFIELD];
   SeqNameValuesPtr loopArgsPtr = NULL;
   char *extName = NULL, *submitDepArgs = NULL;
   int submitCode = 0;

   SeqUtil_stringAppend( &extName, _nodeDataPtr->name );
   if( strlen( _nodeDataPtr->extension ) > 0 ) {
      SeqUtil_stringAppend( &extName, "." );
      SeqUtil_stringAppend( &extName, _nodeDataPtr->extension );
   }      

   memset(filename,'\0',sizeof depUser);
   memset(depUser,'\0',sizeof depUser);
   memset(depExp,'\0',sizeof depExp);
   memset(depNode,'\0',sizeof depNode);
   memset(depDatestamp,'\0',sizeof depDatestamp);
   memset(depArgs,'\0',sizeof depArgs);
   memset(submitCmd,'\0',sizeof submitCmd);

   sprintf(filename,"%s/%s.%s.waited_%s", _nodeDataPtr->workdir,  extName, _nodeDataPtr->datestamp, _signal );
   printf( "maestro.submitDependencies() looking for waited file=%s\n", filename );
   if ( access(filename, R_OK) == 0 ) {
      printf( "maestro.submitDependencies() found waited file=%s\n", filename );
      /* build a node list for all entries found in the waited file */
      if ((waitedFile = fopen(filename,"r")) != NULL ) {
         while ( fgets( line, sizeof(line), waitedFile ) != NULL ) {
            printf( "maestro.submitDependencies() from waited file line: %s\n", line );
            sscanf( line, "user=%s exp=%s node=%s datestamp=%s args=%s", 
               &depUser, &depExp, &depNode, &depDatestamp, &depArgs );
            printf( "maestro.submitDependencies() waited file data depUser:%s depExp:%s depNode:%s depDatestamp:%s depArgs:%s\n", 
               depUser, depExp, depNode, depDatestamp, depArgs );
            if ( strlen( depArgs ) > 0 ) {
	       SeqUtil_stringAppend( &submitDepArgs, "-l" );
	    }
	    SeqUtil_stringAppend( &submitDepArgs, depArgs );
            if( strcmp( depUser, USERNAME ) == 0 && strcmp( depExp, EXPNAME ) != 0 ) {
               /* different exp, same user */
	       if ( getenv("SEQ_BIN") != NULL ) {
                   sprintf( submitCmd, "(export SEQ_EXP_HOME=%s;export SEQ_DATE=%s; %s/maestro -s submit -f continue -n %s %s)", 
		                        depExp, depDatestamp, getenv("SEQ_BIN"), depNode, submitDepArgs );
               } else {
                   sprintf( submitCmd, "(export SEQ_EXP_HOME=%s;export SEQ_DATE=%s;maestro -s submit -f continue -n %s %s)", 
		                        depExp, depDatestamp, depNode, submitDepArgs );
	       }
	       printf( "submitDependencies cmd: %s\n", submitCmd );
               submitCode = system(submitCmd);
            } else {
               /* for now, we treat the rest as same exp, same user */
               /* maestro ( depNode, "submit", "continue" , loopArgsPtr ); */
	       if ( getenv("SEQ_BIN") != NULL ) {
                  sprintf( submitCmd, "(export SEQ_DATE=%s; %s/maestro -s submit -f continue -n %s %s)", depDatestamp, getenv("SEQ_BIN"), depNode, submitDepArgs );
               } else {
                  sprintf( submitCmd, "(export SEQ_DATE=%s;maestro -s submit -f continue -n %s %s)", depDatestamp, depNode, submitDepArgs );
	       }
	       printf( "submitDependencies cmd: %s\n", submitCmd );
               submitCode = system(submitCmd);
            }
	       printf( "submitDependencies submitCode: %d\n", submitCode);
            if( submitCode != 0 ) {
	       raiseError( "An error happened while submitting dependant nodes error number: %d\n", submitCode );
            }
	    submitDepArgs = NULL;
         }
         fclose(waitedFile);

         /* we don't need to keep the file */
         removeFile(filename);
      } else {
         raiseError( "maestro cannot read file: %s (submitDependencies) \n", filename );
      }
   }
   free(extName);
}

/*
writeNodeWaitedFile

Writes the dependency lockfile in the directory of the node that this current node is waiting for.

Inputs:
   dep_user - the user id where the dependant node belongs
   dep_exp_path - the SEQ_EXP_HOME of the dependant node
   dep_node - the path of the node including the container
   dep_status - the status that the node is waiting for (end,abort,etc)
   dep_index - the loop index that this node is waiting for (+1+6)
*/

static int writeNodeWaitedFile(  const SeqNodeDataPtr _nodeDataPtr, char* dep_exp_path,
                              char* dep_node, char* dep_status,
                              char* dep_index, char* dep_datestamp ) {
   struct passwd *current_passwd = NULL;
   FILE *waitingFile = NULL;
   char filename[SEQ_MAXFIELD];
   char tmp_line[SEQ_MAXFIELD];
   char line[SEQ_MAXFIELD];
   char *loopArgs = NULL, *depBase = NULL;
   int found = 0;
   memset(filename,'\0',sizeof filename);
   memset(tmp_line,'\0',sizeof tmp_line);
   memset(line,'\0',sizeof tmp_line);

   SeqUtil_TRACE("maestro.writeNodeWaitedFile() dep_exp_path=%s, dep_node=%s, dep_index=%s dep_datestamp=%s\n",
      dep_exp_path, dep_node, dep_index, dep_datestamp);

   current_passwd = getpwuid(getuid());

   /* create dirs if not there */
   depBase = (char*) SeqUtil_getPathBase( (const char*) dep_node );
   sprintf(filename,"%s/sequencing/status/%s", dep_exp_path, depBase );
   SeqUtil_mkdir( filename, 1 );
   
   sprintf(filename,"%s/sequencing/status/%s%s.%s.waited_%s",
      dep_exp_path, dep_node, dep_index, dep_datestamp, dep_status);

   if ( ! (access(filename, R_OK) == 0) ) {
      printf( "maestro.writeNodeWaitedFile creating %s\n", filename );
      touch( filename );
   }
   if ((waitingFile = fopen(filename,"r+")) == NULL) {
      raiseError( "maestro cannot write to file:%s\n",filename );
   }
   printf( "maestro.writeNodeWaitedFile updating %s\n", filename );
   /* sua need to add more logic for duplication and handle more than one entry in the waited file */
   loopArgs = (char*) SeqLoops_getLoopArgs( LOOP_ARGS );
   sprintf( tmp_line, "user=%s exp=%s node=%s datestamp=%s args=%s\n", current_passwd->pw_name, 
      SEQ_EXP_HOME, _nodeDataPtr->name, _nodeDataPtr->datestamp, loopArgs );
   while( fgets(line, SEQ_MAXFIELD, waitingFile) != NULL ) {
      if( strcmp( line, tmp_line ) == 0 ) { 
         found = 1;
         break;
      }
   }
   if( found == 0 ) { fprintf( waitingFile, tmp_line ); }
   fclose( waitingFile );
   free( loopArgs );
   free( depBase );
   return(0);
}

/* 
validateDependencies

Checks if the dependencies are satisfied for a given node. 

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution

*/

static int validateDependencies (const SeqNodeDataPtr _nodeDataPtr) {
   int isWaiting = 0;
   char filename[SEQ_MAXFIELD];
   char *depName = NULL, *depStatus = NULL, *depUser = NULL, *depExp = NULL,
        *depIndex = NULL, *loopIndexString = NULL, *tmpExt = NULL, *depHour = NULL,
        *expPath = NULL, *localIndex = NULL, *localIndexString = NULL;
   char *depNameMsg = NULL, *depDatestamp = NULL;
   SeqDependenciesPtr depsPtr = NULL;
   SeqNameValuesPtr nameValuesPtr = NULL, loopArgsPtr = NULL;
   SeqDependsScope depScope = IntraSuite;
   struct passwd* current_passwd = NULL;

   memset(filename,'\0',sizeof filename);
   current_passwd = getpwuid(getuid());
   /* check dependencies */
   depsPtr = _nodeDataPtr->depends;
   while( depsPtr != NULL && isWaiting == 0 ) {
      nameValuesPtr =  depsPtr->dependencyItem;


      if ( depsPtr->type == NodeDependancy || depsPtr->type == NpassDependancy ) {
         SeqUtil_TRACE( "maestro.validateDependencies() nodeinfo_depend_type=Node\n" );
         depScope = IntraSuite;
         depName = SeqNameValues_getValue( nameValuesPtr, "NAME" );
         depIndex = SeqNameValues_getValue( nameValuesPtr, "INDEX" );
         depStatus = SeqNameValues_getValue( nameValuesPtr, "STATUS" );
         depUser = SeqNameValues_getValue( nameValuesPtr, "USER" );
         depExp = SeqNameValues_getValue( nameValuesPtr, "EXP" );
         depHour = SeqNameValues_getValue( nameValuesPtr, "HOUR" );
         localIndex = SeqNameValues_getValue( nameValuesPtr, "LOCAL_INDEX" );
         if( depUser == NULL || strlen( depUser ) == 0 ) {
            depUser = strdup( current_passwd->pw_name );
         } else {
            depScope = InterUser;
         }
         if( depExp == NULL || strlen( depExp ) == 0 ) {
            depExp = (char*) SeqUtil_getPathLeaf( (const char*) (SEQ_EXP_HOME) );
         } else if( depScope != InterUser ) {
            depScope = IntraUser;
            SeqUtil_stringAppend( &depNameMsg, "exp=");
            SeqUtil_stringAppend( &depNameMsg, depExp ); 
            SeqUtil_stringAppend( &depNameMsg, " ");
         }
         /* if I'm dependant on a loop iteration, need to process it */
         SeqUtil_stringAppend( &loopIndexString, "" );
         SeqUtil_stringAppend( &depNameMsg, "node=");
         SeqUtil_stringAppend( &depNameMsg, depName );
         if( depIndex != NULL && strlen( depIndex ) > 0 ) {
            printf( "maestro.validateDependencies() depIndex=%s\n", depIndex );
            SeqLoops_parseArgs(&loopArgsPtr, depIndex);
            SeqUtil_stringAppend( &loopIndexString, "." );
            tmpExt = (char*) SeqLoops_getExtFromLoopArgs(loopArgsPtr);
            SeqUtil_stringAppend( &loopIndexString, tmpExt );
            SeqUtil_stringAppend( &depNameMsg, loopIndexString);
            free(tmpExt);
         }
         /* if I'm a loop iteration, process it */
         SeqUtil_stringAppend( &localIndexString, "" );
         if( localIndex != NULL && strlen( localIndex ) > 0 ) {
            printf( "maestro.validateDependencies() localIndex=%s\n", localIndex );
            loopArgsPtr = NULL;
            SeqLoops_parseArgs(&loopArgsPtr, localIndex);
            tmpExt = (char*) SeqLoops_getExtFromLoopArgs(loopArgsPtr);
            SeqUtil_stringAppend( &localIndexString, tmpExt );
            free(tmpExt);
         }
	 if( depHour != NULL && strlen(depHour) > 0 ) {
	    /* calculate relative datestamp based on the current one */
	    depDatestamp = SeqDatesUtil_getPrintableDate( _nodeDataPtr->datestamp, atoi(depHour) );
            SeqUtil_stringAppend( &depNameMsg, " datestamp=");
            SeqUtil_stringAppend( &depNameMsg, depDatestamp );
	 } else {
	    depDatestamp = strdup( _nodeDataPtr->datestamp );
	 }	
         printf( "maestro.validateDependencies() Dependency Scope: %d depDatestamp=%s\n", depScope, depDatestamp);
         if( strcmp( localIndexString, _nodeDataPtr->extension ) == 0 ) {
            switch (depScope) {
               case IntraSuite:
                  sprintf(filename,"%s/sequencing/status/%s%s.%s.%s", SEQ_EXP_HOME, depName, 
                     loopIndexString, depDatestamp, depStatus );
                  printf( "maestro.validateDependencies() IntraSuite looking for status file: %s\n", filename );
                  if ( ! (access(filename, R_OK) == 0) ) {
                     isWaiting = 1;
                     setWaitingState( _nodeDataPtr, depNameMsg, depStatus );
                     writeNodeWaitedFile( _nodeDataPtr, SEQ_EXP_HOME, depName, depStatus, loopIndexString, depDatestamp);
                  }
                  break;
               case IntraUser:
                  /* expPath = (char*) SeqUtil_getExpPath( depUser, depExp ); */
                  sprintf(filename,"%s/sequencing/status/%s%s.%s.%s", depExp, depName, 
                     loopIndexString, depDatestamp, depStatus );
                  printf( "IntraUser maestro.validateDependencies() looking for status file: %s\n", filename );
                  if ( ! (access(filename, R_OK) == 0) ) {
                     isWaiting = 1;
                     setWaitingState( _nodeDataPtr, depNameMsg, depStatus );
                     /* writeNodeWaitedFile( _nodeDataPtr, expPath, depName, depStatus, loopIndexString, depDatestamp); */
                     writeNodeWaitedFile( _nodeDataPtr, depExp, depName, depStatus, loopIndexString, depDatestamp);
                  }
                  break;
               case InterUser:
                  break;
               default:
                  fprintf( stderr, "\nvalidateDependencies application ERROR: invalid dependency scope value=%d\n", depScope );
                  nodelogger( _nodeDataPtr->name, "abort","application error. Invalid dependency scope dependency: %s", depName,_nodeDataPtr->datestamp);
                  isWaiting = -1;
            }
         }
         free(depName); free(depStatus); free(depExp);
         free(depUser); free(loopIndexString); free(localIndexString);
	 free(depNameMsg); free(expPath); free(depDatestamp);
	 free(depHour); free(depIndex); free(localIndex);

         SeqNameValues_deleteWholeList(&loopArgsPtr);

         depName = NULL; depStatus = NULL; depExp=NULL; 
         depUser = NULL; loopIndexString = NULL; localIndexString = NULL;
         depNameMsg = NULL; expPath = NULL; depDatestamp = NULL; 
	 depHour = NULL; depIndex = NULL; tmpExt = NULL; localIndex = NULL;
      } else {
         SeqUtil_TRACE("maestro.validateDependencies() unprocessed nodeinfo_depend_type=%d depsPtr->type\n");
      }
      depsPtr  = depsPtr->nextPtr;
   }
   SeqUtil_TRACE( "maestro.validateDependencies() returning isWaiting=%d\n", isWaiting);

   return isWaiting;
}

/* 
generateConfig

Generates a config file that will be passed to ord_soumet so that the
exported variables are available for the tasks

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution
  flow - pointer to the value of the flow given to the binary ( -f option)

*/
char* generateConfig (const SeqNodeDataPtr _nodeDataPtr, const char* flow) {
   char *extName = NULL;
   char *filename = NULL;
   char pidbuf[100];
   char *tmpdir = NULL, *loopArgs = NULL;
   FILE *tmpFile = NULL;
   SeqNameValuesPtr loopArgsPtr = LOOP_ARGS;
   SeqUtil_stringAppend( &extName, _nodeDataPtr->name );
   if( strlen( _nodeDataPtr->extension ) > 0 ) {
      SeqUtil_stringAppend( &extName, "." );
      SeqUtil_stringAppend( &extName, _nodeDataPtr->extension );
   }
   SeqUtil_stringAppend( &filename, SEQ_EXP_HOME );
   SeqUtil_stringAppend( &filename, "/sequencing/tmpfile/" );
   SeqUtil_stringAppend( &tmpdir, filename );
   SeqUtil_stringAppend( &tmpdir, _nodeDataPtr->container );
   SeqUtil_mkdir( tmpdir, 1 );

   SeqUtil_stringAppend( &filename, extName );
   sprintf(pidbuf, "%d", getpid() ); 
   SeqUtil_stringAppend( &filename, pidbuf );
   SeqUtil_stringAppend( &filename, ".cfg" );
   /* open for write & overwrites whatever if file exists */
   if ((tmpFile = fopen(filename,"w+")) == NULL) {
      raiseError( "maestro cannot write to file:%s\n",filename );
   }
   fprintf( tmpFile, "export SEQ_EXP_HOME=%s\n", SEQ_EXP_HOME );
   fprintf( tmpFile, "export SEQ_EXP_NAME=%s\n", _nodeDataPtr->suiteName); 
   fprintf( tmpFile, "export SEQ_MODULE=%s\n", _nodeDataPtr->module);
   if ( _nodeDataPtr-> npex != NULL ) {
   fprintf( tmpFile, "export SEQ_NPEX=%s\n", _nodeDataPtr->npex);
   } 
   if ( _nodeDataPtr-> npey != NULL ) {
   fprintf( tmpFile, "export SEQ_NPEY=%s\n", _nodeDataPtr->npey);
   }
   if ( _nodeDataPtr-> omp != NULL ) {
   fprintf( tmpFile, "export SEQ_OMP=%s\n", _nodeDataPtr->omp);
   }
   fprintf( tmpFile, "export SEQ_NODE=%s\n", _nodeDataPtr->name );
   fprintf( tmpFile, "export SEQ_NAME=%s\n", _nodeDataPtr->nodeName );
   loopArgs = (char*) SeqLoops_getLoopArgs( LOOP_ARGS );
   if( strlen( loopArgs ) > 0 ) {
      fprintf( tmpFile, "export SEQ_LOOP_ARGS=\"-l %s\"\n", loopArgs );
   } else {
      fprintf( tmpFile, "export SEQ_LOOP_ARGS=\"\"\n" );
   }
   if( strlen( _nodeDataPtr->extension ) > 0 ) {
      fprintf( tmpFile, "export SEQ_LOOP_EXT=\"%s\"\n", _nodeDataPtr->extension );
   } else {
      fprintf( tmpFile, "export SEQ_LOOP_EXT=\"\"\n" );
   } 
   while (loopArgsPtr != NULL) {
      fprintf( tmpFile, "export %s=%s \n", loopArgsPtr->name, loopArgsPtr->value );
      loopArgsPtr=loopArgsPtr->nextPtr;
   }

   fprintf( tmpFile, "export SEQ_XFER=%s\n", flow );
   fprintf( tmpFile, "export SEQ_DATE=%s\n", _nodeDataPtr->datestamp); 

   fclose(tmpFile);
   free(tmpdir);
   free(loopArgs);
   free(loopArgsPtr);

   return filename;
}

/*
maestro

Main function. Creates the node data by calling nodeinfo on the arguments passed. Validates input as well.
Calls the appropriate state function for the desired action.

Inputs:
  _node - pointer to the name of the node targetted by the execution
  _signal - pointer to the name of the action being done (end, begin, etc.)
  _flow - pointer to the argument defining whether flow should continue or stop
  _loops - pointer to the loop arguments (optional)
  ignoreAllDeps - integer representing true or false whether to ignore all dependencies and catchup

*/

int maestro( char* _node, char* _signal, char* _flow, SeqNameValuesPtr _loops, int ignoreAllDeps ) {
   char tmpdir[256], workdir[SEQ_MAXFIELD];
   char *seq_exp_home = NULL, *seq_soumet = NULL, *tmp = NULL;
   char *loopExtension = NULL, *nodeExtension = NULL, *extension = NULL;
   SeqNodeDataPtr nodeDataPtr = NULL;
   int status = 1; /* starting with error condition */
   DIR *dirp = NULL;
   SeqUtil_setDebug(1);
   printf( "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n" );
   printf( "maestro: node=%s signal=%s flow=%s loop_args=%s\n", _node, _signal, _flow, SeqLoops_getLoopArgs(_loops));
   SeqUtil_TRACE( "maestro() ignoreAllDeps=%d \n",ignoreAllDeps );

   memset(workdir,'\0',sizeof workdir);
   memset(SEQ_EXP_HOME,'\0',sizeof SEQ_EXP_HOME);
   memset(USERNAME,'\0',sizeof USERNAME);
   memset(EXPNAME,'\0',sizeof EXPNAME);
   LOOP_ARGS = _loops;
   seq_exp_home = getenv("SEQ_EXP_HOME");

   if ( seq_exp_home != NULL ) {
      dirp = opendir(seq_exp_home);
      if (dirp == NULL) {
         raiseError( "invalid SEQ_EXP_HOME=%s\n",seq_exp_home );
      }
      closedir(dirp);
   } else {
      raiseError( "SEQ_EXP_HOME not set!\n" );
   }

   if ( strcmp(_flow, "continue") == 0 || strcmp(_flow,"stop") == 0 ) {
   	SeqUtil_TRACE( "maestro() flow = %s , valid \n",_flow);
   } else {
      raiseError( "flow value must be \"stop\" or \"continue\"\n" );
   } 

   strcpy( USERNAME, getpwuid(getuid())->pw_name );
   strcpy( EXPNAME, (char*) SeqUtil_getPathLeaf(seq_exp_home) );

   SeqUtil_TRACE( "maestro() SEQ_EXP_HOME=%s\n", seq_exp_home );
   strcpy( SEQ_EXP_HOME, seq_exp_home );
   sprintf(workdir,"%s/sequencing/status", seq_exp_home);

   /* This is needed so messages will be logged into CMCNODELOG */
   putenv("CMCNODELOG=on");
   seq_soumet = getenv("SEQ_SOUMET");
   if ( seq_soumet != NULL ) {
      strcpy(OCSUB,seq_soumet);
   } else {
      strcpy(OCSUB,"ord_soumet");
   }
   SeqUtil_TRACE( "maestro() using submit script=%s\n", OCSUB );
   nodeDataPtr = nodeinfo( _node, "all" );

   if( ! ( strcmp( _signal, "initnode" ) == 0 && nodeDataPtr->type == NpassTask && _loops == NULL ) ) {
      SeqLoops_validateLoopArgs( nodeDataPtr, _loops );
   }
   SeqNode_setWorkdir( nodeDataPtr, workdir );
   SeqNode_setDatestamp( nodeDataPtr, (const char *) tictac_getDate(seq_exp_home,"") );
   SeqUtil_TRACE( "maestro() using DATESTAMP=%s\n", nodeDataPtr->datestamp );
   SeqUtil_TRACE( "maestro() node from nodeinfo=%s\n", nodeDataPtr->name );
   SeqUtil_TRACE( "maestro() node task_path from nodeinfo=%s\n", nodeDataPtr->taskPath );

   /* create working_dir directories */
   sprintf( tmpdir, "%s/%s", nodeDataPtr->workdir, nodeDataPtr->container );
   SeqUtil_mkdir( tmpdir, 1 );

   if ( (strcmp(_signal,"end") == 0 ) || (strcmp(_signal, "endx") == 0 ) || (strcmp(_signal,"endmodel") == 0 ) || 
      (strcmp(_signal,"endloops") == 0) || (strcmp(_signal,"endmodelloops") == 0) ||
      (strncmp(_signal,"endloops_",9) == 0) || (strncmp(_signal,"endmodelloops_",14) == 0)) {
      status=go_end( _signal, _flow, nodeDataPtr );
   }

   if (( strcmp (_signal,"initbranch" ) == 0 ) ||  ( strcmp (_signal,"initnode" ) == 0 ))
   {
      printf("SEQ: call go_initialize() %s %s %s\n",_signal,_flow,_node);
      status=go_initialize( _signal, _flow, nodeDataPtr );
   }

   if (strcmp(_signal,"abort") == 0 || strcmp( _signal, "abortx" ) == 0 ) {
      status=go_abort( _signal, _flow, nodeDataPtr );
   }

   if ( strcmp(_signal,"begin") == 0 || strcmp(_signal,"beginx") == 0 ) {
      SeqUtil_TRACE( "maestro() node from nodeinfo before go_begin=%s\n", nodeDataPtr->name );
      status=go_begin( _signal, _flow, nodeDataPtr );
   }

   if ( strcmp(_signal,"submit") == 0 ) {
   SeqUtil_TRACE( "maestro() ignoreAllDepso2=%d \n",ignoreAllDeps );
      status=go_submit( _signal, _flow, nodeDataPtr, ignoreAllDeps );
   }
   SeqNode_freeNode( nodeDataPtr );
   free( loopExtension );
   free( nodeExtension );
   free( extension );
   free( tmp );
   return status;
}
