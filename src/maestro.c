/* maestro.c - Main engine of the state machine that is the Maestro sequencer software package.
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

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <pwd.h>
#include <glob.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/wait.h>
#include <fcntl.h> 
#include <sys/time.h>

#include "maestro.h"
#include "nodelogger.h"
#include "l2d2_commun.h"
#include "runcontrollib.h"
#include "SeqUtil.h"
#include "nodeinfo.h"
#include "SeqNameValues.h"
#include "SeqNode.h"
#include "SeqLoopsUtil.h"
#include "SeqDatesUtil.h"
#include "expcatchup.h"
#include "QueryServer.h"
#include "SeqUtilServer.h"
#include "ocmjinfo.h"
#include "logreader.h" 

#define CONTAINER_FLOOD_LIMIT 10
#define CONTAINER_FLOOD_TIMER 15
#define LOCAL_DEPENDS_DIR "/sequencing/status/depends/"
#define REMOTE_DEPENDS_DIR "/sequencing/status/remote_depends/"
#define INTER_DEPENDS_DIR "/sequencing/status/inter_depends/"

/*
# level 8 is reserved for normal everyday runs
# level 9 includes normal PLUS discretionary jobs
*/
static const char* CATCHUP_DISCR_MSG = "DISCRETIONARY: this job is not scheduled to run";
static const char* CATCHUP_UNSUBMIT_MSG = "CATCHUP mode: this job will not be submitted";

static char submit_tool[256];
char *CurrentNode;

/* external Function declarations */
extern size_t get_Inode (const char * );

/* Function declarations */
static int go_abort(char *_signal, char *_flow, const SeqNodeDataPtr _nodeDataPtr);
static int go_initialize(char *_signal, char *_flow ,const SeqNodeDataPtr _nodeDataPtr);
static int go_begin(char *_signal, char *_flow ,const SeqNodeDataPtr nodeDataPtr);
static int go_end(char *_signal, char *_flow ,const SeqNodeDataPtr _nodeDataPtr);
static int go_submit(const char *signal, char *_flow ,const SeqNodeDataPtr nodeDataPtr, int ignoreAllDeps );

/* deal with containers states */
static void processContainerBegin ( const SeqNodeDataPtr _nodeDataPtr, char *_flow );
static void processContainerAbort ( const SeqNodeDataPtr _nodeDataPtr);
static void processContainerEnd ( const SeqNodeDataPtr _nodeDataPtr, char *_flow );
static int isLoopComplete ( const SeqNodeDataPtr _nodeDataPtr );
static int isNpassComplete ( const SeqNodeDataPtr _nodeDataPtr );
static int isLoopAborted ( const SeqNodeDataPtr _nodeDataPtr );
static int isNpassAborted ( const SeqNodeDataPtr _nodeDataPtr );
static SeqNameValuesPtr processForEachTarget(const SeqNodeDataPtr _nodeDataPtr);

/* State functions: these deal with the status files */
static void setBeginState(char *_signal, const SeqNodeDataPtr _nodeDataPtr);
static void setSubmitState(const SeqNodeDataPtr _nodeDataPtr);
static void setInitState(const SeqNodeDataPtr _nodeDataPtr);
static int setEndState(const char *_signal, const SeqNodeDataPtr _nodeDataPtr);
static void setAbortState(const SeqNodeDataPtr _nodeDataPtr, char * current_action);
static void setWaitingState(const SeqNodeDataPtr _nodeDataPtr, const char* waited_one, const char* waited_status);
static void clearAllOtherStates( SeqNodeDataPtr _nodeDataPtr, char *fullNodeName, char *originator, char *current_signal); 
int   isNodeXState (const char* node, const char* loopargs, const char * datestamp, const char* exp, const char * state);  

/* submission utilities */
static void submitDependencies ( const SeqNodeDataPtr _nodeDataPtr, const char* signal, const char* _flow );
static void submitForEach ( const SeqNodeDataPtr _nodeDataPtr, const char* signal );
static void submitNodeList ( const SeqNodeDataPtr _nodeDataPtr ); 
static void submitLoopSetNodeList ( const SeqNodeDataPtr _nodeDataPtr, 
                                SeqNameValuesPtr container_args_ptr, SeqNameValuesPtr set_args_ptr); 

/* dependancy related */
static int writeNodeWaitedFile ( const SeqNodeDataPtr _nodeDataPtr, const char* _dep_exp_path, const char* _dep_node, 
                                const char* _dep_status, const char* _dep_index, const char* _dep_datestamp, SeqDependsScope _dep_scope, const char * StatusFile);
static int validateDependencies (const SeqNodeDataPtr _nodeDataPtr, const char *_flow );

char* formatWaitingMsg( SeqDependsScope _dep_scope, const char* _dep_exp, 
                       const char *_dep_node, const char *_dep_index, const char *_dep_datestamp );
 int prepFEFile( const SeqNodeDataPtr _nodeDataPtr, char * _target_state, char * _target_datestamp); 
int prepInterUserFEFile( const SeqNodeDataPtr _nodeDataPtr, char * _target_state, char * _target_datestamp);

int processDepStatus( const SeqNodeDataPtr _nodeDataPtr, SeqDepDataPtr dep, const char *_flow );

/* Rochdi: Server related */
/* Declare pointer to function to be able to replace locking mechanism */
/* see SeqUtil.h for other definitions */
static int ServerConnectionStatus = 1;
static int QueDeqConnection = 0 ;
int MLLServerConnectionFid=0; /* connection for the maestro Lock|Log  server */
extern int OpenConnectionToMLLServer (const char *,const char *,const char*);
extern void CloseConnectionWithMLLServer ( int con  );
static void CreateLockFile_svr (int sock , char *filename, char *caller, const char* _seq_exp_home );
static void (*_CreateLockFile) (int sock , char *filename, char *caller, const char* _seq_exp_home );
static int  (*_WriteNWFile) (const char* seq_xp_home, const char* nname, const char* datestamp,  const char * loopArgs,
                              const char *filename, const char * StatusFile );
static int  (*_WriteInterUserDepFile) (const char *filename , const char * depBuf , const char *ppwdir, const char* maestro_version,
                                        const char *datestamp, const char *md5sum );
static int (*_WriteFEFile) ( const char* _exp, const char* _node, const char* _datestamp, const char * _target_index, const char* _loopArgs,
                              const char* _filename) ;
 
int writeInterUserNodeWaitedFile ( const SeqNodeDataPtr _nodeDataPtr, const char* _dep_name, const char* _dep_index, char *depIndexPtr, const char *_dep_datestamp, 
                                   const char *_dep_status, const char* _dep_exp , const char* _dep_prot , const char* statusFile, const char * _flow);
static void useNFSlocking();
static void useSVRlocking();
 

/* 
 * Wrapper for access and touch , server version
*/
static void CreateLockFile_svr(int sock , char *filename, char *caller, const char* _seq_exp_home)
{
    int ret = 0;
    ServerActions action=SVR_CREATE;

    ret = Query_L2D2_Server( sock , action, filename , "" , _seq_exp_home);
    switch (ret) {
       case 0: /* success */
               SeqUtil_TRACE(TL_FULL_TRACE,"%s created lockfile %s \n", caller, filename );
               break;
       case 1:
               SeqUtil_TRACE(TL_ERROR,"%s ERROR in creation of lock file:%s action=%d\n",caller, filename, action);
               break;
       case 9:
               SeqUtil_TRACE(TL_MEDIUM,"%s not recreating existing lock file:%s \n", caller, filename );
               break;
   }
}
/* 
 * Wrapper for access and touch , nfs version
*/
static void CreateLockFile_nfs(int sock , char *filename, char *caller , const char* _seq_exp_home)
{
   if ( access_nfs (filename, R_OK,_seq_exp_home) != 0 ) {
          /* create the node begin lock file name */
          touch_nfs(filename,_seq_exp_home);
          SeqUtil_TRACE(TL_FULL_TRACE, "%s created lockfile %s\n", caller, filename);
   } else {
          SeqUtil_TRACE(TL_FULL_TRACE, "%s not recreating existing lock file:%s\n",caller, filename );
   }

}
 
static void useNFSlocking()
{
  _isFileExists = isFileExists_nfs;
  _touch =  touch_nfs;
  _access = access_nfs;
  _removeFile = removeFile_nfs;
  _CreateLockFile = CreateLockFile_nfs;
  _SeqUtil_mkdir = SeqUtil_mkdir_nfs;
  _globPath = globPath_nfs;
  _globExtList = globExtList_nfs; 
  _WriteNWFile = WriteNodeWaitedFile_nfs;
  _WriteInterUserDepFile =  WriteInterUserDepFile_nfs;
  _WriteFEFile = WriteForEachFile_nfs;
/*  _WriteInterUserFEFile = WriteInterUserForEachFile_nfs ; */
  _fopen = fopen_nfs;
  _lock  = lock_nfs;
  _unlock  = unlock_nfs;
  fprintf(stderr,"Serverless mode selected\n"); 
}
 
static void useSVRlocking()
{
 _isFileExists = isFileExists_svr;
 _touch = touch_svr;
 _access = access_svr;
 _removeFile = removeFile_svr;
 _CreateLockFile = CreateLockFile_svr;
 _SeqUtil_mkdir = SeqUtil_mkdir_svr;
 _globPath = globPath_svr;
 /* _globExtList = globExtList_svr;*/
 _globExtList = globExtList_nfs;
 _WriteNWFile = WriteNodeWaitedFile_svr;
 _WriteInterUserDepFile = WriteInterUserDepFile_nfs ;  /* for now use nfs */ 
 /* _WriteFEFile = WriteForEachFile_svr; */
 _WriteFEFile = WriteForEachFile_nfs; 
 /* _WriteInterUserFEFile = WriteForEachFile_svr ; */
/* _WriteInterUserFEFile = WriteInterUserForEachFile_nfs ; */
 _fopen = fopen_svr;
 _lock  = lock_svr;
 _unlock  = unlock_svr;
  fprintf(stderr,"mserver mode selected\n"); 
}

/**
 *  handler for timeout ie blocked connection
 *  and also for synchronizing writes to
 *  nodelog file. (see nodelogger.c)
 */
static void alarm_handler() 
{ 
   CloseConnectionWithMLLServer (MLLServerConnectionFid);
   MLLServerConnectionFid = 0;
   ServerConnectionStatus = 1;
   useNFSlocking();
   fprintf(stderr,"@@@@ TIMEOUT: connection to mserver killed by SIGALRM @@@@ \n");
}
/**
 *
 */
static void pipe_handler() 
{ 
   fprintf(stderr,"@@@@ SIGPIPE received: the mserver has probably shutdown!  @@@@ \n");
}

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
   char jobID[50];
   char tmpString[SEQ_MAXFIELD];

   SeqUtil_TRACE(TL_FULL_TRACE, "maestro.go_abort() node=%s signal=%s flow=%s\n", _nodeDataPtr->name, _signal, _flow );
   actions( _signal, _flow, _nodeDataPtr->name );

   extName = (char *) SeqNode_extension( _nodeDataPtr );

   /*
   Go through the list of _nodeDataPtr->abort_actions to find the current status.
   A typical _nodeDataPtr->abort will have two or more elements "rerun stop or cont".
   A lockfile is created for the current action.....
   Using the above example the first time the job aborts it'll create
   a lockfile with an extension "rerun". If it bombs again, the extension
   will be "stop". 
   */
   if (strcmp(_signal,"abort")==0 && strcmp(_flow, "continue") == 0 ){
      tempPtr = _nodeDataPtr->abort_actions;
      while ( tempPtr != NULL ) {
         if (current_action = (char *) malloc(strlen(tempPtr->data)+1)){
             strcpy(current_action,tempPtr->data);
         } else {
             raiseError("OutOfMemory exception in go_abort()\n");
         }
         SeqUtil_TRACE(TL_FULL_TRACE, "maestro.go_abort() checking for action %s on node %s \n", current_action, _nodeDataPtr->name); 
         sprintf(filename,"%s/%s/%s.abort.%s",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, extName,  current_action);
         SeqUtil_TRACE(TL_FULL_TRACE, "maestro.go_abort() checking for file %s \n", filename); 
         if ( _access(filename, R_OK, _nodeDataPtr->expHome) == 0 ) {
         /* We've done this action the last time, so we're up to the next one */
            tempPtr = tempPtr->nextPtr;
            free(current_action);
            current_action = NULL;
            if ( tempPtr != NULL ) {
                if (current_action = (char *) malloc(strlen(tempPtr->data)+1)){
                    strcpy(current_action,tempPtr->data);
                } else {
                    raiseError("OutOfMemory exception in go_abort()\n");
                }  
                SeqUtil_TRACE(TL_FULL_TRACE, "maestro.go_abort() doing action %s on node %s \n", current_action, _nodeDataPtr->name); 
            }
            break;
         } else if ( tempPtr->nextPtr == NULL ) {
            /* no file was found, and we've reached the end of the list, so we must do the first action */
            tempPtr = _nodeDataPtr->abort_actions;
            if (current_action = (char *) malloc(strlen(tempPtr->data)+1)){
                strcpy(current_action,tempPtr->data);
            } else {
                raiseError("OutOfMemory exception in go_abort()\n");
            }
            SeqUtil_TRACE(TL_FULL_TRACE, "maestro.go_abort() doing action %s on node %s \n", current_action, _nodeDataPtr->name); 
            break; 
         } else {
            /* next action, we're not at the end of the list yet */
            tempPtr = tempPtr->nextPtr;
         }
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
         SeqUtil_TRACE(TL_FULL_TRACE, "nodelogger: %s i \"maestro info: abort action list finished. Aborting with stop.\" \n", _nodeDataPtr->name );
      }
      current_action = strdup("stop");
   }

   /* create status file for current action */

   setAbortState ( _nodeDataPtr, current_action );
    
   if ( strcmp( current_action, "rerun" ) == 0 ) {
      /* issue an appropriate message, then rerun the node */
      if (strcmp(_signal,"abort")==0 && strcmp(_flow, "continue") == 0 ) {

         memset(jobID, '\0', sizeof jobID);
         if (getenv("JOB_ID") != NULL){
             sprintf(jobID,"%s",getenv("JOB_ID"));
         }
         if (getenv("LOADL_STEP_ID") != NULL){
             sprintf(jobID,"%s",getenv("LOADL_STEP_ID"));
         }
         
         memset(tmpString, '\0', sizeof tmpString);
	      sprintf(tmpString,"BOMBED: it has been resubmitted. job_ID=%s", jobID);
         SeqUtil_TRACE(TL_ERROR, "nodelogger: %s X \"BOMBED: it has been resubmitted\"\n", _nodeDataPtr->name );
         nodelogger(_nodeDataPtr->name,"info",_nodeDataPtr->extension,tmpString,_nodeDataPtr->datestamp, _nodeDataPtr->expHome);
         go_submit( "submit", _flow , _nodeDataPtr, 1 );
      }
   } else if (strcmp(current_action,"cont") == 0) {
         /*  issue a log message, then submit the jobs in node->submits */
         if (strcmp(_signal,"abort")==0 && strcmp(_flow, "continue") == 0 ) {

             SeqUtil_TRACE(TL_FULL_TRACE, "nodeabort: %s %s %s\n", _nodeDataPtr->name, _signal, "cont" );
             nodeabort( _signal, _nodeDataPtr, "cont", _nodeDataPtr->datestamp);
             /* submit the rest of the jobs it's supposed to submit (still missing dependency submissions)*/
             SeqUtil_TRACE(TL_FULL_TRACE, "maestro.go_abort() doing submissions\n", _nodeDataPtr->name, _signal );
             submitNodeList(_nodeDataPtr);
	 }
   } else if (strcmp(current_action,"stop") == 0) {
      /* issue a message that the job has bombed */
         SeqUtil_TRACE(TL_FULL_TRACE,"nodeabort: %s %s %s\n", _nodeDataPtr->name, _signal, current_action);
         nodeabort(_signal, _nodeDataPtr,current_action, _nodeDataPtr->datestamp);
   } else {
      /*
      if abort_flag is unrecogized, aborts
      */
      SeqUtil_TRACE(TL_ERROR,"invalid abort action flag %s... aborting...nodeabort: %s stop\n", current_action, _nodeDataPtr->name);
      nodeabort(_signal, _nodeDataPtr,"stop", _nodeDataPtr->datestamp);
   }

   /* check if node has a container to propagate the message up */
   if ( strcmp(_nodeDataPtr->container, "") != 0 && _nodeDataPtr->catchup != CatchupDiscretionary ) {
       if (strcmp(current_action,"cont") == 0) 
         processContainerEnd( _nodeDataPtr, _flow );
       else 
         processContainerAbort(_nodeDataPtr);
   }

   if (current_action != NULL) free( current_action ); 
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
   newArgs=SeqNameValues_clone(_nodeDataPtr->loop_args);

   /* deal with L(i) begin -> beginx of L if none are aborted, or Npass(i) -> Npass */
   if((char*) SeqLoops_getLoopAttribute( _nodeDataPtr->loop_args, _nodeDataPtr->nodeName ) != NULL) {
       SeqNameValues_deleteItem(&newArgs, _nodeDataPtr->nodeName );
       SeqUtil_TRACE(TL_FULL_TRACE, "********** processContainerAbort() calling maestro -s abortx -n %s with loop args=%s\n", _nodeDataPtr->name, SeqLoops_getLoopArgs(newArgs)  );
       maestro ( _nodeDataPtr->name, "abortx", "stop", newArgs, 0, NULL, _nodeDataPtr->datestamp , _nodeDataPtr->expHome);
   } else {
       SeqUtil_TRACE(TL_FULL_TRACE, "********** processContainerAbort() calling maestro -s abortx -n %s with loop args=%s\n", _nodeDataPtr->container, SeqLoops_getLoopArgs(newArgs)  );
       maestro ( _nodeDataPtr->container, "abortx", "stop", newArgs, 0, NULL, _nodeDataPtr->datestamp, _nodeDataPtr->expHome);
   }
   SeqNameValues_deleteWholeList(&newArgs); 
}


/* 
setAbortState

Deals with the status files associated with the abort state by clearing the other states then putting the node in abort state.

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution
  current_action - pointer to the string declaring the abort action to do (usually cont, stop, rerun).

*/

static void setAbortState(const SeqNodeDataPtr _nodeDataPtr, char * current_action) {

   char *extName = NULL;
   char filename[SEQ_MAXFIELD];

   extName = (char *) SeqNode_extension( _nodeDataPtr );    

   /* clear any other state */
   clearAllOtherStates( _nodeDataPtr, extName, "maestro.setAbortState()", current_action ); 

   /* create the lock file only if not exists */
   /* create the node status file */
   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s/%s.abort.%s",_nodeDataPtr->workdir,_nodeDataPtr->datestamp, extName, current_action); 

   _CreateLockFile(MLLServerConnectionFid , filename, "maestro.setAbortState() created lockfile", _nodeDataPtr->expHome);

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
   int returnValue=0;
   actions( _signal, _flow , _nodeDataPtr->name );
   SeqUtil_TRACE(TL_FULL_TRACE, "maestro.go_initialize() node=%s signal=%s flow=%s\n", _nodeDataPtr->name, _signal, _flow );

   if ((strcmp (_signal,"initbranch" ) == 0) && (_nodeDataPtr->type == Task || _nodeDataPtr->type == NpassTask)) {
      raiseError( "maestro -s initbranch can only be used on container type nodes (not task type). Exiting. \n" );
   }
   setInitState( _nodeDataPtr ); 
   /* TODO this method might not work when doing a foreach container...*/
   SeqUtil_stringAppend( &extName, "" );
   if( strlen( _nodeDataPtr->extension ) > 0 ) {
      SeqUtil_stringAppend( &extName, "." );
      SeqUtil_stringAppend( &extName, _nodeDataPtr->extension );
   }      

   /* delete lockfiles in branches under current node */
   if (  strcmp (_signal,"initbranch" ) == 0 ) {
       memset( cmd, '\0' , sizeof cmd);
       SeqUtil_TRACE(TL_FULL_TRACE, "maestro.go_initialize() deleting end lockfiles starting at node=%s\n", _nodeDataPtr->name);
       sprintf(cmd, "find %s/%s/%s/%s -name \"*%s.end\" -type f -print -delete -o -name \"*%s.begin\"  -type f -print -delete  -o -name \"*%s.abort.*\"  -type f -print -delete -o -name \"*%s.submit\" -type f -print -delete -o -name \"*%s.waiting*\" -type f -print -delete",_nodeDataPtr->workdir, _nodeDataPtr->datestamp ,_nodeDataPtr->container, _nodeDataPtr->nodeName, extName,extName,extName,extName,extName);
       SeqUtil_TRACE(TL_FULL_TRACE,"cmd=%s\n",cmd); 
       fprintf(stderr,"Following status files are being deleted: \n");
       returnValue=system(cmd);

    /* for npass tasks  */
       memset( cmd, '\0' , sizeof cmd);
       SeqUtil_TRACE(TL_FULL_TRACE, "maestro.go_initialize() deleting end lockfiles starting at node=%s\n", _nodeDataPtr->name);
       sprintf(cmd, "find %s/%s/%s/%s -name \"*%s+*.end\"  -type f -print -delete -o -name \"*%s+*.begin\"  -type f -print -delete -o  -name \"*%s+*.abort.*\"  -type f -print -delete -o -name \"*%s+*.submit\"  -type f -print -delete -o -name \"*%s+*.waiting*\"  -type f -print -delete",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->container, _nodeDataPtr->nodeName, extName, extName, extName, extName, extName);
       SeqUtil_TRACE(TL_FULL_TRACE,"cmd=%s\n",cmd); 
       returnValue=system(cmd);

   } else if  ( strcmp (_signal,"initnode" ) == 0 ) {
       memset( cmd, '\0' , sizeof cmd);
       SeqUtil_TRACE(TL_FULL_TRACE, "maestro.go_initialize() deleting waiting.InterUser lockfiles starting for node=%s\n", _nodeDataPtr->name); 
       sprintf(cmd, "find %s%s%s%s -name \"*.waiting*\" -type f -print -delete",_nodeDataPtr->expHome, INTER_DEPENDS_DIR, _nodeDataPtr->datestamp, _nodeDataPtr->container);
       SeqUtil_TRACE(TL_FULL_TRACE,"cmd=%s\n",cmd); 
       fprintf(stderr,"Following status files are being deleted: \n");
       returnValue=system(cmd);
   }

   nodelogger(_nodeDataPtr->name,"init",_nodeDataPtr->extension,"",_nodeDataPtr->datestamp, _nodeDataPtr->expHome);

   /* to temporarily fight the problem of misordering of init / begin messages */ 
   if (  strcmp (_signal,"initbranch" ) == 0 ) {
      /*sleep .25 secs*/
      usleep(250000);
   }

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
   char *tmpString = NULL; 
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

   if ( _nodeDataPtr->type == Switch ) {
      if( ((char*) SeqLoops_getLoopAttribute( _nodeDataPtr->loop_args, _nodeDataPtr->nodeName )) == NULL ) {
          SeqUtil_TRACE(TL_FULL_TRACE,"maestro.go_initialize() Adding to list switch argument extension: %s\n", SeqNameValues_getValue(_nodeDataPtr->switchAnswers,_nodeDataPtr->nodeName));
          SeqUtil_stringAppend( &tmpString, "+" );
          SeqUtil_stringAppend( &tmpString, SeqNameValues_getValue(_nodeDataPtr->switchAnswers,_nodeDataPtr->nodeName));
          SeqListNode_insertItem( &nodeList, tmpString);
       }
   }   

   while( nodeList != NULL ) {
      SeqUtil_stringAppend( &extName, _nodeDataPtr->name );
      if( strlen( _nodeDataPtr->extension ) > 0 || strlen( nodeList->data ) > 0 ) {
         SeqUtil_stringAppend( &extName, "." );
         SeqUtil_stringAppend( &extName, _nodeDataPtr->extension );
         SeqUtil_stringAppend( &extName, nodeList->data );
      }
      SeqUtil_TRACE(TL_FULL_TRACE,"maestro.go_initialize()() Looking for status files: %s\n", extName);

      /* clear any other state */
      clearAllOtherStates( _nodeDataPtr, extName, "maestro.setInitState()", ""); 

      free( extName );
      extName = NULL;
      nodeList = nodeList->nextPtr;
   }
   free (tmpString);
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
   SeqNameValuesPtr loopArgs = NULL, loopSetArgs = NULL;
   SeqUtil_TRACE(TL_FULL_TRACE, "maestro.go_begin() node=%s signal=%s flow=%s loopargs=%s\n", _nodeDataPtr->name, _signal, _flow, SeqLoops_getLoopArgs(_nodeDataPtr->loop_args));

   actions( _signal, _flow ,_nodeDataPtr->name );

   /* clear status files of nodes underneath containers when they begin */
   
   if (strcmp(_signal,"begin")==0){
      if ( _nodeDataPtr->type != Task && _nodeDataPtr->type != NpassTask && strcmp(_flow, "continue") == 0 ) {
         go_initialize("initbranch", _flow, _nodeDataPtr); 
      } 
   }
   /* create begin lock file and other lock file */
   setBeginState ( _signal, _nodeDataPtr );
   
   if (strcmp(_signal,"begin")==0 && strcmp(_flow, "continue") == 0 ){
      /* containers will submit their direct submits in begin */
      /* Check if it is with or without own loop argument -> L,FE or L(i),FE(i) */
      if( _nodeDataPtr->type == Loop || _nodeDataPtr->type == ForEach  ) {
          /* L submits first set of loops, L(i) just submits its submits */
         if((char*) SeqLoops_getLoopAttribute( _nodeDataPtr->loop_args, _nodeDataPtr->nodeName ) != NULL) { 
            submitNodeList(_nodeDataPtr);
         } else {
             /* we might have to submit a set of iterations instead of only one */
             /* get the list of iterations to submit */
            if( _nodeDataPtr->type == Loop ) { 
               loopSetArgs = (SeqNameValuesPtr) SeqLoops_getLoopSetArgs( _nodeDataPtr, _nodeDataPtr->loop_args, 0 );
               SeqUtil_TRACE(TL_FULL_TRACE, "maestro.go_begin() doing loop iteration submissions\n", _nodeDataPtr->name, _signal );
            } else if ( _nodeDataPtr->type == ForEach ){
               /* FE ->  check ForEach target for launched iterations, launch corresponding  */   
               loopSetArgs = (SeqNameValuesPtr) processForEachTarget(_nodeDataPtr);
               SeqUtil_TRACE(TL_FULL_TRACE, "maestro.go_begin() doing for_each iteration submissions or tagging \n", _nodeDataPtr->name, _signal );
            }
            loopArgs = (SeqNameValuesPtr) SeqLoops_getContainerArgs( _nodeDataPtr, _nodeDataPtr->loop_args );
            submitLoopSetNodeList(_nodeDataPtr, loopArgs, loopSetArgs);
            SeqNameValues_deleteWholeList( &loopArgs );
            SeqNameValues_deleteWholeList( &loopSetArgs );
         }
      }
      if ( _nodeDataPtr->type == Switch ) {   
               /* S submits S(i), S(i) just submits its submits */
         if((char*) SeqLoops_getLoopAttribute( _nodeDataPtr->loop_args, _nodeDataPtr->nodeName ) != NULL) { 
            submitNodeList(_nodeDataPtr);
         } else {
             loopArgs = (SeqNameValuesPtr) SeqLoops_getContainerArgs( _nodeDataPtr, _nodeDataPtr->loop_args );
             /* first get the parent container loop arguments */
             if( loopArgs != NULL ) {
                loopSetArgs = SeqNameValues_clone( loopArgs );
             }
                /* then add the current loop argument */
             SeqNameValues_printList(_nodeDataPtr->switchAnswers);
             SeqNameValues_insertItem( &loopSetArgs, _nodeDataPtr->nodeName, SeqNameValues_getValue(_nodeDataPtr->switchAnswers,_nodeDataPtr->name ));
             /*now submit the child nodes */
             SeqUtil_TRACE(TL_FULL_TRACE, "go_begin calling maestro -n %s -s submit -l %s -f continue\n", _nodeDataPtr->name, SeqLoops_getLoopArgs( loopSetArgs ));
             maestro ( _nodeDataPtr->name, "submit", "continue" , loopSetArgs, 0, NULL, _nodeDataPtr->datestamp, _nodeDataPtr->expHome);
             SeqNameValues_deleteWholeList( &loopArgs );
             SeqNameValues_deleteWholeList( &loopSetArgs );
         }
      }

      /* non-interative containers */
      if (_nodeDataPtr->type == Family || _nodeDataPtr->type == Module) {
         SeqUtil_TRACE(TL_FULL_TRACE, "maestro.go_begin() doing submissions\n", _nodeDataPtr->name, _signal );
         submitNodeList(_nodeDataPtr);
      } 
   }

   /* submit nodes waiting for this one to begin */
   submitDependencies( _nodeDataPtr, "begin",_flow );

   if ( strcmp(_nodeDataPtr->container, "") != 0 ) {
      SeqUtil_TRACE(TL_FULL_TRACE,"go_begin():Node %s has a container %s\n", _nodeDataPtr->nodeName, _nodeDataPtr->container);
      processContainerBegin(_nodeDataPtr, _flow); 
   } else {
      SeqUtil_TRACE(TL_FULL_TRACE,"go_begin():Node %s does not have a container\n", _nodeDataPtr->nodeName);
   }
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

   char *extName = NULL;
   char filename[SEQ_MAXFIELD];

   SeqUtil_stringAppend( &extName, _nodeDataPtr->name );
   SeqUtil_TRACE(TL_FULL_TRACE, "maestro.setBeginState() on node:%s extension: %s\n", _nodeDataPtr->name, _nodeDataPtr->extension );
 
   extName = (char *)SeqNode_extension( _nodeDataPtr );

   /* clear any other state */
   clearAllOtherStates( _nodeDataPtr, extName, "maestro.setBeginState()", "begin"); 

   /* begin lock file */
   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s/%s.begin",_nodeDataPtr->workdir,_nodeDataPtr->datestamp, extName); 

   /* For a container, we don't send the log file entry again if the
      status file already exists and if the signal is beginx */
   if( _nodeDataPtr->type != Task && _nodeDataPtr->type != NpassTask) {
      if( ( strcmp( _signal, "begin" ) == 0 ) ||
          ( strcmp( _signal, "beginx" ) == 0 && !_isFileExists( filename, "setBeginState()", _nodeDataPtr->expHome) ) ) {

         nodebegin( _signal, _nodeDataPtr, _nodeDataPtr->datestamp );
      } 	
   } else {
      nodebegin( _signal, _nodeDataPtr, _nodeDataPtr->datestamp );
   }

   /* we will only create the lock file if it does not already exists */
   SeqUtil_TRACE(TL_FULL_TRACE, "maestro.setBeginState() checking for lockfile %s\n", filename);
   _CreateLockFile(MLLServerConnectionFid , filename , "setBeginState() ", _nodeDataPtr->expHome);
   free( extName );
}

/********************************************************************************
 * Function hasArgs()
 *
 * Serves to determine, for a loop, npastask or switch node N whether the node
 * is N or N(i) by checking if the node has loop_args to it's name.
 *
 * Used in processContainerBegin() and processContainerEnd().
********************************************************************************/
int hasArgs(const SeqNodeDataPtr _nodeDataPtr){

   /* Only loops, NpassTasks and switches may have args */
   if (   _nodeDataPtr->type == Loop
       || _nodeDataPtr->type == NpassTask
       || _nodeDataPtr->type == Switch    )
      if((char*) SeqLoops_getLoopAttribute( _nodeDataPtr->loop_args, _nodeDataPtr->nodeName ) != NULL )
         /* loop_args has an attribute which matches the nodeName */
         return 1;

   return 0;
}


/* 
processContainerBegin

 Treats the begin state of a container by checking the status of the siblings nodes around the targetted node.

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution
  _flow - the execution flow

*/
static void processContainerBegin ( const SeqNodeDataPtr _nodeDataPtr, char *_flow ) {

   char filename[SEQ_MAXFIELD], tmp[SEQ_MAXFIELD];
   LISTNODEPTR siblingIteratorPtr = NULL;
   SeqNameValuesPtr newArgs = SeqNameValues_clone(_nodeDataPtr->loop_args);
   SeqNodeDataPtr siblingDataPtr = NULL;
   int abortedSibling = 0, catchup=CatchupNormal;
   char* extWrite = NULL;

   if ( _nodeDataPtr->catchup == CatchupDiscretionary ) {   
      SeqUtil_TRACE(TL_FULL_TRACE, "maestro.processContainerBegin() bypassing discrete node:%s\n", _nodeDataPtr->name );
      return;
   }

   SeqUtil_TRACE(TL_FULL_TRACE, "processContainerBegin():_nodeDataPtr->loopArgs : \n");
   SeqNameValues_printList(_nodeDataPtr->loop_args);
   SeqUtil_TRACE(TL_FULL_TRACE,"processContainerBegin():_nodeDataPtr->nodeName : %s\n", _nodeDataPtr->nodeName);
   SeqUtil_TRACE(TL_FULL_TRACE,"processContainerBegin():_nodeDataPtr->name : %s\n", _nodeDataPtr->name);
   SeqUtil_TRACE(TL_FULL_TRACE, "processContainerBegin(): _nodeDataPtr->type : %s\n", SeqNode_getTypeString(_nodeDataPtr->type));

    /* deal with L(i) begin -> beginx of L if none are aborted, or Npass(i) -> Npass, or Switch(i) -> Switch */
   if(hasArgs(_nodeDataPtr)) {
        SeqUtil_TRACE(TL_FULL_TRACE, "processContainerBegin(): Entered if (SeqLoops_getLoopAttribute() != NULL).\n");
        if (( _nodeDataPtr->type == Loop && ! isLoopAborted ( _nodeDataPtr )) || (_nodeDataPtr->type == NpassTask && ! isNpassAborted (_nodeDataPtr)) || _nodeDataPtr->type == Switch ) {
            SeqNameValues_deleteItem(&newArgs, _nodeDataPtr->nodeName );
            SeqUtil_TRACE(TL_FULL_TRACE, "********** processContainerBegin() calling maestro -s beginx -n %s with loop args=%s\n", _nodeDataPtr->name, SeqLoops_getLoopArgs(newArgs)  );
            maestro ( _nodeDataPtr->name, "beginx", _flow, newArgs, 0, NULL, _nodeDataPtr->datestamp , _nodeDataPtr->expHome);
        }
   } else {
           /* check non-catchup siblings for aborts */
       siblingIteratorPtr = _nodeDataPtr->siblings;
       SeqUtil_TRACE(TL_FULL_TRACE, "processContainerBegin() container=%s extension=%s\n", _nodeDataPtr->container, _nodeDataPtr->extension );
       if( strlen( _nodeDataPtr->extension ) > 0 ) {
          SeqUtil_stringAppend( &extWrite, "." );
          SeqUtil_stringAppend( &extWrite, _nodeDataPtr->extension );
       } else {
          SeqUtil_stringAppend( &extWrite, "" );
       }

       siblingIteratorPtr = _nodeDataPtr->siblings;
       if( siblingIteratorPtr != NULL && abortedSibling == 0 ) {
          /*get the exp catchup*/
          catchup = catchup_get (_nodeDataPtr->expHome);
          /* check siblings's status for end or abort.continue or higher catchup */
          while(  siblingIteratorPtr != NULL && abortedSibling == 0 ) {
             memset( filename, '\0', sizeof filename );
             sprintf(filename,"%s/%s/%s/%s%s.abort.stop", _nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->container, siblingIteratorPtr->data, extWrite);
             abortedSibling = _isFileExists( filename, "processContainerBegin()", _nodeDataPtr->expHome);
             if ( abortedSibling ) {
             /* check if it's a discretionary or catchup higher than job's value, bypass if yes */
                 memset( tmp, '\0', sizeof tmp );
                 sprintf(tmp, "%s/%s", _nodeDataPtr->container, siblingIteratorPtr->data);
                 SeqUtil_TRACE(TL_FULL_TRACE, "maestro.processContainerBegin() getting sibling info: %s\n", tmp );
                 siblingDataPtr = nodeinfo( tmp, NI_SHOW_TYPE|NI_SHOW_RESOURCE, NULL,
                                             _nodeDataPtr->expHome, NULL,
                                             _nodeDataPtr->datestamp,NULL );
                 if ( siblingDataPtr->catchup > catchup ) {
                     /*reset aborted since we're skipping this node*/
                     abortedSibling = 0;
                     SeqUtil_TRACE(TL_FULL_TRACE, "maestro.processContainerBegin() bypassing discretionary or higher catchup node: %s\n", siblingIteratorPtr->data );
                 }
             }
             siblingIteratorPtr = siblingIteratorPtr->nextPtr;
          }
       }

       if( abortedSibling == 0 ) {
          SeqUtil_TRACE(TL_FULL_TRACE, "processContainerBegin(): Entered if( abortedSibling == 0 )\n");
          SeqUtil_TRACE(TL_FULL_TRACE, "********** processContainerBegin() calling maestro -s beginx -n %s with loop args=%s\n", _nodeDataPtr->container, SeqLoops_getLoopArgs(newArgs)  );
          maestro ( _nodeDataPtr->container, "beginx", _flow, newArgs, 0, NULL, _nodeDataPtr->datestamp , _nodeDataPtr->expHome);
       }
   }
   SeqUtil_TRACE(TL_FULL_TRACE, "maestro.processContainerBegin() abortedSibling value=%d\n", abortedSibling );
}


/*
go_end

This function is used to cause the node to enter the end state. Tasks submit their implicit dependencies in this state.
The function also deals with the parent containers' end state if conditions are met.
See header documentation of SeqLoops_nextLoopArgs() for explanation of how loops are handled.

Inputs:
  signal - pointer to the value of the signal given to the binary (-s option)
  flow - pointer to the value of the flow given to the binary ( -f option)
  _nodeDataPtr - pointer to the node targetted by the execution

*/

static int go_end(char *_signal,char *_flow , const SeqNodeDataPtr _nodeDataPtr) {
	SeqNameValuesPtr newArgs = NULL, loopSetArgs = NULL, containerArgs = NULL;
	int isEndCnt=1;
	int newDefNumber = 0;
	SeqUtil_TRACE(TL_FULL_TRACE, "maestro.go_end() node=%s signal=%s\n", _nodeDataPtr->name, _signal );

	actions( _signal, _flow, _nodeDataPtr->name );
	isEndCnt=setEndState( _signal, _nodeDataPtr );

	if ( (_nodeDataPtr->type == Task || _nodeDataPtr->type == NpassTask) && (strcmp(_flow, "continue") == 0)) {
		submitNodeList(_nodeDataPtr);
	} else if (_nodeDataPtr->type == Loop ) {
		/*is the current loop argument in loop args list and it's not the last one ? */
		if((char*) SeqLoops_getLoopAttribute( _nodeDataPtr->loop_args, _nodeDataPtr->nodeName ) != NULL && !SeqLoops_isLastIteration( _nodeDataPtr, _nodeDataPtr->loop_args ) ) {
			newArgs = (SeqNameValuesPtr) SeqLoops_nextLoopArgs( _nodeDataPtr, _nodeDataPtr->loop_args, &newDefNumber );
			if(  (strcmp(_flow, "continue") == 0) ) { 
				if( newDefNumber != 0 ){
               /* Submit the initial set of a new definition */
					SeqUtil_TRACE(TL_MEDIUM, "go_end() submitting new definition. newDefNumber = %d\n",newDefNumber );
					loopSetArgs = (SeqNameValuesPtr) SeqLoops_getLoopSetArgs( _nodeDataPtr, NULL , newDefNumber);
					containerArgs = (SeqNameValuesPtr) SeqLoops_getContainerArgs(_nodeDataPtr, _nodeDataPtr->loop_args);
					submitLoopSetNodeList(_nodeDataPtr, containerArgs, loopSetArgs );
				} else if (newArgs != NULL) {
					if  ( isEndCnt != 0 ) {
                  /* Submit next iteration */
                  SeqUtil_TRACE(TL_FULL_TRACE, "go_end() submitting next iteration with maestro call:\n\t\
                        maestro( %s, \"submit\" , %s, %s, 0, NULL, %s, %s \n",
                        _nodeDataPtr->name, _flow, newArgs, _nodeDataPtr->datestamp, _nodeDataPtr->expHome);
						maestro (_nodeDataPtr->name, "submit", _flow, newArgs, 0, NULL, _nodeDataPtr->datestamp, _nodeDataPtr->expHome);
					} else {
						SeqUtil_TRACE(TL_FULL_TRACE, "maestro.go_end() Skipping submission of next iteration -- already @ end state.\n");
					}
				}
			}
		}
	}

	/* check if the container has been completed by the end of this */
	if ( strcmp( _nodeDataPtr->container, "" ) != 0) {
		if ( isEndCnt != 0 ) {
			processContainerEnd( _nodeDataPtr, _flow );
		} else {  
			fprintf(stderr, "maestro.go_end() Skipping end execution, already @ end state. No dependencies will be submitted, nor containers will be processed. Clear out end lockfile to be able to run end command.\n");
			return (0);
		}
	}

   /* submit nodes waiting for this one to end */
   submitDependencies( _nodeDataPtr, "end", _flow );
      /*
      if ( _nodeDataPtr -> loop_args != NULL ) {
         submitForEach( _nodeDataPtr, "end" );
      }
      */

	SeqNameValues_deleteWholeList( &newArgs );
	actionsEnd( _signal, _flow, _nodeDataPtr->name );
	return (0);
}

/* 
setEndState

Deals with the status files associated with the end state by clearing the other states then putting the node in end state.

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution

*/

static int setEndState(const char* _signal, const SeqNodeDataPtr _nodeDataPtr) {

   char filename[SEQ_MAXFIELD], filename1[SEQ_MAXFIELD];
   char *extName = NULL,*nptExt = NULL, *containerLoopExt = NULL;
   int ret=0, isEndContainerExist=1;

   SeqNameValuesPtr newArgs = SeqNameValues_clone(_nodeDataPtr->loop_args);
   extName = (char *)SeqNode_extension( _nodeDataPtr );

   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s/%s.end",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, extName); 

   /* For a container, we don't send the log file entry again if the
      status file already exists and the signal is endx */
   if ( _nodeDataPtr->type != Task && _nodeDataPtr->type != NpassTask) {
      if (   ( strcmp( _signal, "end" ) == 0 ) || 
           ( ( strcmp( _signal, "endx" ) == 0) && !_isFileExists( filename, "setEndState()", _nodeDataPtr->expHome))) {
         nodeend( _signal, _nodeDataPtr, _nodeDataPtr->datestamp );
      }
   } else {
      nodeend( _signal, _nodeDataPtr, _nodeDataPtr->datestamp );
   }

   /* clear any other state */
   clearAllOtherStates( _nodeDataPtr, extName, "maestro.setEndState()", "end"); 

   /* Obtain a lock to protect end state */ 
   ret=_lock( filename , _nodeDataPtr->datestamp, _nodeDataPtr->expHome ); 
   /* ADD TIME GIVEN BY THE SERVER */ 

   /* Handling multiple concurrent "endx" submissions of a container 
      The above lock will ensure that only one child is executing
      this section. If the child finds a ${container}.end file, it
      means that a previous child has submitted the "endx" for this 
      container and the current child should exit.
   */

   /* if this node is a container do this */
    if ( _nodeDataPtr->type != Task && _nodeDataPtr->type != NpassTask ) {
             isEndContainerExist = _access(filename, R_OK, _nodeDataPtr->expHome);
             fprintf(stderr,"\nThis Task has Ended the CONTAINER :%s\n",filename);
    }

   /* create the node end lock file name if not exists*/
   _CreateLockFile( MLLServerConnectionFid , filename , "go_end() ", _nodeDataPtr->expHome);

   if ( _nodeDataPtr->type == NpassTask && _nodeDataPtr->isLastArg ) {
      /*container arguments*/

       if((char*) SeqLoops_getLoopAttribute( _nodeDataPtr->loop_args, _nodeDataPtr->nodeName ) != NULL) {
            SeqNameValues_deleteItem(&newArgs, _nodeDataPtr->nodeName );
            containerLoopExt = (char*) SeqLoops_getExtFromLoopArgs(newArgs);
            SeqUtil_TRACE(TL_FULL_TRACE, "maestro.setEndState() containerLoopExt %s\n", containerLoopExt);
            SeqUtil_stringAppend( &nptExt, containerLoopExt );
            free(containerLoopExt);
            SeqUtil_stringAppend( &nptExt, "+last" );
            memset(filename1,'\0',sizeof filename);
            sprintf(filename1,"%s/%s/%s.%s.end",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->name, nptExt); 
            free( nptExt);
       }

       /* create the ^last node end lock file name if not exists*/
      _CreateLockFile( MLLServerConnectionFid , filename1 , "go_end() ", _nodeDataPtr->expHome);

   }

   /* Release lock here */
   ret=_unlock( filename , _nodeDataPtr->datestamp, _nodeDataPtr->expHome );
   /* ADD TIME GIVEN BY THE SERVER */ 
  

   free( extName );
   SeqNameValues_deleteWholeList( &newArgs);

   SeqUtil_TRACE(TL_FULL_TRACE, "maestro.setEndState() return %d\n", isEndContainerExist );
   return ( isEndContainerExist );
}

/* 
clearAllOtherStates

Clears all the states files except current one.

Inputs:
  fullNodeName - pointer to the node's full name (i.e. /testsuite/assimilation/00/randomjob )
  originator - pointer to the name of the originating function that called this

*/


static void clearAllOtherStates (const SeqNodeDataPtr _nodeDataPtr, char * fullNodeName, char * originator, char* current_state ) {

   int ret;
   char filename[SEQ_MAXFIELD];
   char *extension = NULL, *tmpExt = NULL;
   SeqNameValuesPtr newArgs = NULL; SeqNameValues_clone(_nodeDataPtr->loop_args);

   memset(filename,'\0',sizeof filename);
   SeqUtil_TRACE(TL_FULL_TRACE, "maestro.clearAllOtherStates() originator=%s node=%s\n", originator, fullNodeName);

   /* remove the node begin lock file */
   sprintf(filename,"%s/%s/%s.begin",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, fullNodeName); 
   if ( _access(filename, R_OK, _nodeDataPtr->expHome) == 0 && strcmp( current_state, "begin" ) != 0 ) {
      SeqUtil_TRACE(TL_FULL_TRACE, "maestro.maestro.clearAllOtherStates() %s removed lockfile %s\n", originator, filename);
      ret=_removeFile(filename, _nodeDataPtr->expHome);
   }
   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s/%s.end",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, fullNodeName); 
   if ( _access(filename, R_OK, _nodeDataPtr->expHome) == 0 && strcmp( current_state, "end" ) != 0 ) {
      SeqUtil_TRACE(TL_FULL_TRACE, "maestro.clearAllOtherStates() %s removed lockfile %s\n", originator, filename);
      ret=_removeFile(filename, _nodeDataPtr->expHome);
   }

   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s/%s.abort.stop",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, fullNodeName); 
   if ( _access(filename, R_OK, _nodeDataPtr->expHome) == 0 && strcmp( current_state, "stop" ) != 0 ) {
      SeqUtil_TRACE(TL_FULL_TRACE, "maestro.clearAllOtherStates() %s removed lockfile %s\n", originator, filename);
      ret=_removeFile(filename, _nodeDataPtr->expHome);
   }

   memset(filename,'\0',sizeof filename);
   /* Notice that clearing submit will cause a concurrency vs NFS problem when we add dependency */
   sprintf(filename,"%s/%s/%s.submit",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, fullNodeName); 
   if ( _access(filename, R_OK, _nodeDataPtr->expHome) == 0 && strcmp( current_state, "submit" ) != 0 ) {
      SeqUtil_TRACE(TL_FULL_TRACE, "maestro.clearAllOtherStates() %s removed lockfile %s\n", originator, filename);
      ret=_removeFile(filename, _nodeDataPtr->expHome);
   }

   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s/%s.waiting",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, fullNodeName); 
   if ( _access(filename, R_OK, _nodeDataPtr->expHome) == 0 && strcmp( current_state, "waiting" ) != 0 ) {
      SeqUtil_TRACE(TL_FULL_TRACE, "maestro.clearAllOtherStates() %s removed lockfile %s\n", originator, filename);
      ret=_removeFile(filename, _nodeDataPtr->expHome);
   }

   /* NPASS(i) will delete NPASS.end in all states */

   if (_nodeDataPtr->type == NpassTask) {
       if((char*) SeqLoops_getLoopAttribute( _nodeDataPtr->loop_args, _nodeDataPtr->nodeName ) != NULL) {
            SeqUtil_TRACE(TL_FULL_TRACE, "maestro.clearAllOtherStates() NPASS(i) deleting NPASS.end \n", originator, filename);
            newArgs = SeqNameValues_clone(_nodeDataPtr->loop_args);
            SeqNameValues_deleteItem(&newArgs, _nodeDataPtr->nodeName );
            tmpExt = (char *) SeqLoops_getExtFromLoopArgs(newArgs); 
            if (tmpExt != NULL) {
                SeqUtil_stringAppend( &extension, "." );
                SeqUtil_stringAppend( &extension, tmpExt );
            } else {
                SeqUtil_stringAppend( &extension, "" );
            }

            memset(filename,'\0',sizeof filename);
            sprintf(filename,"%s/%s/%s%s.end",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->name, extension); 
            if ( _access(filename, R_OK, _nodeDataPtr->expHome) == 0 && strcmp( current_state, "end" ) != 0 ) {
                SeqUtil_TRACE(TL_FULL_TRACE, "maestro.clearAllOtherStates() %s removed lockfile %s\n", originator, filename);
                ret=_removeFile(filename, _nodeDataPtr->expHome);
            } 
            SeqNameValues_deleteWholeList(&newArgs); 
            free( extension);
            free( tmpExt);
        }
   }

   /* delete abort intermediate states only in init, abort or end */
   if ( strcmp( current_state, "init" ) == 0 || strcmp( current_state, "end" ) == 0 ||
        strcmp( current_state, "stop" ) == 0 ) {
      memset(filename,'\0',sizeof filename);
      sprintf(filename,"%s/%s/%s.abort.rerun",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, fullNodeName); 
      if ( _access(filename, R_OK, _nodeDataPtr->expHome) == 0 && strcmp( current_state, "rerun" ) != 0 ) {
         SeqUtil_TRACE(TL_FULL_TRACE, "maestro.clearAllOtherStates() %s removed lockfile %s\n", originator, filename);
         ret=_removeFile(filename, _nodeDataPtr->expHome);
      }

      memset(filename,'\0',sizeof filename);
      sprintf(filename,"%s/%s/%s.abort.cont",_nodeDataPtr->workdir,  _nodeDataPtr->datestamp, fullNodeName); 
      if ( _access(filename, R_OK, _nodeDataPtr->expHome) == 0 && strcmp( current_state, "cont" ) != 0 ) {
         SeqUtil_TRACE(TL_FULL_TRACE, "maestro.clearAllOtherStates() %s removed lockfile %s\n", originator, filename);
         ret=_removeFile(filename, _nodeDataPtr->expHome);
      }
   }
}

/* 
isLoopComplete

 returns 1 if all iteration of the loop is complete in terms of iteration status file
 returns 0 otherwise

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution
  
*/

static int isLoopComplete ( const SeqNodeDataPtr _nodeDataPtr ) {
   char endfile[SEQ_MAXFIELD];
   char continuefile[SEQ_MAXFIELD];
   LISTNODEPTR extensions = NULL;
   int undoneIteration = 0;

   memset( endfile, '\0', sizeof endfile );
   memset( continuefile, '\0', sizeof continuefile );

   /* check if the loop is completed */
   extensions = (LISTNODEPTR) SeqLoops_childExtensionsInReverse( _nodeDataPtr );
   if( extensions != NULL ) {
      while( extensions != NULL && undoneIteration == 0 ) {
         sprintf(endfile,"%s/%s/%s.%s.end", _nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->name, extensions->data);
         sprintf(continuefile,"%s/%s/%s.%s.abort.cont", _nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->name, extensions->data);
         SeqUtil_TRACE(TL_FULL_TRACE, "maestro.isLoopComplete() loop done? checking for:%s or %s\n", endfile, continuefile);
         undoneIteration = ! ( _isFileExists( endfile,      "isLoopComplete()" , _nodeDataPtr->expHome) || 
                               _isFileExists( continuefile, "isLoopComplete()" , _nodeDataPtr->expHome) ) ;
         extensions = extensions->nextPtr;
      }
   }

   SeqListNode_deleteWholeList(&extensions);
   SeqUtil_TRACE(TL_FULL_TRACE, "maestro.isLoopComplete() return value=%d\n", (! undoneIteration) );
   return ! undoneIteration;
}

/* 
isLoopAborted

 returns 1 if one loop iteration is in abort state in terms of iteration status file
 returns 0 otherwise

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution
  
*/

static int isLoopAborted ( const SeqNodeDataPtr _nodeDataPtr ) {
   char abortedfile[SEQ_MAXFIELD];
   LISTNODEPTR extensions = NULL;
   int abortedIteration = 0;

   /* check if the loop is completed */
   extensions = (LISTNODEPTR) SeqLoops_childExtensionsInReverse( _nodeDataPtr );
   if( extensions != NULL ) {
      while( extensions != NULL && abortedIteration == 0 ) {
         memset( abortedfile, '\0', sizeof abortedfile );
         sprintf(abortedfile,"%s/%s/%s.%s.abort.stop", _nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->name, extensions->data);
         SeqUtil_TRACE(TL_FULL_TRACE, "maestro.isLoopAborted() loop has aborted iteration? checking for:%s\n", abortedfile);
         abortedIteration =  _isFileExists( abortedfile, "isLoopAborted()" , _nodeDataPtr->expHome) ;
         extensions = extensions->nextPtr;
      }
   }
   SeqListNode_deleteWholeList(&extensions);
   SeqUtil_TRACE(TL_FULL_TRACE, "maestro.isLoopAborted() return value=%d\n", abortedIteration );
   return abortedIteration;
}

/* 
isNpassComplete

 returns 1 if all iteration of the npass task is complete in terms of iteration status file
 returns 0 otherwise

Inputs
  _nodeDataPtr - pointer to the node targetted by the execution
*/

static int isNpassComplete ( const SeqNodeDataPtr _nodeDataPtr ) {
   char statePattern[SEQ_MAXFIELD];
   int undoneIteration = 0;
   SeqNameValuesPtr containerLoopArgsList = NULL;
   char *extension=NULL;
   
   /* search for last end states. */

   containerLoopArgsList = (SeqNameValuesPtr) SeqLoops_getContainerArgs(_nodeDataPtr, _nodeDataPtr->loop_args);
   if ( containerLoopArgsList != NULL) {
       SeqUtil_stringAppend( &extension, (char*) SeqLoops_getExtFromLoopArgs(containerLoopArgsList)); 
   } 
   SeqUtil_stringAppend( &extension,"+"); 
 

   memset( statePattern, '\0', sizeof statePattern );
   sprintf( statePattern,"%s/%s/%s.%slast.end",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->name, extension);
   undoneIteration = !(_globPath (statePattern, GLOB_NOSORT,0,_nodeDataPtr->expHome));
   if (undoneIteration)  SeqUtil_TRACE(TL_FULL_TRACE,"maestro.isNpassComplete - last iteration not found\n");
  
   if (! undoneIteration) {
     /* search for submit states. */
     memset( statePattern, '\0', sizeof statePattern );
     sprintf( statePattern,"%s/%s/%s.%s*.submit",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->name, extension);
     undoneIteration = _globPath(statePattern, GLOB_NOSORT,0,_nodeDataPtr->expHome);
     if (undoneIteration) SeqUtil_TRACE(TL_FULL_TRACE,"maestro.isNpassComplete - found submit\n"); 

   }
   if (! undoneIteration) {
     /* search for begin states. */
     memset( statePattern, '\0', sizeof statePattern );
     sprintf( statePattern,"%s/%s/%s.%s*.begin",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->name, extension);
     undoneIteration = _globPath(statePattern, GLOB_NOSORT,0,_nodeDataPtr->expHome);
     if (undoneIteration) SeqUtil_TRACE(TL_FULL_TRACE,"maestro.isNpassComplete - found begin\n");

   }
   if (! undoneIteration) {
     /* search for abort.stop states. */
     memset( statePattern, '\0', sizeof statePattern );
     sprintf( statePattern,"%s/%s/%s.%s*.abort.stop",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->name, extension);
     undoneIteration = _globPath(statePattern, GLOB_NOSORT,0,_nodeDataPtr->expHome);
     if (undoneIteration) SeqUtil_TRACE(TL_FULL_TRACE,"maestro.isNpassComplete - found abort.stop\n");

   }

  free(extension);
  SeqNameValues_deleteWholeList( &containerLoopArgsList);

  SeqUtil_TRACE(TL_FULL_TRACE, "maestro.isNpassComplete() return value=%d\n", (! undoneIteration) );
  return ! undoneIteration;
} 

/* 
isNpassAborted

 returns 1 if an iteration of the npass task is aborted in terms of iteration status file
 returns 0 otherwise

Inputs
  _nodeDataPtr - pointer to the npass node targetted by the execution (it should have its own iteration defined)
*/

static int isNpassAborted ( const SeqNodeDataPtr _nodeDataPtr ) {
   char statePattern[SEQ_MAXFIELD];
   int abortedIteration = 0;
   SeqNameValuesPtr containerLoopArgsList = NULL;
   char *extension=NULL;

   /*check if npt is within a loop*/
   containerLoopArgsList =  (SeqNameValuesPtr) SeqLoops_getContainerArgs(_nodeDataPtr, _nodeDataPtr->loop_args);
   if ( containerLoopArgsList != NULL) {
       SeqUtil_stringAppend( &extension,(char*) SeqLoops_getExtFromLoopArgs(containerLoopArgsList)); 
   }
   SeqUtil_stringAppend( &extension,"+"); 

   /* search for abort.stop states. */
   memset( statePattern, '\0', sizeof statePattern );
   sprintf( statePattern,"%s/%s/%s.%s*.abort.stop",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->name, extension);
   abortedIteration = _globPath(statePattern, GLOB_NOSORT,0,_nodeDataPtr->expHome);
   if (abortedIteration) SeqUtil_TRACE(TL_FULL_TRACE,"maestro.isNpassAborted - found abort.stop\n");

   free(extension);
   SeqNameValues_deleteWholeList( &containerLoopArgsList);

   SeqUtil_TRACE(TL_FULL_TRACE, "maestro.isNpassAborted() return value=%d\n", abortedIteration );
   return abortedIteration;
} 

/* 
processForEachTarget

 returns a name-values list of iterations that can be launched by the foreach container. 
 returns NULL if there are no iteration that can be launched yet. It will also tag the target(s) to launch iterations.  

Inputs
  _nodeDataPtr - pointer to the forEach node targetted by the execution 

*/

static SeqNameValuesPtr processForEachTarget(const SeqNodeDataPtr _nodeDataPtr){
   SeqNameValuesPtr iterationListToReturn = NULL; 
   SeqNodeDataPtr targetNodeDataPtr = NULL; 
   char* target_datestamp=NULL;
   char statePattern[SEQ_MAXFIELD], seqPath[SEQ_MAXFIELD];
   LISTNODEPTR returnedExtensionList = NULL, tempIterator=NULL; 
   int status=0;

   /* Check type of target, LOOP or Switch vs NPT or FE */ 
   target_datestamp=SeqDatesUtil_getPrintableDate( _nodeDataPtr->datestamp, 0, atoi(_nodeDataPtr->forEachTarget->hour), 0, 0);
   targetNodeDataPtr = nodeinfo(_nodeDataPtr->forEachTarget->node , NI_SHOW_ALL, NULL,_nodeDataPtr->forEachTarget->exp, NULL, target_datestamp,NULL );
   SeqUtil_TRACE(TL_FULL_TRACE, "maestro.processForEachTarget() called, targetting exp=%s, name=%s, index=%s, date=%s\n", _nodeDataPtr->forEachTarget->exp, _nodeDataPtr->forEachTarget->node, _nodeDataPtr->forEachTarget->index, target_datestamp );
   
   /* Check status of target node's iterations */
   /* search for end states. */
   memset( statePattern, '\0', sizeof statePattern );
   sprintf(statePattern,"%s/%s/%s.%s+*.end",targetNodeDataPtr->workdir, targetNodeDataPtr->datestamp, targetNodeDataPtr->name, _nodeDataPtr->extension );

   /* TODO add a lock on the target here? might be necessary */

   /* Build list to submit */ 
   returnedExtensionList = _globExtList(statePattern, GLOB_NOSORT, 0);
   tempIterator=returnedExtensionList;
   while (tempIterator != NULL) {
      SeqUtil_TRACE(TL_FULL_TRACE, "maestro.processForEachTarget() inserting loop name=%s value=%s\n", _nodeDataPtr->nodeName, tempIterator->data );
      SeqNameValues_insertItem(&iterationListToReturn, _nodeDataPtr->nodeName,tempIterator->data);  
      tempIterator = tempIterator->nextPtr;
   }

   /* Create the message for the target to launch iterations */

   sprintf(seqPath,"%s/sequencing/status", _nodeDataPtr->forEachTarget->exp);

   if (_access(seqPath,W_OK, _nodeDataPtr->expHome) == 0) {
      status = prepFEFile(_nodeDataPtr,"end", target_datestamp);
   } else {
      status = prepInterUserFEFile(_nodeDataPtr,"end", target_datestamp);  
   }
/*
 // typical code from creating dependency function, use as model
 if( _dep_exp != NULL ) { 
               sprintf(statusFile,"%s/sequencing/status/%s/%s%s.%s", _dep_exp, _dep_datestamp,  _dep_name, depExtension, _dep_status );
           } else {
           }
           ret=_lock( statusFile ,_nodeDataPtr->datestamp ); 
           if ( (undoneIteration=! _isFileExists( statusFile, "maestro.processDepStatus()", _nodeDataPtr->expHome)) ) {
              if( _dep_scope == InterUser ) {
                   isWaiting = writeInterUserNodeWaitedFile( _nodeDataPtr, _dep_name, _dep_index, depExtension, _dep_datestamp, _dep_status, _dep_exp, _dep_prot, statusFile, _flow);
              } else {
                   isWaiting = writeNodeWaitedFile( _nodeDataPtr, _dep_exp, _dep_name, _dep_status, depExtension, _dep_datestamp, _dep_scope, statusFile);
              }
           }
           ret=_unlock( statusFile , _nodeDataPtr->datestamp ); 





*/
   SeqListNode_deleteWholeList(&returnedExtensionList);
   return iterationListToReturn;
}


int prepFEFile( const SeqNodeDataPtr _nodeDataPtr, char * _target_state, char * _target_datestamp) {

   char filename[SEQ_MAXFIELD];
   char *loopArgs = NULL, *depBase = NULL;
   int status = 0 , ret;
   loopArgs = (char*) SeqLoops_getLoopArgs( _nodeDataPtr->loop_args );

   memset(filename,'\0',sizeof filename);

   SeqUtil_TRACE(TL_FULL_TRACE,"maestro.prepFEFile() target exp=%s, target node=%s, target index=%s target datestamp=%s self.index=%s\n",
                 _nodeDataPtr->forEachTarget->exp,_nodeDataPtr->forEachTarget->node,_nodeDataPtr->forEachTarget->index,_target_datestamp , loopArgs);

   /* create dirs if not there */
   
   depBase = (char*) SeqUtil_getPathBase( (const char*) _nodeDataPtr->forEachTarget->node );
   /* write in sequencing/status/depends/$datestamp */
   sprintf(filename,"%s/%s/%s/%s", _nodeDataPtr->forEachTarget->exp, LOCAL_DEPENDS_DIR, _target_datestamp, depBase );
   _SeqUtil_mkdir( filename, 1 ,_nodeDataPtr->expHome);
   if( _nodeDataPtr->extension != NULL && strlen(_nodeDataPtr->extension) > 0) { 
      /* add extra dot for extension */
      sprintf(filename,"%s%s%s/%s.%s.FE_%s", _nodeDataPtr->forEachTarget->exp, LOCAL_DEPENDS_DIR, _target_datestamp, _nodeDataPtr->forEachTarget->node , _nodeDataPtr->extension, _target_state);
   } else {
      sprintf(filename,"%s%s%s/%s.FE_%s", _nodeDataPtr->forEachTarget->exp, LOCAL_DEPENDS_DIR, _target_datestamp, _nodeDataPtr->forEachTarget->node , _target_state);
   }
   
   status = _WriteFEFile ( _nodeDataPtr->expHome, _nodeDataPtr->name , _nodeDataPtr->datestamp, _nodeDataPtr->forEachTarget->index, loopArgs, filename );
 
   switch (status)
   {
       case 0:
           status=1;
           break;
       case 1:
           raiseError( "maestro had problems writing to the following file: %s (prepFEFile)\n", filename );
           break; /* not reached */
       case 9:
           status=0;
           ret=_removeFile(filename, _nodeDataPtr->expHome);
           break;
       default: 
           raiseError( "Unknown switch case value in prepFEFile:%d (prepFEFile)\n", status);
   } 
   free( loopArgs );
   free( depBase );
   return(status);

}
 

int prepInterUserFEFile( const SeqNodeDataPtr _nodeDataPtr, char * _target_state, char * _target_datestamp) {
   char filename[SEQ_MAXFIELD];


   struct passwd *current_passwd = NULL;
   char registration_time[40];
   char depParam[2048],depFile[1024],depBuf[2048];
   char *loopArgs = NULL, *depBase = NULL,  *md5sum = NULL;
   char *maestro_version=NULL , *maestro_shortcut=NULL;
   struct timeval tv;
   struct tm* ptm;
   time_t current_time;
   int ret;
   loopArgs = (char*) SeqLoops_getLoopArgs( _nodeDataPtr->loop_args );

   current_passwd = getpwuid(getuid());  

   /* Build file name to be put in polling directory: This has to be unique!
   * depFile is a combination of datestamp source exp. source node and status
   * this will be a link to the actual file containing dependency parameters.
   * A node is Unique in one Exp. for a specific datestamp !
   */
    
   if (  loopArgs != NULL && strlen(loopArgs) != 0 ) {
      snprintf(depFile,sizeof(depFile),"%s_%s%s_%s.%s",_nodeDataPtr->datestamp, _nodeDataPtr->expHome, _nodeDataPtr->name, loopArgs, _target_state);
   } else {
      snprintf(depFile,sizeof(depFile),"%s_%s%s.%s",_nodeDataPtr->datestamp, _nodeDataPtr->expHome, _nodeDataPtr->name, _target_state);
   }

   /* compute md5sum for the string representating the depfile */
   md5sum = (char *) str2md5(depFile,strlen(depFile));
             
   /* Format the date and time, down to a single second.*/
   gettimeofday (&tv, NULL); 
   current_time = time(NULL);

   ptm = gmtime (&tv.tv_sec); 
   strftime (registration_time, sizeof (registration_time), "%Y%m%d-%H:%M:%S", ptm); 
   
   /* get maestro version & shortcut : what to do when cannot have it ???? */
   if ( (maestro_version=getenv("SEQ_MAESTRO_VERSION")) == NULL ) {
      raiseError("Could not get maestro version from SEQ_MAESTRO_VERSION env variable.\n");
   }

   if ( (maestro_shortcut=getenv("SEQ_MAESTRO_SHORTCUT")) == NULL ) {
      raiseError("Could not get maestro shortcut from SEQ_MAESTRO_SHORTCUT env variable.\n");
   }

   /* 
    * create directory where to put the *waiting.interUser* file 
    * we will be able to erase it when doing init[branche] actions 
    * Also when submiting with no depend.
   */

   depBase = (char*) SeqUtil_getPathBase( (const char*) _nodeDataPtr->name );

   /* write in local xperiment: sequencing/status/inter_depends/$datestamp */
   sprintf(filename,"%s/%s/%s/%s", _nodeDataPtr->expHome, INTER_DEPENDS_DIR, _nodeDataPtr->datestamp, depBase );

   /* create dir if not there  (local Xp) */
   if ( _access(filename,R_OK, _nodeDataPtr->expHome) != 0 ) _SeqUtil_mkdir( filename, 1 , _nodeDataPtr->expHome );
  
   /* name and path of *waiting.interUser* file */ 
   if (  loopArgs != NULL && strlen(loopArgs) != 0 ) {
            sprintf(filename,"%s%s%s%s_%s.waiting.interUser.%s", _nodeDataPtr->expHome, INTER_DEPENDS_DIR, _nodeDataPtr->datestamp, _nodeDataPtr->name, loopArgs, _target_state);
   } else {
            sprintf(filename,"%s%s%s%s.waiting.interUser.%s", _nodeDataPtr->expHome, INTER_DEPENDS_DIR, _nodeDataPtr->datestamp, _nodeDataPtr->name, _target_state);
   }
/*
   snprintf(depParam,sizeof(depParam)," <type>ForEach</type>\n <xp>%s</xp>\n <node>%s</node>\n <indx>%s</indx>\n <xdate>%s</xdate>\n <status>%s</status>\n <largs>%s</largs>\n <lindex>%s</lindex>\
<sxp>%s</sxp>\n <snode>%s</snode>\n <sxdate>%s</sxdate>\n <slargs>%s</slargs>\n <lock>%s</lock>\n <container>%s</container>\n \
<mdomain>%s</mdomain>\n <mversion>%s</mversion>\n <regtime date=\"%s\" epoch=\"%ld\" />\n <flow>%s</flow>\n<key>%s</key>\n", _nodeDataPtr->forEachTarget->exp,_nodeDataPtr->forEachTarget->node,loopArgs,_target_datestamp,_target_state,_dep_index,_nodeDataPtr->expHome,_nodeDataPtr->name,_nodeDataPtr->datestamp,loopArgs,statusFile,_nodeDataPtr->container,maestro_shortcut,maestro_version,registration_time,current_time,_flow,md5sum);
*/

   /* get the protocole : polling(P), .... others to be determined  */
/*
   if ( strncmp(_dep_prot,"pol",3) == 0 || strncmp(_dep_prot,"ocm",3) == 0 ) {      
             snprintf(depBuf,2048,"<dep type=\"pol\">\n%s</dep> ",depParam);
   } else  {
             raiseError("Invalid string for specifying polling dependency type:%s\n",_dep_prot);
   }
*/
   /* TODO -> if ret is not 0 dont write to nodelogger the wait line */
   ret = ! _WriteInterUserDepFile(filename, depBuf, current_passwd->pw_dir, maestro_version,_nodeDataPtr->datestamp, md5sum);
   
   /* check if server is active, if not, send warning that dep will not launch until server is active */
   if ( MLLServerConnectionFid < 0 ) {
      nodelogger( _nodeDataPtr->name, "info", _nodeDataPtr->extension, "ForEach node registered to mserver but mserver inactive. Run mserver to be able to launch future iterations.",_nodeDataPtr->datestamp, _nodeDataPtr->expHome);
   }

   free(md5sum);
   md5sum=NULL;
   free(loopArgs);
   free(depBase);

   return(ret);


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
   SeqUtil_TRACE(TL_FULL_TRACE, "********** processContainerEnd() calling maestro -s endx -n %s with loop args=%s\n", _nodeDataPtr->name, SeqLoops_getLoopArgs(newArgs)  );

   SeqUtil_TRACE(TL_FULL_TRACE, "processContainerEnd() :_nodeDataPtr->loopArgs : \n");
   SeqNameValues_printList(_nodeDataPtr->loop_args);
   SeqUtil_TRACE(TL_FULL_TRACE,"processContainerEnd() :_nodeDataPtr->nodeName : %s\n", _nodeDataPtr->nodeName);
   SeqUtil_TRACE(TL_FULL_TRACE,"processContainerEnd() :_nodeDataPtr->name : %s\n", _nodeDataPtr->name);
   SeqUtil_TRACE(TL_FULL_TRACE, "processContainerEnd(): _nodeDataPtr->type : %s\n", SeqNode_getTypeString(_nodeDataPtr->type));
   /* deal with L(i) ending -> end of L if all iterations are done, or Npass(i) -> Npass */
   if(hasArgs(_nodeDataPtr)) {
      SeqUtil_TRACE(TL_FULL_TRACE, "processContainerEnd(): Entered if (SeqLoops_getLoopAttribute() != NULL).\n");
      if ( (_nodeDataPtr->type == Loop && isLoopComplete(_nodeDataPtr) ) || (_nodeDataPtr->type == NpassTask && isNpassComplete (_nodeDataPtr)) || _nodeDataPtr->type == Switch ) {
         SeqNameValues_deleteItem(&newArgs, _nodeDataPtr->nodeName );
         SeqUtil_TRACE(TL_FULL_TRACE, "********** processContainerEnd() calling maestro -s endx -n %s with loop args=%s\n", _nodeDataPtr->name, SeqLoops_getLoopArgs(newArgs)  );
         maestro ( _nodeDataPtr->name, "endx", _flow, newArgs, 0, NULL, _nodeDataPtr->datestamp, _nodeDataPtr->expHome);
      }
   } else {
       /* all other cases will check siblings for end status */
       siblingIteratorPtr = _nodeDataPtr->siblings;
       SeqUtil_TRACE(TL_FULL_TRACE, "processContainerEnd() container=%s extension=%s\n", _nodeDataPtr->container, _nodeDataPtr->extension );
       if( strlen( _nodeDataPtr->extension ) > 0 ) {
          SeqUtil_stringAppend( &extWrite, "." );
          SeqUtil_stringAppend( &extWrite, _nodeDataPtr->extension );
       } else {
          SeqUtil_stringAppend( &extWrite, "" );
       }
       siblingIteratorPtr = _nodeDataPtr->siblings;
       if( siblingIteratorPtr != NULL ) {
          /*get the exp catchup*/
          catchup = catchup_get (_nodeDataPtr->expHome);
          /* check siblings's status for end or abort.continue or higher catchup */
          while(  siblingIteratorPtr != NULL && undoneChild == 0 ) {
             memset( endfile, '\0', sizeof endfile );
             memset( continuefile, '\0', sizeof continuefile );
             sprintf(endfile,"%s/%s/%s/%s%s.end", _nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->container, siblingIteratorPtr->data, extWrite);
             sprintf(continuefile,"%s/%s/%s/%s%s.abort.cont", _nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->container, siblingIteratorPtr->data, extWrite);
             undoneChild = ! (_isFileExists( endfile,      "processContainerEnd()", _nodeDataPtr->expHome) || 
                              _isFileExists( continuefile, "processContainerEnd()", _nodeDataPtr->expHome)    );
             if ( undoneChild ) {
             /* check if it's a discretionary or catchup higher than job's value, bypass if yes */
                 memset( tmp, '\0', sizeof tmp );
                 sprintf(tmp, "%s/%s", _nodeDataPtr->container, siblingIteratorPtr->data);
                 SeqUtil_TRACE(TL_FULL_TRACE, "maestro.processContainerEnd() getting sibling info: %s\n", tmp );
                 siblingDataPtr = nodeinfo( tmp,NI_SHOW_TYPE|NI_SHOW_RESOURCE,
                                             NULL, _nodeDataPtr->expHome,
                                             NULL, _nodeDataPtr->datestamp,NULL);
                 if (catchup != CatchupStop) {
                     if ( siblingDataPtr->catchup > catchup )  {
                         /*reset undoneChild since we're skipping this node*/
                         undoneChild = 0;
                         SeqUtil_TRACE(TL_FULL_TRACE, "maestro.processContainerEnd() bypassing discretionary or higher catchup node: %s\n", siblingIteratorPtr->data );
                     }
                 } else {
                   /* catchup is set to stop, skip if discretionary only */
                    if ( siblingDataPtr->catchup == CatchupDiscretionary )  {
                         /*reset undoneChild since we're skipping this node*/
                         undoneChild = 0;
                         SeqUtil_TRACE(TL_FULL_TRACE, "maestro.processContainerEnd() bypassing discretionary node: %s\n", siblingIteratorPtr->data );
                    }
                 } 
             }
             siblingIteratorPtr = siblingIteratorPtr->nextPtr;
          }
       }

       if( undoneChild == 0 ) {
          SeqUtil_TRACE(TL_FULL_TRACE, "********** processContainerEnd() calling maestro -s endx -n %s with loop args=%s\n", _nodeDataPtr->container, SeqLoops_getLoopArgs(newArgs)  );
          maestro ( _nodeDataPtr->container, "endx", _flow, newArgs, 0, NULL, _nodeDataPtr->datestamp, _nodeDataPtr->expHome);
       }
    }
   SeqNameValues_deleteWholeList(&newArgs);
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

static int go_submit(const char *_signal, char *_flow , const SeqNodeDataPtr _nodeDataPtr, int ignoreAllDeps ) {
   char tmpfile[SEQ_MAXFIELD], noendwrap[12],immediateMode[11], nodeFullPath[SEQ_MAXFIELD], workerEndFile[SEQ_MAXFIELD], workerAbortFile[SEQ_MAXFIELD], submissionDir[SEQ_MAXFIELD];
   char listingDir[SEQ_MAXFIELD], defFile[SEQ_MAXFIELD], nodetracercmd[SEQ_MAXFIELD];
   char cmd[SEQ_MAXFIELD];
   char pidbuf[100];
   char *cpu = NULL, *tmpdir=NULL;
   char *tmpCfgFile = NULL, *tmpTarPath=NULL, *tarFile=NULL, *movedTmpName=NULL, *movedTarFile=NULL, *readyFile=NULL, *prefix=NULL, *jobName=NULL, *workq=NULL;
   char *loopArgs = NULL, *extName = NULL, *fullExtName = NULL, *containerMethod = NULL;
   int catchup = CatchupNormal;
   int error_status = 0, nodetracer_status=0, ret;
   SeqNodeDataPtr workerDataPtr = NULL;
   char mpi_flag[5];

   loopArgs = (char*) SeqLoops_getLoopArgs( _nodeDataPtr->loop_args );
   
   SeqUtil_TRACE(TL_FULL_TRACE, "maestro.go_submit() node=%s signal=%s flow=%s ignoreAllDeps=%d\n ", _nodeDataPtr->name, _signal, _flow, ignoreAllDeps );
   actions( (char*) _signal, _flow, _nodeDataPtr->name );
   memset(nodeFullPath,'\0',sizeof nodeFullPath);
   memset(listingDir,'\0',sizeof listingDir);
   memset(defFile,'\0',sizeof defFile);
   sprintf( nodeFullPath, "%s/modules/%s.tsk", _nodeDataPtr->expHome, _nodeDataPtr->taskPath );
   sprintf( listingDir, "%s/sequencing/output/%s", _nodeDataPtr->expHome, _nodeDataPtr->container );
   sprintf( defFile, "%s/resources/resources.def", _nodeDataPtr->expHome);
   _SeqUtil_mkdir( listingDir, 1, _nodeDataPtr->expHome );

   if (!(strlen( _nodeDataPtr->extension ) > 0)) {
       sprintf(submissionDir, "%s/sequencing/output%s.submission.%s.pgmout%d", _nodeDataPtr->expHome, _nodeDataPtr->name, _nodeDataPtr->datestamp, getpid());
   } else {
       sprintf(submissionDir, "%s/sequencing/output%s.%s.submission.%s.pgmout%d", _nodeDataPtr->expHome, _nodeDataPtr->name, _nodeDataPtr->extension, _nodeDataPtr->datestamp, getpid());
   }
   
   SeqUtil_stringAppend( &fullExtName, _nodeDataPtr->name );
   if( strlen( _nodeDataPtr->extension ) > 0 ) {
      SeqUtil_stringAppend( &fullExtName, "." );
      SeqUtil_stringAppend( &fullExtName, _nodeDataPtr->extension );
   }

   /* get exp catchup value */
   catchup = catchup_get(_nodeDataPtr->expHome);
   /* check catchup value of the node */
   SeqUtil_TRACE(TL_FULL_TRACE,"node catchup= %d , exp catchup = %d , discretionary catchup = %d  \n",_nodeDataPtr->catchup, catchup, CatchupDiscretionary );
   if (_nodeDataPtr->catchup > catchup && !ignoreAllDeps ) {
      if (_nodeDataPtr->catchup == CatchupDiscretionary ) {
         SeqUtil_TRACE(TL_FULL_TRACE,"nodelogger: -n %s -l %s \"%s\"\n", _nodeDataPtr->name, _nodeDataPtr->extension, CATCHUP_DISCR_MSG );
         nodelogger( _nodeDataPtr->name ,"discret", _nodeDataPtr->extension, CATCHUP_DISCR_MSG,_nodeDataPtr->datestamp, _nodeDataPtr->expHome);
      } else {
         SeqUtil_TRACE(TL_FULL_TRACE,"nodelogger: -n %s -l %s \"%s\"\n", _nodeDataPtr->name, _nodeDataPtr->extension, CATCHUP_UNSUBMIT_MSG );
         nodelogger( _nodeDataPtr->name ,"catchup", _nodeDataPtr->extension, CATCHUP_UNSUBMIT_MSG,_nodeDataPtr->datestamp, _nodeDataPtr->expHome);
      }
      return(0);
   }

   /* check ignoreAllDeps first so that it does not write the waiting file */
   if ( ignoreAllDeps == 1 || validateDependencies( _nodeDataPtr, _flow ) == 0 ) {
      setSubmitState( _nodeDataPtr ) ;
      nodesubmit(_nodeDataPtr, _nodeDataPtr->datestamp);

      /* dependencies are satisfied */
      SeqUtil_stringAppend( &tmpCfgFile, _nodeDataPtr->expHome);
      SeqUtil_stringAppend( &tmpCfgFile, "/sequencing/tmpfile/" );
      SeqUtil_stringAppend( &tmpdir, tmpCfgFile );
      SeqUtil_stringAppend( &tmpdir, _nodeDataPtr->container );
      _SeqUtil_mkdir( tmpdir, 1, _nodeDataPtr->expHome );

      SeqUtil_stringAppend( &tmpCfgFile, fullExtName );
      sprintf(pidbuf, "%d", getpid() ); 
      SeqUtil_stringAppend( &tmpCfgFile, "." );
      SeqUtil_stringAppend( &tmpCfgFile, pidbuf );
      SeqUtil_stringAppend( &tmpCfgFile, ".cfg" );
       
      if ( _access(tmpCfgFile, R_OK, _nodeDataPtr->expHome) == 0) ret=_removeFile(tmpCfgFile, _nodeDataPtr->expHome); 

      SeqNode_generateConfig( _nodeDataPtr, _flow, tmpCfgFile );
      cpu = (char *) SeqUtil_cpuCalculate( _nodeDataPtr->npex, _nodeDataPtr->npey, _nodeDataPtr->omp, _nodeDataPtr->cpu_multiplier );

      /* get short name w/ extension i.e. job+3 */
      SeqUtil_stringAppend( &extName, _nodeDataPtr->nodeName );
      if( strlen( _nodeDataPtr->extension ) > 0 ) {
         SeqUtil_stringAppend( &extName, "." );
         SeqUtil_stringAppend( &extName, _nodeDataPtr->extension );
      }
      if ( (prefix = SeqUtil_getdef( defFile, "SEQ_JOBNAME_PREFIX", _nodeDataPtr->expHome )) != NULL ){
          SeqUtil_stringAppend( &jobName, prefix );
          SeqUtil_stringAppend( &jobName, extName );
      } else {
          jobName=strdup(extName);
      }

      /* for cosched and possibly container */
      if ( (_nodeDataPtr->workq !=NULL ) && strlen(_nodeDataPtr->workq) != 0 ) {
          SeqUtil_stringAppend( &workq, "-workqueue ");
          SeqUtil_stringAppend( &workq, _nodeDataPtr->expHome );
          SeqUtil_stringAppend( &workq, "/sequencing/tmpfile/" );
          SeqUtil_stringAppend( &workq, _nodeDataPtr->datestamp );
          SeqUtil_stringAppend( &workq, _nodeDataPtr->workq );
          _SeqUtil_mkdir(workq,1, _nodeDataPtr->expHome); 
      } else SeqUtil_stringAppend( &workq,"");
      
      /* get mpi flag for the ord_soumet call */
      if ( _nodeDataPtr->mpi == 1 )
	      sprintf(mpi_flag, "-mpi");
      else
	      sprintf(mpi_flag, "");
      
	  /* get immediate flag for the ord_soumet call */
      if ( _nodeDataPtr->immediateMode == 1 )
	      sprintf(immediateMode, "-immediate");
      else
	      sprintf(immediateMode, "");

      /* go and submit the job */
      if ( _nodeDataPtr->type == Task || _nodeDataPtr->type == NpassTask ) {
         
	     /* check if it's work_unit SEQ_WORKER_PATH is not 0 -> immediate mode in ord_soumet (with jobtar option) else it's normal submission  */
	     if (strcmp(_nodeDataPtr->workerPath, "") != 0) {
	     /* create tar of the job in the worker's path */
	         if (tmpTarPath=malloc(strlen(_nodeDataPtr->expHome) + strlen("/sequencing/tmpfile/") + strlen(_nodeDataPtr->datestamp) + strlen("/work_unit_depot/") + 
                                  strlen(_nodeDataPtr->workerPath) + strlen("/") + strlen(_nodeDataPtr->container) +1)) {
	            sprintf(tmpTarPath, "%s/sequencing/tmpfile/%s/work_unit_depot/%s/%s",_nodeDataPtr->expHome, _nodeDataPtr->datestamp ,_nodeDataPtr->workerPath,_nodeDataPtr->container ); 
            } else {
               raiseError("OutOfMemory exception in maestro.go_submit()\n");
            }
	         _SeqUtil_mkdir(tmpTarPath,1, _nodeDataPtr->expHome); 
	         /*clean up tar file in case it's there*/
            if (tarFile=malloc(strlen(tmpTarPath) + strlen("/") + strlen(extName) + strlen(".tar")+1)){
	            sprintf(tarFile, "%s/%s.tar", tmpTarPath, extName);
            } else {
               raiseError("OutOfMemory exception in maestro.go_submit()\n");
            }
            if ( movedTarFile=malloc(strlen(tmpTarPath) + strlen("/") + strlen(extName) + strlen(".tmp")+ strlen(".tar")+1)) {
	            sprintf(movedTarFile, "%s/%s.tmp.tar", tmpTarPath, extName);
            } else {
               raiseError("OutOfMemory exception in maestro.go_submit()\n");
            }
            if ( movedTmpName=malloc(strlen(tmpTarPath) + strlen("/") + strlen(extName) + strlen(".tmp")+1)) {
	            sprintf(movedTmpName, "%s/%s.tmp", tmpTarPath, extName);
            } else {
               raiseError("OutOfMemory exception in maestro.go_submit()\n");
            }
	         if ( readyFile=malloc(strlen(tmpTarPath) +strlen("/") + strlen(extName) + strlen(".tar.ready") + 1 )) {
    	         sprintf(readyFile,"%s/%s.tar.ready", tmpTarPath, extName);
            } else {
               raiseError("OutOfMemory exception in maestro.go_submit()\n");
            }

	         ret=_removeFile(tarFile, _nodeDataPtr->expHome); 
	         ret=_removeFile(movedTarFile, _nodeDataPtr->expHome); 
	         ret=_removeFile(readyFile, _nodeDataPtr->expHome); 

             sprintf(cmd,"%s %s -sys maestro_%s -jobfile %s -node %s -jn %s -d %s -q %s %s -c %s -shell %s -m %s -w %d -v -listing %s -wrapdir %s/sequencing -jobcfg %s -nosubmit -step work_unit -jobtar %s -altcfgdir %s %s -args \"%s\" %s",submit_tool,workq ,getenv("SEQ_MAESTRO_VERSION"), nodeFullPath, _nodeDataPtr->name, jobName,_nodeDataPtr->machine,_nodeDataPtr->queue,mpi_flag,cpu,_nodeDataPtr->shell,_nodeDataPtr->memory,_nodeDataPtr->wallclock, listingDir, _nodeDataPtr->expHome, tmpCfgFile, movedTmpName, getenv("SEQ_BIN"), immediateMode, _nodeDataPtr->args, _nodeDataPtr->soumetArgs);

	         /*check if the running worker has not ended. If it has, launch another one.*/
            workerDataPtr =  nodeinfo( _nodeDataPtr->workerPath, NI_SHOW_ALL,
                                       _nodeDataPtr->loop_args, _nodeDataPtr->expHome,
                                       NULL, _nodeDataPtr->datestamp,NULL );
            SeqLoops_validateLoopArgs( workerDataPtr, _nodeDataPtr->loop_args );        
            memset(workerEndFile,'\0',sizeof workerEndFile);
            memset(workerAbortFile,'\0',sizeof workerAbortFile);
            SeqUtil_TRACE(TL_FULL_TRACE,"maestro.go_submit() checking for worker's extension: %s\n", workerDataPtr->extension);

            if( strlen( workerDataPtr->extension ) > 0 ) {
               sprintf(workerEndFile,"%s/%s/%s.%s.end", _nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->workerPath, workerDataPtr->extension);
            } else {
               sprintf(workerEndFile,"%s/%s/%s.end", _nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->workerPath);
            }

            if( strlen( workerDataPtr->extension ) > 0 ) {
               sprintf(workerAbortFile,"%s/%s/%s.%s.abort.stop", _nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->workerPath, workerDataPtr->extension);
            } else  {
               sprintf(workerAbortFile,"%s/%s/%s.abort.stop", _nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->workerPath);
            }

            SeqUtil_TRACE(TL_FULL_TRACE,"maestro.go_submit() checking for worker status %s or %s", workerEndFile, workerAbortFile);

            if ( (_access(workerEndFile, R_OK, _nodeDataPtr->expHome) == 0) || 
                  (_access(workerAbortFile, R_OK, _nodeDataPtr->expHome) == 0  ) ) {
               SeqUtil_TRACE(TL_FULL_TRACE," Running maestro -s submit on %s\n", _nodeDataPtr->workerPath); 
               /*
                * Erroneous call to maestro:
                * maestro ( _nodeDataPtr->workerPath, "submit", "continue" , workerDataPtr->loop_args , 0, NULL, _nodeDataPtr->expHome);
                * Incorrect number of arguments
                * int maestro( char* _node, char* _sign, char* _flow, SeqNameValuesPtr _loops, int ignoreAllDeps, char * _extraArgs, char *_datestamp, char* _seq_exp_home);
                * IgnoreAllDeps is 0, extraArgs is NULL, and no datestamp is
                * specified.  It was missing an argument before Phil added the
                * ndp->expHome argument at the end, thinking the call was
                * already correct.  Anyway, I'll put ndp->datestamp because
                * that's what is in the other go_something functions.
                */
               maestro ( _nodeDataPtr->workerPath, "submit", "continue" , workerDataPtr->loop_args , 0, NULL, _nodeDataPtr->datestamp,_nodeDataPtr->expHome);
            }
            SeqNode_freeNode( workerDataPtr );
        
         } else {
             /* normal task submission */
             sprintf(cmd,"%s %s -sys maestro_%s -jobfile %s -node %s -jn %s -d %s -q %s %s -c %s -shell %s -m %s -w %d -v -listing %s -wrapdir %s/sequencing -jobcfg %s -altcfgdir %s %s -args \"%s\" %s",submit_tool,workq , getenv("SEQ_MAESTRO_VERSION"), nodeFullPath, _nodeDataPtr->name, jobName,_nodeDataPtr->machine,_nodeDataPtr->queue,mpi_flag,cpu,_nodeDataPtr->shell,_nodeDataPtr->memory,_nodeDataPtr->wallclock, listingDir, _nodeDataPtr->expHome, tmpCfgFile, getenv("SEQ_BIN"),immediateMode, _nodeDataPtr->args, _nodeDataPtr->soumetArgs);
         }

         SeqUtil_TRACE(TL_FULL_TRACE,"Temporarily sending submission output to %s\n", submissionDir );
         strcat(cmd, " > \""); strcat (cmd, submissionDir); strcat (cmd, "\" 2>&1");
         fprintf(stderr,"Task type node submit command: %s\n", cmd );
         error_status = system(cmd);
         error_status = WEXITSTATUS(error_status);

         SeqUtil_TRACE(TL_FULL_TRACE,"maestro.go_submit() ord return status: %d \n",error_status);
         if (strcmp(_nodeDataPtr->workerPath, "") != 0) {
	         rename(movedTarFile,tarFile);
	         SeqUtil_TRACE(TL_FULL_TRACE,"maestro.go_submit() moving temporary tar file %s to %s \n", movedTarFile, tarFile); 
	         _touch(readyFile, _nodeDataPtr->expHome);
	      }

	 /* 
	    remove  *waiting.interUser* file if ignore dependency.
	    AT any time, we will only have 1 dependency file (for any task) in inter_depend_dir, 
	    we should remove the dep. file and mserver will remove link ($datestamp_$key) 
	    in $HOME/.suites/maestrod/dependencies/polling/$version/ when the link became
	    a dangling link. I have hardcoded depstatus to "end", to minimize code and cuse this
	    is the status that we are depending on Now.
	 */
         if ( ignoreAllDeps == 1 ) {
            memset(tmpfile,'\0',sizeof tmpfile);
            if (  loopArgs != NULL && strlen(loopArgs) != 0 ) {
               snprintf(tmpfile,sizeof(tmpfile),"%s%s%s%s_%s.waiting.interUser.end", _nodeDataPtr->expHome, INTER_DEPENDS_DIR, _nodeDataPtr->datestamp, _nodeDataPtr->name, loopArgs);
            } else {
               snprintf(tmpfile,sizeof(tmpfile),"%s%s%s%s.waiting.interUser.end", _nodeDataPtr->expHome, INTER_DEPENDS_DIR, _nodeDataPtr->datestamp, _nodeDataPtr->name);
            }
            if ( _access(tmpfile , R_OK, _nodeDataPtr->expHome) == 0 ) {
               SeqUtil_TRACE(TL_MEDIUM,"removing dependency file:%s\n",tmpfile);
               ret=unlink(tmpfile);
            }
         }

      } else {   /* container */
         memset( noendwrap, '\0', sizeof( noendwrap ) );
         memset(tmpfile,'\0',sizeof tmpfile);
         sprintf(tmpfile,"%s/sequencing/tmpfile/container.tsk",_nodeDataPtr->expHome);
         if ( _touch(tmpfile, _nodeDataPtr->expHome) != 0 ) raiseError( "Cannot create lockfile: %s\n", tmpfile );

         if ((( _nodeDataPtr->type == Switch ) ||  _nodeDataPtr->type == Loop  ) && ((char*) SeqLoops_getLoopAttribute( _nodeDataPtr->loop_args, _nodeDataPtr->nodeName ) == NULL)) {
            strcpy( noendwrap, "-noendwrap" );
         } else {     /* All other containers need to check if it submits nothing*/ 
            _nodeDataPtr->submits == NULL ? strcpy( noendwrap, "" ) : strcpy( noendwrap, "-noendwrap" ) ;
         }
  
         containerMethod = SeqUtil_getdef( defFile, "SEQ_CONTAINER_METHOD", _nodeDataPtr->expHome);
         if ( (containerMethod == NULL) || (strcmp(containerMethod,"immediate") == 0)) {

            /* use immediate for containers on TRUE_HOST */
	         snprintf(cmd,sizeof(cmd),"%s %s -sys maestro_%s -jobfile %s -node %s -jn %s -d %s -q %s %s -c %s -shell %s -m %s -w %d -v -listing %s -wrapdir %s/sequencing -immediate %s -jobcfg %s -altcfgdir %s -args \"%s\" %s",submit_tool,workq , getenv("SEQ_MAESTRO_VERSION"), tmpfile,_nodeDataPtr->name, jobName, getenv("TRUE_HOST"), _nodeDataPtr->queue,mpi_flag,cpu,_nodeDataPtr->shell,_nodeDataPtr->memory,_nodeDataPtr->wallclock, listingDir, _nodeDataPtr->expHome, noendwrap, tmpCfgFile, getenv("SEQ_BIN"),  _nodeDataPtr->args,_nodeDataPtr->soumetArgs);


         } else if (strcmp(containerMethod,"submit") == 0) {
            /* submit containers on designated queue */
	         snprintf(cmd,sizeof(cmd),"%s %s -sys maestro_%s -jobfile %s -node %s -jn %s -d %s -q %s %s -c %s -shell %s -m %s -w %d -v -listing %s -wrapdir %s/sequencing %s -jobcfg %s -altcfgdir %s -args \"%s\" %s",submit_tool,workq , getenv("SEQ_MAESTRO_VERSION"), tmpfile,_nodeDataPtr->name, jobName, _nodeDataPtr->machine, _nodeDataPtr->queue,mpi_flag,cpu,_nodeDataPtr->shell,_nodeDataPtr->memory,_nodeDataPtr->wallclock, listingDir, _nodeDataPtr->expHome, noendwrap, tmpCfgFile, getenv("SEQ_BIN"),  _nodeDataPtr->args,_nodeDataPtr->soumetArgs);

         } else if (strcmp(containerMethod,"exec") == 0) {
            /* execute containers on current host, not supported yet */
	         snprintf(cmd,sizeof(cmd),"%s %s -sys maestro_%s -jobfile %s -node %s -jn %s -d %s -q %s %s -c %s -shell %s -m %s -w %d -v -listing %s -wrapdir %s/sequencing -immediate %s -jobcfg %s -altcfgdir %s -args \"%s\" %s",submit_tool,workq , getenv("SEQ_MAESTRO_VERSION"), tmpfile,_nodeDataPtr->name, jobName, getenv("TRUE_HOST"), _nodeDataPtr->queue,mpi_flag,cpu,_nodeDataPtr->shell,_nodeDataPtr->memory,_nodeDataPtr->wallclock, listingDir, _nodeDataPtr->expHome, noendwrap, tmpCfgFile, getenv("SEQ_BIN"),  _nodeDataPtr->args,_nodeDataPtr->soumetArgs);
         } else {
         /* default: use immediate for containers on TRUE_HOST */
	         snprintf(cmd,sizeof(cmd),"%s %s -sys maestro_%s -jobfile %s -node %s -jn %s -d %s -q %s %s -c %s -shell %s -m %s -w %d -v -listing %s -wrapdir %s/sequencing -immediate %s -jobcfg %s -altcfgdir %s -args \"%s\" %s",submit_tool,workq , getenv("SEQ_MAESTRO_VERSION"), tmpfile,_nodeDataPtr->name, jobName, getenv("TRUE_HOST"), _nodeDataPtr->queue,mpi_flag,cpu,_nodeDataPtr->shell,_nodeDataPtr->memory,_nodeDataPtr->wallclock, listingDir, _nodeDataPtr->expHome, noendwrap, tmpCfgFile, getenv("SEQ_BIN"),  _nodeDataPtr->args,_nodeDataPtr->soumetArgs);

         }
         strcat(cmd, " > \""); strcat (cmd, submissionDir); strcat (cmd, "\" 2>&1");
         fprintf(stderr,"Container submit command: %s\n", cmd );
         error_status=system(cmd);

         SeqUtil_TRACE(TL_FULL_TRACE,"maestro.go_submit() ord return status: %d \n",error_status);
      }
      if ( strlen( loopArgs ) > 0 ) {
         sprintf(nodetracercmd, "%s/nodetracer -n %s -l %s -d %s -e %s -type submission -i %s", getenv("SEQ_UTILS_BIN"), _nodeDataPtr->name, loopArgs, _nodeDataPtr->datestamp, _nodeDataPtr->expHome, submissionDir);
      } else {
         sprintf(nodetracercmd, "%s/nodetracer -n %s -d %s -e %s -type submission -i %s", getenv("SEQ_UTILS_BIN"), _nodeDataPtr->name, _nodeDataPtr->datestamp, _nodeDataPtr->expHome, submissionDir);
      }
      nodetracer_status = system(nodetracercmd);
      if ( nodetracer_status ) {
         SeqUtil_TRACE(TL_CRITICAL,"Problem with nodetracer call, listing may not be available in the listings directory, possibly in $SEQ_EXP_HOME/sequencing/output.\n");
      }
   }

   actionsEnd( (char*) _signal, _flow, _nodeDataPtr->name );
   free( cpu );
   free( loopArgs ); 
   free( tmpCfgFile );
   free( tmpTarPath );
   free( tarFile); 
   free( movedTarFile); 
   free( movedTmpName); 
   free( readyFile); 
   free( fullExtName );
   free( extName );
   free( jobName );
   free( prefix );
   free( workq );
   free( containerMethod );
   SeqUtil_TRACE(TL_FULL_TRACE,"maestro.go_submit returning %d\n", error_status);
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

   extName =(char *) SeqNode_extension( _nodeDataPtr );      

   /* clear any other state */
   clearAllOtherStates( _nodeDataPtr, extName, "maestro.setSubmitState()", "submit" ); 

   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s/%s.submit",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, extName); 

   /* create the node end lock file */

   _CreateLockFile(MLLServerConnectionFid , filename , "setSubmitState() ", _nodeDataPtr->expHome);

   free( extName );
}

/*
submitNodeList

Submits a list of nodes.

Inputs:
  _nodeDataPtr  - pointer to the node whose submission list is to be executed

*/
static void submitNodeList (const SeqNodeDataPtr _nodeDataPtr) {
	LISTNODEPTR listIteratorPtr = _nodeDataPtr->submits;
	int counter = 0;
	SeqUtil_TRACE(TL_FULL_TRACE, "maestro.submitNodeList() called on %s \n", _nodeDataPtr->name );
	while (listIteratorPtr != NULL) {
		SeqUtil_TRACE(TL_FULL_TRACE, "maestro.submitNodeList() submits node %s\n", listIteratorPtr->data );
		maestro ( listIteratorPtr->data, "submit", "continue" ,  _nodeDataPtr->loop_args, 0, NULL, _nodeDataPtr->datestamp , _nodeDataPtr->expHome);
		/*flood control, wait CONTAINER_FLOOD_TIMER seconds between sets of submits*/ 
		if (counter >= CONTAINER_FLOOD_LIMIT){
			counter = 0; 
			usleep(1000000 * CONTAINER_FLOOD_TIMER);
		}
		++counter;
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
   SeqUtil_TRACE(TL_FULL_TRACE, "maestro.submitLoopSetNodeList() container_args_ptr=%s loopset_index=%s\n", SeqLoops_getLoopArgs(container_args_ptr), SeqLoops_getLoopArgs( loopset_index ) );
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
      SeqUtil_TRACE(TL_FULL_TRACE, "submitLoopSetNodeList calling maestro -n %s -s submit -l %s -f continue\n", _nodeDataPtr->name, SeqLoops_getLoopArgs( cmdLoopArg));
      maestro ( _nodeDataPtr->name, "submit", "continue" , cmdLoopArg, 0, NULL, _nodeDataPtr->datestamp ,_nodeDataPtr->expHome);
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




   extName = (char *)SeqNode_extension( _nodeDataPtr );     

   /* create the node waiting lock file name*/
   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s/%s.waiting",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, extName); 

   if (waitMsg = (char *) malloc ( strlen( waited_one ) + strlen( waited_status ) + 2)){ 
       sprintf( waitMsg, "%s %s", waited_status, waited_one );
   } else {
       raiseError("OutOfMemory exception in maestro.setWaitingState()\n");
   }
   nodewait( _nodeDataPtr, waitMsg, _nodeDataPtr->datestamp);  

   /* clear any other state */
   clearAllOtherStates( _nodeDataPtr, extName, "maestro.setWaitingState()", "waiting" ); 

   _CreateLockFile(MLLServerConnectionFid , filename , "setWaitingState() ", _nodeDataPtr->expHome);

   free( extName );
   free( waitMsg );
}

/********************************************************************************
 * submitDependencies(): Submits nodes that are waiting for this node
 * (_nodeDataPtr) to finish.
 *
 * The function looks through it's waited_end file for local and remote
 * experiments.  For each line in these files, it submits the corresponding node
 * if this node is still in waiting state.  
 *
 * If one of the submission fails, the line describing the dependency is put
 * back into the waited_end file, a nodelogger message is emitted and submission
 * of the following nodes continues.
 *
 * Inputs:
 * _nodeDataPtr - pointer to the node targetted by the execution
 * _signal - pointer to the signal being checked
 * _flow        - Controls the decision of whether or not to submit the
 *                dependencies.  If flow is not "continue", then instead of
 *                submitting, a nodelogger message is emitted and the dependency
 *                stays in the waited_end file
 *
 * NOTE: Since the execution of a maestro() call can encounter an exit statement
 * if there is something wrong with the dependency, we are using system calls so
 * that the submissions happen in a child process, and therefore exit calls will
 * terminate the child and not the current process. Defining the macro
 * SEQ_USE_SYSTEM_CALLS_FOR_DEPS controls whether submission is done with a
 * system call or with a function call.
 ********************************************************************************/
#define SEQ_USE_SYSTEM_CALLS_FOR_DEPS
static void submitDependencies ( const SeqNodeDataPtr _nodeDataPtr, const char* _signal, const char* _flow ) {
   char line[512];
   char nodelogger_msg[SEQ_MAXFIELD];
   FILE* waitedFilePtr = NULL;
   SeqNameValuesPtr loopArgsPtr = NULL, depNVArgs = NULL;
   char depExp[256] = {'\0'}, depNode[256] = {'\0'}, depArgs[SEQ_MAXFIELD] = {'\0'}, depDatestamp[20] = {'\0'};
   char waited_filename[SEQ_MAXFIELD] = {'\0'}, submitCmd[SEQ_MAXFIELD] = {'\0'}, statusFile[SEQ_MAXFIELD] = {'\0'};
   char *extName = NULL, * depExtension = NULL, *tmpValue=NULL, *tmpExt=NULL;
   int submitCode = 0, count = 0, line_count=0, ret;
   LISTNODEPTR submittedList = NULL, dependencyLines = NULL, current_dep_line = NULL;
#ifdef SEQ_USE_SYSTEM_CALLS_FOR_DEPS
   char * submitDepArgs = NULL;
   SeqUtil_TRACE(TL_FULL_TRACE,"submitDependencies() in system call mode\n");
#else
   SeqNameValuesPtr submitDepArgs = NULL;
   SeqUtil_TRACE(TL_FULL_TRACE,"submitDependencies() in function call mode\n");
#endif

   SeqUtil_TRACE(TL_FULL_TRACE, "maestro.submitDependencies() executing for %s\n", _nodeDataPtr->nodeName );

   loopArgsPtr = _nodeDataPtr->loop_args;
   if (_nodeDataPtr->isLastArg){
      tmpValue = SeqUtil_striplast( SeqNameValues_getValue(loopArgsPtr, _nodeDataPtr->nodeName) );
      SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_submitDependencies Found ^last argument, replacing %s for %s for node %s \n", 
		    SeqNameValues_getValue(loopArgsPtr, _nodeDataPtr->nodeName), tmpValue, _nodeDataPtr->nodeName); 
      SeqNameValues_setValue( &loopArgsPtr, _nodeDataPtr->nodeName, tmpValue);
   }
   tmpExt = (char*) SeqLoops_getExtFromLoopArgs(loopArgsPtr);
   SeqUtil_stringAppend( &extName, _nodeDataPtr->name );
   if( strlen( _nodeDataPtr->extension ) > 0 ) {
      SeqUtil_stringAppend( &extName, "." );
      SeqUtil_stringAppend( &extName, tmpExt );
   } 

   /* local dependencies (same exp) are fetched from sequencing/status/depends/$datestamp,
      remote dependencies are fetched from sequencing/status/remote_depends/$datestamp */
   for( count=0; count < 2; count++ ) {
      if( count == 0 ) {
         /* local dependencies */
         sprintf(waited_filename,"%s%s%s/%s.waited_%s", _nodeDataPtr->expHome, LOCAL_DEPENDS_DIR, _nodeDataPtr->datestamp, extName, _signal );
      } else {
         /* remote dependencies */
         sprintf(waited_filename,"%s%s%s/%s.waited_%s", _nodeDataPtr->expHome, REMOTE_DEPENDS_DIR, _nodeDataPtr->datestamp, extName, _signal );
      }
      SeqUtil_TRACE(TL_FULL_TRACE, "maestro.submitDependencies() looking for waited file=%s\n", waited_filename );

      if ( _access(waited_filename, R_OK, _nodeDataPtr->expHome) == 0 ) {
         SeqUtil_TRACE(TL_FULL_TRACE, "maestro.submitDependencies() found waited file=%s\n", waited_filename );
         if ((waitedFilePtr = _fopen(waited_filename, MLLServerConnectionFid)) != NULL ) { 
            if ( strcmp( _flow, "continue" ) == 0 ){
               /* Read file into linked list of lines, delete file */
               while( fgets( line , sizeof(line), waitedFilePtr ) != NULL ){
                  SeqListNode_insertItem( &dependencyLines, strdup(line) );
               }
               fclose(waitedFilePtr);
               _removeFile(waited_filename, _nodeDataPtr->expHome);

               /* Process each line in the list */
               current_dep_line = dependencyLines;
               while ( current_dep_line != NULL ) {

                  /* Extract dependant node information from line */
                  SeqUtil_TRACE(TL_FULL_TRACE, "maestro.submitDependencies() from waited file line: %s\n", current_dep_line->data );
                  sscanf( current_dep_line->data, "exp=%s node=%s datestamp=%s args=%s", 
                     depExp, depNode, depDatestamp, depArgs );
                  SeqUtil_TRACE(TL_FULL_TRACE, "maestro.submitDependencies() waited file data depExp:%s depNode:%s depDatestamp:%s depArgs:%s\n", 
                     depExp, depNode, depDatestamp, depArgs );
                  /* is the dependant still waiting? if not, don't submit. */
                  if ( ! isNodeXState (depNode, depArgs, depDatestamp, depExp, "waiting") ) {
                      SeqUtil_TRACE(TL_FULL_TRACE, "maestro.submitDependencies() dependant node not currently in wait state, skipping submission. \n");
                      goto next;
                  }

                  /* Do the submit or send nodelogger message based on flow.  Avoid double submitting */
                  if ( ! SeqListNode_isItemExists( submittedList, depNode ) ) {
                     SeqListNode_insertItem( &submittedList, depNode );
                     /* Attempt to submit dependant node */
#ifdef SEQ_USE_SYSTEM_CALLS_FOR_DEPS
                     SeqUtil_TRACE(TL_FULL_TRACE,"submitDependencies(): Using system calls to submit %s\n",depNode);
                     if ( strlen( depArgs ) > 0 ) {
                        SeqUtil_stringAppend( &submitDepArgs, "-l " );
                     }
                     SeqUtil_stringAppend( &submitDepArgs, depArgs );
                     sprintf( submitCmd, "maestro -e %s -d %s -s submit -f continue -n %s %s",
                           depExp, depDatestamp, depNode, submitDepArgs ); 
                     SeqUtil_TRACE(TL_FULL_TRACE, "submitDependencies(): Running system command: %s\n", submitCmd);
                     submitCode = system ( submitCmd );
                     submitCode = WEXITSTATUS(submitCode); 
                     free(submitDepArgs);
                     submitDepArgs = NULL;
#else
                     SeqUtil_TRACE(TL_FULL_TRACE,"submitDependencies(): Using function call to submit dependency %s\n", depNode);
                     SeqUtil_TRACE(TL_FULL_TRACE, "submitDependencies calling maestro:\n\t\
                           maestro(%s, \"submit\", \"continue\", %s, 0, NULL, %s, %s );\n",
                           depNode, depArgs, depDatestamp, depExp );
                     SeqLoops_parseArgs(&submitDepArgs, depArgs);
                     submitCode = maestro( depNode, "submit", "continue", submitDepArgs, 0, NULL, depDatestamp, depExp);
                     SeqNameValues_deleteWholeList(&submitDepArgs);
#endif
                     SeqUtil_TRACE(TL_FULL_TRACE, "maestro.submitDependencies() submitCode: %d\n", submitCode);
                     if( submitCode != 0 ) {
                        depExtension = SeqLoops_getExtFromLoopArgs(depArgs);
                        sprintf(statusFile,"%s/sequencing/status/%s/%s.%s.%s", depExp, depDatestamp,depNode, depExtension, "end" );
                        free(depExtension);
                        depExtension = NULL;
                        /* Write the line back into the waited_file */
                        _WriteNWFile(depExp,depNode,depDatestamp,depArgs, waited_filename, statusFile);
                        SeqUtil_TRACE(TL_ERROR, "Error submitting node %s of experiment %s \n", depNode, depExp);
                        sprintf(nodelogger_msg, "An error occurred while submitting dependant node %s in experiment %s", depNode, depExp);
                        nodelogger(_nodeDataPtr->name, "info", _nodeDataPtr->extension, nodelogger_msg,
                              _nodeDataPtr->datestamp, _nodeDataPtr->expHome);
                     }
                  }
               next:
                  depExp[0] = '\0'; depNode[0] = '\0'; depDatestamp[0] = '\0'; depArgs[0] = '\0';
                  line_count++;
                  current_dep_line = current_dep_line->nextPtr;
               } /* end while loop */

               /* warn if file empty ... */
               if ( line_count == 0 ) raiseError( "waited_end file:%s (submitDependencies) EMPTY !!!! \n",waited_filename );
            } else {
               /* Flow is not "continue" : Simply emit a nodelogger messge for every line in the file */
               while ( fgets( line, sizeof(line), waitedFilePtr ) != NULL ) {
                  sscanf( line, "exp=%s node=%s datestamp=%s args=%s",
                     depExp, depNode, depDatestamp, depArgs );
                  SeqUtil_TRACE(TL_FULL_TRACE, "maestro.submitDependencies() read in line exp=%s node=%s datestamp=%s args=%s \n", depExp, depNode, depDatestamp,depArgs);
                  if ((depArgs != NULL) && (strlen(depArgs) > 0) && ( SeqLoops_parseArgs( &depNVArgs, depArgs) != -1))  {
                      depExtension = SeqLoops_getExtFromLoopArgs(depNVArgs);
                      SeqNameValues_deleteWholeList(&depNVArgs);
                  }
                  sprintf(nodelogger_msg, "Node was waiting on node %s%s of experiment %s which ended with flow set to stop : remaining in waiting state.",_nodeDataPtr->name,_nodeDataPtr->extension, _nodeDataPtr->expHome);
                  nodelogger(depNode, "info",depExtension, nodelogger_msg,depDatestamp,depExp);
                  free(depExtension);
                  depExtension = NULL;
               }  
               fclose(waitedFilePtr);
            }
         } else {
            raiseError( "maestro cannot read file: %s (submitDependencies) \n", waited_filename );
         } /* if ((waitedFilePtr = _fopen(waited_filename, MLLServerConnectionFid)) != NULL ) */
      } /* if ( _access(waited_filename, R_OK, _nodeDataPtr->expHome) == 0 ) */
   } /* for( count=0; count < 2; count++ ) */
   SeqListNode_deleteWholeList( &submittedList );
   free(extName);
   free(tmpExt);
   free(tmpValue);
}



/*
submitForEach 

 Checks if node has a ForEach node waiting on this, and read the message file to submit the right dependency if the submission hasn't been done yet. 

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution
  _signal - pointer to the signal being checked

*/
static void submitForEach ( const SeqNodeDataPtr _nodeDataPtr, const char* _signal ) {
   char line[512];
   FILE* FEFile = NULL;
   SeqNameValuesPtr loopArgsPtr = NULL, newArgs=NULL, poppedArgs=NULL;
   char depExp[256], depNode[256],depIndex[128], depArgs[SEQ_MAXFIELD], depDatestamp[15], poppedValue[128];
   char filename[SEQ_MAXFIELD], submitCmd[SEQ_MAXFIELD], extendedArgs[256];
   char *extName = NULL, *submitDepArgs = NULL, *tmpValue=NULL, *tmpExt=NULL;
   int submitCode = 0, line_count=0, ret, isLastIter=0;

   SeqUtil_TRACE(TL_FULL_TRACE, "maestro.submitForEach() executing for %s\n", _nodeDataPtr->nodeName );

   LISTNODEPTR cmdList = NULL;

   newArgs=SeqNameValues_clone(_nodeDataPtr->loop_args);
   SeqNameValues_popValue(&newArgs, poppedValue,sizeof(poppedValue));
   tmpExt = (char*) SeqLoops_getExtFromLoopArgs(newArgs);
   SeqUtil_stringAppend( &extName, _nodeDataPtr->name );
   if(  (tmpExt = (char*) SeqLoops_getExtFromLoopArgs(newArgs)) != NULL ) {
      SeqUtil_stringAppend( &extName, "." );
      SeqUtil_stringAppend( &extName, tmpExt );
   }
   ret=SeqLoops_parseArgs( &poppedArgs, poppedValue ); 

   /* not needed  
   if (_nodeDataPtr->isLastArg){
      tmpValue = SeqUtil_striplast( SeqNameValues_getValue(loopArgsPtr, _nodeDataPtr->nodeName) );
      SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_submitDependencies Found ^last argument, replacing %s for %s for node %s \n", 
		    SeqNameValues_getValue(loopArgsPtr, _nodeDataPtr->nodeName), tmpValue, _nodeDataPtr->nodeName); 
      SeqNameValues_setValue( &loopArgsPtr, _nodeDataPtr->nodeName, tmpValue);
   }
   */

   /* same user foreach dependencies */
   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s%s%s/%s.FE_%s", _nodeDataPtr->expHome, LOCAL_DEPENDS_DIR, _nodeDataPtr->datestamp, extName, _signal );
   SeqUtil_TRACE(TL_FULL_TRACE, "maestro.submitForEach() looking for FE file=%s\n", filename );

   if ( _access(filename, R_OK, _nodeDataPtr->expHome) == 0 ) {
      memset(submitCmd,'\0',sizeof submitCmd);
      /*isLastIter=SeqLoops_isLastIteration(_nodeDataPtr) */

      SeqUtil_TRACE(TL_FULL_TRACE, "maestro.submitForEach() found FE file=%s\n", filename );
         /* build a node list for all entries found in the waited file */
         if ((FEFile = _fopen(filename, MLLServerConnectionFid)) != NULL ) { 
            while ( fgets( line, sizeof(line), FEFile ) != NULL ) {
               memset(depExp,'\0',sizeof depExp);
               memset(depNode,'\0',sizeof depNode);
               memset(depDatestamp,'\0',sizeof depDatestamp);
               memset(depIndex,'\0',sizeof depIndex);
               memset(depArgs,'\0',sizeof depArgs);
               memset(extendedArgs,'\0',sizeof extendedArgs);
               line_count++;
               SeqUtil_TRACE(TL_FULL_TRACE, "maestro.submitForEach() from waited file line: %s\n", line );
               ret=sscanf( line, "exp=%s node=%s datestamp=%s index_to_add=%s args=%s", depExp, depNode, depDatestamp, depIndex, depArgs );
               if (ret != 5) {
                  ret=sscanf( line, "exp=%s node=%s datestamp=%s index_to_add=%s args=", depExp, depNode, depDatestamp, depIndex );
                  if (ret != 4) {
                     fprintf(stderr,"Format error in scanning current line: %s . Skipping. \n", line); 
                     continue;
                  }
               }
               SeqUtil_TRACE(TL_FULL_TRACE, "maestro.submitForEach() file line parsed depExp:%s depNode:%s depDatestamp:%s depIndex:%s depArgs:%s\n", 
                              depExp, depNode, depDatestamp,depIndex, depArgs );

               /* build loop args based on depArgs and adding the index to add */ 
               
               if ((depArgs != NULL) && (strlen(depArgs) > 0)) {
                  sprintf(extendedArgs,"-l %s,%s=%s", depArgs, SeqUtil_getPathLeaf(depNode),SeqNameValues_getValue(_nodeDataPtr->loop_args,depIndex));
               } else {
                  sprintf(extendedArgs,"-l %s=%s", SeqUtil_getPathLeaf(depNode),SeqNameValues_getValue(_nodeDataPtr->loop_args,depIndex));
               } 
               SeqUtil_TRACE(TL_FULL_TRACE, "maestro.submitForEach() extendedArgs:%s\n",extendedArgs); 
                  
               sprintf( submitCmd, "(export SEQ_EXP_HOME=%s;maestro -d %s -s submit -f continue -n %s %s)", 
 		                      depExp, depDatestamp, depNode, extendedArgs );
 	            /* add nodes to be submitted if not already there */
 	            if ( SeqListNode_isItemExists( cmdList, submitCmd ) == 0 ) {
 	                SeqListNode_insertItem( &cmdList, submitCmd );
	            }
	       
            } /* end while loop */
            fclose(FEFile);
 
 	    /* warn if file empty ... */
 	    if ( line_count == 0 ) raiseError( "FE file:%s (submitForEach) EMPTY !!!! \n",filename );

            /* TODO if full set is finished, delete the FE file */
             /*ret=_removeFile(filename, _nodeDataPtr->expHome); */
       } else {
          raiseError( "maestro cannot read file: %s (submitForEach) \n", filename );
       }
   }
   /*  go and submit the nodes from the cmd list */
   while ( cmdList != NULL ) {
      SeqUtil_TRACE(TL_FULL_TRACE, "maestro.submitForEach() calling submit cmd: %s\n", cmdList->data );
      submitCode = system( cmdList->data );
      SeqUtil_TRACE(TL_FULL_TRACE, "maestro.submitForEach() submitCode: %d\n", submitCode);
      if( submitCode != 0 ) {
         raiseError( "An error happened while submitting dependant nodes error number: %d\n", submitCode );
      }
      cmdList = cmdList->nextPtr;
   }
   SeqListNode_deleteWholeList( &cmdList );
   free(extName);
   free(tmpExt);
   free(tmpValue);
}




/*
isNodeXState 

Returns an integer saying whether the targetted node is in a given state. 1 if node is in the desired state, 0 if not. 

Inputs:
const char * node - target node 
const char * loopargs - what is the target node's loop index in csv name=value format
const char * datestamp - what datestamp are we verifying
const char * exp - what experiment is the node in 
const char * state - what state are we verifying 

*/
int isNodeXState (const char* node, const char* loopargs, const char* datestamp, const char* exp, const char* state) {  

  SeqNameValuesPtr loopArgs = NULL;
  char stateFile[SEQ_MAXFIELD];
  char * extension=NULL;
  int result=0; 
  memset( stateFile, '\0', sizeof (stateFile));

  SeqUtil_TRACE(TL_FULL_TRACE, "isNodeXState node=%s, loopargs=%s, datestamp=%s, exp=%s, state=%s \n", node, loopargs, datestamp, exp, state ); 

  if(strlen (loopargs) != 0) {

    if( SeqLoops_parseArgs( &loopArgs, loopargs ) == -1 ) {
       raiseError("ERROR: Invalid loop arguments: %s\n",loopargs);
    }
    SeqUtil_stringAppend( &extension, ".");
    SeqUtil_stringAppend( &extension, SeqLoops_getExtFromLoopArgs(loopArgs)); 

  } else {
    SeqUtil_stringAppend( &extension, "");
  } 
  sprintf(stateFile,"%s/sequencing/status/%s/%s%s.%s", exp, datestamp, node, extension, state);
  
  result=(_access(stateFile,R_OK, exp) == 0); 
  if (result) 
     SeqUtil_TRACE(TL_FULL_TRACE, "isNodeXState file=%s found. Returning 1.\n", stateFile); 
  else 
     SeqUtil_TRACE(TL_FULL_TRACE, "isNodeXState file=%s not found. Returning 0.\n", stateFile); 
 
  SeqNameValues_deleteWholeList( &loopArgs );
  free(extension);
  return result; 

}


/*
writeNodeWaitedFile

Writes the dependency lockfile in the directory of the node that this current node is waiting for.

Inputs:
   _dep_exp_path - the SEQ_EXP_HOME of the dependant node
   _dep_node - the path of the node including the container
   _dep_status - the status that the node is waiting for (end,abort,etc)
   _dep_index - the loop index that this node is waiting for (.+1+6)
   _dep_scope - dependency scope

*/

static int writeNodeWaitedFile( const SeqNodeDataPtr _nodeDataPtr, const char* _dep_exp_path,
                                const char* _dep_node, const char* _dep_status,
                                const char* _dep_index, const char* _dep_datestamp, SeqDependsScope _dep_scope, const char *statusfile ) {
   char filename[SEQ_MAXFIELD];
   char *loopArgs = NULL, *depBase = NULL;
   int status = 1, ret ;

   memset(filename,'\0',sizeof filename);

   SeqUtil_TRACE(TL_FULL_TRACE,"maestro.writeNodeWaitedFile() _dep_exp_path=%s, _dep_node=%s, _dep_index=%s _dep_datestamp=%s\n",
                 _dep_exp_path, _dep_node, _dep_index, _dep_datestamp);

   /* create dirs if not there */
   
   depBase = (char*) SeqUtil_getPathBase( (const char*) _dep_node );
   if( _dep_scope == IntraSuite ) {
     /* write in sequencing/status/depends/$datestamp */
      sprintf(filename,"%s/%s/%s/%s", _dep_exp_path, LOCAL_DEPENDS_DIR, _dep_datestamp, depBase );
      _SeqUtil_mkdir( filename, 1, _nodeDataPtr->expHome );
      if( _dep_index != NULL && strlen(_dep_index) > 0 && _dep_index[0] != '.' ) { 
         /* add extra dot for extension */
         sprintf(filename,"%s%s%s/%s.%s.waited_%s", _dep_exp_path, LOCAL_DEPENDS_DIR, _dep_datestamp, _dep_node, _dep_index, _dep_status);
      } else {
         sprintf(filename,"%s%s%s/%s%s.waited_%s", _dep_exp_path, LOCAL_DEPENDS_DIR, _dep_datestamp, _dep_node, _dep_index, _dep_status);
      }
   } else {
   
   /* write in sequencing/status/remote_depends/$datestamp */
      sprintf(filename, "%s%s%s/%s", _dep_exp_path, REMOTE_DEPENDS_DIR, _dep_datestamp, depBase );
      _SeqUtil_mkdir( filename, 1, _nodeDataPtr->expHome );
      if( _dep_index != NULL && strlen(_dep_index) > 0 && _dep_index[0] != '.' ) { 
         /* add extra dot for extension */
         sprintf(filename,"%s%s%s/%s.%s.waited_%s", _dep_exp_path, REMOTE_DEPENDS_DIR, _dep_datestamp, _dep_node, _dep_index, _dep_status);
      } else {
         sprintf(filename,"%s%s%s/%s%s.waited_%s", _dep_exp_path, REMOTE_DEPENDS_DIR, _dep_datestamp, _dep_node, _dep_index, _dep_status);
      }
   }
   /* create  waited_end file if not exists */
   loopArgs = (char*) SeqLoops_getLoopArgs( _nodeDataPtr->loop_args );

   status = _WriteNWFile (_nodeDataPtr->expHome, _nodeDataPtr->name, _nodeDataPtr->datestamp, loopArgs, filename, statusfile );
 
   switch (status)
   {
       case 0:
           status=1;
           break;
       case 1:
           raiseError( "maestro cannot write Node Waited file: %s (writeNodeWaitedFile)\n", filename );
           break; /* not reached */
       case 9:
           status=0;
           ret=_removeFile(filename, _nodeDataPtr->expHome);
           break;
       default: 
           raiseError( "Unknown switch case value in writeNodeWaitedFile:%d (writeNodeWaitedFile)\n", status);
   } 
   free( loopArgs );
   free( depBase );
   return(status);
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

   SeqUtil_TRACE(TL_FULL_TRACE, "formatWaitingMsg waitMsg=%s\n", waitMsg);
   returnValue = strdup( waitMsg );

   return returnValue;
}

/********************************************************************************
 * Sets the experiment scope to IntraSuite, IntraUser, InterUser based on some
 * the use of _access() and some Inode checks
********************************************************************************/
void setDepExpScope(SeqNodeDataPtr ndp, SeqDepDataPtr dep)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "setDepExpScope() begin\n");

   char seqPath[SEQ_MAXFIELD];
   sprintf(seqPath,"%s/sequencing/status",dep->exp);

   if (_access(seqPath,W_OK, ndp->expHome) == 0) {

      if(get_Inode(ndp->expHome) == get_Inode(dep->exp) ){

         dep->exp_scope = IntraSuite;  /* yes -> intraSuite
                                        * will write under ...../status/depends/
                                        */
      } else {
         dep->exp_scope = IntraUser;   /* no  -> intraUser
                                        * will write under ...../status/remote_depends/
                                        */
      }
   } else {
      dep->exp_scope = InterUser;
   }
   SeqUtil_TRACE(TL_FULL_TRACE, "setDepExpScope() end set dep->exp_scope to %d\
      (IntraSuite:%d,IntraUser:%d,InterUser:%d)\n", dep->exp_scope,
      IntraSuite,IntraUser,InterUser);
}

/********************************************************************************
 * Sets the dep->datestamp field based on the nodeDataPtr's datestamp and the
 * time_delta and hour fields of the dependency.
 *
 * If a time_delta s specified, then the hour field is ignored.  If none are
 * specified, then the nodeDataPtr's datestamp is simply copied.
********************************************************************************/
void setDepDatestamp(SeqNodeDataPtr ndp, SeqDepDataPtr dep)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "setDepDatestamp() begin\n");
   if( dep->time_delta != NULL && strlen(dep->time_delta) > 0 ){

      dep->datestamp = SeqDatesUtil_addTimeDelta( ndp->datestamp,
                                                              dep->time_delta);
   } else if( dep->hour != NULL && strlen(dep->hour) > 0 ) {
      /* calculate relative datestamp based on the current one */
      dep->datestamp = SeqDatesUtil_getPrintableDate( ndp->datestamp,
                                                      0, atoi(dep->hour),0,0 );
   } else {

      dep->datestamp = strdup( ndp->datestamp );

   }
   SeqUtil_TRACE(TL_FULL_TRACE, "setDepDatestamp() end\n");
}

/********************************************************************************
 * Check that the dependency is in scope by comparing the extension with the
 * nodeDataPtr's extension, then comparing the nodedataPtr's datestamp against
 * the dependency's valid_dow and valid_hour fields.
 * As soon as a check fails, we return 0, and if all checks pass we return 1.
 *
 * Return value is passed into the dep->isInScope field.
********************************************************************************/
void check_depIsInScope(SeqNodeDataPtr ndp, SeqDepDataPtr dep)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "check_depIsInScope() begin\n");
   int isDepInScope = 0;

   /* Calculate extension */
   dep->local_ext = SeqLoops_indexToExt(dep->local_index);

   /* if extension is specified */
   if( strlen(dep->local_ext) > 0 ){
      /* It must match the ndp's extension */
      if( strcmp(dep->local_ext, ndp->extension) != 0){
         isDepInScope = 0;
         goto out;
      }
   }

   /* if valid_hour is specified */
   if( strlen(dep->valid_hour) > 0){
      /* check valid hour */
      if(!SeqDatesUtil_isDepHourValid(ndp->datestamp, dep->valid_hour)){
         isDepInScope = 0;
         goto out;
      }
   }

   /* If valid_dow is specified */
   if( strlen(dep->valid_dow) > 0){
      if(!SeqDatesUtil_isDepDOWValid(ndp->datestamp,dep->valid_hour)){
         isDepInScope = 0;
         goto out;
      }
   }

   /* If all checks pass, the dep is in scope */
   isDepInScope = 1;

out:
   dep->isInScope = isDepInScope;
   SeqUtil_TRACE(TL_FULL_TRACE,"check_depIsInScope() end : dep->isInScope = %d\n",
                                                               dep->isInScope);
}


/********************************************************************************
 * Checks, for a single dependency, if it is satisfied.  The function calculates
 * or sets certain properties of the dependency first:
 *    - exp
 *    - exp_scope
 *    - datestamp
 *    - isInScope
 * and calls processDepStatus to check the status files to know if the dep is
 * satisfied.
********************************************************************************/
static
int validateSingleDep(SeqNodeDataPtr ndp, SeqDepDataPtr dep, const char *_flow)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "validateSingleDep() begin\n");

   int wait = 0;
   switch(dep->type){
    case NodeDependancy:
      /*
       * Futur: the inside of this case goes into the function validateNodeDep.
       * Since there is only one type, it's not needed, but if you add a type,
       * do it righ away before it gets out of control, or split off what
       * depends on the type and what doesn't.
       */

      /* I would like to put this part about dep->exp in the parseDepends module */
      if( dep->exp == NULL || strlen( dep->exp ) == 0 ) {
         dep->exp = strdup(ndp->expHome);
      }

      setDepExpScope(ndp, dep);
      setDepDatestamp(ndp, dep);
      check_depIsInScope(ndp, dep);

      SeqDep_printDep(TL_FULL_TRACE,dep);

      /* verify status files and write waiting files */
      if (dep->isInScope) {
         wait = processDepStatus( ndp, dep, _flow);
      } else {
         SeqDep_printDep(TL_FULL_TRACE,dep);
      }

    default:
      SeqUtil_TRACE(TL_FULL_TRACE,
         "validateDependencies() unprocessed nodeinfo_depend_type=%d depsPtr->type\n");
   }
   SeqUtil_TRACE(TL_FULL_TRACE, "validateSingleDep() end\n");
   return wait;
}

/********************************************************************************
 * Checks if the dependencies are satisfied for a given node.  Goes through each
 * dependency of the node until it finds one that is not satisfied.
 * Returns 1 if the node must continue waiting (found an unsatisfied dependency)
 * Returns 0 if the node can begin (all deps are satisified).
********************************************************************************/
static
int validateDependencies(SeqNodeDataPtr ndp, const char *_flow ) {
   SeqUtil_TRACE(TL_FULL_TRACE, "maestro.validateDependencies() begin\n");
   int isWaiting = 0;

   SeqDepNodePtr depNode = ndp->dependencies;
   for(;depNode != NULL; depNode = depNode->nextPtr){
      if( validateSingleDep(ndp, depNode->depData,_flow) ){
         isWaiting = 1;
         goto out;
      }
   }

out:
   SeqUtil_TRACE(TL_FULL_TRACE,
         "validateDependencies() returning isWaiting=%d\n", isWaiting);
   return isWaiting;
}

/* this function is used to process dependency status files. It does the following:
 * - verifies if the dependant node's status file(s) is available
 * - it creates waited_end file if the dependancy is not satisfied  
 * - it returns 1 if it is waiting (dependency not satisfied)
 * - it returns 0 if it is not waiting (dependency is satisfied)
 *
 * arguments:
 * _nodeDataPtr:    node data structure
 * _dep_scope:      dependency scope (IntraSuite, IntraUser)
 * _dep_name:       name of the dependant node
 * _dep_index:      loop index statement of dependant node (loop=value)
 * _dep_datestamp:  datestamp of dependant node
 * _dep_status:     status of dependant node
 * _dep_exp:       exp of dependant node if not IntraSuite scope
 *
 * added by Rochdi:
 * _dep_prot:      the way we remote depend
 * _flow    : the server will submit with -f flow
 */
#define SEQ_DEP_WAIT 1
#define SEQ_DEP_GO 0
int processDepStatus_OCM( const SeqNodeDataPtr _nodeDataPtr, SeqDepDataPtr dep, const char * _flow );
int processDepStatus_MAESTRO( const SeqNodeDataPtr _nodeDataPtr, SeqDepDataPtr dep, const char * _flow );
int checkTargetedIterations(SeqNodeDataPtr _nodeDataPtr, SeqDepDataPtr depNodeDataPtr, SeqDepDataPtr dep, const char *_flow);
int checkDepIteration(SeqNodeDataPtr _nodeDataPtr, SeqDepDataPtr dep, const char *_flow, const char *extension, int *writeStatus);
int writeWaitedFile( SeqNodeDataPtr _nodeDataPtr, SeqDepDataPtr dep, const char *extension, const char *_flow, const char *statusFile);

/********************************************************************************
 * Switch function to direct flow through the maestro function or the OCM
 * function.
********************************************************************************/
int processDepStatus( const SeqNodeDataPtr _nodeDataPtr, SeqDepDataPtr dep, const char * _flow )
{
   if (strncmp(dep->protocol,"ocm",3) != 0 ) {
      return processDepStatus_MAESTRO(_nodeDataPtr,dep,_flow);
   } else {
      return processDepStatus_OCM(_nodeDataPtr, dep, _flow);
   }
}

/********************************************************************************
 * Maestro version of processDepStatus.  Checks if target node exists, checks
 * catchup, and checks all iterations targeted by the dependency.  When we find
 * a targeted iteration that is not done, we write ourself into the node's
 * waited_XXX file.
********************************************************************************/
int processDepStatus_MAESTRO( const SeqNodeDataPtr _nodeDataPtr, SeqDepDataPtr dep,
                                                               const char * _flow )
{
   SeqUtil_TRACE(TL_FULL_TRACE, "processDepStatus_MAESTRO() begin\n");

   int retval = SEQ_DEP_GO;

   if  (! doesNodeExist(dep->node_name, dep->exp, dep->datestamp)) {
      char msg[1024];
      snprintf(msg,sizeof(msg), "Ignoring dependency on node:%s, exp:%s (out of scope)",
                                                   dep->node_name, dep->exp);
      nodelogger( _nodeDataPtr->name, "info", _nodeDataPtr->extension, msg ,
                              _nodeDataPtr->datestamp, _nodeDataPtr->expHome);
      retval = SEQ_DEP_GO;
      goto out;
   }

   SeqNodeDataPtr depNodeDataPtr = nodeinfo( dep->node_name, NI_SHOW_ALL, NULL,
                                          dep->exp, NULL, dep->datestamp,NULL );
   /* check catchup value of the node */
   if (depNodeDataPtr->catchup == CatchupDiscretionary) {
      SeqUtil_TRACE(TL_FULL_TRACE,"Node catchup is discretionnary\n");
      retval = SEQ_DEP_GO;
      goto out_free;
   }

   retval = checkTargetedIterations(_nodeDataPtr, depNodeDataPtr, dep, _flow);

   /* loop iterations until we find one that is not satisfied */
out_free:
   SeqNode_freeNode(depNodeDataPtr);
out:
   SeqUtil_TRACE(TL_FULL_TRACE, "processDepStatus_MAESTRO() end, returning %d\n",
                                                                        retval);
   return retval;
}

/********************************************************************************
 * Checks all the iterations of the dependency targeted by dep->index.
 * When we find one of the targeted iterations that is not done, we write an
 * entry in the depNode's waited_XXX file.
 * And we also write a message.
********************************************************************************/
int checkTargetedIterations(SeqNodeDataPtr _nodeDataPtr, SeqDepDataPtr depNodeDataPtr,
                                          SeqDepDataPtr dep, const char *_flow)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "checkTargetedIterations() begin\n");
   int retval = SEQ_DEP_GO;
   char *lastCheckedIndex = NULL;
   int undoneIteration = 0, writeStatus = 0;
   LISTNODEPTR extensions = SeqLoops_getLoopContainerExtensionsInReverse(
         depNodeDataPtr, dep->index );
   LISTNODEPTR itr;

   /*
    * Check iterations until an undone one is found or until they are all done.
    */
   for(itr = extensions; itr != NULL; itr = itr->nextPtr){
      lastCheckedIndex = itr->data;
      if(checkDepIteration(_nodeDataPtr, dep, _flow, itr->data, &writeStatus)
                                                               == SEQ_DEP_WAIT ){
         /* Record the reason for exiting */
         undoneIteration = 1;
         break;
      }
   }

   /*
    * Post-treatement: If*/
   if( undoneIteration ) {
      char *waitingMsg = NULL;
      retval =  SEQ_DEP_WAIT;
      waitingMsg = formatWaitingMsg(  dep->exp_scope, dep->exp, dep->node_name,
            lastCheckedIndex, dep->datestamp );
      setWaitingState( _nodeDataPtr, waitingMsg, dep->status );
      free( waitingMsg );
   } else if ( writeStatus != 0 ){
      retval =  SEQ_DEP_WAIT;
   }
out_free:
   SeqListNode_deleteWholeList(&extensions);
   SeqUtil_TRACE(TL_FULL_TRACE, "checkTargetedIterations() end\n");
   return retval;
}

/********************************************************************************
 * Verifies the status of a single iteration of a dependency and writes the
 * current SeqNode into the appropriate waited_XXX file if that particular
 * iteration is not done.
 * The input is an iteration of a node specified by dep and extension,
 * the output is itrIsUndone, which tells the caller that this iteration is not
 * complete, and writeStatus, which is used to inform the caller of the status
 * of the write to the waited_XXX file.
********************************************************************************/
int checkDepIteration(SeqNodeDataPtr _nodeDataPtr, SeqDepDataPtr dep,
                      const char *_flow, const char *extension, int *writeStatus)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "checkDepIteration() begin checking extension %s\n",
                                                                     extension);
   int itrIsUndone = 0;
   char statusFile[SEQ_MAXFIELD];
   if( dep->exp != NULL ) {
      SeqUtil_sprintStatusFile(statusFile, dep->exp, dep->node_name,
                                       dep->datestamp, extension, dep->status);
   } else {
      /* CAN'T HAPPEN: validateDependencies sets dep->exp to a non-null value,
       * (specifically, the function validateSingleDep */
      SeqUtil_sprintStatusFile(statusFile, _nodeDataPtr->workdir, dep->node_name,
                                       dep->datestamp, extension, dep->status);
   }

   _lock( statusFile , _nodeDataPtr->datestamp, _nodeDataPtr->expHome );
   if(!_isFileExists(statusFile,"maestro.processDepStatus()", _nodeDataPtr->expHome)){
      itrIsUndone = 1;
      *writeStatus = writeWaitedFile(_nodeDataPtr, dep, extension, _flow, statusFile);
   }
   _unlock( statusFile , _nodeDataPtr->datestamp, _nodeDataPtr->expHome );

out:
   SeqUtil_TRACE(TL_FULL_TRACE, "checkDepIteration() end\n");
   return itrIsUndone;
}


/********************************************************************************
 * Writes in the waited file for the dependency and the current extension.
 * NOTE: See version of processDepStatus, and notice that the extension passed
 * to the writeXYZ() functions included a dot "." when there were no wildcards
 * but did not include the dot in the code for wildcards.  It would seem that
 * including the dot or not was OK because the writeXYZ() functions do their own
 * check for the dot and add it if necessary.  This is true of the
 * writeInterUserWaitedFile
********************************************************************************/
int writeWaitedFile( SeqNodeDataPtr _nodeDataPtr, SeqDepDataPtr dep,
               const char *extension, const char *_flow, const char *statusFile)
{
   int writeStatus = 0;
   if( dep->exp_scope == InterUser ) {
      writeStatus = writeInterUserNodeWaitedFile( _nodeDataPtr, dep->node_name,
            dep->index, extension, dep->datestamp, dep->status,
            dep->exp, dep->protocol, statusFile, _flow);
   } else {
      writeStatus = writeNodeWaitedFile( _nodeDataPtr, dep->exp, dep->node_name,
            dep->status, extension, dep->datestamp, dep->exp_scope,
            statusFile);
   }
   return writeStatus;
}

/********************************************************************************
 * OCM Part of the old processDepStatus.
 *
 * Originally, we had
 * <CHUNK A>
 * if( maestro ){
 *    <CHUNK B>
 * } else {
 *    <CHUNK C>
 * }
 * <CHUNK D>
 *
 * This function is
 *
 * <CHUNK A>
 * <CHUNK C>
 * <CHUNK D>
 *
 * This was done to de-couple the maestro function from this one so that the
 * maestro one could be refactored.
********************************************************************************/
int processDepStatus_OCM( const SeqNodeDataPtr _nodeDataPtr, SeqDepDataPtr dep, const char * _flow )
{
   /* Both */
   char statusFile[SEQ_MAXFIELD];
   memset( statusFile, '\0', sizeof statusFile);
   /* ocm related */
   char dhour[3];
   char c[10];
   char ocm_datestamp[11];
   char env[128];
   char job[128];
   int loop_start, loop_total, loop_set, loop_trotte;
   /* check catchup value of the ocm node */
   memset(dhour,'\0',sizeof(dhour));
   memset(ocm_datestamp,'\0',sizeof(ocm_datestamp));

   int undoneIteration = 0, isWaiting = 0, depWildcard=0, ret=0;
   char *waitingMsg = NULL, *depExtension = NULL, *currentIndexPtr = NULL;

   dep->ext = SeqLoops_indexToExt(dep->index);
   depWildcard = ( strstr(dep->ext,"+*") != NULL);

   SeqUtil_TRACE(TL_FULL_TRACE, "processDepStatus dep->node_name=%s dep->ext=%s dep->datestamp=%s dep->status=%s dep->exp=%s dep->exp_scope=%d dep->protocol=%s \n", 
         dep->node_name,dep->ext, dep->datestamp, dep->status, dep->exp, dep->exp_scope , dep->protocol ); 

   if( dep->index == NULL || strlen( dep->index ) == 0 ) {
      SeqUtil_stringAppend( &depExtension, "" );
   } else {
      SeqUtil_stringAppend( &depExtension, "." );
      SeqUtil_stringAppend( &depExtension, strdup(dep->ext ) ); 
   }

   /* to be able to use ocm stuff put env var. */
   snprintf(env,sizeof(env),"CMC_OCMPATH=%s",dep->exp);
   putenv(env);
   strncpy(dhour,&(dep->datestamp)[8],2);
   strncpy(ocm_datestamp,&(dep->datestamp)[0],10);
   char *Ldpname = (char*) SeqUtil_getPathLeaf( dep->node_name );
   snprintf(job,sizeof(job),"%s_%s",Ldpname,dhour);

   /* check catchup */
   struct ocmjinfos *ocmjinfo_res;
   if (ocmjinfo_res = (struct ocmjinfos *) malloc(sizeof(struct ocmjinfos)) ) {
      *ocmjinfo_res = ocmjinfo(job);
      if (ocmjinfo_res->error != 0) {
         raiseError("ocmjinfo on %s in %s returned an error. Check DEPENDS_ON tag for errors. The targeted job may not exist. \n", dep->node_name, dep->exp);
      }
   } else {
      raiseError("OutOfMemory exception in maestro.processDepStatus()\n");
   }
   printOcmjinfo (ocmjinfo_res); /* for debug */

   if (ocmjinfo_res->catchup == 9 ) {
      nodelogger( _nodeDataPtr->name, "info", _nodeDataPtr->extension, "This node has a dependency on an ocm job that is not scheduled to run (catchup = 9). That dependency was ignored.\n",_nodeDataPtr->datestamp, _nodeDataPtr->expHome);
      fprintf(stderr,"The dependent job %s is not scheduled to run (catchup = 9), dependency ignored.\"\n",ocmjinfo_res->job); 
      return (0);
   }

   /* check if a loop job */
   if ( strncmp(ocmjinfo_res->loop_reference,"REG",3) != 0) { /* loop job */
      loop_start=atoi(ocmjinfo_res->loop_start);
      loop_total=atoi(ocmjinfo_res->loop_total);
      loop_set=atoi(ocmjinfo_res->loop_set);
      loop_trotte=loop_start;
   }

   if ( strncmp(dep->index,"ocmloop",7) == 0 ) {
      if ( (dep->index)[8] == '*' ) { 
         while ( loop_trotte <= loop_total ) {
            sprintf(statusFile,"%s/ocm/workingdir_tmp/%s_%s_%d.%s.%s", dep->exp, Ldpname, dhour, loop_trotte, ocm_datestamp, dep->status );
            if ( (undoneIteration= ! _isFileExists(statusFile,"maestro.processDepStatus()", _nodeDataPtr->expHome)) ) {
               snprintf(c,sizeof(c),"%c%c%d",depExtension[0],depExtension[1],loop_trotte);
               free(depExtension);
               depExtension=&c;
               isWaiting = writeInterUserNodeWaitedFile( _nodeDataPtr, dep->node_name, dep->index, depExtension, dep->datestamp, dep->status, dep->exp, dep->protocol, statusFile, _flow);
               break; 
            }
            loop_trotte++;
         }
      } else {
         sprintf(statusFile,"%s/ocm/workingdir_tmp/%s_%s_%s.%s.%s", dep->exp, Ldpname, dhour, &(dep->index)[8], ocm_datestamp, dep->status ); 
         if ( ! (undoneIteration= ! _isFileExists(statusFile,"maestro.processDepStatus()", _nodeDataPtr->expHome))) { 
            return(0); 
         } else {
            isWaiting = writeInterUserNodeWaitedFile( _nodeDataPtr, dep->node_name, dep->index, depExtension, dep->datestamp, dep->status, dep->exp, dep->protocol, statusFile,_flow);
         }
      }
   } else {
      sprintf(statusFile,"%s/ocm/workingdir_tmp/%s_%s.%s.%s", dep->exp, Ldpname, dhour, ocm_datestamp, dep->status );
      if ( ! (undoneIteration = ! _isFileExists(statusFile,"maestro.processDepStatus()", _nodeDataPtr->expHome))) { 
         return(0);
      } else {
         isWaiting = writeInterUserNodeWaitedFile( _nodeDataPtr, dep->node_name, dep->index, depExtension, dep->datestamp, dep->status, dep->exp, dep->protocol, statusFile,_flow);
      }
   }
   if( undoneIteration ) {
      isWaiting = 1;
      if ( depWildcard ) {
         waitingMsg = formatWaitingMsg(  dep->exp_scope, dep->exp, dep->node_name, currentIndexPtr, dep->datestamp ); 
      } else {
         waitingMsg = formatWaitingMsg(  dep->exp_scope, dep->exp, dep->node_name, depExtension, dep->datestamp ); 
      }
      setWaitingState( _nodeDataPtr, waitingMsg, dep->status );
   }

   free( waitingMsg );
   free( depExtension);

   return isWaiting;
}

/**
 *
 * writeInterUserNodeWaitedFile
 *
 */
int writeInterUserNodeWaitedFile ( const SeqNodeDataPtr _nodeDataPtr, const char* _dep_name, const char* _dep_index, char *depIndexPtr, const char *_dep_datestamp, 
                                   const char *_dep_status, const char* _dep_exp , const char* _dep_prot , const char* statusFile, const char * _flow) {

   struct passwd *current_passwd = NULL;
   char filename[SEQ_MAXFIELD];
   char registration_time[40];
   char depParam[2048],depFile[1024],depBuf[2048];
   char *loopArgs = NULL, *depBase = NULL,  *md5sum = NULL;
   char *maestro_version=NULL , *maestro_shortcut=NULL;
   struct timeval tv;
   struct tm* ptm;
   time_t current_time;
   int ret;

   SeqUtil_TRACE(TL_FULL_TRACE, "Processing Inter-User dependency \n" );

   current_passwd = getpwuid(getuid());  
   loopArgs = (char*) SeqLoops_getLoopArgs( _nodeDataPtr->loop_args );

   /* Empty the buffers */
   memset( depParam, '\0', sizeof(depParam));
   memset( depFile, '\0', sizeof (depFile));
   memset( depBuf, '\0', sizeof (depBuf));

  /* Build file name to be put in polling directory: This has to be unique!
   * depFile is a combination of datestamp source exp. source node and status
   * this will be a link to the actual file containing dependency parameters.
   * A node is Unique in one Exp. for a specific datestamp !
   */
    
   if (  loopArgs != NULL && strlen(loopArgs) != 0 ) {
      snprintf(depFile,sizeof(depFile),"%s_%s%s_%s.%s",_nodeDataPtr->datestamp, _nodeDataPtr->expHome, _nodeDataPtr->name, loopArgs, _dep_status);
   } else {
      snprintf(depFile,sizeof(depFile),"%s_%s%s.%s",_nodeDataPtr->datestamp, _nodeDataPtr->expHome, _nodeDataPtr->name, _dep_status);
   }

   /* compute md5sum for the string representating the depfile */
   md5sum = (char *) str2md5(depFile,strlen(depFile));
             
   /* Format the date and time, down to a single second.*/
   gettimeofday (&tv, NULL); 
   current_time = time(NULL);

   ptm = gmtime (&tv.tv_sec); 
   strftime (registration_time, sizeof (registration_time), "%Y%m%d-%H:%M:%S", ptm); 
   
   /* get maestro version & shortcut : what to do when cannot have it ???? */
   if ( (maestro_version=getenv("SEQ_MAESTRO_VERSION")) == NULL ) {
      raiseError("Could not get maestro version from SEQ_MAESTRO_VERSION env variable.\n");
   }

   if ( (maestro_shortcut=getenv("SEQ_MAESTRO_SHORTCUT")) == NULL ) {
      raiseError("Could not get maestro shortcut from SEQ_MAESTRO_SHORTCUT env variable.\n");
   }
   
   /* 
    * create directory where to put the *waiting.interUser* file 
    * we will be able to erase it when doing init[branche] actions 
    * Also when submiting with no depend.
   */

   depBase = (char*) SeqUtil_getPathBase( (const char*) _nodeDataPtr->name );

   /* write in local xperiment: sequencing/status/inter_depends/$datestamp */
   sprintf(filename,"%s/%s/%s/%s", _nodeDataPtr->expHome, INTER_DEPENDS_DIR, _nodeDataPtr->datestamp, depBase );

   /* create dir if not there  (local Xp) */
   if ( _access(filename,R_OK, _nodeDataPtr->expHome) != 0 ) _SeqUtil_mkdir( filename, 1, _nodeDataPtr->expHome );
  
   /* name and path of *waiting.interUser* file */ 
   if (  loopArgs != NULL && strlen(loopArgs) != 0 ) {
            sprintf(filename,"%s%s%s%s_%s.waiting.interUser.%s", _nodeDataPtr->expHome, INTER_DEPENDS_DIR, _nodeDataPtr->datestamp, _nodeDataPtr->name, loopArgs, _dep_status);
   } else {
            sprintf(filename,"%s%s%s%s.waiting.interUser.%s", _nodeDataPtr->expHome, INTER_DEPENDS_DIR, _nodeDataPtr->datestamp, _nodeDataPtr->name, _dep_status);
   }

   snprintf(depParam,sizeof(depParam)," <xp>%s</xp>\n <node>%s</node>\n <indx>%s</indx>\n <xdate>%s</xdate>\n <status>%s</status>\n <largs>%s</largs>\n \
<sxp>%s</sxp>\n <snode>%s</snode>\n <sxdate>%s</sxdate>\n <slargs>%s</slargs>\n <lock>%s</lock>\n <container>%s</container>\n \
<mdomain>%s</mdomain>\n <mversion>%s</mversion>\n <regtime date=\"%s\" epoch=\"%ld\" />\n <flow>%s</flow>\n<key>%s</key>\n",_dep_exp,_dep_name,depIndexPtr,_dep_datestamp,_dep_status,_dep_index,_nodeDataPtr->expHome,_nodeDataPtr->name,_nodeDataPtr->datestamp,loopArgs,statusFile,_nodeDataPtr->container,maestro_shortcut,maestro_version,registration_time,current_time,_flow,md5sum);

   /* get the protocole : polling(P), .... others to be determined  */
   if ( strncmp(_dep_prot,"pol",3) == 0 || strncmp(_dep_prot,"ocm",3) == 0 ) {        /* === Doing maestro Polling === */
             snprintf(depBuf,2048,"<dep type=\"pol\">\n%s</dep> ",depParam);
   } else  {
             raiseError("Invalid string for specifying polling dependency type:%s\n",_dep_prot);
   }

   /* TODO -> if ret is not 0 dont write to nodelogger the wait line */
   ret = ! _WriteInterUserDepFile(filename, depBuf, current_passwd->pw_dir, maestro_version,_nodeDataPtr->datestamp, md5sum);
   
   /* check if server is active, if not, send warning that dep will not launch until server is active */
   if ( MLLServerConnectionFid < 0 ) {
      nodelogger( _nodeDataPtr->name, "info", _nodeDataPtr->extension, "Dependency registered to mserver but mserver inactive. Run mserver to be able to launch dependency.",_nodeDataPtr->datestamp, _nodeDataPtr->expHome);
   }

   free(md5sum);
   md5sum=NULL;
   free(loopArgs);
   free(depBase);

   return(ret);

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

int maestro( char* _node, char* _signal, char* _flow, SeqNameValuesPtr _loops, int ignoreAllDeps, char* _extraArgs, char *_datestamp, char* _seq_exp_home ) {
   char buffer[SEQ_MAXFIELD] = {'\0'};
   char tmpdir[256];
   char *seq_soumet = NULL, *tmp = NULL, *logMech=NULL, *defFile=NULL, *windowAverage = NULL, *runStats = NULL, *shortcut=NULL ;
   char *loopExtension = NULL, *nodeExtension = NULL, *extension = NULL, *tmpFullOrigin=NULL, *tmpLoopExt=NULL, *tmpJobID=NULL, *tmpNodeOrigin=NULL, *tmpHost=NULL, *fixedPath;
   SeqNodeDataPtr nodeDataPtr = NULL;
   int status = 1; /* starting with error condition */
   int r;
   struct sigaction alrm, pipe;
   DIR *dirp = NULL;
   
   fprintf( stderr, "maestro: node=%s signal=%s flow=%s loop_args=%s extraArgs=%s expHome=%s\n", _node, _signal, _flow, SeqLoops_getLoopArgs(_loops), _extraArgs, _seq_exp_home);

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
       SeqUtil_TRACE(TL_FULL_TRACE, "Command called:\nmaestro -s %s -n %s -f %s %s -e %s -d %s\n",_signal, _node, _flow, tmp, _seq_exp_home, _datestamp);
   } else {
       SeqUtil_TRACE(TL_FULL_TRACE, "Command called:\nmaestro -s %s -n %s -f %s -e %s -d %s \n",_signal , _node, _flow, _seq_exp_home, _datestamp);
   } 

   SeqUtil_TRACE(TL_FULL_TRACE, "maestro() ignoreAllDeps=%d \n",ignoreAllDeps );

   /* Verify validity of _seq_exp_home */
   if ( _seq_exp_home != NULL ) {
      SeqUtil_normpath(buffer,_seq_exp_home);
      fixedPath = SeqUtil_fixPath(buffer);
      if ( (dirp = opendir(fixedPath)) == NULL){
         raiseError( "SEQ_EXP_HOME %s is an invalid link or directory!\n",fixedPath );
      }
      free(fixedPath);
      closedir(dirp);
   } else {
      raiseError( "maestro(): argument _seq_exp_home of maestro() function must be supplied \n" );
   }

   /* save current node name if we must close & re-open connection */
   if (CurrentNode=(char *) malloc(strlen(_node) + 1)){
       strcpy(CurrentNode,_node);
   } else {
      raiseError("OutOfMemory exception in maestro()\n");
   }

   if ( strcmp(_flow, "continue") == 0 || strcmp(_flow,"stop") == 0 ) {
   	SeqUtil_TRACE(TL_FULL_TRACE, "maestro() flow = %s , valid \n",_flow);
   } else {
      raiseError( "flow value must be \"stop\" or \"continue\"\n" );
   } 

   seq_soumet = getenv("SEQ_SOUMET");
   if ( seq_soumet != NULL ) {
      strcpy(submit_tool,seq_soumet);
   } else {
      strcpy(submit_tool,"m.ord_soumet");
   }

   /* need to tell nodelogger that we are running from maestro
   so as not try to acquire a socket */
   putenv("FROM_MAESTRO=yes");

   SeqUtil_TRACE(TL_FULL_TRACE, "maestro() using submit script=%s\n", submit_tool );
   nodeDataPtr = nodeinfo( _node, NI_SHOW_ALL, _loops, _seq_exp_home , _extraArgs, _datestamp,NULL);
   SeqUtil_TRACE(TL_FULL_TRACE, "maestro() nodeinfo done, loop_args=\n");
   SeqNameValues_printList(nodeDataPtr->loop_args);
   
   SeqLoops_validateLoopArgs( nodeDataPtr, _loops );

   SeqUtil_TRACE(TL_FULL_TRACE, "maestro() using DATESTAMP=%s\n", nodeDataPtr->datestamp );
   SeqUtil_TRACE(TL_FULL_TRACE, "maestro() node from nodeinfo=%s\n", nodeDataPtr->name );
   SeqUtil_TRACE(TL_FULL_TRACE, "maestro() node task_path from nodeinfo=%s\n", nodeDataPtr->taskPath );

   /* Deciding on locking mecanism: the decision will be based on acquiring 
    * SEQ_LOGGING_MECH string value in .maestrorc file */
   if (defFile = malloc ( strlen (getenv("HOME")) + strlen("/.maestrorc") + 2 )) {
      sprintf( defFile, "%s/.maestrorc", getenv("HOME"));
   } else {
      raiseError("OutOfMemory exception in maestro()\n");
   }
   
   if ( (logMech=SeqUtil_getdef( defFile, "SEQ_LOGGING_MECH", nodeDataPtr->expHome )) != NULL ) {
          free(defFile);defFile=NULL;
   } else {
          logMech=strdup("nfs");
   }

   /* inform only at beginning of tasks */
   if ( strcmp(_signal,"begin") == 0 ) {
          fprintf(stderr,"logging mechanism set to:%s\n",logMech);
   }


   /* Install handler for 
    *   SIGALRM to be able to time out socket routines This handler must be installed only once 
    *   SIGPIPE : in case of socket closed */
   if ( QueDeqConnection == 0 ) {
       memset (&alrm, '\0', sizeof(alrm));
       alrm.sa_handler = &alarm_handler;
       alrm.sa_flags = 0;
       sigemptyset (&alrm.sa_mask);
       r = sigaction (SIGALRM, &alrm, NULL);
       if (r < 0) perror (__func__);
      
       memset (&pipe, '\0', sizeof(pipe));
       pipe.sa_handler = &pipe_handler;
       pipe.sa_flags = 0;
       sigemptyset (&pipe.sa_mask);
       r = sigaction (SIGPIPE, &pipe, NULL);
       if (r < 0) perror (__func__);
   }

   if ( ServerConnectionStatus == 1 ) { 
       if  ( strcmp(logMech,"server") == 0 ) {
          MLLServerConnectionFid = OpenConnectionToMLLServer(_node, _signal,_seq_exp_home);
          if ( MLLServerConnectionFid > 0 ) {
             ServerConnectionStatus = 0;
             SeqUtil_TRACE(TL_CRITICAL,"##Server Connection Open And Login Accepted Signal:%s ##\n",_signal);
             useSVRlocking();
          } else {
             switch (MLLServerConnectionFid) {
                case  -1:
                   fprintf(stderr,">>>>>Warning could not open connection with server<<<<<<\n");
                   break;
                case  -2:
                   fprintf(stderr,">>>>>Authentification Failed with server<<<<<<\n");
                   break;
                default :
                   fprintf(stderr,">>>>>I dont know what has happened <<<<<<\n");
                   break;
             }
             useNFSlocking();
          }
      } else {
          useNFSlocking();
      }
   }
   
   QueDeqConnection++;
   free(logMech); 

   /* create working_dir directories */
   sprintf( tmpdir, "%s/%s/%s", nodeDataPtr->workdir, nodeDataPtr->datestamp, nodeDataPtr->container );
   _SeqUtil_mkdir( tmpdir, 1, nodeDataPtr->expHome );

   if ( (strcmp(_signal,"end") == 0 ) || (strcmp(_signal, "endx") == 0 ) || (strcmp(_signal,"endmodel") == 0 ) || 
      (strcmp(_signal,"endloops") == 0) || (strcmp(_signal,"endmodelloops") == 0) ||
      (strncmp(_signal,"endloops_",9) == 0) || (strncmp(_signal,"endmodelloops_",14) == 0)) {
      status=go_end( _signal, _flow, nodeDataPtr );
      /* run stats and averaging if topnode and user requested it */ 
      if (strcmp(nodeDataPtr->container,"") == 0 ) {
         if (defFile = malloc ( strlen ( nodeDataPtr->expHome ) + strlen("/resources/resources.def") + 1 )) {
            sprintf( defFile, "%s/resources/resources.def", nodeDataPtr->expHome );
         } else {
            raiseError("OutOfMemory exception in maestro()\n");
         }     
         if ( (runStats=SeqUtil_getdef( defFile, "SEQ_RUN_STATS_ON", nodeDataPtr->expHome )) != NULL ) {
            SeqUtil_TRACE(TL_FULL_TRACE, "maestro() running job statictics.\n");
            logreader(NULL,NULL,nodeDataPtr->expHome,nodeDataPtr->datestamp, "stats", 0,0); 
            if ( (windowAverage=SeqUtil_getdef( defFile, "SEQ_AVERAGE_WINDOW", nodeDataPtr->expHome )) != NULL ) {
               SeqUtil_TRACE(TL_FULL_TRACE, "maestro() running averaging.\n");
               logreader(NULL,NULL,nodeDataPtr->expHome,nodeDataPtr->datestamp, "avg", atoi(windowAverage),0); 
            } 
         }
         free(defFile);
      }
   }

   if (( strcmp (_signal,"initbranch" ) == 0 ) ||  ( strcmp (_signal,"initnode" ) == 0 )) {
      SeqUtil_TRACE(TL_FULL_TRACE,"SEQ: call go_initialize() %s %s %s\n",_signal,_flow,_node);
      status=go_initialize( _signal, _flow, nodeDataPtr );
   }

   if (strcmp(_signal,"abort") == 0 || strcmp( _signal, "abortx" ) == 0 ) {
      status=go_abort( _signal, _flow, nodeDataPtr );
   }

   if ( strcmp(_signal,"begin") == 0 || strcmp(_signal,"beginx") == 0 ) {
      SeqUtil_TRACE(TL_FULL_TRACE, "maestro() node from nodeinfo before go_begin=%s, loopargs=%s, extension=%s \n", nodeDataPtr->name, SeqLoops_getLoopArgs(nodeDataPtr->loop_args), nodeDataPtr->extension );
      status=go_begin( _signal, _flow, nodeDataPtr );
   }

   if ( strcmp(_signal,"submit") == 0 ) {
      SeqUtil_TRACE(TL_FULL_TRACE, "maestro() ignoreAllDepso2=%d \n",ignoreAllDeps );
      shortcut = getenv("SEQ_MAESTRO_SHORTCUT");
      if ( shortcut == NULL ) {
          raiseError("SEQ_MAESTRO_SHORTCUT environment variable not set but required for submissions. Please load the maestro package properly.\n");
      }

      /*get origin of the submission*/
      if ((tmpJobID=getenv("JOB_ID")) == NULL) {
         tmpJobID=getenv("LOADL_STEP_ID");
      }
      tmpHost=getenv("HOST");
      if (tmpJobID != NULL) {
          SeqUtil_stringAppend( &tmpFullOrigin,"Submitted by jobID=");
          SeqUtil_stringAppend( &tmpFullOrigin, tmpJobID );
          if ((tmpNodeOrigin=getenv("SEQ_NAME")) !=NULL) {
              SeqUtil_stringAppend( &tmpFullOrigin, " Node Name=" );
              SeqUtil_stringAppend( &tmpFullOrigin, tmpNodeOrigin );
              if ((tmpLoopExt=getenv("SEQ_LOOP_EXT")) != NULL) {
                  SeqUtil_stringAppend( &tmpFullOrigin, tmpLoopExt );
              } 
          } 
      } else {
          SeqUtil_stringAppend( &tmpFullOrigin,"Manual or cron submitted");
      }
      SeqUtil_stringAppend( &tmpFullOrigin, " from host " );
      SeqUtil_stringAppend( &tmpFullOrigin, tmpHost);
      SeqNode_setSubmitOrigin(nodeDataPtr,tmpFullOrigin);
      free( tmpFullOrigin );

      status=go_submit( _signal, _flow, nodeDataPtr, ignoreAllDeps );
   }

   /* Release connection with server */
   QueDeqConnection--;
   if ( ServerConnectionStatus == 0 ) {
      if ( QueDeqConnection == 0 ) {
         CloseConnectionWithMLLServer (MLLServerConnectionFid);
         SeqUtil_TRACE(TL_CRITICAL,"##Server Connection Closed Signal:%s ##\n",_signal);
      }
   }
 
   if ( QueDeqConnection == 0 ) {
      /* remove installed SIGALRM handler */
      alrm.sa_handler = SIG_DFL;
      alrm.sa_flags = 0;
      sigemptyset (&alrm.sa_mask);
      r = sigaction (SIGALRM, &alrm, NULL);
      if (r < 0) perror (__func__);
          
      /* remove installed SIGPIPE handler */
      pipe.sa_handler = SIG_DFL;
      pipe.sa_flags = 0;
      sigemptyset (&pipe.sa_mask);
      r = sigaction (SIGPIPE, &pipe, NULL);
      if (r < 0) perror (__func__);
   }

   SeqNode_freeNode( nodeDataPtr );
   free( loopExtension );
   free( nodeExtension );
   free( extension );
   free( tmp );
 
   SeqUtil_TRACE(TL_FULL_TRACE,"maestro() returning %d\n",status);
   return status;
}
