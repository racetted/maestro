/* nodeinfo_main.c - Command-line API to use the node contruction mechanism in the Maestro sequencer software package.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "SeqUtil.h"
#include "nodeinfo.h"
#include "SeqLoopsUtil.h"
#include "SeqNameValues.h"

int MLLServerConnectionFid=0;

static void printUsage()
{
   char *seq_exp_home = NULL;
   
   seq_exp_home=getenv("SEQ_EXP_HOME");
   printf("Usage:\n");
   printf("      nodeinfo -n node [-f filters -l loopargs -d datestamp -v]\n");
   printf("         where:\n");
   printf("         node     is full path of task or family node (mandatory (except -f root)):\n");
   printf("         datestamp is the datestamp of the action. Default: SEQ_DATE env var, then latest modified log under $SEQ_EXP_HOME/logs\n");
   printf("         -v       verbosity level\n");
   printf("         filters  is a comma separated list of filters (optional):\n");
   printf("                  all (default)\n");
   printf("                  task (node task path only)\n");
   printf("                  cfg (node config path only)\n");
   printf("                  res (batch resource only)\n");
   printf("                  res_path (batch resource path only)\n");
   printf("                  type (node type only)\n");
   printf("                  node (node name and extention if applicable)\n");
   printf("                  root (root node name)\n");
   printf("                  var  (variables exported in wrapper)\n");
   printf("         loopargs is a comma separated list of loop arguments (optional):\n");
   /* printf("      SEQ_EXP_HOME=%s\n",seq_exp_home); */
   printf("Example: nodeinfo -n regional/assimilation/00/task_0\n");
}

int main ( int argc, char * argv[] )
{
   extern char *optarg;
   char * loops = NULL; 

   SeqNodeDataPtr  nodeDataPtr = NULL;
   SeqNameValuesPtr loopsArgs = NULL;
   char *node = NULL, *outputFile=NULL, *datestamp=NULL ;
   char filters[256];
   int errflg = 0, nodeFound = 0;
   int c, gotLoops=0, showRootOnly = 0;
   if ( argc == 1 || argc == 2) {
      printUsage();
      exit(1);
   }
   strcpy(filters,"all");
   while ((c = getopt(argc, (char* const*) argv, "n:f:l:o:d:v")) != -1) {
         switch(c) {
         case 'n':
	    node = malloc( strlen( optarg ) + 1 );
            strcpy(node,optarg);
            nodeFound = 1;
            break;
         case 'd':
	    datestamp = malloc( strlen( optarg ) + 1 );
            strcpy(datestamp,optarg);
            break;
         case 'f':
            strcpy(filters,optarg);
            break;
         case 'v':
				SeqUtil_setTraceFlag( TRACE_LEVEL , TL_MINIMAL );
				SeqUtil_setTraceFlag( TF_TIMESTAMP , TF_ON );
            break;
	      case 'o':
	         outputFile=malloc( strlen( optarg ) + 1 );
	         strcpy(outputFile, optarg);
	         break;
         case 'l':
            /* loops argument */
            gotLoops=1;
            loops = malloc( strlen( optarg ) + 1 );
            strcpy(loops,optarg);
	         if( SeqLoops_parseArgs( &loopsArgs, loops ) == -1 ) {
               fprintf( stderr, "ERROR: Invalid loop arguments: %s\n", loops );
               exit(1);
            }
            break;
         case '?':
            printUsage();
            exit(1);
         }
   }

   if ( nodeFound == 0 && strstr( filters, "root" ) == NULL ) {
      printUsage();
      exit(1);
   }

   nodeDataPtr = nodeinfo( node, filters, loopsArgs, NULL, NULL, datestamp );

   if (gotLoops){
      SeqLoops_validateLoopArgs( nodeDataPtr, loopsArgs );
   }

   SeqNode_printNode( nodeDataPtr, filters, outputFile );
   SeqNode_freeNode( nodeDataPtr );
   free( node );
   free(outputFile);
   free(datestamp);
   exit(0);
}
