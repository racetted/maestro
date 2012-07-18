#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "SeqUtil.h"
static void printUsage()
{
   printf("Usage:\n");
   printf("      getdef [-d] file key\n");
   printf("         where:\n");
   printf("         file     is full path of of the definition file\n");
   printf("         key      is name of the key to search for\n");
   printf("Example: getdef ${SEQ_EXP_HOME}/resources/resources.def loop_max\n");
   exit(1);
}

#ifdef Mop_linux
main_getdef ( int argc, char * argv[] )
#else
main ( int argc, char * argv[] )
#endif
{
   char *value;
   int file,key,c;

   if ( argc < 3 ) {
      printUsage();
   }
   file=argc-2; key=argc-1;
   while ((c = getopt(argc, (char* const*) argv, "d")) != -1) {
     switch(c) {
     case 'd':
       SeqUtil_setTraceLevel(1);
       break;
     case '?':
       printUsage();
     }
   }
   if ( (value = SeqUtil_getdef( argv[file], argv[key] )) == NULL ){
     fprintf( stderr, "ERROR: Unable to find key %s in %s\n", argv[key], argv[file] );
     exit(1);}
   else{
     printf( "%s\n", value );
   }
   free(value);
   free(0);
}
