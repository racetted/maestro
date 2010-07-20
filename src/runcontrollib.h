/******************************************************************************
*FILE: runcontrollib.h
*
*AUTHOR: Ping An Tan
******************************************************************************/
#include "SeqNode.h"
#define RUN_CONTROLBUFSIZE    1024

/****************************************************************************
* NAME: nodewait
* PURPOSE: send 'wait' message to operational logging system.
* It is called when a job is waiting for another to finish.
* INPUT: job   - a job
*        jobw  - 'job' is waiting for 'jobw' to finish.
*****************************************************************************/
extern void nodewait( const SeqNodeDataPtr node_ptr, const char* msg, char *datestamp);

/****************************************************************************
* NAME: nodeend
* PURPOSE: send 'end' message to operational logging system.
* It is normally called at the end of an operational job.
* INPUT: node - full path of the node
*****************************************************************************/
/*extern void nodeend(char* node); */
extern void nodeend( const SeqNodeDataPtr node_ptr, char *datestamp);

/****************************************************************
*nodesubmit: send 'submit' message to logging system.
*INPUT: job  - the job
****************************************************************/
void nodesubmit( const SeqNodeDataPtr node_ptr, char *datestamp);

/****************************************************************************
* NAME: nodeabort
* PURPOSE: send 'abort' message to operational logging system.
* It is normally called when an operational job has aborted.
* INPUT: num  - the numbers of arguments passed to the function.
*        job  - the job
*       [type] (optional)
*               <rerun> - "ABORTED $run: Job has been rerun"
*                       - 3bells "Job Has Bombed...Run Continues"
*                <stem> - "ABORTED $run STEM STOPS"
*                       - 3bells "Job Has Bombed....Run Stopped"
*               <nosub> - "JOB OF $run NOT SUBMITTED"
*                       - 3bells "Job Was Not Submitted......."
*               <abort> - "ABORTED $run STEM CONTINUES"
*                       - 3bells "Job Has Bombed...Run Continues"
*                 <aji> - "AUTO JOB ABORT"
*                       - 1bell "****   MESSAGE  ****"
*                <cron> - "CRON JOB ABORT"
*                       - 1bell "****   MESSAGE  ****"
*               <xxjob> - "XXJOB ABORT"
*                       - 1bell "****   MESSAGE  ****"
*                <orji> - "ORJI JOB NOT SUBMITTED"
*               <bcont> - "JOB ABORT $run-BRANCH CONTINUED"
*                       - 3bells "Job Has Bombed...Run Continues"
*               <bstop> - "JOB ABORT $run-BRANCH STOPPED"
*                       - 3bells "Job Has Bombed...Run Stopped"
*      [errno] (optional)
*              - A message with a corresponding number is sent to the
*                oprun log.
*   
* NOTE: the variable numbers of arguments must be  string type
*****************************************************************************/
/* extern void nodeabort(int num, ...); */
void nodeabort(const SeqNodeDataPtr _nodeDataPtr, char* abort_type, char *datestamp);


/****************************************************************************
* NAME: nodebegin
* PURPOSE: send 'begin' message to operational logging system.
* It is normally called at the beginning of an operational job.
* INPUT: job  - the job
* NOTE: the variable numbers of arguments must be string type
*****************************************************************************/
/* extern void nodebegin(char *node); */
extern void nodebegin( const SeqNodeDataPtr node_ptr, char *datestamp);

extern void CopyStrTillCarac(char *result, const char *original, char c);

extern int match(const char *string, char *pattern);
