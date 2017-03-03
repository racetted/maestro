/* logreader_main.c - Command-line API for the logreader and statistics tool used in the Maestro sequencer software package.
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
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include "logreader.h"
#include <errno.h>
#include "SeqUtil.h"
#include "getopt.h"
#include "SeqDatesUtil.h"

static void printUsage()
{
   char * usage = "DESCRIPTION: Logreader\n\
\n\
USAGE:\n\
    \n\
    logreader [-i inputfile] [-t type] [-o outputfile] | -t avg [-n days]) [-e exp] [-d datestamp] [-v] [-c]\n\
\n\
OPTIONS:\n\
\n\
    -i, --input-file\n\
        Specify the logfile to read (default ${SEQ_EXP_HOME}/logs/${datestamp}_nodelog)\n\
\n\
    -o, --output-file\n\
        Specify the file where the stats are logged (if defined)\n\
\n\
    -t, --type\n\
        Specify an output filter : log (statuses & stats, used for xflow), statuses, \n\
        stats or avg (default is log)\n\
\n\
    -n, --days\n\
        Specify a number of days for averaging: Used with -t avg to define the number \n\
        of days for the averages since \"datestamp\" (default is 7). This is a 10% truncated \n\
        average to account for extremes.\n\
\n\
    -c, --check\n\
        check if output file is present before trying to write. Will not write if file is present.\n\
\n\
    \n\
    -d, --datestamp\n\
        Specify the 14 character date of the experiment ex: 20080530000000\n\
        (anything shorter will be padded with 0s until 14 characters) Default\n\
        value is the date of the experiment.\n\
\n\
    -e, --exp \n\
        Experiment path.  If it is not supplied, the environment variable \n\
        SEQ_EXP_HOME will be used.\n\
\n\
    -v, --verbose\n\
        Turn on full tracing\n\
\n\
    -h, --help\n\
        Show this help screen\n\
\n\
EXAMPLES:\n\
\n\
    logreader -d $datestamp             \n\
        will read that experiment's log, output the statuses and stats to stdout \n\
\n\
    logreader -d $datestamp -t stats    \n\
        will read that experiment's log, create a statistics file \n\
        (default ${SEQ_EXP_HOME}/stats/${datestamp}\n\
\n\
    logreader -d $datestamp -t avg      \n\
        will calculate the x-day truncated average and create a averages file \n\
        (default ${SEQ_EXP_HOME}/stats/${datestamp}_avg\n";
   puts(usage);
}

int main ( int argc, char * argv[] )
{
   char *type=NULL, *inputFile=NULL, *outputFile=NULL, *exp=NULL, *datestamp=NULL, *tmpDate=NULL, *tmpExp=NULL; 
   int stats_days=7, clobberFile=1, i, read_type=0; 
   char * short_opts = "i:t:n:o:d:e:vch";

   extern char *optarg;
   extern int   optind;
   struct       option long_opts[] =
   { /*  NAME        ,    has_arg       , flag  val(ID) */
      {"exp"         , required_argument,   0,     'e'},
      {"type"        , required_argument,   0,     't'},
      {"datestamp"   , required_argument,   0,     'd'},
      {"input-file"   , required_argument,   0,     'i'},
      {"output-file"  , required_argument,   0,     'o'},
      {"days"        , required_argument,   0,     'n'},
      {"check"       , no_argument      ,   0,     'c'},
      {"verbose"     , no_argument      ,   0,     'v'},
      {"help"        , no_argument      ,   0,     'h'},
      {NULL,0,0,0} /* End indicator */
   };
   int opt_index, c = 0;

   if ( argc == 1 || argc == 2) {
      printUsage();
      exit(1);
   }
   
   while ((c = getopt_long(argc, argv, short_opts, long_opts, &opt_index)) != -1) {
      switch(c) {
	   case 'i':
         inputFile=strdup( optarg );
         break;
	   case 't':
	      type = strdup(optarg);
	      break;
	   case 'n':
	      stats_days = atoi(optarg);
	      break;
	   case 'o':
	      outputFile = strdup(optarg);
	      break;
	   case 'd':
          datestamp = malloc( PADDED_DATE_LENGTH + 1 );
          strncpy(datestamp,optarg,PADDED_DATE_LENGTH);
	      break;
	   case 'e':
	      exp = strdup(optarg);
	      break;
	   case 'v':
			SeqUtil_setTraceFlag( TRACE_LEVEL , TL_FULL_TRACE );
			SeqUtil_setTraceFlag( TF_TIMESTAMP , TF_ON );
	      break;
      case 'h':
         printUsage();
         exit(0);
      case 'c':
         clobberFile=0;
	      break;
      case '?':
         printUsage();
         exit(1);
         break;
    }

   }

   if  (( datestamp == NULL ) && ( (tmpDate = getenv("SEQ_DATE")) != NULL ))  {
      datestamp = malloc( PADDED_DATE_LENGTH + 1 );
      strncpy(datestamp,tmpDate,PADDED_DATE_LENGTH);
   }

   if ( datestamp != NULL ) {
      i = strlen(datestamp);
      while ( i < PADDED_DATE_LENGTH ){
         datestamp[i++] = '0';
      }
      datestamp[PADDED_DATE_LENGTH] = '\0';
   } else {
      raiseError("-d or SEQ_DATE must be defined.\n");
   }

   if (exp == NULL) {
      if ((tmpExp = getenv("SEQ_EXP_HOME")) == NULL) {
         raiseError("nodelogger_main(): -e or SEQ_EXP_HOME must be defined.\n");
      }
      exp=strdup(tmpExp);
   }

   if (type != NULL) {
      if(strcmp(type, "log") == 0) {
         read_type=LR_SHOW_ALL;
      } else if(strcmp(type, "statuses") == 0) {
	      SeqUtil_TRACE(TL_FULL_TRACE,"logreader type: statuses\n");
	      read_type=LR_SHOW_STATUS;
      } else if (strcmp(type, "stats") == 0){
	      SeqUtil_TRACE(TL_FULL_TRACE,"logreader type: stats\n"); 
	      read_type=LR_SHOW_STATS;
      } else if (strcmp(type, "avg") == 0) {  
         SeqUtil_TRACE(TL_FULL_TRACE,"logreader type: show averages\n");
         read_type=LR_SHOW_AVG;
      } else if (strcmp(type, "compute_avg") == 0) {
	      SeqUtil_TRACE(TL_FULL_TRACE,"logreader type: compute averages\n");
	      read_type=LR_CALC_AVG;
      } else {
         fprintf ( stderr, "Unsupported type (-t argument).\n");
         exit(EXIT_FAILURE);
      }
      
   }

   logreader(inputFile,outputFile,exp,datestamp,read_type,stats_days,clobberFile);

   free(type); 
   free(exp); 
   free(datestamp); 
   free(inputFile);
   free(outputFile);

   exit (0);
}
