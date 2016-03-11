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
*FILE: nodelogger.h
*
*AUTHOR: Ping An Tan
******************************************************************************/

/*****************************************************************************
* nodelogger: write a formatted message to the oprun log
*        job: the name of the job
*        signl: 
*           C - 3bells "Job Has Bombed...Run Continues"
*           S - 3bells "Job Has Bombed...Run Stopped"
*           N - 3bells "Job Was Not Submitted"
*           B - 3bells "Job Has Bombed...Branch Stopped"
*           E - 3bells "Informative....Stem Ended"
*           I - 1bell  "*** MESSAGE ***"
*           A - 3bells "Job Has Bombed...Branch Continues"
*           R - 5bells "*** MESSAGE ***"
*           X - 0bells no additional message
*        message: the message we wanted to log.
*
* example: nodelogger("r1start_12",'x',"log test 1-2-3")           
* NOTE: call putenv("CMCNODELOG=on"); before using the nodelogger procedure to log all
*       messages, otherwise the  message will be written only to stdout.
* 
******************************************************************************/
extern void nodelogger(const char *job, const char *type, const char* loop_ext, const char *message, const char *datestamp, const char* _seq_exp_home);
extern void get_time (char * , int );

