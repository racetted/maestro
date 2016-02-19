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

static void printUsage()
{
   printf("Logreader usage:\n");
   printf("     logreader ([-i inputfile] [-t type] [-o outputfile] | -t avg [-n days]) [-e exp] [-d datestamp] [-v] [-c] \n");
   printf("         where:\n");
   printf("            -i inputfile     the logfile to read (default ${SEQ_EXP_HOME}/logs/${datestamp}_nodelog)\n");
   printf("            -t type          the output filter. log (statuses & stats, used for xflow), statuses, stats or avg (default is log)\n");
   printf("            -o outputfile    the file where the stats are logged (if defined)\n");
   printf("            -e exp           the experiment path (default is SEQ_EXP_HOME env. variable)\n");
   printf("            -d datestamp     the initial date for averages computation, or simply the target datestamp\n");
   printf("            -n days          used with -t avg to define the number of days for the averages since \"datestamp\" (default is 7). This is a 10%% truncated average to account for extremes.\n");
   printf("            -v               verbose mode\n");
   printf("            -c               check if output file is present before trying to write. Will not write if file is present.\n");
   printf("\nStandard usage:\n"); 
   printf("            logreader -d $datestamp             will read that experiment's log, output the statuses and stats to stdout \n"); 
   printf("            logreader -d $datestamp -t stats    will read that experiment's log, create a statistics file (default ${SEQ_EXP_HOME}/stats/${datestamp}\n"); 
   printf("            logreader -d $datestamp -t avg      will calculate the x-day truncated average and create a averages file (default ${SEQ_EXP_HOME}/stats/${datestamp}_avg\n"); 
}

int main ( int argc, char * argv[] )
{
   char *type=NULL, *inputFile=NULL, *outputFile=NULL, *exp=NULL, *datestamp=NULL; 
   int stats_days=7, c, clobberFile=1; 

   if ( argc == 1 || argc == 2) {
      printUsage();
      exit(1);
   }
   
   while ((c = getopt(argc, (char* const*) argv, "i:t:n:o:d:e:vc")) != -1) {
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
	      datestamp = strdup(optarg); 
	      break;
	   case 'e':
	      exp = strdup(optarg);
	      break;
	   case 'v':
			SeqUtil_setTraceFlag( TRACE_LEVEL , TL_MINIMAL );
			SeqUtil_setTraceFlag( TF_TIMESTAMP , TF_ON );
	      break;
      case 'c':
         clobberFile=0;
	      break;
      case '?':
         printUsage();
         exit(1);
         break;
    }

   }

   if (datestamp == NULL) { 
      if (getenv("SEQ_DATE") == NULL) {
         raiseError("-d or SEQ_DATE must be defined.\n");
      } else {
         datestamp=strdup(getenv("SEQ_DATE")); 
      }
   }

   if (exp == NULL) {
      if (getenv("SEQ_EXP_HOME") == NULL) {
         raiseError("-e or SEQ_EXP_HOME must be defined.\n");
      } else {
         exp=strdup(getenv("SEQ_EXP_HOME"));
      }
   }
   
   logreader(inputFile,outputFile,exp,datestamp,type,stats_days,clobberFile);

   if ( type != NULL) free(type); 
   if ( exp != NULL) free(exp); 
   if ( datestamp != NULL) free(datestamp); 
   if ( inputFile != NULL ) free(inputFile);
   if ( outputFile != NULL ) free(outputFile);

   exit (0);
}
