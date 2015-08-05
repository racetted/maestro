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
#include <errno.h>

/* global */
struct _ListListNodes MyListListNodes = { -1 , NULL , NULL };
struct _NodeLoopList MyNodeLoopList = {"first",NULL,NULL};

struct _StatsNode *rootStatsNode;

/* read_type: 0=statuses & stats, 1=statuses, 2=stats, 3=averages*/
extern int read_type = 0;
int stats_days = 7;

struct stat pt;

static void printUsage()
{
   printf("Logreader usage:\n");
   printf("     logreader ([-i inputfile] [-t type] [-o outputfile] | -t avg [-n days]) [-e exp] [-d datestamp] [-v]\n");
   printf("         where:\n");
   printf("         inputfile        is the logfile to read (default ${SEQ_EXP_HOME}/logs/${datestamp}_nodelog)\n");
   printf("         type             is the output filter. log (statuses & stats, used for xflow), statuses, stats or avg (default is log)\n");
   printf("         statsoutputfile  is the file where the stats are logged (if defined)\n");
   printf("         exp              is the experiment path (default is SEQ_EXP_HOME env. variable)\n");
   printf("         datestamp        is the initial date for averages computation, or simply the target datestamp\n");
   printf("         days             is used with -t avg to define the number of days for the averages since \"datestamp\" (default is 7). This is a 10% truncated average to account for extremes.\n");
   printf("Standard usage: logreader -d $datestamp             will read that experiment's log, output the statuses and stats to stdout"); 
   printf("                logreader -d $datestamp -t stats    will read that experiment's log, create a statistics file (default ${SEQ_EXP_HOME}/stats/${datestamp}"); 
   printf("                logreader -d $datestamp -t avg      will calculate the x-day truncated average and create a averages file (default ${SEQ_EXP_HOME}/stats/${datestamp}_avg"); 
   exit(1);
}

main ( int argc, char * argv[] )
{
   /*extern char *optarg;*/
   FILE *output_file;
   char *base, *type=NULL, *n_buffer=NULL, *exp=NULL;
   char *datestamp = NULL;
   char filename[128], default_input_file_path[1024], optional_output_path[1024], optional_output_dir[1024];
   int fp=-1,  c;
   int i_defined=0, t_defined=0, n_defined=0, o_defined=0, d_defined=0, e_defined=0;
   
   if ( argc == 1 || argc == 2) {
      printUsage();
      exit(1);
   }
   
   output_file = NULL;
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
	    output_file = fopen(optarg, "w+");
	    if (output_file == NULL) {
	       fprintf(stderr, "Cannot Open output_file output file");
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

   if (! e_defined) {
     if ((exp=getenv("SEQ_EXP_HOME")) == NULL){
       raiseError("-e or SEQ_EXP_HOME must be defined.\n");
     }

   }

   if (! i_defined) {
       sprintf(default_input_file_path, "%s/logs/%s_nodelog" , exp,datestamp);  
       fp = open(default_input_file_path, O_RDONLY, 0 );
   }

   if (type != NULL) {
      if(strcmp(type, "log") == 0) {
         read_type=0;
      } else if(strcmp(type, "statuses") == 0) {
	      SeqUtil_TRACE("logreader type: statuses\n");
	      read_type=1;
      } else if (strcmp(type, "stats") == 0){
	      SeqUtil_TRACE("logreader type: stats\n"); 
	      read_type=2;
         if (! o_defined){
            sprintf(optional_output_dir, "%s/stats" , exp);
            if( ! SeqUtil_isDirExists( optional_output_dir )) {
               SeqUtil_TRACE ( "mkdir: creating dir %s\n", optional_output_dir );
               if ( mkdir( optional_output_dir, 0755 ) == -1 ) {
                  fprintf ( stderr, "Cannot create: %s Reason: %s\n",optional_output_dir, strerror(errno) );
                  exit(EXIT_FAILURE);
               }
            }
            sprintf(optional_output_path, "%s/stats/%s" , exp,datestamp);  
            if ( ( output_file = fopen(optional_output_path, "w+") ) == NULL) { 
               fprintf(stderr, "Cannot open output file %s", optional_output_path);
               exit(1);
            }
         }
      } else if (strcmp(type, "avg") == 0) {
	      SeqUtil_TRACE("logreader type: compute averages\n");
	      read_type=3;
         if (! o_defined){
            sprintf(optional_output_dir, "%s/stats" , exp);
            if( ! SeqUtil_isDirExists( optional_output_dir )) {
               SeqUtil_TRACE ( "mkdir: creating dir %s\n", optional_output_dir );
               if ( mkdir( optional_output_dir, 0755 ) == -1 ) {
                  fprintf ( stderr, "Cannot create: %s Reason: %s\n",optional_output_dir, strerror(errno) );
                  exit(EXIT_FAILURE);
               }
            }
            sprintf(optional_output_path, "%s/stats/%s_avg" , exp,datestamp);  
            if ( ( output_file = fopen(optional_output_path, "w+") ) == NULL) { 
               fprintf(stderr, "Cannot open output file %s", optional_output_path);
               exit(1);
            }
         }
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
      print_LListe ( MyListListNodes, output_file );
   } else {
      computeAverage(exp, datestamp);
   }
   
   if (output_file != NULL) fclose(output_file);

   free(type); 
   free(n_buffer); 
   if ( e_defined ) free(exp);
   if ( d_defined ) free(datestamp);

}
