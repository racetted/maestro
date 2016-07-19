/* nodelogger_main.c - Command-line API of the log-writing functions of the Maestro sequencer software package.
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
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "nodelogger.h"
#include "tictac.h"
#include "nodeinfo.h"
#include "SeqLoopsUtil.h"
#include "SeqNameValues.h"
#include "SeqUtil.h"
#include "getopt.h"

/***************************************************************
   Name: nodelogger
   description: write a formatted message to the oprun log
   usage: nodelogger job signal message
   Douglas Bender

Revision:

   April 2013:  R. Lahlou
     - add signal to abort loop in nodeinfo.c
   September 1999 Ping-An Tan
             - Using nodelogger the procedure from runcontrollib.a library.
*****************************************************************/


int MLLServerConnectionFid=0;

static void alarm_handler() { fprintf(stderr,"@@@@@@ EXCEEDED TIME IN LOOP ITERATIONS @@@@@@\n"); };

static void printUsage()
{
char * usage = "DESCRIPTION: Nodelogger\n\
\n\
USAGE:\n\
\n\
    nodelogger -n node -s signal -m message [-l loop_args] [-e exp] [-d datestamp]\n\
\n\
OPTIONS:\n\
\n\
    -n, --node\n\
        Specify the full path of the node (mandatory)\n\
\n\
    -s, --signal\n\
        Specify the message type (i.e. abort, event, info, begin, end, etc) (mandatory)\n\
\n\
    -l, --loop-args\n\
        Specify the loop arguments as a comma seperated value loop index: inner_Loop=1,outer_Loop=3\n\
\n\
    -m, --message \n\
        Supply a detailed message (optionnal)\n\
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
    -h, --help\n\
        Show this help screen\n\
\n\
    -v, --verbose\n\
        Turn on full tracing\n\
\n\
EXAMPLES:\n\
    \n\
    nodelogger -n regional/assimilation/00/task_0 -s abort -m \"invalid hour number\"\n";
puts(usage);
}

int main (int argc, char * argv[])

{
   char * short_opts = "n:s:l:m:d:e:v";
   extern char *optarg;
   extern int   optind;
   struct       option long_opts[] =
   { /*  NAME        ,    has_arg       , flag  val(ID) */
      {"exp"         , required_argument,   0,     'e'}, 
      {"node"        , required_argument,   0,     'n'}, 
      {"loop-args"   , required_argument,   0,     'l'}, 
      {"datestamp"   , required_argument,   0,     'd'}, 
      {"signal"      , required_argument,   0,     's'}, 
      {"message"     , required_argument,   0,     'm'}, 
      {"verbose"     , no_argument      ,   0,     'v'}, 
      {NULL,0,0,0} /* End indicator */
   };
   int opt_index, c = 0;


   char *node = NULL, *signal = NULL , *message = NULL, *loops = NULL, *datestamp = NULL, *validDate=NULL, *seq_exp_home = NULL;
   int errflg = 0, hasSignal = 0, hasNode = 0, hasDate = 0, hasLoops=0, dateSize=14; 
   int r;
 
   struct sigaction act;
   memset (&act, '\0', sizeof(act));

   SeqNodeDataPtr  nodeDataPtr = NULL;
   SeqNameValuesPtr loopsArgsPtr = NULL;

   if (argc >= 6) {
      while ((c = getopt_long(argc, argv, short_opts, long_opts, &opt_index)) != -1) {
            switch(c) {
            case 'n':
               hasNode = 1;
               node = malloc( strlen( optarg ) + 1 ); 
               strcpy(node,optarg);
               printf("Node = %s \n", node);
               break;
            case 's':
               hasSignal = 1;
               signal = malloc( strlen( optarg ) + 1 ); 
               strcpy(signal,optarg);
               printf("Signal = %s \n", signal);
               break;
            case 'l':
               /* loops argument */
               hasLoops=1;
               loops = malloc( strlen( optarg ) + 1 );
               strcpy(loops,optarg);
               if( SeqLoops_parseArgs( &loopsArgsPtr, loops ) == -1 ) {
                    fprintf( stderr, "ERROR: Invalid loop arguments: %s\n", loops );
                    exit(1);
               }
               break;
            case 'm':
               message = malloc( strlen( optarg ) + 1 ); 
               strcpy(message,optarg);
               printf("Message = %s \n", message);
               break;
            case 'd':
               hasDate=1;
               datestamp = malloc( strlen( optarg ) + 1 ); 
               strcpy(datestamp,optarg);
               printf("Datestamp = %s \n", datestamp);
               break;
            case 'e':
               seq_exp_home = strdup ( optarg );
               break;
            case 'v':
               SeqUtil_setTraceFlag( TRACE_LEVEL , TL_FULL_TRACE );
               SeqUtil_setTraceFlag( TF_TIMESTAMP , TF_ON );
               break;   
            case '?':
                 errflg++;
            }
      }

      if ( seq_exp_home == NULL ){
         seq_exp_home=getenv("SEQ_EXP_HOME");
      }
      SeqUtil_checkExpHome(seq_exp_home);

      if ( hasNode == 0 || hasSignal == 0 ) {
         printf ("Node and signal must be provided!\n");
         errflg++;
      }
      if (errflg) {
          printUsage();
          free(node);
          free(message);
          free(loops);
          free(signal);
          free(datestamp);
          free(validDate);
          exit(1);
      }

      /* register SIGALRM signal */
      act.sa_handler = &alarm_handler;
      act.sa_flags = 0;
      sigemptyset (&act.sa_mask);
      r = sigaction (SIGALRM, &act, NULL);
      if (r < 0) perror (__func__);

      validDate=malloc(dateSize+1); 
      if ( hasDate == 0) {
           /* getting the experiment date */
           strcpy(validDate,(char *)tictac_getDate(seq_exp_home,"", NULL));
      }
      else {
           checkValidDatestamp(datestamp);
           strcpy(validDate,datestamp); 
      }

      printf("validDate = %s \n", validDate);

      nodeDataPtr = nodeinfo( node, NI_SHOW_ALL, loopsArgsPtr, seq_exp_home, NULL, validDate,NULL );
      if (hasLoops){
          SeqLoops_validateLoopArgs( nodeDataPtr, loopsArgsPtr );
      }

      nodelogger(nodeDataPtr->name,signal,nodeDataPtr->extension,message,validDate,seq_exp_home);
      free(node);
      free(message);
      free(loops);
      free(signal);
      free(validDate);
      free(datestamp);
      SeqNode_freeNode( nodeDataPtr );

  } else {
      printUsage();
      exit(1);
  }

  exit(0);
}
