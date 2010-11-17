#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "nodeinfo.h"
#include "SeqLoopsUtil.h"
#include "SeqNameValues.h"
static void printUsage()
{
   char *seq_exp_home = NULL;
   
   seq_exp_home=getenv("SEQ_EXP_HOME");
   printf("Usage:\n");
   printf("      nodeinfo -n node [-f filters -l loopargs]\n");
   printf("         where:\n");
   printf("         node     is full path of task or family node (mandatory (except -f root)):\n");
   printf("         filters  is a comma separated list of filters (optional):\n");
   printf("                  all (default)\n");
   printf("                  task (node task path only)\n");
   printf("                  cfg (node config path only)\n");
   printf("                  res (batch resource only)\n");
   printf("                  type (node type only)\n");
   printf("                  node (node name and extention if applicable)\n");
   printf("                  root (root node name)\n");
   printf("         loopargs is a comma separated list of loop arguments (optional):\n");
   /* printf("      SEQ_EXP_HOME=%s\n",seq_exp_home); */
   printf("Example: nodeinfo -n regional/assimilation/00/task_0\n");
   exit(1);
}

#ifdef Mop_linux
main_nodeinfo ( int argc, char * argv[] )
#else
main ( int argc, char * argv[] )
#endif
{
   extern char *optarg;
   char * loops = NULL; 

   SeqNodeDataPtr  nodeDataPtr = NULL;
   SeqNameValuesPtr loopsArgs = NULL;
   char node[100];
   char filters[100];
   int errflg = 0, nodeFound = 0;
   int c, gotLoops=0, showRootOnly = 0;
   //SeqUtil_DEBUG( "testing SeqUtil_DEBUG testvar=%s\n", "testvar_value" );
   if ( argc == 1 || argc == 2) {
      printUsage();
   }
   memset(node,'\0',sizeof node);
   strcpy(filters,"all");
   while ((c = getopt(argc, (char* const*) argv, "n:f:l:d")) != -1) {
         switch(c) {
         case 'n':
            strcpy(node,optarg);
            nodeFound = 1;
            break;
         case 'f':
            strcpy(filters,optarg);
            break;
         case 'd':
            SeqUtil_setTrace(1);
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
         }
   }

   if ( nodeFound == 0 && strstr( filters, "root" ) == NULL ) {
      printUsage();
   }
   //test();

   nodeDataPtr = nodeinfo( node, filters );
   if (gotLoops){
      SeqLoops_validateLoopArgs( nodeDataPtr, loopsArgs );
   }
   SeqNode_printNode( nodeDataPtr, filters );
   SeqNode_freeNode( nodeDataPtr );
   return 0;
}
