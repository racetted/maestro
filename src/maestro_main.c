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
#include "maestro.h"
#include "SeqListNode.h"
#include "SeqNameValues.h"
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
   char *seq_exp_home = NULL;
   
   seq_exp_home=getenv("SEQ_EXP_HOME");
   printf("Usage:\n");
   printf("      maestro -n node -s signal [-i] [-d datestamp] [-v] [-l loopargs] [-f flow] [-o extra_soumet_args]  \n");
   printf("         where:\n");
   printf("         node is full path of task or family node (mandatory):\n");
   printf("         signal is one of:\n");
   printf("            submit begin end abort initbranch initnode\n");
   printf("         -i is to ignore dependencies and catchup values\n");
   printf("         -v is to run in debug mode\n");
   printf("         datestamp is the date on which to execute the command. Defaults: 1) SEQ_DATE 2) latest modified log in $SEQ_EXP_HOME/logs :\n");
   printf("         flow is continue (default) or stop, representing whether the flow should continue after this node\n");
   printf("         loopargs is the comma-separated list of loop arguments. ex: -l loopa=1,loopb=2\n");
   printf("         extra_soumet_args are arguments being given to ord_soumet by the job (ex. -waste=50)\n");

   printf("      SEQ_EXP_HOME=%s\n",seq_exp_home);
   printf("Example: maestro -s submit -n regional/assimilation/00/task_0 -f continue\n");
   exit(1);
}


#ifdef Mop_linux
main_maestro (int argc, char * argv[])
#else
main (int argc, char * argv [])
#endif

{
   extern char *optarg;
   char* node = NULL, *sign = NULL, *loops = NULL, *flow = NULL, *extraArgs = NULL, *datestamp =NULL;
   int errflg = 0, status = 0;
   int c, ignoreAllDeps = 0;
   int gotNode = 0, gotSignal = 0, gotLoops = 0;
	SeqNameValuesPtr loopsArgs = NULL;

   if ( argc < 5 ) {
      printSeqUsage();
   }
   flow = malloc( 9 * sizeof(char) + 1 );
   sprintf( flow, "%s", "continue" );
   while ((c = getopt(argc, argv, "n:s:f:l:d:o:iv")) != -1) {
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
            datestamp = malloc( strlen( optarg ) + 1 );
            strcpy(datestamp,optarg);
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
            SeqUtil_setTraceLevel(1);
            break;
	 case 'i':
	    ignoreAllDeps=1;
	    break;
	 case 'o':
            extraArgs = malloc( strlen( optarg ) + 1 );
            strcpy(extraArgs,optarg);
	    break;
         case '?':
            printSeqUsage();
      }
   }
   if ( gotNode == 0 || gotSignal == 0 ) {
      printSeqUsage();
   }
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
   /* printf( "node=%s signal=%s\n", node, sign ); */
   status = maestro( node, sign, flow, loopsArgs, ignoreAllDeps, extraArgs, datestamp );

   free(flow);
   free(node);
   free(sign);
   free(extraArgs);
   free(datestamp);
   exit(status);
}

