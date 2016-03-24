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
#include "getopt.h"

static void printUsage()
{
   char *seq_exp_home = NULL;
   char * usage = "DESCRIPTION: Accessor/modifier interface for experiment catchup value.\n\
\n\
USAGE:\n\
\n\
    expcatchup [-s value] [-g] [-e experiment_home]\n\
\n\
OPTIONS:\n\
\n\
    -s, --set-catchup\n\
        Set catchup value (integer in [0,9])\n\
\n\
    -g, --get-catchup\n\
        Get catchup value\n\
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
    expcatchup -s 4\n\
        will set the experiment's catchup value to 4\n\
\n\
    expcatchup -g\n\
        will return to stdout the catchup value\n";
   puts(usage);
}

int main (int argc, char * argv [])
{
   extern char *optarg;
   extern int   optind;
   char        *short_opts = "s:e:ghv";
   struct       option long_opts[] =
   { /*  NAME        ,    has_arg       , flag  val(ID) */
      {"set-catchup" , required_argument,   0,     's'},
      {"exp-home"    , required_argument,   0,     'e'},
      {"get-catchup" , no_argument      ,   0,     'g'},
      {"help"        , no_argument      ,   0,     'h'},
      {"verbose"     , no_argument      ,   0,     'v'},
      {NULL,0,0,0} /* End indicator */
   };
   int opt_index, c = 0;
   int catchupValue = 8;
   char *expHome = NULL;
   int isDebugArg = 0, 
         isSetArg = 0, 
         isGetArg = 0;
   if (argc > 1) {
      expHome = getenv("SEQ_EXP_HOME");
      if ( expHome == NULL ) {
         fprintf(stderr, "ERROR: SEQ_EXP_HOME environment variable not set!\n" );
         exit(1);
      }
      while ((c = getopt_long(argc, argv, short_opts,long_opts, &opt_index)) != -1) {
            switch(c) {
            case 's':
               catchupValue = atoi( optarg );
               isSetArg = 1;
               if ( catchupValue == 0 && strcmp( optarg, "0" ) != 0 ) {
                  fprintf(stderr, "ERROR: invalid catchup value=%s, must be integer value between [0-%d]\n", optarg, CatchupMax );
                  exit(1);
               }
               break;
            case 'e':
               expHome = strdup( optarg );
               break;
            case 'g':
               isGetArg = 1;
               break;
            case 'h':
               printUsage();
               break;
            case 'v':
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
