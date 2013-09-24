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
#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h> 

#define TRUE 1
#define FALSE 0
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

static char OCSUB[256];
static char SEQ_EXP_HOME[256];
char *CurrentNode;
pid_t ChildPid;

/* static char DATESTAMP[SEQ_MAXFIELD]; */
static char USERNAME[32];
static char EXPNAME[128];

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

/* State functions: these deal with the status files */
static void setBeginState(char *_signal, const SeqNodeDataPtr _nodeDataPtr);
static void setSubmitState(const SeqNodeDataPtr _nodeDataPtr);
static void setInitState(const SeqNodeDataPtr _nodeDataPtr);
static void setEndState(const char *_signal, const SeqNodeDataPtr _nodeDataPtr);
static void setAbortState(const SeqNodeDataPtr _nodeDataPtr, char * current_action);
static void setWaitingState(const SeqNodeDataPtr _nodeDataPtr, const char* waited_one, const char* waited_status);
static void clearAllOtherStates( SeqNodeDataPtr _nodeDataPtr, char *fullNodeName, char *originator, char *current_signal); 

/* submission utilities */
static void submitDependencies ( const SeqNodeDataPtr _nodeDataPtr, const char* signal );
static void submitNodeList ( const SeqNodeDataPtr _nodeDataPtr ); 
static void submitLoopSetNodeList ( const SeqNodeDataPtr _nodeDataPtr, 
                                SeqNameValuesPtr container_args_ptr, SeqNameValuesPtr set_args_ptr); 

/* dependancy related */
static int writeNodeWaitedFile ( const SeqNodeDataPtr _nodeDataPtr, const char* _dep_exp_path, const char* _dep_node, 
                                const char* _dep_status, const char* _dep_index, const char* _dep_datestamp, SeqDependsScope _dep_scope, const char * StatusFile);
static int validateDependencies (const SeqNodeDataPtr _nodeDataPtr);

char* formatWaitingMsg( SeqDependsScope _dep_scope, const char* _dep_exp, 
                       const char *_dep_node, const char *_dep_index, const char *_dep_datestamp );

int processDepStatus( const SeqNodeDataPtr _nodeDataPtr, SeqDependsScope  _dep_scope, const char* _dep_name, const char* _dep_index,
                          const char *_dep_datestamp, const char *_dep_status, const char* _dep_exp, const char* _depProt, const char * _dep_user);


/* Rochdi: Server related */
/* Declare pointer to function to be able to replace locking mechanism */
/* see SeqUtil.h for other definitions */
static int ServerConnectionStatus = 1;
static int QueDeqConnection = 0 ;
int MLLServerConnectionFid=0; /* connection for the maestro Lock|Log  server */
extern int OpenConnectionToMLLServer ( char *,  char * );
extern void CloseConnectionWithMLLServer ( int con  );
static void CreateLockFile_svr (int sock , char *filename, char *caller );
static void (*_CreateLockFile) (int sock , char *filename, char *caller );
static int  (*_WriteNWFile) (const char* pwname, const char* seq_xp_home, const char* nname, const char* datestamp,  const char * loopArgs,
                              const char *filename, const char * StatusFile );
static int  (*_WriteInterUserDepFile) (const char *filename , const char * depBuf , const char *ppwdir, const char* maestro_version,
                                        const char *datestamp, const char *md5sum );
 
int writeInterUserNodeWaitedFile ( const SeqNodeDataPtr _nodeDataPtr, const char* _dep_name, const char* _dep_index, char *depIndexPtr, const char *_dep_datestamp, 
                                   const char *_dep_status, const char* _dep_exp , const char* _dep_prot , const char * _dep_user , const char* statusFile);
static void useNFSlocking();
static void useSVRlocking();
 

/* 
 * Wrapper for access and touch , server version
*/
static void CreateLockFile_svr(int sock , char *filename, char *caller)
{
    int ret = 0;
    ServerActions action=SVR_CREATE;

    ret = Query_L2D2_Server( sock , action, filename , "" );
    switch (ret) {
       case 0: /* success */
               SeqUtil_TRACE("%s created lockfile %s \n", caller, filename );
               break;
       case 1:
               SeqUtil_TRACE("%s ERROR in creation of lock file:%s action=%d\n",caller, filename, action);
               break;
       case 9:
               SeqUtil_TRACE("%s not recreating existing lock file:%s \n", caller, filename );
               break;
   }
}
/* 
 * Wrapper for access and touch , nfs version
*/
static void CreateLockFile_nfs(int sock , char *filename, char *caller )
{
   if ( access_nfs (filename, R_OK) != 0 ) {
          /* create the node begin lock file name */
          touch_nfs(filename);
          SeqUtil_TRACE( "%s created lockfile %s\n", caller, filename);
   } else {
          SeqUtil_TRACE( "%s not recreating existing lock file:%s\n",caller, filename );
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
  _WriteNWFile = WriteNodeWaitedFile_nfs;
  _WriteInterUserDepFile =  WriteInterUserDepFile_nfs;
  _fopen = fopen_nfs;
  _lock  = lock_nfs;
  _unlock  = unlock_nfs;
  fprintf(stderr,"@@@@@@@@@@@@@@@@@@@@@@ Using Normal locking mecanism @@@@@@@@@@@@@@@@@@@@@@\n"); 
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
 _WriteNWFile = WriteNodeWaitedFile_svr;
 _WriteInterUserDepFile = WriteInterUserDepFile_nfs ;  /* for now use nfs */ 
 _fopen = fopen_svr;
 _lock  = lock_svr;
 _unlock  = unlock_svr;
  fprintf(stderr,"@@@@@@@@@@@@@@@@@@@@@@ Using Server locking mecanism @@@@@@@@@@@@@@@@@@@@@@\n"); 
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
   fprintf(stderr,"%%%%%%%%%% TIMEOUT: KILLED BY SIGALRM %%%%%%%%%% \n");
}
/**
 *
 */
static void pipe_handler() 
{ 
   fprintf(stderr,"%%%%%%%%%% SIGPIPE RECEIVED : The mserver has probably shutdown!  %%%%%%%%%% \n");
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

   SeqUtil_TRACE( "maestro.go_abort() node=%s signal=%s flow=%s\n", _nodeDataPtr->name, _signal, _flow );
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
         SeqUtil_TRACE( "maestro.go_abort() checking for action %s on node %s \n", current_action, _nodeDataPtr->name); 
         sprintf(filename,"%s/%s/%s.abort.%s",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, extName,  current_action);
         SeqUtil_TRACE( "maestro.go_abort() checking for file %s \n", filename); 
         if ( _access(filename, R_OK) == 0 ) {
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
                SeqUtil_TRACE( "maestro.go_abort() doing action %s on node %s \n", current_action, _nodeDataPtr->name); 
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
            SeqUtil_TRACE( "maestro.go_abort() doing action %s on node %s \n", current_action, _nodeDataPtr->name); 
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
         SeqUtil_TRACE( "nodelogger: %s i \"maestro info: abort action list finished. Aborting with stop.\" \n", _nodeDataPtr->name );
         nodelogger( _nodeDataPtr->name, "info", _nodeDataPtr->extension, "maestro info: abort action list finished. Aborting with stop.",_nodeDataPtr->datestamp);
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
         SeqUtil_TRACE( "nodelogger: %s X \"BOMBED: it has been resubmitted\"\n", _nodeDataPtr->name );
         nodelogger(_nodeDataPtr->name,"info",_nodeDataPtr->extension,tmpString,_nodeDataPtr->datestamp, jobID);
         go_submit( "submit", _flow , _nodeDataPtr, 1 );
      }
   } else if (strcmp(current_action,"cont") == 0) {
         /*  issue a log message, then submit the jobs in node->submits */
         if (strcmp(_signal,"abort")==0 && strcmp(_flow, "continue") == 0 ) {

             SeqUtil_TRACE( "nodeabort: %s %s %s\n", _nodeDataPtr->name, _signal, "cont" );
             nodeabort( _signal, _nodeDataPtr, "cont", _nodeDataPtr->datestamp);
             /* submit the rest of the jobs it's supposed to submit (still missing dependency submissions)*/
             SeqUtil_TRACE( "maestro.go_abort() doing submissions\n", _nodeDataPtr->name, _signal );
             submitNodeList(_nodeDataPtr);
	 }
   } else if (strcmp(current_action,"stop") == 0) {
      /* issue a message that the job has bombed */
         SeqUtil_TRACE("nodeabort: %s %s %s\n", _nodeDataPtr->name, _signal, current_action);
         nodeabort(_signal, _nodeDataPtr,current_action, _nodeDataPtr->datestamp);
   } else {
      /*
      if abort_flag is unrecogized, aborts
      */
      SeqUtil_TRACE("invalid abort action flag %s... aborting...nodeabort: %s stop\n", current_action, _nodeDataPtr->name);
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
   newArgs=SeqNameValues_clone(_nodeDataPtr->loop_args);

   /* deal with L(i) begin -> beginx of L if none are aborted, or Npass(i) -> Npass */
   if((char*) SeqLoops_getLoopAttribute( _nodeDataPtr->loop_args, _nodeDataPtr->nodeName ) != NULL) {
       SeqNameValues_deleteItem(&newArgs, _nodeDataPtr->nodeName );
       SeqUtil_TRACE( "********** processContainerAbort() calling maestro -s abortx -n %s with loop args=%s\n", _nodeDataPtr->name, SeqLoops_getLoopArgs(newArgs)  );
       maestro ( _nodeDataPtr->name, "abortx", "stop", newArgs, 0, NULL, _nodeDataPtr->datestamp );
   } else {
       SeqUtil_TRACE( "********** processContainerAbort() calling maestro -s abortx -n %s with loop args=%s\n", _nodeDataPtr->container, SeqLoops_getLoopArgs(newArgs)  );
       maestro ( _nodeDataPtr->container, "abortx", "stop", newArgs, 0, NULL, _nodeDataPtr->datestamp  );
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

   char *extName = NULL;
   char filename[SEQ_MAXFIELD];

   extName = (char *) SeqNode_extension( _nodeDataPtr );    

   /* clear any other state */
   clearAllOtherStates( _nodeDataPtr, extName, "maestro.setAbortState()", current_action ); 

   /* create the lock file only if not exists */
   /* create the node status file */
   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s/%s.abort.%s",_nodeDataPtr->workdir,_nodeDataPtr->datestamp, extName, current_action); 

   _CreateLockFile(MLLServerConnectionFid , filename, "maestro.setAbortState() created lockfile");

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
   SeqUtil_TRACE( "maestro.go_initialize() node=%s signal=%s flow=%s\n", _nodeDataPtr->name, _signal, _flow );

   if ((strcmp (_signal,"initbranch" ) == 0) && (_nodeDataPtr->type == Task || _nodeDataPtr->type == NpassTask)) {
      raiseError( "maestro -s initbranch cannot be called on task nodes. Exiting. \n" );
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
       SeqUtil_TRACE("Following lockfiles are being deleted: \n");
       SeqUtil_TRACE( "maestro.go_initialize() deleting end lockfiles starting at node=%s\n", _nodeDataPtr->name);
       sprintf(cmd, "find %s/%s/%s/%s -name \"*%s.end\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir, _nodeDataPtr->datestamp ,_nodeDataPtr->container, _nodeDataPtr->nodeName, extName);
       returnValue=system(cmd);
       SeqUtil_TRACE( "maestro.go_initialize() deleting begin lockfiles starting at node=%s\n", _nodeDataPtr->name); 
       sprintf(cmd, "find %s/%s/%s/%s -name \"*%s.begin\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->container, _nodeDataPtr->nodeName, extName);
       returnValue=system(cmd);
       SeqUtil_TRACE( "maestro.go_initialize() deleting abort lockfiles starting at node=%s\n", _nodeDataPtr->name); 
       sprintf(cmd, "find %s/%s/%s/%s -name \"*%s.abort.*\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->container, _nodeDataPtr->nodeName, extName);
       returnValue=system(cmd);
       SeqUtil_TRACE( "maestro.go_initialize() deleting submit lockfiles starting at node=%s\n", _nodeDataPtr->name); 
       sprintf(cmd, "find %s/%s/%s/%s -name \"*%s.submit\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->container, _nodeDataPtr->nodeName, extName);
       returnValue=system(cmd);
       SeqUtil_TRACE( "maestro.go_initialize() deleting waiting lockfiles starting at node=%s\n", _nodeDataPtr->name); 
       sprintf(cmd, "find %s/%s/%s/%s -name \"*%s.waiting*\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->container, _nodeDataPtr->nodeName, extName);
       returnValue=system(cmd);

     /* for npass tasks  */
       SeqUtil_TRACE("Following lockfiles are being deleted: \n");
       SeqUtil_TRACE( "maestro.go_initialize() deleting end lockfiles starting at node=%s\n", _nodeDataPtr->name);
       sprintf(cmd, "find %s/%s/%s/%s -name \"*%s+*.end\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->container, _nodeDataPtr->nodeName, extName);
       returnValue=system(cmd);
       SeqUtil_TRACE( "maestro.go_initialize() deleting begin lockfiles starting at node=%s\n", _nodeDataPtr->name); 
       sprintf(cmd, "find %s/%s/%s/%s -name \"*%s+*.begin\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->container, _nodeDataPtr->nodeName, extName);
       returnValue=system(cmd);
       SeqUtil_TRACE( "maestro.go_initialize() deleting abort lockfiles starting at node=%s\n", _nodeDataPtr->name); 
       sprintf(cmd, "find %s/%s/%s/%s -name \"*%s+*.abort.*\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->container, _nodeDataPtr->nodeName, extName);
       returnValue=system(cmd);
       SeqUtil_TRACE( "maestro.go_initialize() deleting submit lockfiles starting at node=%s\n", _nodeDataPtr->name); 
       sprintf(cmd, "find %s/%s/%s/%s -name \"*%s+*.submit\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->container, _nodeDataPtr->nodeName, extName);
       returnValue=system(cmd);
       SeqUtil_TRACE( "maestro.go_initialize() deleting waiting lockfiles starting at node=%s\n", _nodeDataPtr->name); 
       sprintf(cmd, "find %s/%s/%s/%s -name \"*%s+*.waiting*\" -type f -print -exec rm -f {} \\;",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->container, _nodeDataPtr->nodeName, extName);
       returnValue=system(cmd);
   } else if  ( strcmp (_signal,"initnode" ) == 0 ) {
       SeqUtil_TRACE( "maestro.go_initialize() deleting waiting.InterUser lockfiles starting for node=%s\n", _nodeDataPtr->name); 
       sprintf(cmd, "find %s%s%s%s -name \"*.waiting*\" -type f -print -exec rm -f {} \\;",SEQ_EXP_HOME, INTER_DEPENDS_DIR, _nodeDataPtr->datestamp, _nodeDataPtr->container);
       returnValue=system(cmd);
   }

   /* delete every iterations if no extension specified for npasstask */
   if ( _nodeDataPtr->type == NpassTask && strlen( _nodeDataPtr->extension ) == 0 ) {
      sprintf(cmd, "find %s/%s/%s/ -name \"%s.*.*\" -type f -print",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->container, _nodeDataPtr->nodeName);
      returnValue=system(cmd);
   }
   nodelogger(_nodeDataPtr->name,"init",_nodeDataPtr->extension,"",_nodeDataPtr->datestamp);

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

   if ( _nodeDataPtr->type == Switch ) {
      if( ((char*) SeqLoops_getLoopAttribute( _nodeDataPtr->loop_args, _nodeDataPtr->nodeName )) == NULL ) {
          SeqUtil_TRACE("maestro.go_initialize() Adding to list switch argument extension: %s\n", SeqNameValues_getValue(_nodeDataPtr->switchAnswers,_nodeDataPtr->nodeName));
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
      SeqUtil_TRACE("maestro.go_initialize()() Looking for status files: %s\n", extName);

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
   SeqUtil_TRACE( "maestro.go_begin() node=%s signal=%s flow=%s loopargs=%s\n", _nodeDataPtr->name, _signal, _flow, SeqLoops_getLoopArgs(_nodeDataPtr->loop_args));

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
       if( _nodeDataPtr->type == Loop ) {
           /* L submits first set of loops, L(i) just submits its submits */
	   if((char*) SeqLoops_getLoopAttribute( _nodeDataPtr->loop_args, _nodeDataPtr->nodeName ) != NULL) { 
               submitNodeList(_nodeDataPtr);
           } else {
               /* we might have to submit a set of iterations instead of only one */
               /* get the list of iterations to submit */
               loopSetArgs = (SeqNameValuesPtr) SeqLoops_getLoopSetArgs( _nodeDataPtr, _nodeDataPtr->loop_args );
               loopArgs =  (SeqNameValuesPtr) SeqLoops_getContainerArgs( _nodeDataPtr, _nodeDataPtr->loop_args );
               SeqUtil_TRACE( "maestro.go_begin() doing loop iteration submissions\n", _nodeDataPtr->name, _signal );
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
                 SeqUtil_TRACE( "go_begin calling maestro -n %s -s submit -l %s -f continue\n", _nodeDataPtr->name, SeqLoops_getLoopArgs( loopSetArgs ));
                 maestro ( _nodeDataPtr->name, "submit", "continue" , loopSetArgs, 0, NULL, _nodeDataPtr->datestamp  );
		 SeqNameValues_deleteWholeList( &loopArgs );
                 SeqNameValues_deleteWholeList( &loopSetArgs );

           }
       }
       /* non-loop containers */
       if (_nodeDataPtr->type == Family || _nodeDataPtr->type == Module) {
            SeqUtil_TRACE( "maestro.go_begin() doing submissions\n", _nodeDataPtr->name, _signal );
            submitNodeList(_nodeDataPtr);
       } 
   }

   /* submit nodes waiting for this one to begin */
   if  (strcmp(_flow, "continue") == 0) {
       submitDependencies( _nodeDataPtr, "begin" );
   }

   if ( strcmp(_nodeDataPtr->container, "") != 0 ) {
      processContainerBegin(_nodeDataPtr, _flow); 
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
   SeqUtil_TRACE( "maestro.setBeginState() on node:%s extension: %s\n", _nodeDataPtr->name, _nodeDataPtr->extension );
 
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
          ( strcmp( _signal, "beginx" ) == 0 && !_isFileExists( filename, "setBeginState()") ) ) {

         nodebegin( _signal, _nodeDataPtr, _nodeDataPtr->datestamp );
      } 	
   } else {
      nodebegin( _signal, _nodeDataPtr, _nodeDataPtr->datestamp );
   }

   /* we will only create the lock file if it does not already exists */
   SeqUtil_TRACE( "maestro.setBeginState() checking for lockfile %s\n", filename);
   _CreateLockFile(MLLServerConnectionFid , filename , "setBeginState() ");
   free( extName );
}

/* 
processContainerBegin

 Treats the begin state of a container by checking the status of the siblings nodes around the targetted node.

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution

*/

static void processContainerBegin ( const SeqNodeDataPtr _nodeDataPtr, char *_flow ) {

   char filename[SEQ_MAXFIELD], tmp[SEQ_MAXFIELD];
   LISTNODEPTR siblingIteratorPtr = NULL;
   SeqNameValuesPtr newArgs = SeqNameValues_clone(_nodeDataPtr->loop_args);
   SeqNodeDataPtr siblingDataPtr = NULL;
   int abortedSibling = 0, catchup=CatchupNormal;;
   char* extWrite = NULL;

   if ( _nodeDataPtr->catchup == CatchupDiscretionary ) {   
      SeqUtil_TRACE( "maestro.processContainerBegin() bypassing discreet node:%s\n", _nodeDataPtr->name );
      return;
   }

    /* deal with L(i) begin -> beginx of L if none are aborted, or Npass(i) -> Npass, or Switch(i) -> Switch */
   if((char*) SeqLoops_getLoopAttribute( _nodeDataPtr->loop_args, _nodeDataPtr->nodeName ) != NULL) {
        if (( _nodeDataPtr->type == Loop && ! isLoopAborted ( _nodeDataPtr )) || (_nodeDataPtr->type == NpassTask && ! isNpassAborted (_nodeDataPtr)) || _nodeDataPtr->type == Switch ) {
            SeqNameValues_deleteItem(&newArgs, _nodeDataPtr->nodeName );
            SeqUtil_TRACE( "********** processContainerBegin() calling maestro -s beginx -n %s with loop args=%s\n", _nodeDataPtr->name, SeqLoops_getLoopArgs(newArgs)  );
            maestro ( _nodeDataPtr->name, "beginx", _flow, newArgs, 0, NULL, _nodeDataPtr->datestamp  );
        }
   } else {
           /* check non-catchup siblings for aborts */
       siblingIteratorPtr = _nodeDataPtr->siblings;
       SeqUtil_TRACE( "processContainerBegin() container=%s extension=%s\n", _nodeDataPtr->container, _nodeDataPtr->extension );
       if( strlen( _nodeDataPtr->extension ) > 0 ) {
          SeqUtil_stringAppend( &extWrite, "." );
          SeqUtil_stringAppend( &extWrite, _nodeDataPtr->extension );
       } else {
          SeqUtil_stringAppend( &extWrite, "" );
       }

       siblingIteratorPtr = _nodeDataPtr->siblings;
       if( siblingIteratorPtr != NULL && abortedSibling == 0 ) {
          /*get the exp catchup*/
          catchup = catchup_get (SEQ_EXP_HOME);
          /* check siblings's status for end or abort.continue or higher catchup */
          while(  siblingIteratorPtr != NULL && abortedSibling == 0 ) {
             memset( filename, '\0', sizeof filename );
             sprintf(filename,"%s/%s/%s/%s%s.abort.stop", _nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->container, siblingIteratorPtr->data, extWrite);
             abortedSibling = _isFileExists( filename, "processContainerBegin()");
             if ( abortedSibling ) {
             /* check if it's a discretionary or catchup higher than job's value, bypass if yes */
                 memset( tmp, '\0', sizeof tmp );
                 sprintf(tmp, "%s/%s", _nodeDataPtr->container, siblingIteratorPtr->data);
                 SeqUtil_TRACE( "maestro.processContainerBegin() getting sibling info: %s\n", tmp );
                 siblingDataPtr = nodeinfo( tmp, "type,res", NULL, NULL, NULL, _nodeDataPtr->datestamp );
                 if ( siblingDataPtr->catchup > catchup ) {
                     /*reset aborted since we're skipping this node*/
                     abortedSibling = 0;
                     SeqUtil_TRACE( "maestro.processContainerBegin() bypassing discretionary or higher catchup node: %s\n", siblingIteratorPtr->data );
                 }
             }
             siblingIteratorPtr = siblingIteratorPtr->nextPtr;
          }
       }

       if( abortedSibling == 0 ) {
          SeqUtil_TRACE( "********** processContainerBegin() calling maestro -s beginx -n %s with loop args=%s\n", _nodeDataPtr->container, SeqLoops_getLoopArgs(newArgs)  );
          maestro ( _nodeDataPtr->container, "beginx", _flow, newArgs, 0, NULL, _nodeDataPtr->datestamp  );
       }
   }
   SeqUtil_TRACE( "maestro.processContainerBegin() abortedSibling value=%d\n", abortedSibling );
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
   char filename[SEQ_MAXFIELD];
   SeqNameValuesPtr newArgs = NULL;

   SeqUtil_TRACE( "maestro.go_end() node=%s signal=%s\n", _nodeDataPtr->name, _signal );
   
   actions( _signal, _flow, _nodeDataPtr->name );
   setEndState( _signal, _nodeDataPtr );

   if ( (_nodeDataPtr->type == Task || _nodeDataPtr->type == NpassTask) && (strcmp(_flow, "continue") == 0)) {
        submitNodeList(_nodeDataPtr);
   } else if (_nodeDataPtr->type == Loop ) {
      /*is the current loop argument in loop args list and it's not the last one ? */
      if((char*) SeqLoops_getLoopAttribute( _nodeDataPtr->loop_args, _nodeDataPtr->nodeName ) != NULL && ! SeqLoops_isLastIteration( _nodeDataPtr, _nodeDataPtr->loop_args )) {
            if( (newArgs = (SeqNameValuesPtr) SeqLoops_nextLoopArgs( _nodeDataPtr, _nodeDataPtr->loop_args )) != NULL && (strcmp(_flow, "continue") == 0)) {
               maestro (_nodeDataPtr->name, "submit", _flow, newArgs, 0, NULL, _nodeDataPtr->datestamp );
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
   char *extName = NULL,*nptExt = NULL, *containerLoopExt = NULL;
   int ret=0;

   SeqNameValuesPtr newArgs = SeqNameValues_clone(_nodeDataPtr->loop_args);
   extName = (char *)SeqNode_extension( _nodeDataPtr );

   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s/%s.end",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, extName); 


   /* For a container, we don't send the log file entry again if the
      status file already exists and the signal is endx */
   if ( _nodeDataPtr->type != Task && _nodeDataPtr->type != NpassTask) {
      if (( strcmp( _signal, "end" ) == 0 ) || ( strcmp( _signal, "endx" ) == 0 && !_isFileExists( filename, "setEndState()"))) {
         nodeend( _signal, _nodeDataPtr, _nodeDataPtr->datestamp );
      }
   } else {
      nodeend( _signal, _nodeDataPtr, _nodeDataPtr->datestamp );
   }

   /* clear any other state */
   clearAllOtherStates( _nodeDataPtr, extName, "maestro.setEndState()", "end"); 

   /* Obtain a lock to protect end state */ 
   ret=_lock( filename , _nodeDataPtr->datestamp ); 

   /* create the node end lock file name if not exists*/
   _CreateLockFile( MLLServerConnectionFid , filename , "go_end() ");

   if ( _nodeDataPtr->type == NpassTask && _nodeDataPtr->isLastNPTArg ) {
      /*container arguments*/

       if((char*) SeqLoops_getLoopAttribute( _nodeDataPtr->loop_args, _nodeDataPtr->nodeName ) != NULL) {
            SeqNameValues_deleteItem(&newArgs, _nodeDataPtr->nodeName );
            containerLoopExt = (char*) SeqLoops_getExtFromLoopArgs(newArgs);
            SeqUtil_TRACE( "maestro.setEndState() containerLoopExt %s\n", containerLoopExt);
            SeqUtil_stringAppend( &nptExt, containerLoopExt );
            free(containerLoopExt);
            SeqUtil_stringAppend( &nptExt, "+last" );
            memset(filename,'\0',sizeof filename);
            sprintf(filename,"%s/%s/%s.%s.end",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->name, nptExt); 
            free( nptExt);
       }

       /* create the ^last node end lock file name if not exists*/
      _CreateLockFile( MLLServerConnectionFid , filename , "go_end() ");

   }

   /* Release lock here */
   ret=_unlock( filename , _nodeDataPtr->datestamp  );

   free( extName );
   SeqNameValues_deleteWholeList( &newArgs);
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

   memset(filename,'\0',sizeof filename);
   SeqUtil_TRACE( "maestro.clearAllOtherStates() originator=%s node=%s\n", originator, fullNodeName);

   /* remove the node begin lock file */
   sprintf(filename,"%s/%s/%s.begin",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, fullNodeName); 
   if ( _access(filename, R_OK) == 0 && strcmp( current_state, "begin" ) != 0 ) {
      SeqUtil_TRACE( "maestro.maestro.clearAllOtherStates() %s removed lockfile %s\n", originator, filename);
      ret=_removeFile(filename);
   }
   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s/%s.end",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, fullNodeName); 
   if ( _access(filename, R_OK) == 0 && strcmp( current_state, "end" ) != 0 ) {
      SeqUtil_TRACE( "maestro.clearAllOtherStates() %s removed lockfile %s\n", originator, filename);
      ret=_removeFile(filename);
   }

   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s/%s.abort.stop",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, fullNodeName); 
   if ( _access(filename, R_OK) == 0 && strcmp( current_state, "stop" ) != 0 ) {
      SeqUtil_TRACE( "maestro.clearAllOtherStates() %s removed lockfile %s\n", originator, filename);
      ret=_removeFile(filename);
   }

   memset(filename,'\0',sizeof filename);
   /* Notice that clearing submit will cause a concurrency vs NFS problem when we add dependency */
   sprintf(filename,"%s/%s/%s.submit",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, fullNodeName); 
   if ( _access(filename, R_OK) == 0 && strcmp( current_state, "submit" ) != 0 ) {
      SeqUtil_TRACE( "maestro.clearAllOtherStates() %s removed lockfile %s\n", originator, filename);
      ret=_removeFile(filename);
   }

   memset(filename,'\0',sizeof filename);
   sprintf(filename,"%s/%s/%s.waiting",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, fullNodeName); 
   if ( _access(filename, R_OK) == 0 && strcmp( current_state, "waiting" ) != 0 ) {
      SeqUtil_TRACE( "maestro.clearAllOtherStates() %s removed lockfile %s\n", originator, filename);
      ret=_removeFile(filename);
   }

   /* delete abort intermediate states only in init, abort or end */
   if ( strcmp( current_state, "init" ) == 0 || strcmp( current_state, "end" ) == 0 ||
        strcmp( current_state, "stop" ) == 0 ) {
      memset(filename,'\0',sizeof filename);
      sprintf(filename,"%s/%s/%s.abort.rerun",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, fullNodeName); 
      if ( _access(filename, R_OK) == 0 && strcmp( current_state, "rerun" ) != 0 ) {
         SeqUtil_TRACE( "maestro.clearAllOtherStates() %s removed lockfile %s\n", originator, filename);
         ret=_removeFile(filename);
      }

      memset(filename,'\0',sizeof filename);
      sprintf(filename,"%s/%s/%s.abort.cont",_nodeDataPtr->workdir,  _nodeDataPtr->datestamp, fullNodeName); 
      if ( _access(filename, R_OK) == 0 && strcmp( current_state, "cont" ) != 0 ) {
         SeqUtil_TRACE( "maestro.clearAllOtherStates() %s removed lockfile %s\n", originator, filename);
         ret=_removeFile(filename);
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
   extensions = (LISTNODEPTR) SeqLoops_childExtensions( _nodeDataPtr );
   if( extensions != NULL ) {
      while( extensions != NULL && undoneIteration == 0 ) {
         sprintf(endfile,"%s/%s/%s.%s.end", _nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->name, extensions->data);
         sprintf(continuefile,"%s/%s/%s.%s.abort.cont", _nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->name, extensions->data);
         SeqUtil_TRACE( "maestro.isLoopComplete() loop done? checking for:%s or %s\n", endfile, continuefile);
         undoneIteration = ! ( _isFileExists( endfile, "isLoopComplete()" ) || _isFileExists( continuefile, "isLoopComplete()" ) ) ;
         extensions = extensions->nextPtr;
      }
   }

   SeqListNode_deleteWholeList(&extensions);
   SeqUtil_TRACE( "maestro.isLoopComplete() return value=%d\n", (! undoneIteration) );
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
   extensions = (LISTNODEPTR) SeqLoops_childExtensions( _nodeDataPtr );
   if( extensions != NULL ) {
      while( extensions != NULL && abortedIteration == 0 ) {
         memset( abortedfile, '\0', sizeof abortedfile );
         sprintf(abortedfile,"%s/%s/%s.%s.abort.stop", _nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->name, extensions->data);
         SeqUtil_TRACE( "maestro.isLoopAborted() loop has aborted iteration? checking for:%s\n", abortedfile);
         abortedIteration =  _isFileExists( abortedfile, "isLoopAborted()" ) ;
         extensions = extensions->nextPtr;
      }
   }
   SeqListNode_deleteWholeList(&extensions);
   SeqUtil_TRACE( "maestro.isLoopAborted() return value=%d\n", abortedIteration );
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
   char filename[SEQ_MAXFIELD];
   glob_t glob_last, glob_begin, glob_submit, glob_abort;
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
   undoneIteration = !(_globPath (statePattern, GLOB_NOSORT,0));
   if (undoneIteration)  SeqUtil_TRACE("maestro.isNpassComplete - last iteration not found\n");
  
   if (! undoneIteration) {
     /* search for submit states. */
     memset( statePattern, '\0', sizeof statePattern );
     sprintf( statePattern,"%s/%s/%s.%s*.submit",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->name, extension);
     undoneIteration = _globPath(statePattern, GLOB_NOSORT,0);
     if (undoneIteration) SeqUtil_TRACE("maestro.isNpassComplete - found submit\n"); 

   }
   if (! undoneIteration) {
     /* search for begin states. */
     memset( statePattern, '\0', sizeof statePattern );
     sprintf( statePattern,"%s/%s/%s.%s*.begin",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->name, extension);
     undoneIteration = _globPath(statePattern, GLOB_NOSORT,0);
     if (undoneIteration) SeqUtil_TRACE("maestro.isNpassComplete - found begin\n");

   }
   if (! undoneIteration) {
     /* search for abort.stop states. */
     memset( statePattern, '\0', sizeof statePattern );
     sprintf( statePattern,"%s/%s/%s.%s*.abort.stop",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->name, extension);
     undoneIteration = _globPath(statePattern, GLOB_NOSORT,0);
     if (undoneIteration) SeqUtil_TRACE("maestro.isNpassComplete - found abort.stop\n");

   }

  free(extension);
  SeqNameValues_deleteWholeList( &containerLoopArgsList);

  SeqUtil_TRACE( "maestro.isNpassComplete() return value=%d\n", (! undoneIteration) );
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
   glob_t glob_abort;
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
   abortedIteration = _globPath(statePattern, GLOB_NOSORT,0);
   if (abortedIteration) SeqUtil_TRACE("maestro.isNpassAborted - found abort.stop\n");

   free(extension);
   SeqNameValues_deleteWholeList( &containerLoopArgsList);

   SeqUtil_TRACE( "maestro.isNpassAborted() return value=%d\n", abortedIteration );
   return abortedIteration;
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
   SeqUtil_TRACE( "********** processContainerEnd() calling maestro -s endx -n %s with loop args=%s\n", _nodeDataPtr->name, SeqLoops_getLoopArgs(newArgs)  );

    /* deal with L(i) ending -> end of L if all iterations are done, or Npass(i) -> Npass */
   if((char*) SeqLoops_getLoopAttribute( _nodeDataPtr->loop_args, _nodeDataPtr->nodeName ) != NULL) {
        if (( _nodeDataPtr->type == Loop && isLoopComplete ( _nodeDataPtr )) || (_nodeDataPtr->type == NpassTask && isNpassComplete (_nodeDataPtr)) || _nodeDataPtr->type == Switch ) {
            SeqNameValues_deleteItem(&newArgs, _nodeDataPtr->nodeName );
            SeqUtil_TRACE( "********** processContainerEnd() calling maestro -s endx -n %s with loop args=%s\n", _nodeDataPtr->name, SeqLoops_getLoopArgs(newArgs)  );
            maestro ( _nodeDataPtr->name, "endx", _flow, newArgs, 0, NULL, _nodeDataPtr->datestamp  );
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
       siblingIteratorPtr = _nodeDataPtr->siblings;
       if( siblingIteratorPtr != NULL && undoneChild == 0 ) {
          /*get the exp catchup*/
          catchup = catchup_get (SEQ_EXP_HOME);
          /* check siblings's status for end or abort.continue or higher catchup */
          while(  siblingIteratorPtr != NULL && undoneChild == 0 ) {
             memset( endfile, '\0', sizeof endfile );
             memset( continuefile, '\0', sizeof continuefile );
             sprintf(endfile,"%s/%s/%s/%s%s.end", _nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->container, siblingIteratorPtr->data, extWrite);
             sprintf(continuefile,"%s/%s/%s/%s%s.abort.cont", _nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->container, siblingIteratorPtr->data, extWrite);
             undoneChild = ! (_isFileExists( endfile, "processContainerEnd()") || _isFileExists( continuefile, "processContainerEnd()") );
             if ( undoneChild ) {
             /* check if it's a discretionary or catchup higher than job's value, bypass if yes */
                 memset( tmp, '\0', sizeof tmp );
                 sprintf(tmp, "%s/%s", _nodeDataPtr->container, siblingIteratorPtr->data);
                 SeqUtil_TRACE( "maestro.processContainerEnd() getting sibling info: %s\n", tmp );
                 siblingDataPtr = nodeinfo( tmp, "type,res", NULL, NULL, NULL, _nodeDataPtr->datestamp );
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
          SeqUtil_TRACE( "********** processContainerEnd() calling maestro -s endx -n %s with loop args=%s\n", _nodeDataPtr->container, SeqLoops_getLoopArgs(newArgs)  );
          maestro ( _nodeDataPtr->container, "endx", _flow, newArgs, 0, NULL, _nodeDataPtr->datestamp  );
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
   char listingDir[SEQ_MAXFIELD], defFile[SEQ_MAXFIELD];
   char cmd[SEQ_MAXFIELD];
   char tmpDate[5];
   char pidbuf[100];
   char *cpu = NULL, *tmpdir=NULL;
   char *tmpCfgFile = NULL, *tmpTarPath=NULL, *tarFile=NULL, *movedTmpName=NULL, *movedTarFile=NULL, *workerEndFile=NULL, *readyFile=NULL, *prefix=NULL, *jobName=NULL;
   char *loopArgs = NULL, *extName = NULL, *fullExtName = NULL;
   int catchup = CatchupNormal;
   int error_status = 0, ret;
   
   SeqUtil_TRACE( "maestro.go_submit() node=%s signal=%s flow=%s ignoreAllDeps=%d\n ", _nodeDataPtr->name, _signal, _flow, ignoreAllDeps );
   actions( (char*) _signal, _flow, _nodeDataPtr->name );
   memset(nodeFullPath,'\0',sizeof nodeFullPath);
   memset(listingDir,'\0',sizeof listingDir);
   memset(defFile,'\0',sizeof defFile);
   sprintf( nodeFullPath, "%s/modules/%s.tsk", SEQ_EXP_HOME, _nodeDataPtr->taskPath );
   sprintf( listingDir, "%s/sequencing/output/%s", SEQ_EXP_HOME, _nodeDataPtr->container );
   sprintf( defFile, "%s/resources/resources.def", SEQ_EXP_HOME);

   
   SeqUtil_stringAppend( &fullExtName, _nodeDataPtr->name );
   if( strlen( _nodeDataPtr->extension ) > 0 ) {
      SeqUtil_stringAppend( &fullExtName, "." );
      SeqUtil_stringAppend( &fullExtName, _nodeDataPtr->extension );
   }

   /* get exp catchup value */
   catchup = catchup_get(SEQ_EXP_HOME);
   /* check catchup value of the node */
   SeqUtil_TRACE("node catchup= %d , exp catchup = %d , discretionary catchup = %d  \n",_nodeDataPtr->catchup, catchup, CatchupDiscretionary );
   if (_nodeDataPtr->catchup > catchup && !ignoreAllDeps ) {
      if (_nodeDataPtr->catchup == CatchupDiscretionary ) {
         SeqUtil_TRACE("nodelogger: -n %s -l %s \"%s\"\n", _nodeDataPtr->name, _nodeDataPtr->extension, CATCHUP_DISCR_MSG );
         nodelogger( _nodeDataPtr->name ,"discret", _nodeDataPtr->extension, CATCHUP_DISCR_MSG,_nodeDataPtr->datestamp);
      } else {
         SeqUtil_TRACE("nodelogger: -n %s -l %s \"%s\"\n", _nodeDataPtr->name, _nodeDataPtr->extension, CATCHUP_UNSUBMIT_MSG );
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
      
      SeqUtil_stringAppend( &tmpCfgFile,  getenv("SEQ_EXP_HOME"));
      SeqUtil_stringAppend( &tmpCfgFile, "/sequencing/tmpfile/" );
      SeqUtil_stringAppend( &tmpdir, tmpCfgFile );
      SeqUtil_stringAppend( &tmpdir, _nodeDataPtr->container );
      _SeqUtil_mkdir( tmpdir, 1 );

      SeqUtil_stringAppend( &tmpCfgFile, fullExtName );
      sprintf(pidbuf, "%d", getpid() ); 
      SeqUtil_stringAppend( &tmpCfgFile, "." );
      SeqUtil_stringAppend( &tmpCfgFile, pidbuf );
      SeqUtil_stringAppend( &tmpCfgFile, ".cfg" );
       
      if ( _access(tmpCfgFile, R_OK) == 0) ret=_removeFile(tmpCfgFile); 

      SeqNode_generateConfig( _nodeDataPtr, _flow, tmpCfgFile);
      cpu = (char *) SeqUtil_cpuCalculate( _nodeDataPtr->npex, _nodeDataPtr->npey, _nodeDataPtr->omp, _nodeDataPtr->cpu_multiplier );

      /* get short name w/ extension i.e. job+3 */
      SeqUtil_stringAppend( &extName, _nodeDataPtr->nodeName );
      if( strlen( _nodeDataPtr->extension ) > 0 ) {
         SeqUtil_stringAppend( &extName, "." );
         SeqUtil_stringAppend( &extName, _nodeDataPtr->extension );
      }
      if ( (prefix = SeqUtil_getdef( defFile, "SEQ_JOBNAME_PREFIX" )) != NULL ){
          SeqUtil_stringAppend( &jobName, prefix );
          SeqUtil_stringAppend( &jobName, extName );
      } else {
          jobName=strdup(extName);
      }

      /* go and submit the job */
      if ( _nodeDataPtr->type == Task || _nodeDataPtr->type == NpassTask ) {
         
	 /* check if it's in single reservation mode and SEQ_WORKER_PATH is not 0 -> immediate mode else it's normal submit  */
	 if (strcmp(_nodeDataPtr->workerPath, "") != 0) {
	     /* create tar of the job in the worker's path */
	     if (tmpTarPath=malloc(strlen(SEQ_EXP_HOME) + strlen("/sequencing/tmpfile/") + strlen(_nodeDataPtr->datestamp) + strlen("/work_unit_depot/") + strlen(_nodeDataPtr->workerPath) + strlen("/") + strlen(_nodeDataPtr->container) +1)){
	         sprintf(tmpTarPath, "%s/sequencing/tmpfile/%s/work_unit_depot/%s/%s",SEQ_EXP_HOME, _nodeDataPtr->datestamp ,_nodeDataPtr->workerPath,_nodeDataPtr->container ); 
             } else {
                 raiseError("OutOfMemory exception in maestro.go_submit()\n");
             }
	     _SeqUtil_mkdir(tmpTarPath,1); 
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

	     ret=_removeFile(tarFile); 
	     ret=_removeFile(movedTarFile); 
	     ret=_removeFile(readyFile); 

	     /*check if the running worker has not ended. If it has, launch another one.*/
	     if (workerEndFile=(char *) malloc(strlen(_nodeDataPtr->workdir) + strlen("/") + strlen(_nodeDataPtr->datestamp) + strlen("/") + strlen(_nodeDataPtr->workerPath) + strlen(".end") + 1)){
	         sprintf(workerEndFile,"%s/%s/%s.end", _nodeDataPtr->workdir, _nodeDataPtr->datestamp, _nodeDataPtr->workerPath);
             } else {
                 raiseError("OutOfMemory exception in maestro.go_submit()\n");
             }
	     SeqUtil_TRACE("maestro.go_submit() checking for workerEndFile %s, access return value %d \n", workerEndFile, _access(workerEndFile, R_OK) ); 
	     if ( _access(workerEndFile, R_OK) == 0 ) {
	        SeqUtil_TRACE(" Running maestro -s submit on %s\n", _nodeDataPtr->workerPath); 
	        maestro ( _nodeDataPtr->workerPath, "submit", "stop" , NULL , 0, NULL, _nodeDataPtr->datestamp  );
	     }	

	     sprintf(cmd,"%s -sys %s -jobfile %s -node %s -jn %s -d %s -q %s -p %d -c %s -m %s -w %d -v -listing %s -wrapdir %s/sequencing -jobcfg %s -nosubmit -step work_unit -jobtar %s -altcfgdir %s -args \"%s\" %s",OCSUB, getenv("SEQ_WRAPPER"), nodeFullPath, _nodeDataPtr->name, jobName,_nodeDataPtr->machine,_nodeDataPtr->queue,_nodeDataPtr->mpi,cpu,_nodeDataPtr->memory,_nodeDataPtr->wallclock, listingDir, SEQ_EXP_HOME, tmpCfgFile, movedTmpName, getenv("SEQ_BIN"), _nodeDataPtr->args, _nodeDataPtr->soumetArgs);

         } else {
	     /* normal submit mode */
             sprintf(cmd,"%s -sys %s -jobfile %s -node %s -jn %s -d %s -q %s -p %d -c %s -m %s -w %d -v -listing %s -wrapdir %s/sequencing -jobcfg %s -altcfgdir %s -args \"%s\" %s",OCSUB, getenv("SEQ_WRAPPER"), nodeFullPath, _nodeDataPtr->name, jobName,_nodeDataPtr->machine,_nodeDataPtr->queue,_nodeDataPtr->mpi,cpu,_nodeDataPtr->memory,_nodeDataPtr->wallclock, listingDir, SEQ_EXP_HOME, tmpCfgFile, getenv("SEQ_BIN"),_nodeDataPtr->args, _nodeDataPtr->soumetArgs);
	 }

         fprintf(stdout,"normal submit cmd=%s\n", cmd );
         error_status = system(cmd);
         SeqUtil_TRACE("maestro.go_submit() ord return status: %d \n",error_status);
         if (strcmp(_nodeDataPtr->workerPath, "") != 0) {
             rename(movedTarFile,tarFile);
	     SeqUtil_TRACE("maestro.go_submit() moving temporary tar file %s to %s \n", movedTarFile, tarFile); 
	     _touch(readyFile);
	 }
         if (!error_status){
             nodesubmit(_nodeDataPtr, _nodeDataPtr->datestamp);
         } 
      } else {   /* container */
         memset( noendwrap, '\0', sizeof( noendwrap ) );
         memset(tmpfile,'\0',sizeof tmpfile);
         sprintf(tmpfile,"%s/sequencing/tmpfile/container.tsk",SEQ_EXP_HOME);
         if ( _touch(tmpfile) != 0 ) raiseError( "Cannot create lockfile: %s\n", tmpfile );

         _nodeDataPtr->submits == NULL ? strcpy( noendwrap, "" ) : strcpy( noendwrap, "-noendwrap" ) ;

	  sprintf(cmd,"%s -sys %s -jobfile %s -node %s -jn %s -d %s -q %s -p %d -c %s -m %s -w %d -v -listing %s -wrapdir %s/sequencing -immediate %s -jobcfg %s -altcfgdir %s -args \"%s\" %s",OCSUB, getenv("SEQ_WRAPPER"), tmpfile,_nodeDataPtr->name, jobName, getenv("TRUE_HOST"), _nodeDataPtr->queue,_nodeDataPtr->mpi,cpu,_nodeDataPtr->memory,_nodeDataPtr->wallclock, listingDir, SEQ_EXP_HOME, noendwrap, tmpCfgFile, getenv("SEQ_BIN"),  _nodeDataPtr->args,_nodeDataPtr->soumetArgs);
	 
	 fprintf(stdout,"container submit cmd=%s\n", cmd );
         error_status=system(cmd);
         SeqUtil_TRACE("maestro.go_submit() ord return status: %d \n",error_status);

         if (!error_status){
             nodesubmit(_nodeDataPtr, _nodeDataPtr->datestamp);
         } 
      }
      free(cpu);
   }
   actionsEnd( (char*) _signal, _flow, _nodeDataPtr->name );
   free( tmpCfgFile );
   free( tmpTarPath );
   free( workerEndFile );
   free( tarFile); 
   free( movedTarFile); 
   free( movedTmpName); 
   free( readyFile); 
   free( fullExtName );
   free( extName );
   free( jobName );
   free( prefix );
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

   _CreateLockFile(MLLServerConnectionFid , filename , "setSubmitState() ");

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
   SeqUtil_TRACE( "maestro.submitNodeList() called on %s \n", _nodeDataPtr->name );
   while (listIteratorPtr != NULL) {
      SeqUtil_TRACE( "maestro.submitNodeList() submits node %s\n", listIteratorPtr->data );
      maestro ( listIteratorPtr->data, "submit", "continue" ,  _nodeDataPtr->loop_args, 0, NULL, _nodeDataPtr->datestamp  );
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
      maestro ( _nodeDataPtr->name, "submit", "continue" , cmdLoopArg, 0, NULL, _nodeDataPtr->datestamp  );
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

   _CreateLockFile(MLLServerConnectionFid , filename , "setWaitingState() ");

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
   char *extName = NULL, *submitDepArgs = NULL, *tmpValue=NULL, *tmpExt=NULL;
   int submitCode = 0, count = 0, line_count=0, ret;

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

   memset(filename,'\0',sizeof filename);
   memset(depUser,'\0',sizeof depUser);
   memset(depExp,'\0',sizeof depExp);
   memset(depNode,'\0',sizeof depNode);
   memset(depDatestamp,'\0',sizeof depDatestamp);
   memset(depArgs,'\0',sizeof depArgs);
   memset(submitCmd,'\0',sizeof submitCmd);


   /* local dependencies (same exp) are fetched from sequencing/status/depends/$datestamp,
      remote dependencies are fetched from sequencing/status/remote_depends/$datestamp */
   for( count=0; count < 2; count++ ) {
      if( count == 0 ) {
         /* local dependencies */
         sprintf(filename,"%s%s%s/%s.waited_%s", SEQ_EXP_HOME, LOCAL_DEPENDS_DIR, _nodeDataPtr->datestamp, extName, _signal );
      } else {
         /* remote dependencies */
         sprintf(filename,"%s%s%s/%s.waited_%s", SEQ_EXP_HOME, REMOTE_DEPENDS_DIR, _nodeDataPtr->datestamp, extName, _signal );
      }
      SeqUtil_TRACE( "maestro.submitDependencies() looking for waited file=%s\n", filename );

      if ( _access(filename, R_OK) == 0 ) {
         SeqUtil_TRACE( "maestro.submitDependencies() found waited file=%s\n", filename );
         /* build a node list for all entries found in the waited file */
         if ((waitedFile = _fopen(filename, MLLServerConnectionFid)) != NULL ) { 
            while ( fgets( line, sizeof(line), waitedFile ) != NULL ) {
               line_count++;
               SeqUtil_TRACE( "maestro.submitDependencies() from waited file line: %s\n", line );
               sscanf( line, "user=%s exp=%s node=%s datestamp=%s args=%s", 
                  depUser, depExp, depNode, depDatestamp, depArgs );
               SeqUtil_TRACE( "maestro.submitDependencies() waited file data depUser:%s depExp:%s depNode:%s depDatestamp:%s depArgs:%s\n", 
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
            } /* end while loop */
            fclose(waitedFile);
 
 	    /* warn if file empty ... */
 	    if ( line_count == 0 ) raiseError( "waited_end file:%s (submitDependencies) EMPTY !!!! \n",filename );

            /* we don't need to keep the file */
            ret=_removeFile(filename);
         } else {
            raiseError( "maestro cannot read file: %s (submitDependencies) \n", filename );
         }
      }
   }
   /*  go and submit the nodes from the cmd list */
   while ( cmdList != NULL ) {
      SeqUtil_TRACE( "maestro.submitDependencies() calling submit cmd: %s\n", cmdList->data );
      submitCode = system( cmdList->data );
      SeqUtil_TRACE( "maestro.submitDependencies() submitCode: %d\n", submitCode);
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
writeNodeWaitedFile

Writes the dependency lockfile in the directory of the node that this current node is waiting for.

Inputs:
   _dep_user - the user id where the dependant node belongs
   _dep_exp_path - the SEQ_EXP_HOME of the dependant node
   _dep_node - the path of the node including the container
   _dep_status - the status that the node is waiting for (end,abort,etc)
   _dep_index - the loop index that this node is waiting for (.+1+6)
   _dep_scope - dependency scope

*/

static int writeNodeWaitedFile(  const SeqNodeDataPtr _nodeDataPtr, const char* _dep_exp_path,
                               const char* _dep_node, const char* _dep_status,
                               const char* _dep_index, const char* _dep_datestamp, SeqDependsScope _dep_scope, const char *statusfile ) {
   struct passwd *current_passwd = NULL;
   FILE *waitingFile = NULL;
   char filename[SEQ_MAXFIELD];
   char *loopArgs = NULL, *depBase = NULL;
   int status = 1, ret ;

   memset(filename,'\0',sizeof filename);

   SeqUtil_TRACE("maestro.writeNodeWaitedFile() _dep_exp_path=%s, _dep_node=%s, _dep_index=%s _dep_datestamp=%s\n",
                 _dep_exp_path, _dep_node, _dep_index, _dep_datestamp);

   current_passwd = getpwuid(getuid());

   /* create dirs if not there */
   
   depBase = (char*) SeqUtil_getPathBase( (const char*) _dep_node );
   if( _dep_scope == IntraSuite ) {
     /* write in sequencing/status/depends/$datestamp */
      sprintf(filename,"%s/%s/%s/%s", _dep_exp_path, LOCAL_DEPENDS_DIR, _dep_datestamp, depBase );
      _SeqUtil_mkdir( filename, 1 );
      if( _dep_index != NULL && strlen(_dep_index) > 0 && _dep_index[0] != '.' ) { 
         /* add extra dot for extension */
         sprintf(filename,"%s%s%s/%s.%s.waited_%s", _dep_exp_path, LOCAL_DEPENDS_DIR, _dep_datestamp, _dep_node, _dep_index, _dep_status);
      } else {
         sprintf(filename,"%s%s%s/%s%s.waited_%s", _dep_exp_path, LOCAL_DEPENDS_DIR, _dep_datestamp, _dep_node, _dep_index, _dep_status);
      }
   } else {
   
   /* write in sequencing/status/remote_depends/$datestamp */
      sprintf(filename, "%s%s%s/%s", _dep_exp_path, REMOTE_DEPENDS_DIR, _dep_datestamp, depBase );
      _SeqUtil_mkdir( filename, 1 );
      if( _dep_index != NULL && strlen(_dep_index) > 0 && _dep_index[0] != '.' ) { 
         /* add extra dot for extension */
         sprintf(filename,"%s%s%s/%s.%s.waited_%s", _dep_exp_path, REMOTE_DEPENDS_DIR, _dep_datestamp, _dep_node, _dep_index, _dep_status);
      } else {
         sprintf(filename,"%s%s%s/%s%s.waited_%s", _dep_exp_path, REMOTE_DEPENDS_DIR, _dep_datestamp, _dep_node, _dep_index, _dep_status);
      }
   }
   /* create  waited_end file if not exists */
   loopArgs = (char*) SeqLoops_getLoopArgs( _nodeDataPtr->loop_args );

   status = _WriteNWFile (current_passwd->pw_name,SEQ_EXP_HOME, _nodeDataPtr->name, _nodeDataPtr->datestamp, loopArgs, filename, statusfile );
 
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
           ret=_removeFile(filename);
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

   SeqUtil_TRACE( "formatWaitingMsg waitMsg=%s\n", waitMsg);
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
        *depIndex = NULL, *tmpExt = NULL, *depHour = NULL, *depProt = NULL,
        *localIndex = NULL, *localIndexString = NULL, *depIndexString = NULL;
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
 	 depProt = SeqNameValues_getValue( nameValuesPtr, "PROT" );

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
            SeqUtil_TRACE( "maestro.validateDependencies() localIndex=%s\n", localIndex );
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
         SeqUtil_TRACE( "maestro.validateDependencies() Dependency Scope: %d depDatestamp=%s\n", depScope, depDatestamp);
	 /* verify status files and write waiting files */
         if( (strcmp( localIndexString, _nodeDataPtr->extension ) == 0 ) || (strcmp( localIndexString,"" ) == 0) ) { 
	    if( depScope == IntraSuite ) {
	       SeqUtil_TRACE( "maestro.validateDependencies()  calling processDepStatus depName=%s depIndex=%s depDatestamp=%s depStatus=%s\n", depName, depIndex, depDatestamp, depStatus );
	       isWaiting = processDepStatus( _nodeDataPtr, depScope, depName, depIndex, depDatestamp, depStatus, SEQ_EXP_HOME, depProt, depUser);
            } else {
 	       SeqUtil_TRACE( "maestro.validateDependencies()  calling processDepStatus depName=%s depIndex=%s depDatestamp=%s depStatus=%s\n", depName, depIndex, depDatestamp, depStatus );
 	       isWaiting = processDepStatus( _nodeDataPtr, depScope, depName, depIndex, depDatestamp, depStatus, depExp, depProt, depUser);
	    }
         }
         free(depName); free(depStatus); free(depExp);
         free(depUser); free(localIndexString); free(depProt);
	 free(depDatestamp); free(depIndexString);
	 free(depHour); free(depIndex); free(localIndex);

         SeqNameValues_deleteWholeList(&loopArgsPtr);

         depName = NULL; depStatus = NULL; depExp=NULL; 
         depUser = NULL; depIndexString = NULL; localIndexString = NULL;
         depDatestamp = NULL; depProt = NULL; 
	 depHour = NULL; depIndex = NULL; tmpExt = NULL; localIndex = NULL;
      } else {
         SeqUtil_TRACE("maestro.validateDependencies() unprocessed nodeinfo_depend_type=%d depsPtr->type\n");
      }
      depsPtr  = depsPtr->nextPtr;
   }
   SeqUtil_TRACE( "maestro.validateDependencies() returning isWaiting=%d\n", isWaiting);

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
 * _dep_user:      the user on whom we depend
 */
int processDepStatus( const SeqNodeDataPtr _nodeDataPtr, SeqDependsScope _dep_scope, const char* _dep_name, const char* _dep_index,
                          const char *_dep_datestamp, const char *_dep_status, const char* _dep_exp, const char * _dep_prot, const char * _dep_user ) {
   char statusFile[SEQ_MAXFIELD];
   int undoneIteration = 0, isWaiting = 0, depWildcard=0, ret=0;
   char *waitingMsg = NULL, *depIndexPtr = NULL, *extString = NULL;
   /* ocm related */
   char dhour[3];
   char c[10];
   char ocm_datestamp[11];
   char env[128];
   char job[128];
   char cfile[128];
   char cfile2[128];
   FILE *catchupFile=NULL;
   int catchup, loop_start, loop_total, loop_set, loop_trotte;

   SeqNodeDataPtr depNodeDataPtr = NULL;
   LISTNODEPTR extensions = NULL;
   SeqNameValuesPtr loopArgsPtr = NULL;

   /* if I'm dependant on a loop iteration, need to process it */
   if( _dep_index != NULL && strlen( _dep_index ) > 0 ) {
       SeqUtil_TRACE( "maestro.processDepStatus() depIndex=%s length:%d\n", _dep_index, strlen(_dep_index) );
       SeqLoops_parseArgs(&loopArgsPtr, _dep_index);
       extString = (char*) SeqLoops_getExtFromLoopArgs(loopArgsPtr);
       if( strstr( extString, "+*" ) != NULL ) {
           depWildcard  = 1;
       }
   } else {
          SeqUtil_stringAppend( &extString, "" );
   }

   SeqUtil_TRACE( "processDepStatus _dep_name=%s _extString=%s _dep_datestamp=%s _dep_status=%s _dep_exp=%s _dep_scope=%d _dep_prot=%s _dep_user=%s\n", 
       _dep_name, extString, _dep_datestamp, _dep_status, _dep_exp, _dep_scope , _dep_prot , _dep_user ); 

   if( _dep_index == NULL || strlen( _dep_index ) == 0 ) {
      SeqUtil_stringAppend( &depIndexPtr, "" );
   } else {
      SeqUtil_stringAppend( &depIndexPtr, "." );
      SeqUtil_stringAppend( &depIndexPtr, strdup( extString ) ); 
   }

   memset( statusFile, '\0', sizeof statusFile);


   if (strncmp(_dep_prot,"ocm",3) != 0 ) {
    /* maestro stuff */
       if  (! doesNodeExist(_dep_name, _dep_exp, _dep_datestamp)) {
           SeqUtil_TRACE("maestro.processDepStatus() dependant node (%s) of exp (%s) does not exist, skipping dependency \n",_dep_name,_dep_exp);  
           return(0);
       }
       depNodeDataPtr = nodeinfo( _dep_name, "all", NULL, _dep_exp, NULL, NULL );
       /* check catchup value of the node */
       SeqUtil_TRACE("dependant node catchup= %d discretionary catchup = %d  \n",depNodeDataPtr->catchup, CatchupDiscretionary );
       if (depNodeDataPtr->catchup == CatchupDiscretionary) {
           SeqUtil_TRACE("dependant node catchup (%d) is discretionary (%d), skipping dependency \n",depNodeDataPtr->catchup);  
           return(0);
       }
       if( ! depWildcard ) {
           /* no wilcard, we check only one iteration */
           if( _dep_exp != NULL ) { 
               sprintf(statusFile,"%s/sequencing/status/%s/%s%s.%s", _dep_exp, _dep_datestamp,  _dep_name, depIndexPtr, _dep_status );
           } else {
               sprintf(statusFile,"%s/sequencing/status/%s/%s%s.%s", _nodeDataPtr->workdir, _dep_datestamp, _dep_name, depIndexPtr, _dep_status );
           }
           ret=_lock( statusFile ,_nodeDataPtr->datestamp ); 
           if ( (undoneIteration=! _isFileExists( statusFile, "maestro.processDepStatus()")) ) {
              if( _dep_scope == InterUser ) {
	           isWaiting = writeInterUserNodeWaitedFile( _nodeDataPtr, _dep_name, _dep_index, depIndexPtr, _dep_datestamp, _dep_status, _dep_exp, _dep_prot, _dep_user, statusFile);
              } else {
                   isWaiting = writeNodeWaitedFile( _nodeDataPtr, _dep_exp, _dep_name, _dep_status, depIndexPtr, _dep_datestamp, _dep_scope, statusFile);
 	      }
           }
           ret=_unlock( statusFile , _nodeDataPtr->datestamp ); 
       } else {
           /* wildcard, we need to check for all iterations and stop on the first iteration that is not done */
           /* get all the node extensions to be checked */
           extensions = (LISTNODEPTR) SeqLoops_getLoopContainerExtensions( depNodeDataPtr, _dep_index );
 
           /* loop iterations until we find one that is not satisfied */
           while( extensions != NULL && undoneIteration == 0 ) {
               if( _dep_exp != NULL ) { 
                   sprintf(statusFile,"%s/sequencing/status/%s/%s.%s.%s", _dep_exp, _dep_datestamp, _dep_name, extensions->data, _dep_status );
               } else {
                   sprintf(statusFile,"%s/sequencing/status/%s/%s.%s.%s", _nodeDataPtr->workdir, _dep_datestamp, _dep_name, extensions->data, _dep_status );
               }
       
               ret=_lock( statusFile , _nodeDataPtr->datestamp ); 
 	       if( ! (undoneIteration = ! _isFileExists( statusFile, "maestro.processDepStatus()"))) {
                    extensions = extensions->nextPtr; /* the iteration status file exists, go to next */
               } else {
 	            depIndexPtr = extensions->data;
                    if( _dep_scope == InterUser ) {
 	                isWaiting = writeInterUserNodeWaitedFile( _nodeDataPtr, _dep_name, _dep_index, depIndexPtr, _dep_datestamp, _dep_status, _dep_exp, _dep_prot, _dep_user, statusFile);
 		    } else {
                        isWaiting = writeNodeWaitedFile( _nodeDataPtr, _dep_exp, _dep_name, _dep_status, depIndexPtr, _dep_datestamp, _dep_scope, statusFile);
 		    }
 	       }
               ret=_unlock( statusFile , _nodeDataPtr->datestamp ); 
           }
       }

   } else {

      /* check catchup value of the ocm node */
      memset(dhour,'\0',sizeof(dhour));
      memset(ocm_datestamp,'\0',sizeof(ocm_datestamp));
      memset(cfile,'\0',sizeof(cfile));
      memset(cfile2,'\0',sizeof(cfile2));

      /* to be able to use ocm stuff put env var. */
      snprintf(env,sizeof(env),"CMC_OCMPATH=%s",_dep_exp);
      putenv(env);
      strncpy(dhour,&_dep_datestamp[8],2);
      strncpy(ocm_datestamp,&_dep_datestamp[0],10);
      char *Ldpname = (char*) SeqUtil_getPathLeaf( _dep_name );
      snprintf(job,sizeof(job),"%s_%s",Ldpname,dhour);
 
      /* check catchup */
      struct ocmjinfos *ocmjinfo_res;
      if (ocmjinfo_res = (struct ocmjinfos *) malloc(sizeof(struct ocmjinfos)) ) {
          *ocmjinfo_res = ocmjinfo(job);
      } else {
          raiseError("OutOfMemory exception in maestro.processDepStatus()\n");
      }
      printOcmjinfo (ocmjinfo_res); /* for debug */
      sprintf(cfile,"%s/ocm/catchup/catchup_%s",_dep_exp,ocmjinfo_res->run);
      sprintf(cfile2,"%s/ocm/catchup/catchup_default",_dep_exp);

      if ( access(cfile,R_OK) == 0 ) {
         if ((catchupFile = fopen(cfile,"r")) == NULL) {
            fprintf(stderr,"Error: Cannot open Catchup file:%s\n",cfile);
            return(1);
 	 }
      } else if  ( access(cfile2,R_OK) == 0 ) {
         if ((catchupFile = fopen(cfile2,"r")) == NULL) {
              fprintf(stderr,"Error: Cannot open default Catchup file:%s\n",cfile2);
              return(1);
 	 }
      } 

      ret=fscanf(catchupFile,"%d",&catchup);
      if (ocmjinfo_res->catchup > catchup || ocmjinfo_res->catchup == 9 ) {
         fprintf(stderr,"The Dependent job:%s is not scheduled to run\"\n",ocmjinfo_res->job); 
         nodelogger( _nodeDataPtr->name ,"discret", _nodeDataPtr->extension, CATCHUP_DISCR_MSG,_nodeDataPtr->datestamp);
	 return(1);
      }
 	
      /* check if a loop job */
      if ( strncmp(ocmjinfo_res->loop_reference,"REG",3) != 0) { /* loop job */
	 loop_start=atoi(ocmjinfo_res->loop_start);
	 loop_total=atoi(ocmjinfo_res->loop_total);
         loop_set=atoi(ocmjinfo_res->loop_set);
         loop_trotte=loop_start;
      }
 
      if ( strncmp(_dep_index,"ocmloop",7) == 0 ) {
         if ( _dep_index[8] == '*' ) { 
            while ( loop_trotte <= loop_total ) {
               sprintf(statusFile,"%s/ocm/workingdir_tmp/%s_%s_%d.%s.%s", _dep_exp, Ldpname, dhour, loop_trotte, ocm_datestamp, _dep_status );
               if ( (undoneIteration= ! _isFileExists(statusFile,"maestro.processDepStatus()")) ) {
                  snprintf(c,sizeof(c),"%c%c%d",depIndexPtr[0],depIndexPtr[1],loop_trotte);
 		  free(depIndexPtr);
 		  depIndexPtr=&c;
 	          isWaiting = writeInterUserNodeWaitedFile( _nodeDataPtr, _dep_name, _dep_index, depIndexPtr, _dep_datestamp, _dep_status, _dep_exp, _dep_prot, _dep_user, statusFile);
 	          break; 
               }
               loop_trotte++;
            }
         } else {
             sprintf(statusFile,"%s/ocm/workingdir_tmp/%s_%s_%s.%s.%s", _dep_exp, Ldpname, dhour, &_dep_index[8], ocm_datestamp, _dep_status ); 
             if ( ! (undoneIteration= ! _isFileExists(statusFile,"maestro.processDepStatus()"))) { 
 	        return(0); 
 	     } else {
 	        isWaiting = writeInterUserNodeWaitedFile( _nodeDataPtr, _dep_name, _dep_index, depIndexPtr, _dep_datestamp, _dep_status, _dep_exp, _dep_prot, _dep_user, statusFile);
 	     }
 	 }
      } else {
          sprintf(statusFile,"%s/ocm/workingdir_tmp/%s_%s.%s.%s", _dep_exp, Ldpname, dhour, ocm_datestamp, _dep_status );
          if ( ! (undoneIteration = ! _isFileExists(statusFile,"maestro.processDepStatus()"))) { 
             return(0);
 	  } else {
 	     isWaiting = writeInterUserNodeWaitedFile( _nodeDataPtr, _dep_name, _dep_index, depIndexPtr, _dep_datestamp, _dep_status, _dep_exp, _dep_prot, _dep_user, statusFile);
 	  }
      }
   }

   if( undoneIteration ) {
      isWaiting = 1;
      waitingMsg = formatWaitingMsg(  _dep_scope, _dep_exp, _dep_name, depIndexPtr, _dep_datestamp ); 
      setWaitingState( _nodeDataPtr, waitingMsg, _dep_status );
   }

   SeqListNode_deleteWholeList(&extensions);
   free( waitingMsg );
   free( extString );

   return isWaiting;
}


/**
 *
 * writeInterUserNodeWaitedFile
 *
 */
int writeInterUserNodeWaitedFile ( const SeqNodeDataPtr _nodeDataPtr, const char* _dep_name, const char* _dep_index, char *depIndexPtr, const char *_dep_datestamp, 
                                   const char *_dep_status, const char* _dep_exp , const char* _dep_prot , const char * _dep_user , const char* statusFile) {

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

   SeqUtil_TRACE( "Processing Inter-User dependency \n" );

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
      snprintf(depFile,sizeof(depFile),"%s_%s%s_%s.%s",_dep_datestamp, SEQ_EXP_HOME, _nodeDataPtr->name, loopArgs, _dep_status);
   } else {
      snprintf(depFile,sizeof(depFile),"%s_%s%s.%s",_dep_datestamp, SEQ_EXP_HOME, _nodeDataPtr->name, _dep_status);
   }

   /* compute md5sum for the string representating the depfile */
   md5sum = (char *) str2md5(depFile,strlen(depFile));
             
   /* Format the date and time, down to a single second.*/
   gettimeofday (&tv, NULL); 
   current_time = time(NULL);

   ptm = localtime (&tv.tv_sec); 
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
   */

   depBase = (char*) SeqUtil_getPathBase( (const char*) _nodeDataPtr->name );

   /* write in local xperiment: sequencing/status/inter_depends/$datestamp */
   sprintf(filename,"%s/%s/%s/%s", SEQ_EXP_HOME, INTER_DEPENDS_DIR, _dep_datestamp, depBase );

   /* create dir if not there  (local Xp) */
   if ( _access(filename,R_OK) != 0 ) _SeqUtil_mkdir( filename, 1 );
  
   /* name and path of *waiting.interUser* file */ 
   if (  loopArgs != NULL && strlen(loopArgs) != 0 ) {
            sprintf(filename,"%s%s%s%s_%s.waiting.interUser.%s", SEQ_EXP_HOME, INTER_DEPENDS_DIR, _dep_datestamp, _nodeDataPtr->name, loopArgs, _dep_status);
   } else {
            sprintf(filename,"%s%s%s%s.waiting.interUser.%s", SEQ_EXP_HOME, INTER_DEPENDS_DIR, _dep_datestamp, _nodeDataPtr->name, _dep_status);
   }

   snprintf(depParam,sizeof(depParam)," <xp>%s</xp>\n <node>%s</node>\n <indx>%s</indx>\n <xdate>%s</xdate>\n <status>%s</status>\n <largs>%s</largs>\n \
<susr>%s</susr>\n <sxp>%s</sxp>\n <snode>%s</snode>\n <sxdate>%s</sxdate>\n <slargs>%s</slargs>\n <lock>%s</lock>\n <container>%s</container>\n \
<mdomain>%s</mdomain>\n <mversion>%s</mversion>\n <regtime date=\"%s\" epoch=\"%ld\" />\n <key>%s</key>\n",_dep_exp,_dep_name,depIndexPtr,_dep_datestamp,_dep_status,_dep_index,current_passwd->pw_name,SEQ_EXP_HOME,_nodeDataPtr->name,_nodeDataPtr->datestamp,loopArgs,statusFile,_nodeDataPtr->container,maestro_shortcut,maestro_version,registration_time,current_time,md5sum);

   /* get the protocole : polling(P), .... others to be determined  */
   if ( strncmp(_dep_prot,"pol",3) == 0 || strncmp(_dep_prot,"ocm",3) == 0 ) {        /* === Doing maestro Polling === */
             snprintf(depBuf,2048,"<dep type=\"pol\">\n%s</dep> ",depParam);
   } else  {
             raiseError("Invalid string for specifying polling dependency type:%s\n",_dep_prot);
   }

   /* TODO -> if ret is not 0 dont write to nodelogger the wait line */
   ret = ! _WriteInterUserDepFile(filename, depBuf, current_passwd->pw_dir, maestro_version, _dep_datestamp, md5sum);
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

int maestro( char* _node, char* _signal, char* _flow, SeqNameValuesPtr _loops, int ignoreAllDeps, char* _extraArgs, char *_datestamp ) {
   char tmpdir[256], workdir[SEQ_MAXFIELD], normPathCommand[SEQ_MAXFIELD];
   char *seq_soumet = NULL, *tmp = NULL, *seq_exp_home = NULL, *logMech=NULL, *defFile=NULL;
   char *loopExtension = NULL, *nodeExtension = NULL, *extension = NULL, *tmpFullOrigin=NULL, *tmpLoopExt=NULL, *tmpJobID=NULL, *tmpNodeOrigin=NULL, *tmpHost=NULL;
   SeqNodeDataPtr nodeDataPtr = NULL;
   FILE *file;
   int status = 1, traceLevel=0; /* starting with error condition */
   int r,wstate;
   int USE_SERVER=0;
   struct sigaction alrm, pipe;
   DIR *dirp = NULL;

   if (getenv("SEQ_TRACE_LEVEL") != NULL){
       traceLevel=atoi(getenv("SEQ_TRACE_LEVEL")); 
       SeqUtil_setTraceLevel(traceLevel);
       printf("SEQ_TRACE_LEVEL set to %d\n", traceLevel); 
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
       SeqUtil_TRACE( "Command called:\nmaestro -s %s -n %s -f %s %s \n",_signal, _node, _flow, tmp);
   } else {
       SeqUtil_TRACE( "Command called:\nmaestro -s %s -n %s -f %s \n",_signal , _node, _flow);
   } 

   SeqUtil_TRACE( "maestro() ignoreAllDeps=%d \n",ignoreAllDeps );

   memset(workdir,'\0',sizeof workdir);
   memset(SEQ_EXP_HOME,'\0',sizeof SEQ_EXP_HOME);
   memset(USERNAME,'\0',sizeof USERNAME);
   memset(EXPNAME,'\0',sizeof EXPNAME);
   memset(normPathCommand,'\0',sizeof normPathCommand);
   seq_exp_home = getenv("SEQ_EXP_HOME");
   if ( seq_exp_home != NULL ) {
    /*  returnstring=realpath(getenv("SEQ_EXP_HOME"),SEQ_EXP_HOME);*/
     /* Open the command for reading. */
       if (getenv("SEQ_UTILS_BIN") == NULL){
         raiseError("SEQ_UTILS_BIN undefined, please check your maestro package is loaded properly\n");
       }
       sprintf(normPathCommand,"%s/normpath.py -p %s",getenv("SEQ_UTILS_BIN"),seq_exp_home); 
       file = popen(normPathCommand, "r");
       if (file == NULL) {
         raiseError("Failed to run command %s\n",normPathCommand);
       }
       /* Read the output*/
       if ((fgets(SEQ_EXP_HOME, sizeof(SEQ_EXP_HOME)-1, file)) == NULL) {
         raiseError("Empty return of command %s\n",normPathCommand);
       }

       /* close */
       pclose(file);

       if ((dirp = opendir(SEQ_EXP_HOME)) == NULL) { 
          raiseError( "SEQ_EXP_HOME %s is an invalid link or directory!\n",SEQ_EXP_HOME );
       }
       closedir(dirp);
   } else {
      raiseError( "Error: SEQ_EXP_HOME not set. \n");
   }

   /* save current node name if we must close & re-open connection */
   if (CurrentNode=(char *) malloc(strlen(_node) + 1)){
       strcpy(CurrentNode,_node);
   } else {
      raiseError("OutOfMemory exception in maestro()\n");
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
   strcpy( EXPNAME, (char*) SeqUtil_getPathLeaf(SEQ_EXP_HOME) );

   SeqUtil_TRACE( "maestro() SEQ_EXP_HOME=%s\n", SEQ_EXP_HOME );
   sprintf(workdir,"%s/sequencing/status", SEQ_EXP_HOME);

   /* This is needed so messages will be logged into CMCNODELOG */
   seq_soumet = getenv("SEQ_SOUMET");
   if ( seq_soumet != NULL ) {
      strcpy(OCSUB,seq_soumet);
   } else {
      strcpy(OCSUB,"ord_soumet");
   }

   /* need to tell nodelogger that we are running from maestro
   so as not try to acquire a socket */
   putenv("FROM_MAESTRO=yes");

   SeqUtil_TRACE( "maestro() using submit script=%s\n", OCSUB );
   nodeDataPtr = nodeinfo( _node, "all", _loops, NULL, _extraArgs, _datestamp );
   SeqUtil_TRACE( "maestro() nodeinfo done, loop_args=\n");
   SeqNameValues_printList(nodeDataPtr->loop_args);

   SeqLoops_validateLoopArgs( nodeDataPtr, _loops );

   SeqNode_setWorkdir( nodeDataPtr, workdir );
   SeqUtil_TRACE( "maestro() using DATESTAMP=%s\n", nodeDataPtr->datestamp );
   SeqUtil_TRACE( "maestro() node from nodeinfo=%s\n", nodeDataPtr->name );
   SeqUtil_TRACE( "maestro() node task_path from nodeinfo=%s\n", nodeDataPtr->taskPath );

   /* Deciding on locking mecanism: the decision will be based on acquiring 
    * SEQ_LOGGING_MECH string value in .maestrorc file */
   if (defFile = malloc ( strlen (getenv("HOME")) + strlen("/.maestrorc") + 2 )) {
      sprintf( defFile, "%s/.maestrorc", getenv("HOME"));
   } else {
      raiseError("OutOfMemory exception in maestro()\n");
   }
   
   if ( (logMech=SeqUtil_getdef( defFile, "SEQ_LOGGING_MECH" )) != NULL ) {
      fprintf(stdout,"found logging mechanism SEQ_LOGGING_MECH=%s\n",logMech);
      free(defFile);defFile=NULL;
   }
/*   
test to run all interactions via nfs except end state
   if ( ! ((strcmp(_signal,"end") == 0 ) || (strcmp(_signal, "endx") == 0 )) ) {
      free(logMech); logMech=NULL;
      logMech=strdup("NFS");
   }
*/ 
   /* Install handler for 
    *   SIGALRM to be able to time out socket routines This handler must be installed only once 
    *   SIGPIPE : in case of socket closed */
   if ( QueDeqConnection == 0 ) {
       alrm.sa_handler = alarm_handler;
       alrm.sa_flags = 0;
       sigemptyset (&alrm.sa_mask);
       r = sigaction (SIGALRM, &alrm, NULL);
       if (r < 0) perror (__func__);
       
       pipe.sa_handler = pipe_handler;
       pipe.sa_flags = 0;
       sigemptyset (&pipe.sa_mask);
       r = sigaction (SIGPIPE, &pipe, NULL);
       if (r < 0) perror (__func__);
   }

   if ( ServerConnectionStatus == 1 ) { 
       if  ( strcmp(logMech,"server") == 0 ) {
          MLLServerConnectionFid = OpenConnectionToMLLServer(_node, _signal);
          if ( MLLServerConnectionFid > 0 ) {
             ServerConnectionStatus = 0;
             fprintf(stdout,"#########Server Connection Open And Login Accepted Signal:%s #########\n",_signal);
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
   _SeqUtil_mkdir( tmpdir, 1 );

   if ( (strcmp(_signal,"end") == 0 ) || (strcmp(_signal, "endx") == 0 ) || (strcmp(_signal,"endmodel") == 0 ) || 
      (strcmp(_signal,"endloops") == 0) || (strcmp(_signal,"endmodelloops") == 0) ||
      (strncmp(_signal,"endloops_",9) == 0) || (strncmp(_signal,"endmodelloops_",14) == 0)) {
      status=go_end( _signal, _flow, nodeDataPtr );
   }

   if (( strcmp (_signal,"initbranch" ) == 0 ) ||  ( strcmp (_signal,"initnode" ) == 0 )) {
      SeqUtil_TRACE("SEQ: call go_initialize() %s %s %s\n",_signal,_flow,_node);
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
         fprintf(stdout,"#########Server Connection Closed Signal:%s #########\n",_signal);
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
 
   return status;
}
