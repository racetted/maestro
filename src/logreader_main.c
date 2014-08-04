#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "logreader.h"

static void printUsage()
{
   printf("Logreader usage:\n");
   printf("     log reader -i inputfile [-o outputfile -n node -l loopargs -s signal -a start_offset -z end_offset -v]\n");
   printf("         where:\n");
   printf("         inputfile    is the logfile to read (mandatory)\n");
   printf("         outputfile   is the file where the output is written to (optional)\n");
   printf("         node         is full path of task or family node (optional)\n");
   printf("         -v           verbosity level\n");
   printf("         loopargs     is a comma separated list of loop arguments (optional)\n");
   printf("         signal       is one of:\n");
   printf("            submit begin end abort initbranch initnode\n");
   printf("         start_offset is the offset value where to start reading (optional)\n");
   printf("         end_offset   is theoffset value where to stop reading (optional)\n");
   exit(1);
}

#ifdef Mop_linux
main_logreader ( int argc, char * argv[] )
#else
main ( int argc, char * argv[] )
#endif
{
   extern char *optarg;
   char *loops=NULL, *inputfile=NULL, *outputfile=NULL, *signal=NULL, *node=NULL;
   char *a_buffer=NULL, *z_buffer=NULL;
   int start_offset=0, end_offset=0; 
   SeqNameValuesPtr loopsArgs = NULL;
   int gotSignal = 0, gotLoops = 0, nodeFound = 0, c;
   int result;
   
   if ( argc == 1 || argc == 2) {
      printUsage();
   }
   
   while ((c = getopt(argc, (char* const*) argv, "i:o:n:l:s:a:z:v")) != -1) {
         switch(c) {
         case 'n':
            node = malloc( strlen( optarg ) + 1 );
            strcpy(node,optarg);
            nodeFound = 1;
            break;
         case 'i':
            inputfile = malloc( strlen( optarg ) + 1 );
            strcpy(inputfile,optarg);
            break;
         case 'o':
            outputfile = malloc( strlen( optarg ) + 1 );
            strcpy(outputfile,optarg);
            break;
         case 'v':
            SeqUtil_setTraceLevel(1);
            break;
         case 's':
            signal = malloc( strlen( optarg ) + 1 );
            strcpy(signal,optarg);
            gotSignal = 1;
            break;
         case 'a':
            a_buffer = malloc( strlen( optarg ) + 1 );
	    strcpy(a_buffer,optarg);
            start_offset=atoi(a_buffer);
            break;
         case 'z':
            z_buffer = malloc( strlen( optarg ) + 1 );
	    strcpy(z_buffer,optarg);
            end_offset=atoi(z_buffer);
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

   if ( inputfile == NULL ) {
      printUsage();
   }
   
   if ( gotLoops ) {
      loops = SeqLoops_getExtFromLoopArgs(loopsArgs);
      SeqUtil_TRACE("logreader: loop arguments detected, extension: %s\n", loops);
   }
   
   result = logreader( inputfile, outputfile, node, loops, signal, start_offset, end_offset );
   if (result != 0) {
      SeqUtil_TRACE("logreader: ERROR while reading input file %s, formatted output might not be complete\n", inputfile);
   }
   
   free(node);
   free(inputfile);
   free(outputfile);
   free(signal);
   free(loops);
   free(a_buffer);
   free(z_buffer);
   
   return 0;
}
