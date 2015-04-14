#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "logreader.h"

/* global */
struct _ListListNodes MyListListNodes = { -1 , NULL , NULL };
struct _NodeLoopList MyNodeLoopList = {"first",NULL,NULL};
/* read_type: 0=all, 1=statuses, 2=stats*/
int read_type = 0;

struct stat pt;
FILE *log;

static void printUsage()
{
   printf("Logreader usage:\n");
   printf("     log reader -i inputfile [-t type]\n");
   printf("         where:\n");
   printf("         inputfile    is the logfile to read (mandatory)\n");
   printf("         type   is one of all, statuses or stats (default is all)\n");
   exit(1);
}

#ifdef Mop_linux
main_logreader ( int argc, char * argv[] )
#else
main ( int argc, char * argv[] )
#endif
{
   /*extern char *optarg;*/
   char *base, *type=NULL;
   char filename[128];
   int fp=-1, c;
   
   if ( argc == 1 || argc == 2) {
      printUsage();
   }
   
   while ((c = getopt(argc, (char* const*) argv, "i:t:")) != -1) {
      switch(c) {
	 case 'i':
	    fp = open(optarg, O_RDONLY, 0 );
	    break;
	 case 't':
	    type = malloc( strlen( optarg ) + 1 );
	    strcpy(type,optarg);
	    break;
      }
   }
   
   if ( fp < 0 ) {
      fprintf(stderr,"Cannot Open file\n");
      exit(1);
   }
   
   if (type != NULL) {
      if(strcmp(type, "statuses") == 0) {
	 read_type=1;
      } else if (strcmp(type, "stats") == 0){
	 read_type=2;
      }
   }
   
   if ( fstat(fp,&pt) != 0 ) {
      fprintf(stderr,"Cannot fstat file\n");
      exit(1);
   }
   
   if ( ( base = mmap(NULL, pt.st_size, PROT_READ, MAP_SHARED, fp, (off_t)0) ) == (char *) MAP_FAILED ) {
      fprintf(stderr,"Map failed \n");
      close(fp);
      exit(1);
   }
   
   
   if ( (log=fopen("./output","w+")) == NULL ) {
      fprintf(stderr,"could not open in write mode output file\n");
      exit(1);
   } else {
      setvbuf(log, NULL, _IONBF, 0);
   }
   
   read_file(base,log); 
   
   /* unmap */
   munmap(base, pt.st_size);  
   
   /* Print Nodes */
   print_LListe ( MyListListNodes , log );
   
   fclose(log);
}
