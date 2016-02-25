/* Part of the Maestro sequencer software package.
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
extern void nodewait( const SeqNodeDataPtr node_ptr, const char* msg, const char *datestamp);

/****************************************************************************
* NAME: nodeend
* PURPOSE: send 'end' message to operational logging system.
* It is normally called at the end of an operational job.
* INPUT: node - full path of the node
*****************************************************************************/
extern void nodeend( const char *_signal, const SeqNodeDataPtr node_ptr, const char *datestamp);

/****************************************************************
*nodesubmit: send 'submit' message to logging system.
*INPUT: job  - the job
****************************************************************/
void nodesubmit( const SeqNodeDataPtr node_ptr, const char *datestamp);

/****************************************************************************
* NAME: nodeabort
* PURPOSE: send 'abort' message to operational logging system.
* It is normally called when an operational job has aborted.
* INPUT: num  - the numbers of arguments passed to the function.
*        job  - the job
*       [type] (optional)
*               <rerun> - "ABORTED $run: Job has been rerun"
*                       - 3bells "Job Has Bombed...Run Continues"
*               <abort> - "ABORTED $run STEM CONTINUES"
*                       - 3bells "Job Has Bombed...Run Continues"
*               <xxjob> - "XXJOB ABORT"
*                       - 1bell "****   MESSAGE  ****"
*      [errno] (optional)
*              - A message with a corresponding number is sent to the
*                oprun log.
*   
* NOTE: the variable numbers of arguments must be  string type
*****************************************************************************/
/* extern void nodeabort(int num, ...); */
void nodeabort(const char *_signal, const SeqNodeDataPtr _nodeDataPtr, const char* abort_type, const char *datestamp);

/****************************************************************************
* NAME: nodebegin
* PURPOSE: send 'begin' message to operational logging system.
* It is normally called at the beginning of an operational job.
* INPUT: job  - the job
* NOTE: the variable numbers of arguments must be string type
*****************************************************************************/
extern void nodebegin( const char *_signal, const SeqNodeDataPtr node_ptr, const char *datestamp);

extern void CopyStrTillCarac(char *result, const char *original, char c);
extern int match(const char *string, char *pattern);

