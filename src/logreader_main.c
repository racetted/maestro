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

struct _StatsNode *rootStatsNode;

/* read_type: 0=statuses & stats, 1=statuses, 2=stats, 3=averages*/
int read_type = 0;
int stats_days = 7;
char *datestamp = NULL;
char *exp = NULL;

struct stat pt;
FILE *stats;

static void printUsage()
{
   printf("Logreader usage:\n");
   printf("     logreader (-i inputfile [-t type] [-o statsoutputfile] | -t avg [-n days]) [-e exp] -d datestamp [-v]\n");
   printf("         where:\n");
   printf("         inputfile        is the logfile to read (mandatory if type is statuses or stats)\n");
   printf("         type             is one of all (statuses & stats), statuses, stats or avg (default is all)\n");
   printf("         statsoutputfile  is the file where the stats are logged (if defined)\n");
   printf("         exp              is the experiment path (default is SEQ_EXP_HOME env. variable)\n");
   printf("         datestamp        is the initial date for averages computation (mandatory)\n");
   printf("         days             is used with -t avg to define the number of days for the averages since \"datestamp\" (default is 7)\n");
   exit(1);
}

#ifdef Mop_linux
main_logreader ( int argc, char * argv[] )
#else
main ( int argc, char * argv[] )
#endif
{
   /*extern char *optarg;*/
   char *base, *type=NULL, *n_buffer=NULL, *exp=NULL;
   char filename[128];
   int fp=-1,  c;
   int i_defined=0, t_defined=0, n_defined=0, o_defined=0, d_defined=0, e_defined=0;
   
   if ( argc == 1 || argc == 2) {
      printUsage();
      exit(1);
   }
   
   stats = NULL;
   rootStatsNode = NULL;
   
   while ((c = getopt(argc, (char* const*) argv, "i:t:n:o:d:e:v")) != -1) {
      switch(c) {
	 case 'i':
	    i_defined=1;
	    fp = open(optarg, O_RDONLY, 0 );
	    break;
	 case 't':
	    t_defined=1;
	    type = malloc( strlen( optarg ) + 1 );
	    strcpy(type,optarg);
	    break;
	 case 'n':
	    n_defined=1;
	    n_buffer = malloc( strlen( optarg ) + 1 );
	    strcpy(n_buffer, optarg);
	    stats_days = atoi(n_buffer);
	    break;
	 case 'o':
	    o_defined=1;
	    stats = fopen(optarg, "w+");
	    if (stats == NULL) {
	       fprintf(stderr, "Cannot Open stats output file");
	    }
	    break;
	 case 'd':
	    d_defined=1;
	    datestamp = malloc( strlen( optarg ) + 1 );
	    strcpy(datestamp,optarg);
	    break;
	 case 'e':
	    e_defined=1;
	    exp = malloc( strlen( optarg ) + 1 );
	    strcpy(exp,optarg);
	    break;
	 case 'v':
	    SeqUtil_setTraceLevel(1);
	    break;
      }
   }

   if (! d_defined) { 
     if ((datestamp=getenv("SEQ_DATE")) == NULL){
       raiseError("-d or SEQ_DATE must be defined.\n");
     }
   }

   if (type != NULL) {
      if(strcmp(type, "statuses") == 0) {
	 SeqUtil_TRACE("logreader type: statuses\n");
	 read_type=1;
      } else if (strcmp(type, "stats") == 0){
	 SeqUtil_TRACE("logreader type: stats\n"); 
	 read_type=2;
      } else if (strcmp(type, "avg") == 0) {
	 SeqUtil_TRACE("logreader type: compute averages\n");
	 read_type=3;
      }
   }
   
   if (! e_defined) {
     if ((exp=getenv("SEQ_EXP_HOME")) == NULL){
       raiseError("-e or SEQ_EXP_HOME must be defined.\n");
     }

   }
   
   if(read_type != 3) {
      if ( fp < 0 ) {
         fprintf(stderr,"Cannot Open input file\n");
         exit(1);
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
   
      read_file(base); 
   
      /* unmap */
      munmap(base, pt.st_size);  
      
      /*parse avg file*/
      getAverage(exp, datestamp);
      
      /*print nodes*/
      print_LListe ( MyListListNodes, stats );
   } else {
      computeAverage(exp, datestamp);
   }
   
   if (stats != NULL) fclose(stats);

   free(type); 
   free(n_buffer); 

}
