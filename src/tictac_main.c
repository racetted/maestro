#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "tictac.h"

/*****************************************************************************
* tictac_main:
* API call to read or set the datestamp of a given experiment.
*
*
******************************************************************************/

static void printUsage()
{
char *seq_exp_home = NULL;

seq_exp_home=getenv("SEQ_EXP_HOME");
printf("Tictac - date accessor/modifier interface for experiments \n");
printf("         \n\n");
printf("Usage:\n");
printf("      tictac -[s date,f format] [-d datestamp] \n");
printf("         where:\n");
printf("         date is the date that is to be set (in a YYYYMMDD[HHMMSS] format)\n");
printf("         format is the format of the date to return\n");
printf("                         %%Y = year\n");
printf("                         %%M = month\n");
printf("                         %%D = day\n");
printf("                         %%H = hour\n");
printf("                         %%Min = minute\n");
printf("                         %%S = second\n");
printf("                         %%CMC = 9 digit CMC format datestamp\n");
printf("      EXPHOME=%s\n\n",seq_exp_home);
printf("Example: tictac -s 2009053000\n");
printf("             will set the experiment's datefile to that date\n");
printf("Example: tictac -f %%Y%%M%%S\n");
printf("             will return to stdout the value of the date in a %%Y%%M%%S format\n");
}

#ifdef Mop_linux
main_tictac (int argc, char * argv [])
#else
main (int argc, char * argv [])
#endif

{
   extern char *optarg;
   char *dateValue = NULL, *expHome = NULL, *format=NULL;
   int c=0,returnDate=0;

   if (argc >= 2) {
      while ((c = getopt(argc, argv, "d:s:f:hv")) != -1) {
            switch(c) {
            case 's':
               expHome = getenv("SEQ_EXP_HOME");
               dateValue = malloc( strlen( optarg ) + 1 );
               strcpy(dateValue,optarg);
               break;
            case 'd':
               expHome = getenv("SEQ_EXP_HOME");
               dateValue = malloc( strlen( optarg ) + 1 );
               strcpy(dateValue,optarg);
               break;
            case 'f':
               expHome = getenv("SEQ_EXP_HOME");
               format = malloc( strlen( optarg ) + 1 );
               strcpy(format,optarg);
	       returnDate=1;
               break;
	    case 'v':
	       SeqUtil_setTraceLevel(1);
	       break; 
            case 'h':
               printUsage();
               break;
            case '?':
                 printf ("Argument s or f must be specified!\n");
                 printUsage();
                 exit(1);
            }
      }

      if (returnDate) {
          tictac_getDate( expHome,format,dateValue);
      } else {
	  SeqUtil_TRACE( "maestro.tictac() setting date to=%s\n", dateValue); 
          tictac_setDate( expHome,dateValue);
      }



   } else {
      printUsage();
      exit(1);
   }
   free(dateValue);
   free(format);
   exit(0);
}
