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



static void alarm_handler() { fprintf(stderr,"%%%%%%% EXCEEDED TIME IN LOOP ITERATIONS %%%%%%%%%\n"); };

static void printUsage()
{
   char *seq_exp_home = NULL;

   seq_exp_home=getenv("SEQ_EXP_HOME");
   printf("Usage:\n");
   printf("      nodelogger -n node -s signal -m message [-l loop_members -d datestamp]\n");
   printf("         where:\n");
   printf("         node is full path of the node (mandatory):\n");
   printf("         signal is message type i.e abort,event,info,begin,end,etc (mandatory)\n");
   printf("         loop_members comma seperated value loop index: inner_Loop=1,outer_Loop=3\n");
   printf("         message is a detailed description (optional)\n");
   printf("         datestamp is the 14 character date of the experiment ex: 20080530000000\n");
   printf("                   (anything shorter will be padded with 0s until 14 characters)\n");
   printf("                   Default value is the date of the experiment. \n");
   printf("      SEQ_EXP_HOME=%s\n",seq_exp_home);
   printf("Example: nodelogger -n regional/assimilation/00/task_0 -s abort -m \"invalid hour number\"\n");
}

#ifdef Mop_linux
main_nodelogger (int argc,char * argv[])
#else
main (int argc, char * argv[])
#endif

{
   extern char *optarg;

   char *node = NULL, *signal = NULL , *message = NULL, *loops = NULL, *datestamp = NULL, *validDate=NULL, *seq_exp_home = NULL;
   int errflg = 0, hasSignal = 0, hasNode = 0, hasDate = 0, hasLoops=0, dateSize=14; 
   int c,r;
 
   struct sigaction act;

   SeqNodeDataPtr  nodeDataPtr = NULL;
   SeqNameValuesPtr loopsArgsPtr = NULL;

   seq_exp_home=getenv("SEQ_EXP_HOME");
   SeqUtil_checkExpHome(seq_exp_home);

   if (argc >= 5) {
      while ((c = getopt(argc, argv, "n:s:l:m:d:")) != -1) {
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
            case '?':
                 errflg++;
            }
      }
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
      act.sa_handler = alarm_handler;
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

      nodeDataPtr = nodeinfo( node, "all", loopsArgsPtr, NULL, NULL, validDate );
      if (hasLoops){
          SeqLoops_validateLoopArgs( nodeDataPtr, loopsArgsPtr );
      }

      nodelogger(nodeDataPtr->name,signal,nodeDataPtr->extension,message,validDate);
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
