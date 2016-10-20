/* tictac_main.c - Command-line API for datestamp utility tictac, part of the Maestro sequencer software package.
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
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <unistd.h> 
#include "tictac.h"
#include "SeqUtil.h"
#include "getopt.h"



/*****************************************************************************
* tictac_main:
* API call to read or set the datestamp of a given experiment.
*
*
******************************************************************************/


int MLLServerConnectionFid;
static void alarm_handler() { /* nothing */ };

static void printUsage()
{
char *seq_exp_home = NULL;
char * usage = "DESCRIPTION: Date accessor/modifier interface for experiments\n\
\n\
USAGE:\N\
    \n\
    tictac [-s date,-f format] [-e exp] [-d datestamp] [-v]\n\
\n\
OPTIONS:\n\
\n\
    -s, --set-Date\n\
        Set the date for the specified experiment.  The date must be given in\n\
        a YYYYMMDD[HHMMSS] format.\n\
\n\
    -f, --format\n\
        Specify the format of the date to return using the following format specifiers.\n\
            %Y = year\n\
            %M = month\n\
            %D = day\n\
            %H = hour\n\
            %Min = minute\n\
            %S = second\n\
\n\
    -e, --exp \n\
        Experiment path.  If it is not supplied, the environment variable SEQ_EXP_HOME will be used.\n\
\n\
    -d, --datestamp\n\
        Specify the 14 character date of the experiment ex: 20080530000000 (anything \n\
        shorter will be padded with 0s until 14 characters) Default value is the date \n\
        of the experiment.\n\
\n\
    -h, --help\n\
        Show this help screen\n\
\n\
    -v, --verbose\n\
        Turn on full tracing\n\
\n\
EXAMPLES:\n\
    \n\
    tictac -s 2009053000\n\
        will set the experiment's datefile to that date.\n\
\n\
    tictac -f %Y%M%S\n\
        will return to stdout the value of the date in a %Y%M%S format.\n";
printf("%s",usage);
}

main (int argc, char * argv [])
{
   char * short_opts = "s:f:e:d:hv";
   extern char *optarg;
   extern int   optind;
   struct       option long_opts[] =
   { /*  NAME        ,    has_arg       , flag  val(ID) */
      {"exp"         , required_argument,   0,     'e'}, /* Used everywhere */
      {"datestamp"   , required_argument,   0,     'd'}, /* Used in logreader maestro nodeinfo nodelogger */
      {"set-date"    , required_argument,   0,     's'}, /* Used in tictac */
      {"format"      , required_argument,   0,     'f'}, /* tictac */
      {"help"        , no_argument      ,   0,     'h'}, /* Not everywhere */
      {"verbose"     , no_argument      ,   0,     'v'}, /* Everywhere */
      {NULL,0,0,0} /* End indicator */
   };
   int opt_index, c = 0;
   int i = 0;

   char *datestamp = NULL, *expHome = NULL, *format=NULL, *tmpDate = NULL;
   int setDate=0, r, padding;
   struct sigaction act;

   memset (&act, '\0', sizeof(act));

   if (argc >= 2) {
      while ((c = getopt_long(argc, argv, short_opts, long_opts, &opt_index)) != -1) {
         switch(c) {
            case 'e':
               expHome = strdup( optarg );
               break;
            case 's':
               datestamp = malloc( PADDED_DATE_LENGTH + 1 );
               strcpy(datestamp,optarg);
               setDate=1;
               break;
            case 'd':
               datestamp = malloc( PADDED_DATE_LENGTH + 1 );
               strcpy(datestamp,optarg);
               break;
            case 'f':
               format = malloc( strlen( optarg ) + 1 );
               strcpy(format,optarg);
               break;
            case 'v':
               SeqUtil_setTraceFlag( TRACE_LEVEL , TL_FULL_TRACE );
               SeqUtil_setTraceFlag( TF_TIMESTAMP , TF_ON );
               break; 
            case 'h':
               printUsage();
               exit(0); 
               break;
            case '?':
               printf ("Argument s or f must be specified!\n");
               printUsage();
               exit(1);
         }
      }
      if (format == NULL){
          format = strdup("%Y%M%D%H%Min%S"); 
      }
      
      if (expHome == NULL){
         if ( (expHome = getenv("SEQ_EXP_HOME")) == NULL ) {
            fprintf( stderr , "SEQ_EXP_HOME must be set either through the environment or with -e (--exp)\n");
            exit(1);
         }
      }
      if  (( datestamp == NULL ) && ( (tmpDate = getenv("SEQ_DATE")) != NULL ))  {
          datestamp = malloc( PADDED_DATE_LENGTH + 1 );
          strcpy(datestamp,tmpDate);
      }

      if ( datestamp != NULL ) {
         i = strlen(datestamp);
         while ( i < PADDED_DATE_LENGTH ){
            datestamp[i++] = '0';
         }
         datestamp[PADDED_DATE_LENGTH] = '\0';
      }

      if (setDate) {
        /* register SIGALRM signal */
        act.sa_handler = &alarm_handler;
        act.sa_flags = 0;
        sigemptyset (&act.sa_mask);
        r = sigaction (SIGALRM, &act, NULL);
        if (r < 0) perror (__func__);
      
        SeqUtil_TRACE(TL_FULL_TRACE, "maestro.tictac_main() setting date to=%s\n", datestamp); 
        tictac_setDate( expHome,datestamp);
        /* remove installed SIGALRM handler */
        act.sa_handler = SIG_DFL;
        act.sa_flags = 0;
        sigemptyset (&act.sa_mask);
        r = sigaction (SIGALRM, &act, NULL);
        if (r < 0) perror (__func__);
      } else {
          tictac_getDate( expHome,format,datestamp);
      }         
   } else {
      printUsage();
      exit(1);
   }
   free(datestamp);
   free(format);
   exit(0);
}
