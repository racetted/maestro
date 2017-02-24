/* maestro_main.c - Command-line API for the main engine in the Maestro sequencer software package.
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
#include <string.h>
#include <unistd.h> 
#include "maestro.h"
#include "SeqListNode.h"
#include "SeqUtil.h"
#include "SeqNameValues.h"
#include "SeqDatesUtil.h"
#include "getopt.h"
/***********************************************************************************
* name: maestro
*
* author: cmois
*
* description: maestro - operational run control manager
*
* revision: 
*
*************************************************************************************/
static void printSeqUsage()
{
   char * usage = "DESCRIPTION: Maestro\n\
\n\
USAGE\n\
\n\
    maestro -n node -s signal [-i] [-e exp] [-d datestamp] [-v] [-l loopargs] [-f flow] [-o extra_soumet_args]\n\
\n\
OPTIONS\n\
\n\
    -n, --node\n\
        Specify the full path of task or family node (mandatory (except -f root))\n\
\n\
    -l, --loop-args\n\
        Specify the loop arguments as a comma seperated value loop index: inner_Loop=1,\n\
        outer_Loop=3\n\
\n\
    -f, --flow\n\
        Set the flow to continue (default) or stop, representing whether the flow should \n\
        continue after this node.\n\
\n\
    -s, --signal\n\
        Specify the signal as one of:\n\
            submit\n\
            begin\n\
            end\n\
            abort\n\
            initbranch\n\
            initnode\n\
\n\
    -o, --extra-soumet-args\n\
        Specify extra arguments to be given to ord_soumet by the job (ex. -waste=50)\n\
\n\
    -i, --ignore-dependencies\n\
        Ignore dependencies and catchup values\n\
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
    maestro -s submit -n regional/assimilation/00/task_0 -f continue\n"; 
   puts(usage);
}

int main (int argc, char * argv [])

{
   const char  *short_opts = "n:s:f:l:d:o:e:ivh";
   extern char *optarg;
   extern int   optind;
   struct       option long_opts[] =
   { /*  NAME               ,    has_arg       , flag  val(ID) */
      {"exp"                , required_argument,   0,     'e'},
      {"node"               , required_argument,   0,     'n'},
      {"loop-args"          , required_argument,   0,     'l'},
      {"datestamp"          , required_argument,   0,     'd'},
      {"signal"             , required_argument,   0,     's'},
      {"flow"               , required_argument,   0,     'f'},
      {"extra-soumet-args"  , required_argument,   0,     'o'},
      {"ignore-dependencies", no_argument      ,   0,     'i'},
      {"help"               , no_argument      ,   0,     'h'},
      {"verbose"            , no_argument      ,   0,     'v'},
      {NULL,0,0,0} /* End indicator */
   };
   int opt_index, c = 0;

   char* node = NULL, *sign = NULL, *loops = NULL, *flow = NULL, *extraArgs = NULL, *datestamp =NULL, *seq_exp_home= NULL, *tmpDate=NULL;
   int errflg = 0, status = 0, i, vset=0;
   int ignoreAllDeps = 0;
   int gotNode = 0, gotSignal = 0, gotLoops = 0;
	SeqNameValuesPtr loopsArgs = NULL;

   if ( argc < 5 ) {
      printSeqUsage();
      exit(1);
   }
   flow = malloc( 9 * sizeof(char) + 1 );
   sprintf( flow, "%s", "continue" );
   while ((c = getopt_long(argc, argv, short_opts, long_opts, &opt_index )) != -1) {
      switch(c) {
         case 'n':
            node = malloc( strlen( optarg ) + 1 );
            strcpy(node,optarg);
            gotNode = 1;
            break;
         case 's':
            sign = malloc( strlen( optarg ) + 1 );
            strcpy(sign,optarg);
            gotSignal = 1;
            break;
         case 'd':
            datestamp = malloc( PADDED_DATE_LENGTH + 1 );
            strncpy(datestamp,optarg, PADDED_DATE_LENGTH );
            break;
         case 'f':
            strcpy(flow,optarg);
            break;
         case 'l':
            /* loops argument */
            loops = malloc( strlen( optarg ) + 1 );
            strcpy(loops,optarg);
            gotLoops = 1;
            break;
         case 'v':
				SeqUtil_setTraceFlag( TRACE_LEVEL , TL_FULL_TRACE );
				SeqUtil_setTraceFlag( TF_TIMESTAMP , TF_ON );
                vset=1;
            break;
	      case 'i':
	         ignoreAllDeps=1;
	         break;
	      case 'o':
            extraArgs = malloc( strlen( optarg ) + 1 );
            strcpy(extraArgs,optarg);
	         break;
         case 'e':
            seq_exp_home = strdup( optarg );
            break;
         case '?':
            printSeqUsage();
            exit(1);
      }
   }
   if ( gotNode == 0 || gotSignal == 0 ) {
      printSeqUsage();
      exit(1);
   }
   if (vset == 0) SeqUtil_setTraceEnv();
   if ( gotLoops ) {
		/*
      SeqListNode_printList( loopsArgs ); */
      if( SeqLoops_parseArgs( &loopsArgs, loops ) == -1 ) {
         errflg = 1;
         fprintf( stderr, "ERROR: Invalid loop arguments: %s\n", loops );
         exit(1);
      }
      /* SeqNameValues_printList( loopsArgs ); */
   }
   if (seq_exp_home == NULL){
      seq_exp_home = getenv("SEQ_EXP_HOME");
   }
   if  (( datestamp == NULL ) && ( (tmpDate = getenv("SEQ_DATE")) != NULL ))  {
          datestamp = malloc( PADDED_DATE_LENGTH + 1 );
          strncpy(datestamp,tmpDate, PADDED_DATE_LENGTH);
   }

   if ( datestamp != NULL ) {
      i = strlen(datestamp);
      while ( i < PADDED_DATE_LENGTH ){
         datestamp[i++] = '0';
      }
      datestamp[PADDED_DATE_LENGTH] = '\0';
   }

   status = maestro( node, sign, flow, loopsArgs, ignoreAllDeps, extraArgs, datestamp,seq_exp_home );

   free(flow);
   free(node);
   free(sign);
   free(extraArgs);
   fprintf(stderr, "maestro_main exiting with code %d\n", status );
   exit(status);
}

