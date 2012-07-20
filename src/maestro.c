#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <glob.h>
#include "SeqUtil.h"
#include "tictac.h"
#include "nodeinfo.h"
#include "SeqLoopsUtil.h"
#include "SeqDatesUtil.h"
#include "expcatchup.h"
#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h> 

#define TRUE 1
#define FALSE 0
#define CONTAINER_FLOOD_LIMIT 10
#define CONTAINER_FLOOD_TIMER 15


/*
# level 8 is reserved for normal everyday runs
# level 9 includes normal PLUS discretionary jobs
*/
static const char* CATCHUP_DISCR_MSG = "DISCRETIONARY: this job is not scheduled to run";
static const char* CATCHUP_UNSUBMIT_MSG = "CATCHUP mode: this job will not be submitted";

static char OCSUB[256];
static char SEQ_EXP_HOME[256];

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
static void processContainerBegin ( const SeqNodeDataPtr _nodeDataPtr);
static void processContainerAbort ( const SeqNodeDataPtr _nodeDataPtr);
static void processContainerEnd ( const SeqNodeDataPtr _nodeDataPtr, char *_flow );
static void processLoopContainerBegin( const SeqNodeDataPtr _nodeDataPtr);
static int isLoopComplete ( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args );
static int isNpassComplete ( const SeqNodeDataPtr _nodeDataPtr );

/* State functions: these deal with the status files */
static void setBeginState(char *_signal, const SeqNodeDataPtr _nodeDataPtr);
static void setSubmitState(const SeqNodeDataPtr _nodeDataPtr);
static void setInitState(const SeqNodeDataPtr _nodeDataPtr);
static void setEndState(const char *_signal, const SeqNodeDataPtr _nodeDataPtr);
static void setAbortState(const SeqNodeDataPtr _nodeDataPtr, char * current_action);
static void setWaitingState(const SeqNodeDataPtr _nodeDataPtr, const char* waited_one, const char* waited_status);
/* static void clearAllFinalStates (SeqNodeDataPtr _nodeDataPtr, char * fullNodeName, char * originator ); */
static void clearAllOtherStates( SeqNodeDataPtr _nodeDataPtr, char *fullNodeName, char *originator, char *current_signal); 

/* submission utilities */
static void submitDependencies ( const SeqNodeDataPtr _nodeDataPtr, const char* signal );
static void submitNodeList ( const LISTNODEPTR listToSubmit, SeqNameValuesPtr loop_args_ptr); 
static void submitLoopSetNodeList ( const SeqNodeDataPtr _nodeDataPtr, 
                                SeqNameValuesPtr container_args_ptr, SeqNameValuesPtr set_args_ptr); 

/* dependancy related */
static int writeNodeWaitedFile( const SeqNodeDataPtr _nodeDataPtr, char* dep_exp_path, char* dep_node, 
                                char* dep_status, char* dep_index, char* dep_datestamp );
static int validateDependencies (const SeqNodeDataPtr _nodeDataPtr);

char* formatWaitingMsg( SeqDependsScope _dep_scope, const char* _dep_exp, 
                       const char *_dep_node, const char *_dep_index, const char *_dep_datestamp );

int processDepStatus( const SeqNodeDataPtr _nodeDataPtr, SeqDependsScope  _dep_scope, const char* _dep_name, const char* _dep_index,
                          const char *_dep_datestamp, const char *_dep_status, const char* _dep_exp );

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
         submitNodeList(_nodeDataPtr->submits, _nodeDataPtr->loop_args);
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

   /* check if node has a container to propagate the message up */
   if ( strcmp(_nodeDataPtr->container, "") != 0 && _nodeDataPtr->catchup != CatchupDiscretionary ) {
       processContainerAbort(_nodeDataPtr);
   }

   free( extName );
   free( extensionBase );
   actionsEnd( _signal, _flow, _nodeDataPtr->name );
   return 0;
}


/* 
processContainerAbort

 Treats the abort state of a node that has a container to see how the signal has to be propagated up.

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution

*/

static void processContainerAbort ( const SeqNodeDataPtr _nodeDataPtr) {
   SeqNameValuesPtr newArgs = NULL; 

   if( _nodeDataPtr->loops != NULL ) {
   /*node is part of a loop*/
       newArgs=SeqNameValues_clone(_nodeDataPtr->loop_args);
       if( _nodeDataPtr->type == NpassTask ) {
       /* remove the loop argument for the current loop first */
           SeqNameValues_deleteItem(&newArgs, _nodeDataPtr->nodeName );
       }
       /* if loop has its argument defined, begin the loop without that argument, else begin the container over it*/
       if ( _nodeDataPtr->type == Loop && (char*) SeqLoops_getLoopAttribute( _nodeDataPtr->loop_args, _nodeDataPtr->nodeName ) != NULL ) {
            SeqNameValues_deleteItem(&newArgs, _nodeDataPtr->nodeName );
            printf( "********** processContainerAbort() calling maestro -s abortx -n %s with loop args=%s\n", _nodeDataPtr->name, SeqLoops_getLoopArgs(newArgs)  );
            maestro ( _nodeDataPtr->name, "abortx", "stop", newArgs, 0, NULL );
       } else {
            printf( "********** processContainerAbort() calling maestro -s abortx -n %s with loop args=%s\n", _nodeDataPtr->container, SeqLoops_getLoopArgs(newArgs)  );
            maestro ( _nodeDataPtr->container, "abortx", "stop", newArgs, 0 , NULL);
       }
   } else  {
   /* node is not part of a loop*/
            if ( _nodeDataPtr->type == Loop && (char*) SeqLoops_getLoopAttribute( _nodeDataPtr->loop_args, _nodeDataPtr->nodeName ) != NULL ) {
                 printf( "********** processContainerAbort() calling maestro -s abortx -n %s \n", _nodeDataPtr->name);
                 maestro ( _nodeDataPtr->name, "abortx", "stop",NULL, 0 , NULL);
             } else {
                 printf( "********** processContainerAbort() calling maestro -s abortx -n %s\n", _nodeDataPtr->container  );
                 maestro ( _nodeDataPtr->container, "abortx", "stop", NULL, 0, NULL );
             }
   }
   SeqNode_freeNameValues(newArgs); 
}


/* 
setAbortState

Deals with the status files associated with the abort state by clearing the other states then putting the node in abort state.

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution
  current_action - pointer to the string declaring the abort action to do (usually cont, stop, rerun).

*/

static void setAbortState(const SeqNodeDataPtr _nodeDataPtr, char * current_action) {

   char *extName = NULL, *extension = NULL ;
   char filename[SEQ_MAXFIELD];

   extName = SeqNode_extension( _nodeDataPtr );    

   /* clear any other state */
   //clearAllFinalStates( _nodeDataPtr, extName, "initialize" ); 
   clearAllOtherStates( _nodeDataPtr, extName, "maestro.setAbortState()", current_action ); 

   /* create the lock file only if not exists */
   /* create the node status file */
   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s.%s.abort.%s",_nodeDataPtr->workdir,extName, _nodeDataPtr->datestamp, current_action); 
   if ( access( filename, R_OK) != 0) {
      SeqUtil_TRACE( "maestro.setAbortState() created lockfile %s\n", filename);
      touch(filename);
   }

   /* clear intermidiary states only if not currrent_action*/
   /* remove the node abort.cont lock file */
   /* TO BE REMOVED  */
   /*
   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s.%s.abort.cont",_nodeDataPtr->workdir, extName , _nodeDataPtr->datestamp); 
   if ( access(filename, R_OK) == 0 && strcmp( current_action, "cont" ) != 0 )  {
      SeqUtil_TRACE( "maestro.setAbortState() removed lockfile %s\n", filename);
      removeFile(filename);
   }

   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s.%s.abort.rerun",_nodeDataPtr->workdir, extName, _nodeDataPtr->datestamp); 
   if ( access(filename, R_OK) == 0 && strcmp( current_action, "rerun" ) != 0) {
      SeqUtil_TRACE( "maestro.setAbortState() removed lockfile %s\n", filename);
      removeFile(filename);
   }
   */

   /* for npasstask, we need to create a lock file without the extension
    * so that its container gets the same state */
   if( _nodeDataPtr->type == NpassTask) {
      /* sprintf(filename,"%s/%s.%s.abort",_nodeDataPtr->workdir, _nodeDataPtr->name, _nodeDataPtr->datestamp); */
      extension = (char*) SeqLoops_getExtensionBase( _nodeDataPtr );
      if( strlen( extension ) > 0 ) {
         sprintf(filename,"%s/%s.%s.%s.abort",_nodeDataPtr->workdir, _nodeDataPtr->name, extension, _nodeDataPtr->datestamp); 
      } else {
         sprintf(filename,"%s/%s.%s.abort",_nodeDataPtr->workdir, _nodeDataPtr->name, _nodeDataPtr->datestamp); 
      }
      /* do it only if not exists */
      if ( access(filename, R_OK) != 0 ) touch(filename);
   }

   free( extName );
   free( extension );

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
       sprintf(cmd, "find %s/%s/%s -name \"*%s.*%s.end\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir,_nodeDataPtr->container, _nodeDataPtr->nodeName, extName, _nodeDataPtr->datestamp );
       system(cmd);
       SeqUtil_TRACE( "maestro.go_initialize() deleting begin lockfiles starting at node=%s\n", _nodeDataPtr->name); 
       sprintf(cmd, "find %s/%s/%s -name \"*%s.*%s.begin\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir,_nodeDataPtr->container, _nodeDataPtr->nodeName, extName, _nodeDataPtr->datestamp );
       system(cmd);
       SeqUtil_TRACE( "maestro.go_initialize() deleting abort lockfiles starting at node=%s\n", _nodeDataPtr->name); 
       sprintf(cmd, "find %s/%s/%s -name \"*%s.*%s.abort.*\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir,_nodeDataPtr->container, _nodeDataPtr->nodeName, extName, _nodeDataPtr->datestamp );
       system(cmd);
       SeqUtil_TRACE( "maestro.go_initialize() deleting submit lockfiles starting at node=%s\n", _nodeDataPtr->name); 
       sprintf(cmd, "find %s/%s/%s -name \"*%s.*%s.submit\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir,_nodeDataPtr->container, _nodeDataPtr->nodeName, extName, _nodeDataPtr->datestamp );
       system(cmd);
       SeqUtil_TRACE( "maestro.go_initialize() deleting waiting lockfiles starting at node=%s\n", _nodeDataPtr->name); 
       sprintf(cmd, "find %s/%s/%s -name \"*%s.*%s.waiting\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir,_nodeDataPtr->container, _nodeDataPtr->nodeName, extName, _nodeDataPtr->datestamp  );
       system(cmd);

     /* for npass tasks  */
       printf("Following lockfiles are being deleted: \n");
       SeqUtil_TRACE( "maestro.go_initialize() deleting end lockfiles starting at node=%s\n", _nodeDataPtr->name);
       sprintf(cmd, "find %s/%s/%s -name \"*%s+*.*%s.end\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir,_nodeDataPtr->container, _nodeDataPtr->nodeName, extName, _nodeDataPtr->datestamp );
       system(cmd);
       SeqUtil_TRACE( "maestro.go_initialize() deleting begin lockfiles starting at node=%s\n", _nodeDataPtr->name); 
       sprintf(cmd, "find %s/%s/%s -name \"*%s+*.*%s.begin\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir,_nodeDataPtr->container, _nodeDataPtr->nodeName, extName, _nodeDataPtr->datestamp );
       system(cmd);
       SeqUtil_TRACE( "maestro.go_initialize() deleting abort lockfiles starting at node=%s\n", _nodeDataPtr->name); 
       sprintf(cmd, "find %s/%s/%s -name \"*%s+*.*%s.abort.*\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir,_nodeDataPtr->container, _nodeDataPtr->nodeName, extName, _nodeDataPtr->datestamp );
       system(cmd);
       SeqUtil_TRACE( "maestro.go_initialize() deleting submit lockfiles starting at node=%s\n", _nodeDataPtr->name); 
       sprintf(cmd, "find %s/%s/%s -name \"*%s+*.*%s.submit\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir,_nodeDataPtr->container, _nodeDataPtr->nodeName, extName, _nodeDataPtr->datestamp );
       system(cmd);
       SeqUtil_TRACE( "maestro.go_initialize() deleting waiting lockfiles starting at node=%s\n", _nodeDataPtr->name); 
       sprintf(cmd, "find %s/%s/%s -name \"*%s+*.*%s.waiting\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir,_nodeDataPtr->container, _nodeDataPtr->nodeName, extName, _nodeDataPtr->datestamp  );
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
      if( ((char*) SeqLoops_getLoopAttribute( _nodeDataPtr->loop_args, _nodeDataPtr->nodeName )) == NULL ) {
         /* user has not selected an extension for the current loop, clear them all */
         /* get the list of child leaf extensions for the node */
         nodeList = (LISTNODEPTR) SeqLoops_childExtensions( _nodeDataPtr );
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
      //clearAllFinalStates( _nodeDataPtr, extName, "initialize" ); 
      clearAllOtherStates( _nodeDataPtr, extName, "maestro.setInitState()", ""); 

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
   char *extensionBase = NULL;
   SeqNameValuesPtr loopArgs = NULL, loopSetArgs = NULL, tmpArgs = NULL;
   int loop_args_foundIt=0;
   SeqUtil_TRACE( "maestro.go_begin() node=%s signal=%s flow=%s loopargs=%s\n", _nodeDataPtr->name, _signal, _flow, SeqLoops_getLoopArgs(_nodeDataPtr->loop_args));

   actions( _signal, _flow ,_nodeDataPtr->name );

   /* clear status files of nodes underneath containers when they begin */
   if ( _nodeDataPtr->type != Task && _nodeDataPtr->type != NpassTask && strcmp(_flow, "continue") == 0 ) {
       go_initialize("initbranch", _flow, _nodeDataPtr); 
   } 
 
   /* create begin lock file and other lock file */
   setBeginState ( _signal, _nodeDataPtr );

   /* containers will submit their direct submits in begin */
   if( _nodeDataPtr->type == Loop && (strcmp(_flow, "continue") == 0) ) {
         /* we might have to submit a set of iterations instead of only one */
	 /* get the list of iterations to submit */
         loopSetArgs = (SeqNameValuesPtr) SeqLoops_getLoopSetArgs( _nodeDataPtr, _nodeDataPtr->loop_args );
	 loopArgs =  (SeqNameValuesPtr) SeqLoops_getContainerArgs( _nodeDataPtr, _nodeDataPtr->loop_args );
	 /* submit the set iterations */
	 /* if loop args of the current loop container is not defined, submit a set of loops with arguments, else submit the nodes normally*/

         tmpArgs=_nodeDataPtr->loop_args;
         while (tmpArgs != NULL && !loop_args_foundIt) {
             SeqUtil_TRACE( "go_begin() tmpArgs=%s node=%s\n", tmpArgs->name, _nodeDataPtr->nodeName);
             if (strcmp(_nodeDataPtr->nodeName,tmpArgs->name)==0) {
                 loop_args_foundIt=1;
             } else {
 		 tmpArgs = tmpArgs -> nextPtr;
  	     }
	 } 
	 if (loop_args_foundIt){
              submitNodeList(_nodeDataPtr->submits, _nodeDataPtr->loop_args);
	 } else {
              SeqUtil_TRACE( "maestro.go_begin() doing loop iteration submissions\n", _nodeDataPtr->name, _signal );
	      submitLoopSetNodeList(_nodeDataPtr, loopArgs, loopSetArgs);
         }

	   /* non-loop containers */
   } else if (_nodeDataPtr->type != Task && _nodeDataPtr->type != NpassTask 
             && _nodeDataPtr->type != Case && (strcmp(_flow, "continue") == 0) ) {
        SeqUtil_TRACE( "maestro.go_begin() doing submissions\n", _nodeDataPtr->name, _signal );
        submitNodeList(_nodeDataPtr->submits, _nodeDataPtr->loop_args);
   } 

   /* submit nodes waiting for this one to begin */
   if  (strcmp(_flow, "continue") == 0) {
       submitDependencies( _nodeDataPtr, "begin" );
   }


   if ( strcmp(_nodeDataPtr->container, "") != 0 ) {
      processContainerBegin(_nodeDataPtr); 
   }
   SeqNameValues_deleteWholeList( &loopArgs );
   SeqNameValues_deleteWholeList( &loopSetArgs );
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

   char *extName = NULL, *extension = NULL ;
   char filename[SEQ_MAXFIELD];

   SeqUtil_stringAppend( &extName, _nodeDataPtr->name );
   SeqUtil_TRACE( "maestro.setBeginState() on node:%s extension: %s\n", _nodeDataPtr->name, _nodeDataPtr->extension );
 
   extName = SeqNode_extension( _nodeDataPtr );

   /* clear any other state */
   // clearAllFinalStates( _nodeDataPtr, extName, "begin" ); 
   clearAllOtherStates( _nodeDataPtr, extName, "maestro.setBeginState()", "begin"); 

   /* begin lock file */
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

   /* we will only create the lock file if it does not already exists */
   SeqUtil_TRACE( "maestro.setBeginState() checking for lockfile %s\n", filename);
   if ( access(filename, R_OK) != 0 ) {
      /* create the node begin lock file name */
      touch(filename);
      SeqUtil_TRACE( "maestro.setBeginState() created lockfile %s\n", filename);
   } else {
      printf( "setBeginState() not recreating existing lock file:%s\n", filename );
   }

   /* for npasstask, we need to create a lock file without the extension
   * so that its container gets the same state */
   if( _nodeDataPtr->type == NpassTask) {
      /* sprintf(filename,"%s/%s.%s.begin",_nodeDataPtr->workdir, _nodeDataPtr->name, _nodeDataPtr->datestamp); */
      extension = (char*) SeqLoops_getExtensionBase( _nodeDataPtr );
      if( strlen( extension ) > 0 ) {
         sprintf(filename,"%s/%s.%s.%s.begin",_nodeDataPtr->workdir, _nodeDataPtr->name, extension, _nodeDataPtr->datestamp); 
      } else {
         sprintf(filename,"%s/%s.%s.begin",_nodeDataPtr->workdir, _nodeDataPtr->name, _nodeDataPtr->datestamp); 
      }
      if( access(filename, R_OK) != 0 ) touch(filename);
   }

   free( extName );
   free( extension );
}

/* 
processContainerBegin

 Treats the begin state of a container by checking the status of the siblings nodes around the targetted node.

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution

*/

static void processContainerBegin ( const SeqNodeDataPtr _nodeDataPtr ) {

   char filename[SEQ_MAXFIELD];
   LISTNODEPTR siblingIteratorPtr = NULL, extensions = NULL;
   int abortedSibling = 0;
   if ( _nodeDataPtr->catchup == CatchupDiscretionary ) {   
      SeqUtil_TRACE( "maestro.processContainerBegin() bypassing discreet node:%s\n", _nodeDataPtr->name );
      return;
   }
   if ( _nodeDataPtr->type != CaseItem ) {
      if( _nodeDataPtr->loops != NULL ) {
         /* node is part of a loop */
         processLoopContainerBegin(_nodeDataPtr);
      } else {
         /*node is not part of a loop*/

         if ( _nodeDataPtr->type == Loop ) {
                 /* check other iterations of the loop to see if begin passes */
                 extensions = (LISTNODEPTR) SeqLoops_childExtensions( _nodeDataPtr );
                 if( extensions != NULL ) {
                      while( extensions != NULL && abortedSibling == 0 ) {
                          sprintf(filename,"%s/%s.%s.%s.abort.stop", _nodeDataPtr->workdir, _nodeDataPtr->name, extensions->data, _nodeDataPtr->datestamp);
                          SeqUtil_TRACE( "maestro.processContainerBegins() loop still aborted? checking for:%s\n", filename);
                          abortedSibling = isFileExists( filename, "processContainerBegin()" ) ;
                          extensions = extensions->nextPtr;
                      }
                 }
	 }

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
             /* if loop has its argument defined, begin the loop without that argument, else begin the container over it*/
             if ( _nodeDataPtr->type == Loop && (char*) SeqLoops_getLoopAttribute( _nodeDataPtr->loop_args, _nodeDataPtr->nodeName ) != NULL ) {
                 printf( "********** processContainerBegin() calling maestro -s beginx -n %s \n", _nodeDataPtr->name);
                 maestro ( _nodeDataPtr->name, "beginx", "stop",NULL, 0, NULL );
             } else {
                 printf( "********** processContainerBegin() calling maestro -s beginx -n %s\n", _nodeDataPtr->container  );
                 maestro ( _nodeDataPtr->container, "beginx", "stop", NULL, 0, NULL );
             }
         }
      }
   }
   else {
       /* caseItem always finish their container since only one should execute */
      maestro (_nodeDataPtr->container, "beginx", "stop", NULL, 0, NULL); 
   }
   SeqUtil_TRACE( "maestro.processContainerBegin() return value=%d\n", (abortedSibling == 1 ? 0 : 1) );
}

/* 
processLoopContainerBegin

 Treats the begin state of a container by checking the status of the siblings loop members around the targetted node.

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution

*/

static void processLoopContainerBegin( const SeqNodeDataPtr _nodeDataPtr) {
   char filename[SEQ_MAXFIELD];
   char *extension = NULL, *extWrite = NULL, *extensionBase = NULL;
   int abortedChild = 0, loop_args_foundIt = 0;
   SeqNameValuesPtr tmpArgs = NULL, newArgs = SeqNameValues_clone(_nodeDataPtr->loop_args);
   LISTNODEPTR siblingIteratorPtr = NULL, extensionList = NULL;

   SeqUtil_TRACE( "processLoopContainerBegin() node=%s extension=%s\n", _nodeDataPtr->nodeName, _nodeDataPtr->extension );
   SeqNameValues_printList( newArgs );

   siblingIteratorPtr = _nodeDataPtr->siblings;
   memset( filename, '\0', sizeof filename );
   
   /* do we need to modify the loop extension */
   if (_nodeDataPtr->type == Loop) { 
        tmpArgs=_nodeDataPtr->loop_args;
        extension = strdup( _nodeDataPtr->extension ); 
        while (tmpArgs != NULL && !loop_args_foundIt) {
            SeqUtil_TRACE( "processLoopContainerBegin() tmpArgs=%s node=%s\n", tmpArgs->name, _nodeDataPtr->nodeName);
            if (strcmp(_nodeDataPtr->nodeName,tmpArgs->name)==0) {
                loop_args_foundIt=1;
	        free(extension);
                extension = (char*) SeqLoops_getExtensionBase( _nodeDataPtr ); 
            } else {
		tmpArgs = tmpArgs -> nextPtr;
	    }
	} 
   } else {
        extension = (char*) SeqLoops_getExtensionBase( _nodeDataPtr ); 
   }

   SeqUtil_TRACE( "processLoopContainerBegin() new extension=%s\n",extension );

   if( strlen( extension ) > 0 ) {
      SeqUtil_stringAppend( &extWrite, "." );
      SeqUtil_stringAppend( &extWrite, extension );
   } else {
      SeqUtil_stringAppend( &extWrite, "" );
   }

   /* check the loop's other iterations */

   if ( _nodeDataPtr->type == Loop ) {
         /* check other iterations of the loop to see if begin passes */
         extensionList = (LISTNODEPTR) SeqLoops_childExtensions( _nodeDataPtr );
         SeqUtil_TRACE( "maestro.processLoopContainerBegins() checking iterations around loop node\n");
         if( extensionList != NULL ) {
              while( extensionList != NULL && abortedChild == 0 ) {
                  sprintf(filename,"%s/%s.%s.%s.abort.stop", _nodeDataPtr->workdir, _nodeDataPtr->name, extensionList->data, _nodeDataPtr->datestamp);
                  abortedChild = isFileExists( filename, "processLoopContainerBegin()" ) ;
                  extensionList = extensionList->nextPtr;
              }
         }
    }


   /* check the loop children */

   if( siblingIteratorPtr != NULL && abortedChild == 0 ) {
      SeqUtil_TRACE( "maestro.processLoopContainerBegins() checking siblings\n");
      /* need to process multile childs within loop context */
      while(  siblingIteratorPtr != NULL && abortedChild == 0 ) {
         sprintf(filename,"%s/%s/%s%s.%s.abort.stop", _nodeDataPtr->workdir, _nodeDataPtr->container, siblingIteratorPtr->data, extWrite, _nodeDataPtr->datestamp);
         abortedChild = isFileExists( filename, "processLoopContainerBegin()");
         siblingIteratorPtr = siblingIteratorPtr->nextPtr;
      }
   }

   if( abortedChild == 0 ) {
       if( _nodeDataPtr->type == NpassTask ) {
           /* remove the loop argument for the current loop first */
           SeqNameValues_deleteItem(&newArgs, _nodeDataPtr->nodeName );
       }
       /* if loop has its argument defined, begin the loop without that argument, else begin the container over it*/
       if ( _nodeDataPtr->type == Loop && loop_args_foundIt) {
            SeqNameValues_deleteItem(&newArgs, _nodeDataPtr->nodeName );
            printf( "********** processLoopContainerBegin() calling maestro -s beginx -n %s with loop args=%s\n", _nodeDataPtr->name, SeqLoops_getLoopArgs(newArgs)  );
            maestro ( _nodeDataPtr->name, "beginx", "stop", newArgs, 0, NULL );
       } else {
          printf( "********** processLoopContainerBegin() calling maestro -s beginx -n %s with loop args=%s\n", _nodeDataPtr->container, SeqLoops_getLoopArgs(newArgs)  );
          maestro ( _nodeDataPtr->container, "beginx", "stop", newArgs, 0, NULL );
       }
   }
   SeqNode_freeNameValues(newArgs); 
   free( extensionBase );
   free( extension );
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
   SeqNameValuesPtr newArgs = NULL;
   SeqUtil_TRACE( "maestro.go_end() node=%s signal=%s\n", _nodeDataPtr->name, _signal );
   
   actions( _signal, _flow, _nodeDataPtr->name );
   setEndState( _signal, _nodeDataPtr );

   if ( (_nodeDataPtr->type == Task || _nodeDataPtr->type == NpassTask) && (strcmp(_flow, "continue") == 0)) {
        submitNodeList(_nodeDataPtr->submits, _nodeDataPtr->loop_args);
   } else if (_nodeDataPtr->type == Loop ) {
      /*is the current loop argument in loop args list and it's not the last one ? */
      if((char*) SeqLoops_getLoopAttribute( _nodeDataPtr->loop_args, _nodeDataPtr->nodeName ) != NULL && ! SeqLoops_isLastIteration( _nodeDataPtr, _nodeDataPtr->loop_args )) {
            if( (newArgs = (SeqNameValuesPtr) SeqLoops_nextLoopArgs( _nodeDataPtr, _nodeDataPtr->loop_args )) != NULL && (strcmp(_flow, "continue") == 0)) {
               maestro (_nodeDataPtr->name, "submit", _flow, newArgs, 0, NULL);
            }
         }
      }
   /* check if the container has been completed by the end of this */
   if ( strcmp( _nodeDataPtr->container, "" ) != 0) {
      processContainerEnd( _nodeDataPtr, _flow );
   }

   /* submit nodes waiting for this one to end */
   if  (strcmp(_flow, "continue") == 0) {
       submitDependencies( _nodeDataPtr, "end" );
   }

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
   char *extName = NULL, *extension = NULL, *nptExt = NULL, containerLoopExt = NULL ;
   SeqNameValuesPtr containerLoopArgsList = NULL;

   extName = SeqNode_extension( _nodeDataPtr, 0 );

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
   clearAllOtherStates( _nodeDataPtr, extName, "maestro.setEndState()", "end"); 

   /* create the node end lock file name if not exists*/
   if ( access(filename, R_OK) != 0 ) {
      SeqUtil_TRACE( "maestro.go_end() created lockfile %s\n", filename);
      touch(filename);
   } else {
      printf( "setEndState() not recreating existing lock file:%s\n", filename );
   }

   if ( _nodeDataPtr->type == NpassTask && _nodeDataPtr->isLastNPTArg ) {
      /*container arguments*/
       containerLoopArgsList = SeqLoops_getContainerArgs(_nodeDataPtr, _nodeDataPtr->loop_args);

       if ( containerLoopArgsList != NULL) {
            containerLoopExt =  (char*) SeqLoops_getExtFromLoopArgs(containerLoopArgsList);
            SeqUtil_stringAppend( &nptExt, containerLoopExt );
            free(containerLoopExt);
       }
       SeqUtil_stringAppend( &nptExt, "+last" );
       memset(filename,'\0',sizeof filename);
       sprintf(filename,"%s/%s.%s.%s.end",_nodeDataPtr->workdir,_nodeDataPtr->name, nptExt, _nodeDataPtr->datestamp); 
       free( nptExt);

       /* create the node end lock file name if not exists*/
       if ( access(filename, R_OK) != 0 ) {
          SeqUtil_TRACE( "maestro.go_end() created lockfile %s\n", filename);
          touch(filename);
       } else {
          printf( "setEndState() not recreating existing lock file:%s\n", filename );
       }
   }

   free( extName );
   free( extension );
   SeqNameValues_deleteWholeList( &containerLoopArgsList);
}

/* 
clearAllOtherStates

Clears all the states files except current one.

Inputs:
  fullNodeName - pointer to the node's full name (i.e. /testsuite/assimilation/00/randomjob )
  originator - pointer to the name of the originating function that called this

*/


static void clearAllOtherStates (const SeqNodeDataPtr _nodeDataPtr, char * fullNodeName, char * originator, char* current_state ) {

   char filename[SEQ_MAXFIELD];
   memset(filename,'\0',sizeof filename);

   SeqUtil_TRACE( "maestro.clearAllOtherStatess() originator=%s node=%s\n", originator, fullNodeName);

   /* remove the node begin lock file */
   sprintf(filename,"%s/%s.%s.begin",_nodeDataPtr->workdir,fullNodeName, _nodeDataPtr->datestamp); 
   if ( access(filename, R_OK) == 0 && strcmp( current_state, "begin" ) != 0 ) {
      SeqUtil_TRACE( "maestro.maestro.clearAllOtherStatess() %s removed lockfile %s\n", originator, filename);
      removeFile(filename);
   }
   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s.%s.end",_nodeDataPtr->workdir,fullNodeName, _nodeDataPtr->datestamp); 
   if ( access(filename, R_OK) == 0 && strcmp( current_state, "end" ) != 0 ) {
      SeqUtil_TRACE( "maestro.clearAllOtherStatess() %s removed lockfile %s\n", originator, filename);
      removeFile(filename);
   }

   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s.%s.abort.stop",_nodeDataPtr->workdir,fullNodeName, _nodeDataPtr->datestamp); 
   if ( access(filename, R_OK) == 0 && strcmp( current_state, "stop" ) != 0 ) {
      SeqUtil_TRACE( "maestro.clearAllOtherStatess() %s removed lockfile %s\n", originator, filename);
      removeFile(filename);
   }

   memset(filename,'\0',sizeof filename);
   /* Notice that clearing submit will cause a concurrency vs NFS problem when we add dependency */
   sprintf(filename,"%s/%s.%s.submit",_nodeDataPtr->workdir,fullNodeName, _nodeDataPtr->datestamp); 
   if ( access(filename, R_OK) == 0 && strcmp( current_state, "submit" ) != 0 ) {
      SeqUtil_TRACE( "maestro.clearAllOtherStatess() %s removed lockfile %s\n", originator, filename);
      removeFile(filename);
   }

   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s.%s.waiting",_nodeDataPtr->workdir,fullNodeName, _nodeDataPtr->datestamp); 
   if ( access(filename, R_OK) == 0 && strcmp( current_state, "waiting" ) != 0 ) {
      SeqUtil_TRACE( "maestro.clearAllOtherStatess() %s removed lockfile %s\n", originator, filename);
      removeFile(filename);
   }

   /* delete abort intermediate states only in init, abort or end */
   if ( strcmp( current_state, "init" ) == 0 || strcmp( current_state, "end" ) == 0 ||
        strcmp( current_state, "stop" ) == 0 ) {
      memset(filename,'\0',sizeof filename);
      sprintf(filename,"%s/%s.%s.abort.rerun",_nodeDataPtr->workdir,fullNodeName, _nodeDataPtr->datestamp); 
      if ( access(filename, R_OK) == 0 && strcmp( current_state, "rerun" ) != 0 ) {
         SeqUtil_TRACE( "maestro.clearAllOtherStatess() %s removed lockfile %s\n", originator, filename);
         removeFile(filename);
      }

      memset(filename,'\0',sizeof filename);
      sprintf(filename,"%s/%s.%s.abort.cont",_nodeDataPtr->workdir,fullNodeName, _nodeDataPtr->datestamp); 
      if ( access(filename, R_OK) == 0 && strcmp( current_state, "cont" ) != 0 ) {
         SeqUtil_TRACE( "maestro.clearAllOtherStatess() %s removed lockfile %s\n", originator, filename);
         removeFile(filename);
      }
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
   extensions = (LISTNODEPTR) SeqLoops_childExtensions( _nodeDataPtr );
   if( extensions != NULL ) {
      while( extensions != NULL && undoneIteration == 0 ) {
         sprintf(endfile,"%s/%s.%s.%s.end", _nodeDataPtr->workdir, _nodeDataPtr->name, extensions->data, _nodeDataPtr->datestamp);
         sprintf(continuefile,"%s/%s.%s.%s.abort.cont", _nodeDataPtr->workdir, _nodeDataPtr->name, extensions->data, _nodeDataPtr->datestamp);
         SeqUtil_TRACE( "maestro.isLoopComplete() loop done? checking for:%s or %s\n", endfile, continuefile);
         undoneIteration = ! ( isFileExists( endfile, "isLoopComplete()" ) || isFileExists( continuefile, "isLoopComplete()" ) ) ;
         extensions = extensions->nextPtr;
      }
   }

   SeqListNode_deleteWholeList(&extensions);
   SeqUtil_TRACE( "maestro.isLoopComplete() return value=%d\n", (! undoneIteration) );
   return ! undoneIteration;
}

/* 
isNpassComplete

 returns 1 if all iteration of the npass task is complete in terms of iteration status file
 returns 0 otherwise

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution
*/

static int isNpassComplete ( const SeqNodeDataPtr _nodeDataPtr ) {
   char statePattern[SEQ_MAXFIELD];
   glob_t glob_last, glob_begin, glob_submit, glob_abort;
   int undoneIteration = 0;
   SeqNameValuesPtr containerLoopArgsList = NULL;
   char *filename=NULL, *extension=NULL, *containerLoopExt=NULL;
   size_t lastDot, length, i;

   /* search for last end states. */
   containerLoopArgsList = SeqLoops_getContainerArgs(_nodeDataPtr, _nodeDataPtr->loop_args);
   if ( containerLoopArgsList != NULL) {
       extension =  (char*) SeqLoops_getExtFromLoopArgs(containerLoopArgsList); 
   }
   SeqUtil_stringAppend( &extension,"+last" );

   memset( statePattern, '\0', sizeof statePattern );
   /*sprintf( statePattern,"%s/%s.*.%s.(begin|submit|abort.stop)",_nodeDataPtr->workdir, _nodeDataPtr->name, _nodeDataPtr->datestamp);*/
   sprintf( statePattern,"%s/%s.%s.%s.end",_nodeDataPtr->workdir, _nodeDataPtr->name, extension , _nodeDataPtr->datestamp);
   glob(statePattern, GLOB_NOSORT,0 ,&glob_last);
   undoneIteration = !(glob_last.gl_pathc);
   if (undoneIteration)  SeqUtil_TRACE("maestro.isNpassComplete - last iteration not found. \n"); 
   globfree(&glob_last);
  
   if (! undoneIteration) {
     /* search for submit states. */
      memset( statePattern, '\0', sizeof statePattern );
     /*sprintf( statePattern,"%s/%s.*.%s.(begin|submit|abort.stop)",_nodeDataPtr->workdir, _nodeDataPtr->name, _nodeDataPtr->datestamp);*/
     sprintf( statePattern,"%s/%s.*.%s.submit",_nodeDataPtr->workdir, _nodeDataPtr->name, _nodeDataPtr->datestamp);
     printf("Using glob regex statement: %s \n",statePattern); 
     glob(statePattern, GLOB_NOSORT,0 ,&glob_submit);
     undoneIteration = glob_submit.gl_pathc;
     if (undoneIteration) SeqUtil_TRACE("maestro.isNpassComplete - found submit: %s \n",glob_submit.gl_pathv[0]); 
     globfree(&glob_submit);
   }
   if (! undoneIteration) {
     /* search for begin states. */
     memset( statePattern, '\0', sizeof statePattern );
     /*sprintf( statePattern,"%s/%s.*.%s.(begin|submit|abort.stop)",_nodeDataPtr->workdir, _nodeDataPtr->name, _nodeDataPtr->datestamp);*/
     sprintf( statePattern,"%s/%s.*.%s.begin",_nodeDataPtr->workdir, _nodeDataPtr->name, _nodeDataPtr->datestamp);
     glob(statePattern, GLOB_NOSORT,0 ,&glob_begin);
     undoneIteration = glob_begin.gl_pathc;
     if (undoneIteration) SeqUtil_TRACE("maestro.isNpassComplete - found begin: %s \n",glob_begin.gl_pathv[0]); 
     globfree(&glob_begin);
   }
   if (! undoneIteration) {
     /* search for abort.stop states. */
     memset( statePattern, '\0', sizeof statePattern );
     /*sprintf( statePattern,"%s/%s.*.%s.(begin|submit|abort.stop)",_nodeDataPtr->workdir, _nodeDataPtr->name, _nodeDataPtr->datestamp);*/
     sprintf( statePattern,"%s/%s.*.%s.abort.stop",_nodeDataPtr->workdir, _nodeDataPtr->name, _nodeDataPtr->datestamp);
     glob(statePattern, GLOB_NOSORT,0 ,&glob_abort);
     undoneIteration = glob_abort.gl_pathc;
     if (undoneIteration) SeqUtil_TRACE("maestro.isNpassComplete - found abort.stop: %s \n",glob_abort.gl_pathv[0]); 
     globfree(&glob_abort);
   }

  free(containerLoopExt);
  SeqNameValues_deleteWholeList( &containerLoopArgsList);

  SeqUtil_TRACE( "maestro.isNpassComplete() return value=%d\n", (! undoneIteration) );
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
   char *extension = NULL, *extWrite = NULL;
   int undoneChild = 0, catchup=CatchupNormal;
   LISTNODEPTR siblingIteratorPtr = NULL;
   SeqNodeDataPtr siblingDataPtr = NULL;
   SeqNameValuesPtr newArgs = SeqNameValues_clone(_nodeDataPtr->loop_args);
   printf( "********** processContainerEnd() calling maestro -s endx -n %s with loop args=%s\n", _nodeDataPtr->name, SeqLoops_getLoopArgs(newArgs)  );

    /* deal with L(i) ending -> end of L if all iterations are done, or Npass(i) -> Npass */
   if((char*) SeqLoops_getLoopAttribute( _nodeDataPtr->loop_args, _nodeDataPtr->nodeName ) != NULL) {
        if (( _nodeDataPtr->type == Loop && isLoopComplete ( _nodeDataPtr, _nodeDataPtr->loop_args )) || (_nodeDataPtr->type == NpassTask && isNpassComplete (_nodeDataPtr)) ) {
            SeqNameValues_deleteItem(&newArgs, _nodeDataPtr->nodeName );
            printf( "********** processContainerEnd() calling maestro -s endx -n %s with loop args=%s\n", _nodeDataPtr->name, SeqLoops_getLoopArgs(newArgs)  );
            maestro ( _nodeDataPtr->name, "endx", _flow, newArgs, 0, NULL );
        }
   } else {
       /* all other cases will check siblings for end status */
       siblingIteratorPtr = _nodeDataPtr->siblings;
       SeqUtil_TRACE( "processContainerEnd() container=%s extension=%s\n", _nodeDataPtr->container, _nodeDataPtr->extension );
       if( strlen( _nodeDataPtr->extension ) > 0 ) {
          SeqUtil_stringAppend( &extWrite, "." );
          SeqUtil_stringAppend( &extWrite, _nodeDataPtr->extension );
       } else {
          SeqUtil_stringAppend( &extWrite, "" );
       }
       sprintf(endfile,"%s/%s%s.%s.end", _nodeDataPtr->workdir, _nodeDataPtr->name, extWrite, _nodeDataPtr->datestamp);
       sprintf(continuefile,"%s/%s%s.%s.abort.cont", _nodeDataPtr->workdir, _nodeDataPtr->name, extWrite, _nodeDataPtr->datestamp);

       memset( endfile, '\0', sizeof endfile );
       memset( continuefile, '\0', sizeof continuefile );
       memset( tmp, '\0', sizeof tmp );

       siblingIteratorPtr = _nodeDataPtr->siblings;
       if( siblingIteratorPtr != NULL && undoneChild == 0 ) {
          /*get the exp catchup*/
          catchup = catchup_get (SEQ_EXP_HOME);
          /* check siblings's status for end or abort.continue or higher catchup */
          while(  siblingIteratorPtr != NULL && undoneChild == 0 ) {
             sprintf(endfile,"%s/%s/%s%s.%s.end", _nodeDataPtr->workdir, _nodeDataPtr->container, siblingIteratorPtr->data, extWrite, _nodeDataPtr->datestamp);
             sprintf(continuefile,"%s/%s/%s%s.%s.abort.cont", _nodeDataPtr->workdir, _nodeDataPtr->container, siblingIteratorPtr->data, extWrite, _nodeDataPtr->datestamp);
             undoneChild = ! (isFileExists( endfile, "processContainerEnd()") || isFileExists( continuefile, "processContainerEnd()") );
             if ( undoneChild ) {
             /* check if it's a discretionary or catchup higher than job's value, bypass if yes */
                 sprintf(tmp, "%s/%s", _nodeDataPtr->container, siblingIteratorPtr->data);
                 SeqUtil_TRACE( "maestro.processContainerEnd() getting sibling info: %s\n", tmp );
                 siblingDataPtr = nodeinfo( tmp, "type,res", NULL, NULL, NULL );
                 if ( siblingDataPtr->catchup > catchup ) {
                     /*reset undoneChild since we're skipping this node*/
                     undoneChild = 0;
                     SeqUtil_TRACE( "maestro.processContainerEnd() bypassing discretionary or higher catchup node: %s\n", siblingIteratorPtr->data );
                 }
             }
             siblingIteratorPtr = siblingIteratorPtr->nextPtr;
          }
       }

       if( undoneChild == 0 ) {
          printf( "********** processContainerEnd() calling maestro -s endx -n %s with loop args=%s\n", _nodeDataPtr->container, SeqLoops_getLoopArgs(newArgs)  );
          maestro ( _nodeDataPtr->container, "endx", _flow, newArgs, 0, NULL );
       }
    }
   SeqNode_freeNameValues(newArgs); 
   free( extension);
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
   char cmd[SEQ_MAXFIELD];
   char *cpu = NULL;
   char *tmpCfgFile = NULL;
   char *loopArgs = NULL, *extName = NULL, *fullExtName = NULL;
   int catchup = CatchupNormal;
   int error_status = 0;
   
   SeqUtil_TRACE( "maestro.go_submit() node=%s signal=%s flow=%s ignoreAllDeps=%d\n ", _nodeDataPtr->name, _signal, _flow, ignoreAllDeps );
   actions( (char*) _signal, _flow, _nodeDataPtr->name );
   memset(nodeFullPath,'\0',sizeof nodeFullPath);
   memset(listingDir,'\0',sizeof listingDir);
   sprintf( nodeFullPath, "%s/modules/%s.tsk", SEQ_EXP_HOME, _nodeDataPtr->taskPath );
   sprintf( listingDir, "%s/sequencing/output/%s", SEQ_EXP_HOME, _nodeDataPtr->container );
   
   SeqUtil_stringAppend( &fullExtName, _nodeDataPtr->name );
   if( strlen( _nodeDataPtr->extension ) > 0 ) {
      SeqUtil_stringAppend( &fullExtName, "." );
      SeqUtil_stringAppend( &fullExtName, _nodeDataPtr->extension );
   }

   /* get exp catchup value */
   catchup = catchup_get(SEQ_EXP_HOME);
   /* check catchup value of the node */
   printf("node catchup= %d , exp catchup = %d , discretionary catchup = %d  \n",_nodeDataPtr->catchup, catchup, CatchupDiscretionary );
   if (_nodeDataPtr->catchup > catchup && !ignoreAllDeps ) {
      if (_nodeDataPtr->catchup == CatchupDiscretionary ) {
         printf("nodelogger: -n %s -l %s \"%s\"\n", _nodeDataPtr->name, _nodeDataPtr->extension, CATCHUP_DISCR_MSG );
         nodelogger( _nodeDataPtr->name ,"discret", _nodeDataPtr->extension, CATCHUP_DISCR_MSG,_nodeDataPtr->datestamp);
      } else {
         printf("nodelogger: -n %s -l %s \"%s\"\n", _nodeDataPtr->name, _nodeDataPtr->extension, CATCHUP_UNSUBMIT_MSG );
         nodelogger( _nodeDataPtr->name ,"catchup", _nodeDataPtr->extension, CATCHUP_UNSUBMIT_MSG,_nodeDataPtr->datestamp);
      }
      return(0);
   }

   /* check ignoreAllDeps first so that it does not write the waiting file */
   if( ignoreAllDeps == 1 || validateDependencies( _nodeDataPtr ) == 0 ) {
      /*  clear states here also in case setBeginState is not called (discreet or catchup) */
      clearAllOtherStates( _nodeDataPtr, fullExtName, "maestro.go_submit()", "submit" ); 
      setSubmitState( _nodeDataPtr ) ;

      /* dependencies are satisfied */
      loopArgs = (char*) SeqLoops_getLoopArgs( _nodeDataPtr->loop_args );
   
      tmpCfgFile = generateConfig( _nodeDataPtr, _flow );

      SeqUtil_stringAppend( &extName, _nodeDataPtr->nodeName );
      if( strlen( _nodeDataPtr->extension ) > 0 ) {
         SeqUtil_stringAppend( &extName, "." );
         SeqUtil_stringAppend( &extName, _nodeDataPtr->extension );
      }
      cpu = (char *) SeqUtil_cpuCalculate( _nodeDataPtr->npex, _nodeDataPtr->npey, _nodeDataPtr->omp, _nodeDataPtr->cpu_multiplier );

      /* go and submit the job */
      if ( _nodeDataPtr->type == Task || _nodeDataPtr->type == NpassTask ) {
         sprintf(cmd,"%s -sys %s -jobfile %s -node %s -jn %s -d %s -q %s -p %d -c %s -m %s -w %d -v -listing %s -wrapdir %s/sequencing -jobcfg %s -args \"%s\" %s",OCSUB, getenv("SEQ_WRAPPER"), nodeFullPath, _nodeDataPtr->name, extName,_nodeDataPtr->machine,_nodeDataPtr->queue,_nodeDataPtr->mpi,cpu,_nodeDataPtr->memory,_nodeDataPtr->wallclock, listingDir, SEQ_EXP_HOME, tmpCfgFile, _nodeDataPtr->args, _nodeDataPtr->soumetArgs);
         printf( "%s\n", cmd );
         SeqUtil_TRACE("maestro.go_submit() cmd_length=%d %s\n",strlen(cmd), cmd);
         error_status = system(cmd);
         SeqUtil_TRACE("maestro.go_submit() ord return status: %d \n",error_status);
         if (!error_status){
             nodesubmit(_nodeDataPtr, _nodeDataPtr->datestamp);
         } 
      } else {
         memset( noendwrap, '\0', sizeof( noendwrap ) );
         memset(tmpfile,'\0',sizeof tmpfile);
         sprintf(tmpfile,"%s/sequencing/tmpfile/container.tsk",SEQ_EXP_HOME);
         touch(tmpfile);         
         _nodeDataPtr->submits == NULL ? strcpy( noendwrap, "" ) : strcpy( noendwrap, "-noendwrap" ) ;
	 sprintf(cmd,"%s -sys %s -jobfile %s -node %s -jn %s -d %s -q %s -p %d -c %s -m %s -w %d -v -listing %s -wrapdir %s/sequencing -immediate %s -jobcfg %s -args \"%s\" %s",OCSUB, getenv("SEQ_WRAPPER"), tmpfile,_nodeDataPtr->name, extName, getenv("TRUE_HOST"), _nodeDataPtr->queue,_nodeDataPtr->mpi,cpu,_nodeDataPtr->memory,_nodeDataPtr->wallclock, listingDir, SEQ_EXP_HOME, noendwrap, tmpCfgFile, _nodeDataPtr->args,_nodeDataPtr->soumetArgs);
         printf( "%s\n", cmd );
         SeqUtil_TRACE("maestro.go_submit() cmd_length=%d %s\n",strlen(cmd), cmd);
         error_status=system(cmd);
         SeqUtil_TRACE("maestro.go_submit() ord return status: %d \n",error_status);
         if (!error_status){
             nodesubmit(_nodeDataPtr, _nodeDataPtr->datestamp);
         } 
      }
      free(cpu);
      /* don't forget to remove config file later on */
   }
   actionsEnd( (char*) _signal, _flow, _nodeDataPtr->name );
   free( tmpCfgFile );
   free( extName );
   free( fullExtName );
   return(error_status);
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

   extName = SeqNode_extension( _nodeDataPtr );      

   /* clear any other state */
   //clearAllFinalStates( _nodeDataPtr, extName, "submit" ); 
   clearAllOtherStates( _nodeDataPtr, extName, "maestro.setSubmitState()", "submit" ); 

   /* create the node end lock file */
   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s.%s.submit",_nodeDataPtr->workdir,extName, _nodeDataPtr->datestamp); 

   if ( access(filename, R_OK) != 0 ) {
      touch(filename);
      SeqUtil_TRACE( "maestro.setSubmitState() created lockfile %s\n", filename);
   } else {
      printf( "setSubmitState() not recreating existing lock file:%s\n", filename );
   }

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
      maestro ( listIteratorPtr->data, "submit", "continue" , loop_args_ptr, 0, NULL );
      listIteratorPtr =  listIteratorPtr->nextPtr;
   }
}

/*
submitLoopSetNodeList

Submits a list of nodes.

Inputs:
  _nodeDataPtr - pointer to a node to be executed
  container_args_ptr -  pointer to container loops
  loopset_index - list of index for the set iterations
*/
static void submitLoopSetNodeList ( const SeqNodeDataPtr _nodeDataPtr, 
                                    SeqNameValuesPtr container_args_ptr, SeqNameValuesPtr loopset_index ) {
   SeqNameValuesPtr myLoopArgsPtr = loopset_index;
   SeqNameValuesPtr cmdLoopArg = NULL;
   int counter=0;
   SeqUtil_TRACE( "maestro.submitLoopSetNodeList() container_args_ptr=%s loopset_index=%s\n", SeqLoops_getLoopArgs(container_args_ptr), SeqLoops_getLoopArgs( loopset_index ) );
   while ( myLoopArgsPtr != NULL) {
      /* first get the parent container loop arguments */
      if( container_args_ptr != NULL ) {
         cmdLoopArg = SeqNameValues_clone( container_args_ptr );
      }
      /*flood control, wait CONTAINER_FLOOD_TIMER seconds between sets of submits*/ 
      if (counter >= CONTAINER_FLOOD_LIMIT){
          counter = 0; 
          usleep(1000000 * CONTAINER_FLOOD_TIMER);
      }

      /* then add the current loop argument */
      SeqNameValues_insertItem( &cmdLoopArg, myLoopArgsPtr->name, myLoopArgsPtr->value );
      /*now submit the child nodes */
      SeqUtil_TRACE( "submitLoopSetNodeList calling maestro -n %s -s submit -l %s -f continue\n", _nodeDataPtr->name, SeqLoops_getLoopArgs( cmdLoopArg));
      maestro ( _nodeDataPtr->name, "submit", "continue" , cmdLoopArg, 0, NULL );
      cmdLoopArg = NULL;
      myLoopArgsPtr = myLoopArgsPtr->nextPtr;
      ++counter;
      
   }
}

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

   extName = SeqNode_extension( _nodeDataPtr );     

   /* create the node waiting lock file name*/
   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s.%s.waiting",_nodeDataPtr->workdir,extName, _nodeDataPtr->datestamp); 

   sprintf( waitMsg, "%s %s", waited_status, waited_one );
   nodewait( _nodeDataPtr, waitMsg, _nodeDataPtr->datestamp);  

   /* clear any other state */
   //clearAllFinalStates( _nodeDataPtr, extName, "waiting" ); 
   clearAllOtherStates( _nodeDataPtr, extName, "maestro.setWaitingState()", "waiting" ); 

   if ( access(filename, R_OK) != 0 ) {
      SeqUtil_TRACE( "maestro.setWaitingState() created lockfile %s\n", filename);
      touch(filename);
   } else {
      printf( "setWaitingState() not recreating existing lock file:%s\n", filename );
   }

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
   SeqNameValuesPtr loopArgsPtr = NULL;
   char depUser[12], depExp[256], depNode[256], depArgs[SEQ_MAXFIELD], depDatestamp[20];
   char filename[SEQ_MAXFIELD], submitCmd[SEQ_MAXFIELD];
   char *extName = NULL, *submitDepArgs = NULL, *tmpExt = NULL, *tmpValue=NULL;
   int submitCode = 0;
   LISTNODEPTR cmdList = NULL;

   loopArgsPtr = _nodeDataPtr->loop_args;
   if (_nodeDataPtr->isLastNPTArg){
      tmpValue = SeqUtil_striplast( SeqNameValues_getValue(loopArgsPtr, _nodeDataPtr->nodeName) );
      SeqUtil_TRACE("SeqLoops_submitDependencies Found ^last argument, replacing %s for %s for node %s \n", 
		    SeqNameValues_getValue(loopArgsPtr, _nodeDataPtr->nodeName), tmpValue, _nodeDataPtr->nodeName); 
      SeqNameValues_setValue( &loopArgsPtr, _nodeDataPtr->nodeName, tmpValue);
   }
   tmpExt = (char*) SeqLoops_getExtFromLoopArgs(loopArgsPtr);
   SeqUtil_stringAppend( &extName, _nodeDataPtr->name );
   if( strlen( _nodeDataPtr->extension ) > 0 ) {
      SeqUtil_stringAppend( &extName, "." );
      SeqUtil_stringAppend( &extName, tmpExt );
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
	       SeqUtil_stringAppend( &submitDepArgs, "-l " );
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
            } else {
               /* for now, we treat the rest as same exp, same user */
	       if ( getenv("SEQ_BIN") != NULL ) {
                  sprintf( submitCmd, "(export SEQ_DATE=%s; %s/maestro -s submit -f continue -n %s %s)", depDatestamp, getenv("SEQ_BIN"), depNode, submitDepArgs );
               } else {
                  sprintf( submitCmd, "(export SEQ_DATE=%s;maestro -s submit -f continue -n %s %s)", depDatestamp, depNode, submitDepArgs );
	       }
            }
	    /* add nodes to be submitted if not already there */
	    if ( SeqListNode_isItemExists( cmdList, submitCmd ) == 0 ) {
	       SeqListNode_insertItem( &cmdList, submitCmd );
	    }
	    submitDepArgs = NULL;
	    depUser[0] = '\0'; depExp[0] = '\0'; depNode[0] = '\0'; depDatestamp[0] = '\0'; depArgs[0] = '\0';
         }
         fclose(waitedFile);

         /* we don't need to keep the file */
         removeFile(filename);
      } else {
         raiseError( "maestro cannot read file: %s (submitDependencies) \n", filename );
      }
   }

   /*  go and submit the nodes from the cmd list */
   while ( cmdList != NULL ) {
      printf( "maestro.submitDependencies() calling submit cmd: %s\n", cmdList->data );
      submitCode = system( cmdList->data );
      printf( "maestro.submitDependencies() submitCode: %d\n", submitCode);
      if( submitCode != 0 ) {
         raiseError( "An error happened while submitting dependant nodes error number: %d\n", submitCode );
      }
      cmdList = cmdList->nextPtr;
   }
   SeqListNode_deleteWholeList( &cmdList );
   free(tmpExt);
   free(tmpValue);
   free(extName);
   free(submitDepArgs);
   
}

/*
writeNodeWaitedFile

Writes the dependency lockfile in the directory of the node that this current node is waiting for.

Inputs:
   dep_user - the user id where the dependant node belongs
   dep_exp_path - the SEQ_EXP_HOME of the dependant node
   dep_node - the path of the node including the container
   dep_status - the status that the node is waiting for (end,abort,etc)
   dep_index - the loop index that this node is waiting for (.+1+6)
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
   if( dep_index != NULL && strlen(dep_index) > 0 && dep_index[0] != '.' ) { 
      /* add extra dot for extension */
      sprintf(filename,"%s/sequencing/status/%s.%s.%s.waited_%s",
         dep_exp_path, dep_node, dep_index, dep_datestamp, dep_status);
   } else {
      sprintf(filename,"%s/sequencing/status/%s%s.%s.waited_%s",
         dep_exp_path, dep_node, dep_index, dep_datestamp, dep_status);
   }

   if ( ! (access(filename, R_OK) == 0) ) {
      printf( "maestro.writeNodeWaitedFile creating %s\n", filename );
      touch( filename );
   }
   if ((waitingFile = fopen(filename,"r+")) == NULL) {
      raiseError( "maestro cannot write to file:%s\n",filename );
   }
   printf( "maestro.writeNodeWaitedFile updating %s\n", filename );
   /* sua need to add more logic for duplication and handle more than one entry in the waited file */
   loopArgs = (char*) SeqLoops_getLoopArgs( _nodeDataPtr->loop_args );
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

char* formatWaitingMsg( SeqDependsScope _dep_scope, const char* _dep_exp, 
                       const char *_dep_node, const char *_dep_index, const char *_dep_datestamp ) {
   char *returnValue = NULL;
   char waitMsg[SEQ_MAXFIELD];
   memset(waitMsg,'\0',sizeof waitMsg);

   if( _dep_scope == IntraSuite ) {
      sprintf( waitMsg, "node=%s%s datestamp=%s", _dep_node, _dep_index, _dep_datestamp );
   } else {
      sprintf( waitMsg, "exp=%s node=%s%s datestamp=%s", _dep_exp, _dep_node, _dep_index, _dep_datestamp );
   }

   printf( "formatWaitingMsg waitMsg=%s\n", waitMsg);
   returnValue = strdup( waitMsg );

   return returnValue;
}


/* 
validateDependencies

Checks if the dependencies are satisfied for a given node. 

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution

*/

static int validateDependencies (const SeqNodeDataPtr _nodeDataPtr) {
   int isWaiting = 0, isDepIndexWildcard = 0;
   char filename[SEQ_MAXFIELD];
   char *depName = NULL, *depStatus = NULL, *depUser = NULL, *depExp = NULL,
        *depIndex = NULL, *tmpExt = NULL, *depHour = NULL,
        *expPath = NULL, *localIndex = NULL, *localIndexString = NULL, *depIndexString = NULL;
   char *waitingMsg = NULL, *depDatestamp = NULL;
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

      if ( depsPtr->type == NodeDependancy) {
         SeqUtil_TRACE( "maestro.validateDependencies() nodeinfo_depend_type=Node\n" );
         depScope = IntraSuite;
         depName = SeqNameValues_getValue( nameValuesPtr, "NAME" );
	 SeqUtil_TRACE( "maestro.validateDependencies() nodeinfo_depend_name=%s\n", depName );
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
         }
         /* if I'm a loop iteration, process it */
         SeqUtil_stringAppend( &localIndexString, "" );
         if( localIndex != NULL && strlen( localIndex ) > 0 ) {
            printf( "maestro.validateDependencies() localIndex=%s\n", localIndex );
            SeqLoops_parseArgs(&loopArgsPtr, localIndex);
            tmpExt = (char*) SeqLoops_getExtFromLoopArgs(loopArgsPtr);
            SeqUtil_stringAppend( &localIndexString, tmpExt );
            free(tmpExt);
         }
	 if( depHour != NULL && strlen(depHour) > 0 ) {
	    /* calculate relative datestamp based on the current one */
	    depDatestamp = SeqDatesUtil_getPrintableDate( _nodeDataPtr->datestamp, atoi(depHour) );
	 } else {
	    depDatestamp = strdup( _nodeDataPtr->datestamp );
	 }	
         printf( "maestro.validateDependencies() Dependency Scope: %d depDatestamp=%s\n", depScope, depDatestamp);
	 /* verify status files and write waiting files */
         if( strcmp( localIndexString, _nodeDataPtr->extension ) == 0 ) {
	    if( depScope == IntraSuite ) {
	       printf( "maestro.validateDependencies()  calling processDepStatus depName=%s depIndex=%s depDatestamp=%s depStatus=%s\n", depName, depIndex, depDatestamp, depStatus );
	       isWaiting = processDepStatus( _nodeDataPtr, depScope, depName, depIndex, depDatestamp, depStatus, SEQ_EXP_HOME);
            } else {
	       isWaiting = processDepStatus( _nodeDataPtr, depScope, depName, depIndex, depDatestamp, depStatus, depExp);
	    }
         }
         free(depName); free(depStatus); free(depExp);
         free(depUser); free(localIndexString);
	 free(expPath); free(depDatestamp); free(depIndexString);
	 free(depHour); free(depIndex); free(localIndex);

         SeqNameValues_deleteWholeList(&loopArgsPtr);

         depName = NULL; depStatus = NULL; depExp=NULL; 
         depUser = NULL; depIndexString = NULL; localIndexString = NULL;
         expPath = NULL; depDatestamp = NULL; 
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
   int stringLength = 0; 
   char pidbuf[100];
   char *tmpdir = NULL, *loopArgs = NULL, *containerLoopArgs = NULL, *containerLoopExt = NULL, *tmpValue = NULL, *tmp2Value = NULL;
   FILE *tmpFile = NULL;
   SeqNameValuesPtr loopArgsPtr=NULL , containerLoopArgsList = NULL;
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
   SeqUtil_stringAppend( &filename, "." );
   SeqUtil_stringAppend( &filename, pidbuf );
   SeqUtil_stringAppend( &filename, ".cfg" );
   /* open for write & overwrites whatever if file exists */
   if ((tmpFile = fopen(filename,"w+")) == NULL) {
      raiseError( "maestro cannot write to file:%s\n",filename );
   }
   fprintf( tmpFile, "eval $(ssmuse sh -d %s -p maestro_%s)\n", getenv("SEQ_MAESTRO_DOMAIN"), getenv("SEQ_MAESTRO_VERSION"));
   fprintf( tmpFile, "eval $(ssmuse sh -d %s -p maestro-utils_%s)\n", getenv("SEQ_UTILS_DOMAIN"), getenv("SEQ_UTILS_VERSION"));
   fprintf( tmpFile, "export SEQ_EXP_HOME=%s\n", SEQ_EXP_HOME );
   fprintf( tmpFile, "export SEQ_EXP_NAME=%s\n", _nodeDataPtr->suiteName); 
   fprintf( tmpFile, "export SEQ_WRAPPER=%s\n", getenv("SEQ_WRAPPER"));
   if (SeqUtil_getTraceLevel() != NULL){
     fprintf( tmpFile, "export SEQ_TRACE_LEVEL=%s\n", SeqUtil_getTraceLevel());
   }
   fprintf( tmpFile, "export SEQ_MODULE=%s\n", _nodeDataPtr->module);
   fprintf( tmpFile, "export SEQ_CONTAINER=%s\n", _nodeDataPtr->container); 
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
   loopArgs = (char*) SeqLoops_getLoopArgs( _nodeDataPtr->loop_args );
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

   /*container arguments, used in npass tasks mostly*/
   containerLoopArgsList = SeqLoops_getContainerArgs(_nodeDataPtr, _nodeDataPtr->loop_args);
   if ( containerLoopArgsList != NULL) {
      containerLoopArgs = (char*) SeqLoops_getLoopArgs(containerLoopArgsList);
      containerLoopExt =  (char*) SeqLoops_getExtFromLoopArgs(containerLoopArgsList);
   }
   if ( containerLoopArgs != NULL ) {
      fprintf( tmpFile, "export SEQ_CONTAINER_LOOP_ARGS=\"-l %s\"\n", containerLoopArgs );
      free(containerLoopArgs);
   } else {
      fprintf( tmpFile, "export SEQ_CONTAINER_LOOP_ARGS=\"\"\n" );
   }
   if ( containerLoopExt != NULL ) {
      fprintf( tmpFile, "export SEQ_CONTAINER_LOOP_EXT=\"%s\"\n", containerLoopExt);
      free(containerLoopExt);
   } else {
      fprintf( tmpFile, "export SEQ_CONTAINER_LOOP_EXT=\"\"\n" );
   } 

   loopArgsPtr = _nodeDataPtr->loop_args;
   /* Check for :last NPT arg */
   if (_nodeDataPtr->isLastNPTArg){
      tmpValue=SeqNameValues_getValue(loopArgsPtr, _nodeDataPtr->nodeName); 
      /*remove the :last, raise flag that node has a :last*/
      stringLength=strlen(tmpValue)-5;
      tmp2Value=malloc(stringLength+1); 
      memset(tmp2Value,'\0', stringLength+1);
      strncpy(tmp2Value, tmpValue, stringLength); 
      SeqUtil_stringAppend( &tmp2Value, "" );
      SeqUtil_TRACE("SeqLoops_GenerateConfig Found ^last argument, replacing %s for %s for node %s \n", tmpValue, tmp2Value, _nodeDataPtr->nodeName); 
      SeqNameValues_setValue( &loopArgsPtr, _nodeDataPtr->nodeName, tmp2Value);
      SeqLoops_printLoopArgs(_nodeDataPtr->loop_args,"test"); 
   }

   /* Loop args exported as env variables */
   while (loopArgsPtr != NULL) {
      fprintf( tmpFile, "export %s=%s \n", loopArgsPtr->name, loopArgsPtr->value );
      loopArgsPtr=loopArgsPtr->nextPtr;
   }

   fprintf( tmpFile, "export SEQ_XFER=%s\n", flow );
   fprintf( tmpFile, "export SEQ_TMP_CFG=%s\n", filename);
   fprintf( tmpFile, "export SEQ_DATE=%s\n", _nodeDataPtr->datestamp); 

   fclose(tmpFile);
   free(tmpdir);
   free(tmpValue);
   free(tmp2Value);
   free(loopArgs);
   free(loopArgsPtr);
   SeqNameValues_deleteWholeList( &containerLoopArgsList);

   return filename;
}

/* this function is used to process dependency status files. It does the following:
 * - verifies if the dependant node's status file(s) is available
 * - it creates waited_end file if the dependancy is not satisfied  
 * - it returns 1 if it is waiting (dependency not satisfied)
 * - it returns 0 if it is not waiting (dependency is satisfied)
 *
 * arguments:
 * _nodeDataPtr: node data structure
 * _dep_scope:   dependency scope (IntraSuite, IntraUser)
 * _dep_name:    name of the dependant node
 * _dep_index:   loop index statement of dependant node (loop=value)
 * _dep_datestamp: datestamp of dependant node
 * _dep_status:    status of dependant node
 * _dep_exp:      exp of dependant node if not IntraSuite scope
 */
int processDepStatus( const SeqNodeDataPtr _nodeDataPtr,SeqDependsScope  _dep_scope, const char* _dep_name,const  char* _dep_index,
                          const char *_dep_datestamp, const char *_dep_status, const char* _dep_exp ) {
   char statusFile[SEQ_MAXFIELD];
   int undoneIteration = 0, isWaiting = 0, depWildcard=0, depCatchup=CatchupNormal;
   char *waitingMsg = NULL, *depIndexPtr = NULL, *extString = NULL;
   SeqNodeDataPtr depNodeDataPtr = NULL;
   LISTNODEPTR extensions = NULL;
   SeqNameValuesPtr loopArgsPtr = NULL;


   /* if I'm dependant on a loop iteration, need to process it */
   if( _dep_index != NULL && strlen( _dep_index ) > 0 ) {
       printf( "maestro.processDepStatus() depIndex=%s length:%d\n", _dep_index, strlen(_dep_index) );
       SeqLoops_parseArgs(&loopArgsPtr, _dep_index);
       extString = (char*) SeqLoops_getExtFromLoopArgs(loopArgsPtr);
       if( strstr( extString, "+*" ) != NULL ) {
           depWildcard  = 1;
       }
   } else {
          SeqUtil_stringAppend( &extString, "" );
   }


   printf( "processDepStatus _dep_name=%s _extString=%s _dep_datestamp=%s _dep_status=%s _dep_exp=%s\n", 
      _dep_name, extString, _dep_datestamp, _dep_status, _dep_exp ); 

   if( _dep_index == NULL || strlen( _dep_index ) == 0 ) {
      SeqUtil_stringAppend( &depIndexPtr, "" );
   } else {
      SeqUtil_stringAppend( &depIndexPtr, "." );
      SeqUtil_stringAppend( &depIndexPtr, strdup( extString ) ); 
   }

   memset( statusFile, '\0', sizeof statusFile);

   /* get info from the dependant node */
   depNodeDataPtr = nodeinfo( _dep_name, "all", NULL, _dep_exp, NULL );
   
   /* get exp catchup value */
   depCatchup = catchup_get(_dep_exp);
   /* check catchup value of the node */
   printf("dependant node catchup= %d , exp catchup = %d , discretionary catchup = %d  \n",depNodeDataPtr->catchup, depCatchup, CatchupDiscretionary );
   if (depNodeDataPtr->catchup > depCatchup) {
      printf("dependant node catchup (%d) is higher than the experiment catchup (%d), skipping dependency \n",depNodeDataPtr->catchup, depCatchup);  
      return(0);
   }

   if( ! depWildcard ) {
      /* no wilcard, we check only one iteration */
      if( _dep_exp != NULL ) { 
         sprintf(statusFile,"%s/sequencing/status/%s%s.%s.%s", _dep_exp, _dep_name, depIndexPtr, _dep_datestamp, _dep_status );
      } else {
         sprintf(statusFile,"%s/sequencing/status/%s%s.%s.%s", _nodeDataPtr->workdir, _dep_name, depIndexPtr, _dep_datestamp, _dep_status );
      }
      undoneIteration = ! isFileExists( statusFile, "maestro.processDepStatus()" );
   } else {
      /* wildcard, we need to check for all iterations and stop on the first iteration that is not done */
 
      /* get all the node extensions to be checked */
      extensions = (LISTNODEPTR) SeqLoops_getLoopContainerExtensions( depNodeDataPtr, _dep_index );

      /* loop iterations until we find one that is not satisfied */
      while( extensions != NULL && undoneIteration == 0 ) {
         if( _dep_exp != NULL ) { 
            sprintf(statusFile,"%s/sequencing/status/%s.%s.%s.%s", _dep_exp, _dep_name, extensions->data, _dep_datestamp, _dep_status );
         } else {
            sprintf(statusFile,"%s/sequencing/status/%s.%s.%s.%s", _nodeDataPtr->workdir, _dep_name, extensions->data, _dep_datestamp, _dep_status );
         }
         undoneIteration = ! isFileExists( statusFile, "maestro.processDepStatus()" );
	 if( ! (undoneIteration = ! isFileExists( statusFile, "maestro.processDepStatus()" ))) {
	    /* the iteration status file exists, go to next */
            extensions = extensions->nextPtr;
         } else {
	    depIndexPtr = extensions->data;
	 }
      }
   }

   if( undoneIteration ) {
      isWaiting = 1;
      waitingMsg = formatWaitingMsg(  _dep_scope, _dep_exp, _dep_name, depIndexPtr, _dep_datestamp ); 
      setWaitingState( _nodeDataPtr, waitingMsg, _dep_status );
      if( _dep_scope == InterUser ) {
         printf( "Inter User dependency currently not supported!" );
      } else {
         writeNodeWaitedFile( _nodeDataPtr, _dep_exp, _dep_name, _dep_status, depIndexPtr, _dep_datestamp);
      }
   }

   SeqListNode_deleteWholeList(&extensions);
   free( waitingMsg );

   return isWaiting;
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
  extraArgs - pointer to extra soumet arguments provided

*/

int maestro( char* _node, char* _signal, char* _flow, SeqNameValuesPtr _loops, int ignoreAllDeps, char* _extraArgs ) {
   char tmpdir[256], workdir[SEQ_MAXFIELD];
   char *seq_exp_home = NULL, *seq_soumet = NULL, *tmp = NULL;
   char *loopExtension = NULL, *nodeExtension = NULL, *extension = NULL;
   SeqNodeDataPtr nodeDataPtr = NULL;
   int status = 1; /* starting with error condition */
   DIR *dirp = NULL;
   if (getenv("SEQ_TRACE_LEVEL") != NULL){
       SeqUtil_setTraceLevel(1);
       SeqUtil_TRACE("maestro() SEQ_TRACE_LEVEL=1 \n");
   }
   printf( "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n" );
   printf( "maestro: node=%s signal=%s flow=%s loop_args=%s extraArgs=%s\n", _node, _signal, _flow, SeqLoops_getLoopArgs(_loops), _extraArgs);

   if ( _loops != NULL ) {
       SeqUtil_stringAppend(&tmp, " -l ");
       SeqUtil_stringAppend(&tmp, SeqLoops_getLoopArgs(_loops));
   }
   if ( ignoreAllDeps ) {
       SeqUtil_stringAppend(&tmp, " -i 1");
   }
   if( _extraArgs != NULL ) {
       SeqUtil_stringAppend(&tmp, " -o ");
       SeqUtil_stringAppend(&tmp, _extraArgs);
   } 
   if ( tmp != NULL ){
       printf( "Command called:\nmaestro -s %s -n %s -f %s %s \n",_signal, _node, _flow, tmp);
   } else {
       printf( "Command called:\nmaestro -s %s -n %s -f %s \n",_signal , _node, _flow);
   } 

   SeqUtil_TRACE( "maestro() ignoreAllDeps=%d \n",ignoreAllDeps );

   memset(workdir,'\0',sizeof workdir);
   memset(SEQ_EXP_HOME,'\0',sizeof SEQ_EXP_HOME);
   memset(USERNAME,'\0',sizeof USERNAME);
   memset(EXPNAME,'\0',sizeof EXPNAME);
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
   if ( getenv("SEQ_WRAPPER") == NULL ) {
      raiseError( "SEQ_WRAPPER not set!\n" );
   }

   if ( strcmp(_flow, "continue") == 0 || strcmp(_flow,"stop") == 0 ) {
   	SeqUtil_TRACE( "maestro() flow = %s , valid \n",_flow);
   } else {
      raiseError( "flow value must be \"stop\" or \"continue\"\n" );
   } 

   strcpy( USERNAME, getpwuid(getuid())->pw_name );
   strcpy( EXPNAME, (char*) SeqUtil_getPathLeaf(seq_exp_home) );

   strcpy( SEQ_EXP_HOME, (char*)SeqUtil_fixPath(seq_exp_home));
   SeqUtil_TRACE( "maestro() SEQ_EXP_HOME=%s\n", SEQ_EXP_HOME );
   sprintf(workdir,"%s/sequencing/status", SEQ_EXP_HOME);

   /* This is needed so messages will be logged into CMCNODELOG */
   putenv("CMCNODELOG=on");
   seq_soumet = getenv("SEQ_SOUMET");
   if ( seq_soumet != NULL ) {
      strcpy(OCSUB,seq_soumet);
   } else {
      strcpy(OCSUB,"ord_soumet");
   }
   SeqUtil_TRACE( "maestro() using submit script=%s\n", OCSUB );
   nodeDataPtr = nodeinfo( _node, "all", _loops, NULL, _extraArgs );
   SeqUtil_TRACE( "maestro() nodeinfo done, loop_args=\n");
   SeqNameValues_printList(nodeDataPtr->loop_args);

   SeqLoops_validateLoopArgs( nodeDataPtr, _loops );

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

   if (( strcmp (_signal,"initbranch" ) == 0 ) ||  ( strcmp (_signal,"initnode" ) == 0 )) {
      printf("SEQ: call go_initialize() %s %s %s\n",_signal,_flow,_node);
      status=go_initialize( _signal, _flow, nodeDataPtr );
   }

   if (strcmp(_signal,"abort") == 0 || strcmp( _signal, "abortx" ) == 0 ) {
      status=go_abort( _signal, _flow, nodeDataPtr );
   }

   if ( strcmp(_signal,"begin") == 0 || strcmp(_signal,"beginx") == 0 ) {
      SeqUtil_TRACE( "maestro() node from nodeinfo before go_begin=%s, loopargs=%s, extension=%s \n", nodeDataPtr->name, SeqLoops_getLoopArgs(nodeDataPtr->loop_args), nodeDataPtr->extension );
      status=go_begin( _signal, _flow, nodeDataPtr );
   }

   if ( strcmp(_signal,"submit") == 0 ) {
   SeqUtil_TRACE( "maestro() ignoreAllDepso2=%d \n",ignoreAllDeps );
      status=go_submit( _signal, _flow, nodeDataPtr, ignoreAllDeps );
   }

   SeqUtil_TRACE( "maestro() printing loop args before free\n");
   SeqNameValues_printList(nodeDataPtr->loop_args);
   SeqNode_freeNode( nodeDataPtr );
   free( loopExtension );
   free( nodeExtension );
   free( extension );
   free( tmp );
   return status;
}
