/* runcontrollib.c - Utility functions for logging, for the Maestro sequencer software package.
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
#include <string.h>
#include <unistd.h>
#include "runcontrollib.h"
#include "nodelogger.h"
#include "SeqUtil.h"

/***************************************************************
*nodewait: send 'wait' message to operational logging system.
****************************************************************/
void nodewait( const SeqNodeDataPtr node_ptr, const char* msg, const char *datestamp)
{
   nodelogger(node_ptr->name,"wait",node_ptr->extension,msg,datestamp);
}

/****************************************************************/

/****************************************************************************
* NAME: nodeend
* PURPOSE: send 'end' message to operational logging system.
* It is normally called at the end of an operational job.
* INPUT: node - full path of the node
*****************************************************************************/
void nodeend( const char *_signal, const SeqNodeDataPtr node_ptr, const char *datestamp)
{
   char jobID[50];
   char message[300];

   memset(jobID, '\0', sizeof jobID);
   if (getenv("JOB_ID") != NULL){
         sprintf(jobID,"%s",getenv("JOB_ID"));
   }
   if (getenv("LOADL_STEP_ID") != NULL){
         sprintf(jobID,"%s",getenv("LOADL_STEP_ID"));
   }

   memset(message,'\0',sizeof message);
   sprintf(message,"job_ID=%s",jobID);

   nodelogger(node_ptr->name,_signal,node_ptr->extension,message,datestamp);

}


/****************************************************************/

/****************************************************************
*nodesubmit: send 'submit' message to logging system.
*INPUT: job  - the job

****************************************************************/
void nodesubmit( const SeqNodeDataPtr node_ptr, const char *datestamp)
{
   char message[400];
   char *cpu = NULL;

   memset(message,'\0',sizeof message);
   cpu = (char *) SeqUtil_cpuCalculate(node_ptr->npex,node_ptr->npey,node_ptr->omp,node_ptr->cpu_multiplier);

   /* containers use TRUE_HOST for execution ... TODO check if immediate or exec or submit modes*/
   if ( node_ptr->type == Task || node_ptr->type == NpassTask ) {
   sprintf(message,"Machine=%s Queue=%s CPU=%s (x%s CPU Multiplier as %s MPIxOMP) Memory=%s Wallclock Limit=%d mpi=%d Submit method:%s soumetArgs=\"%s\"",node_ptr->machine, node_ptr->queue, node_ptr->cpu, node_ptr->cpu_multiplier, cpu, node_ptr->memory, node_ptr->wallclock, node_ptr->mpi, node_ptr->submitOrigin,  node_ptr->soumetArgs);
   } else {
   sprintf(message,"Machine=%s Queue=%s CPU=%s (x%s CPU Multiplier as %s MPIxOMP) Memory=%s Wallclock Limit=%d mpi=%d Submit method=%s soumetArgs=\"%s\" container mode",node_ptr->machine, node_ptr->queue, node_ptr->cpu ,node_ptr->cpu_multiplier, cpu, node_ptr->memory, node_ptr->wallclock, node_ptr->mpi, node_ptr->submitOrigin, node_ptr->soumetArgs);
   }

   SeqUtil_TRACE(TL_MINIMAL,"nodesubmit.Message=%s",message);
   free(cpu);

   nodelogger(node_ptr->name,"submit",node_ptr->extension,message,datestamp);
}
/****************************************************************/


/****************************************************************/

/****************************************************************
*nodebegin: send 'begin' message to operational logging system.
*INPUT: job  - the job
****************************************************************/
void nodebegin( const char *_signal, const SeqNodeDataPtr node_ptr, const char *datestamp)
{
   char hostname[50];
   char message[300];
   char jobID[50];
   
   
   memset(hostname, '\0', sizeof hostname);
   gethostname(hostname,sizeof hostname);

   memset(jobID, '\0', sizeof jobID);
   if (getenv("JOB_ID") != NULL){
         sprintf(jobID,"%s",getenv("JOB_ID"));
   }
   if (getenv("LOADL_STEP_ID") != NULL){
         sprintf(jobID,"%s",getenv("LOADL_STEP_ID"));
   }
   
   memset(message,'\0',sizeof message);
   sprintf(message,"host=%s job_ID=%s",hostname,jobID);
   /* nodelogger(job,"begin",message); */
   nodelogger(node_ptr->name,_signal,node_ptr->extension,message,datestamp);
}
/****************************************************************/

/****************************************************************
*nodeabort: send 'abort' message to operational logging system.
*INPUT: num - the numbers of arguments passed to the function.
*       job - the job
*       [type] (optional)
*               <rerun> - "ABORTED $run: Job has been rerun"
*                       - 3bells "Job Has Bombed...Run Continues"
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
*      [errno] (optional)
*              - A message with a corresponding number is sent to the
*                oprun log.
****************************************************************/
void nodeabort(const char *_signal, const SeqNodeDataPtr _nodeDataPtr, const char* abort_type, const char *datestamp)
{
   static char aborted[] = "ABORTED";
   static char runc[]    = ": run continues";
   static char joba[]    = "JOB ABORT:";
   static char abortnb[] = "job aborted: ";
   static char jobc[]    = "next job(s) submitted";
   static char jobo[]    = "JOB OF";
   static char jobs[]    = "job stopped ";
   static char rerun[]   = "job has been resubmitted";
   static char xxjob[]   = "XXJOB ABORT";
   int  i=0, errno=0 ;
   char buf[130], *job = NULL, *loopExt = NULL;
   char* thisAbortType = NULL;
   char jobID[50];

   memset(jobID, '\0', sizeof jobID);
   if (getenv("JOB_ID") != NULL){
         sprintf(jobID,"%s",getenv("JOB_ID"));
   }
   if (getenv("LOADL_STEP_ID") != NULL){
         sprintf(jobID,"%s",getenv("LOADL_STEP_ID"));
   }
   

   thisAbortType = strdup(abort_type);
   while( abort_type[i] != '\0' ) {
      thisAbortType[i] = toupper( abort_type[i] );
      i++;
   }
   thisAbortType[i]='\0';
      SeqUtil_TRACE(TL_ERROR,"nodeabort: thisAbortType: %s for datestamp %s\n", thisAbortType, datestamp);

   job = _nodeDataPtr->name;
   loopExt = _nodeDataPtr->extension;

	memset(buf,'\0',sizeof buf);

   if ( strncmp(abort_type,"ABORTNB",7) == 0 ) {
      nodelogger(job,_signal,loopExt,abortnb,datestamp);
   } else if ( strncmp(thisAbortType,"ABORT",5) == 0 ) {
      sprintf(buf,"%s %s, job_ID=%s",aborted, runc, jobID);
      nodelogger(job,_signal,loopExt,buf,datestamp);
   } else if ( strncmp(thisAbortType,"CONT",4) == 0 ) {
      sprintf(buf,"%s %s, job_ID=%s",aborted, jobc, jobID);
      nodelogger(job,_signal,loopExt,buf,datestamp);
   } else if ( strncmp(thisAbortType,"RERUN",5) == 0 ) {
      sprintf(buf,"%s %s, job_ID=%s", aborted, rerun, jobID);
      nodelogger(job,_signal,loopExt,buf,datestamp);
   } else if ( strncmp(thisAbortType,"STOP",4) == 0 ) {
      sprintf(buf,"%s %s, job_ID=%s", aborted, jobs, jobID);
      nodelogger(job,_signal,loopExt,buf,datestamp);
   } else if ( strncmp(thisAbortType,"XXJOB",5) == 0 ) {
      nodelogger(job,"info",loopExt,xxjob,datestamp);
   }	else {
		SeqUtil_TRACE(TL_ERROR,"nodeabort: illegal type: %s\n", thisAbortType);
		exit(1);
	}

	if ( errno != 0 ) {
      sprintf(buf,"MSG NO. = %d, job_ID=%s", errno, jobID);
      nodelogger(job,_signal,loopExt,buf,datestamp);
	}
   free(thisAbortType);
}


