/* expcatchup_main.c - Command-line API for expcatchup, contained in the Maestro sequencer software package.
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
#include <unistd.h> 
#include "SeqUtil.h"
#include "expcatchup.h"

static void printUsage()
{
   char *seq_exp_home = NULL;

   seq_exp_home=getenv("SEQ_EXP_HOME");
   printf("Description - accessor/modifier interface for experiment catchup value \n");
   printf("         \n\n");
   printf("Usage:\n");
   printf("      catchup [-s value] [-g]\n");
   printf("         where:\n");
   printf("         -s value \t\tset catchup value, between [0-9]\n");
   printf("         -g       \t\tget catchup value\n");
   printf("         -d       \t\tset debug mode\n");

   printf("\n      EXPHOME=%s\n\n",seq_exp_home);
   printf("Example: catchup -s 4\n");
   printf("             will set the experiment's catchup value to 4\n");
   printf("Example: catchup -g \n");
   printf("             will return to stdout the catchup value\n");

}

int main (int argc, char * argv [])
{
   extern char *optarg;
   int catchupValue = 8;
   char *expHome = NULL;
   int c=0, isDebugArg = 0, isSetArg = 0, isGetArg = 0;
   if (argc > 1) {
      expHome = getenv("SEQ_EXP_HOME");
      if ( expHome == NULL ) {
         fprintf(stderr, "ERROR: SEQ_EXP_HOME environment variable not set!\n" );
         exit(1);
      }
      while ((c = getopt(argc, argv, "s:ghd")) != -1) {
            switch(c) {
            case 's':
               catchupValue = atoi( optarg );
               isSetArg = 1;
               if ( catchupValue == 0 && strcmp( optarg, "0" ) != 0 ) {
                  fprintf(stderr, "ERROR: invalid catchup value=%s, must be integer value between [0-%d]\n", optarg, CatchupMax );
                  exit(1);
               }
               break;
            case 'g':
               isGetArg = 1;
               break;
            case 'h':
               printUsage();
               break;
            case 'd':
               isDebugArg = 1;
               break;
            case '?':
                 fprintf (stderr, "ERROR:Argument s or g must be specified!\n");
                 printUsage();
                 exit(1);
            }
      }
      if ( isSetArg && isGetArg ) {
         fprintf (stderr, "ERROR:Argument s and g are mutually exclusive!\n");
         exit(1);
      }
      if ( isDebugArg ) {
			SeqUtil_setTraceFlag( TRACE_LEVEL , TL_FULL_TRACE );
			SeqUtil_setTraceFlag( TF_TIMESTAMP , TF_ON );
		}
      if ( isSetArg ) catchup_set( expHome,catchupValue);
      if ( isGetArg ) {
         catchupValue = catchup_get( expHome );
         printf( "%d\n", catchupValue );
      }
   } else {
      printUsage();
      exit(1);
   }
   exit(0);
}
