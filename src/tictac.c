#include "tictac.h"


/*****************************************************************************
* tictac:
* Read or set the datestamp of a given experiment.
*
*
******************************************************************************/



/* 
tictac_setDate

Sets the date inside the $EXP_HOME/ExpDate file to the value defined.

Inputs:
  _expHome - pointer to the entrance of the experiment
  datestamp - pointer to the modified value of the date

*/

extern void tictac_setDate( char* _expHome, char* datestamp ) {

   char *dateFileName = NULL ;

   SeqUtil_checkExpHome(_expHome);
   SeqUtil_TRACE( "maestro.tictac_setDate() setting date to=%s\n", datestamp); 

   checkValidDatestamp(datestamp); 
   SeqUtil_TRACE( "maestro.tictac_setDate() setting date to=%s\n", datestamp); 
   printf("Warning: use of tictac -s $datestamp is deprecated. Please use SEQ_DATE environment variable or a -d $datestamp argument to maestro or expbegin calls.\n"); 
   SeqUtil_stringAppend( &dateFileName, _expHome );
   SeqUtil_stringAppend( &dateFileName, "/logs/" );
   SeqUtil_stringAppend( &dateFileName, datestamp );
   SeqUtil_stringAppend( &dateFileName, "_nodelog" );

   if ( touch(dateFileName) != 0 ) raiseError( "Cannot touch log file: %s\n", dateFileName );
   free (dateFileName);

}


/* 
tictac_getDate

Gets the date defined inside the $EXP_HOME/ExpDate file.

Inputs:

  _expHome - pointer to the entrance of the experiment
  format - pointer to the output format required
  datestamp - pointer to a datestamp value (used only if non-null)

*/

extern char* tictac_getDate( char* _expHome, char *format, char * datestamp ) {

   char *dateFileName = NULL, *tmpstrtok = NULL, *tmpLatestFile=NULL;
   char statePattern[SEQ_MAXFIELD];
   char dateValue[128], cmd[128];
   char* returnDate = NULL;
   size_t counter=0;
   FILE *dateFile = NULL;
   glob_t glob_logs; 
   struct stat *statbuf = NULL; 
   time_t latest = 0;
   memset( statePattern, '\0', sizeof statePattern );
   sprintf( statePattern,"%s/logs/*_nodelog", _expHome);


   SeqUtil_checkExpHome (_expHome);
   /* first get from env variable if exists */
   if( datestamp != NULL) {
         strcpy( dateValue, datestamp );
   } else {
      if( getenv("SEQ_DATE") != NULL ) {
         strcpy( dateValue, getenv("SEQ_DATE") );
      } else {
         glob(statePattern, GLOB_NOSORT,0 ,&glob_logs);
         while(counter < glob_logs.gl_pathc) {
	     statbuf=malloc(sizeof(struct stat));
	     /* Get entry's information. */
             if (stat(glob_logs.gl_pathv[counter], statbuf) == -1)
                 continue;
             if (difftime(statbuf->st_mtime,latest) > 0) {
	        latest=statbuf->st_mtime;
                free(tmpLatestFile);
		tmpLatestFile=strdup((glob_logs.gl_pathv[counter]));
	      } 
	     ++counter;
	     free(statbuf);
	 }
         globfree(&glob_logs);
         dateFileName = (char*) SeqUtil_getPathLeaf( (const char*) (tmpLatestFile) );
	 sprintf(dateValue,"%s", (char*) strtok( dateFileName, "_" ));
      }
   }
   tmpstrtok = (char*) strtok( format, "%" );
   while ( tmpstrtok != NULL ) {
      if (strcmp(tmpstrtok,"Y")==0)
         printf("%.*s", 4, &dateValue[0] );
      if (strcmp(tmpstrtok,"M")==0)
         printf("%.*s", 2, &dateValue[4] );
      if (strcmp(tmpstrtok,"D")==0)
         printf("%.*s", 2, &dateValue[6] );
      if (strcmp(tmpstrtok,"H")==0)
         printf("%.*s", 2, &dateValue[8] );
      if (strcmp(tmpstrtok,"Min")==0)
         printf("%.*s", 2, &dateValue[10] );
      if (strcmp(tmpstrtok,"S")==0)
         printf("%.*s", 2, &dateValue[12] );
      if (strcmp(tmpstrtok,"CMC")==0) {
         sprintf(cmd, "r.date %.*s", 10, &dateValue[0]);
         system(cmd);
      }
      tmpstrtok = (char*) strtok(NULL,"%");
   }
   free (tmpstrtok);

   returnDate = malloc( strlen(dateValue) + 1 );
   strcpy( returnDate, dateValue );
   free(dateFileName);
   free(tmpLatestFile);

   return returnDate;
}


/* 
checkValidDatestamp

verifies the datestamp for the proper format and bounds

Inputs:

  datestamp - datestamp to verify 

*/


extern void checkValidDatestamp(char *datestamp){

   char *tmpDateString=NULL;
   int validationInt = 0, dateLength=0;

   dateLength=strlen(datestamp);
   if ( dateLength < 8 || dateLength > 14 ) 
      raiseError("ERROR: Datestamp must contain between 8 and 14 characters (YYYYMMDD[HHMMSS]).\n"); 

   SeqUtil_TRACE( "maestro.tictac_setDate() setting date to=%s\n", datestamp); 
   tmpDateString= (char*) malloc(5);
   sprintf(tmpDateString, "%.*s",4,&datestamp[0]);
   validationInt = atoi(tmpDateString);

   if (validationInt < 0  || validationInt > 9999)
     raiseError("ERROR: Year %d outside set bounds of [0,9999].\n", validationInt); 

   SeqUtil_TRACE( "maestro.tictac_setDate() setting date to=%s\n", datestamp); 
   free(tmpDateString);

   tmpDateString= (char*) malloc(3);
   sprintf(tmpDateString, "%.*s",2,&datestamp[4]);
   validationInt = atoi(tmpDateString);

   if (validationInt < 0  || validationInt > 12)
      raiseError("ERROR: Month %d outside set bounds of [0,12].\n", validationInt); 

    SeqUtil_TRACE( "maestro.tictac_setDate() setting date to=%s\n", datestamp); 
   free(tmpDateString);

   tmpDateString= (char*) malloc(3);
   sprintf(tmpDateString, "%.*s",2,&datestamp[6]);
   validationInt = atoi(tmpDateString);

   if (validationInt < 0  || validationInt > 31)
      raiseError("ERROR: Day %d outside set bounds of [0,31].\n", validationInt); 
   SeqUtil_TRACE( "maestro.tictac_setDate() setting date to=%s\n", datestamp); 

   free(tmpDateString);

   if ( dateLength >= 10) {
      tmpDateString= (char*) malloc(3);
      sprintf(tmpDateString, "%.*s",2,&datestamp[8]);
      validationInt = atoi(tmpDateString);

      if (validationInt < 0  || validationInt > 23)
        raiseError("ERROR: Hour %d outside set bounds of [0,23].\n", validationInt); 

      SeqUtil_TRACE( "maestro.tictac_setDate() setting date to=%s\n", datestamp); 
      free(tmpDateString);
   }

   if ( dateLength >= 12) {

      tmpDateString= (char*) malloc(3);
      sprintf(tmpDateString, "%.*s",2,&datestamp[10]);
      validationInt = atoi(tmpDateString);

      if (validationInt < 0  || validationInt > 59)
         raiseError("ERROR: Minute %d outside set bounds of [0,59].\n", validationInt); 

      free(tmpDateString);
   }

   if ( dateLength == 14) {
      tmpDateString= (char*) malloc(3);
      sprintf(tmpDateString, "%.*s",2,&datestamp[12]);
      validationInt = atoi(tmpDateString);

      if (validationInt < 0  || validationInt > 59)
         raiseError("ERROR: Second %d outside set bounds of [0,59].\n", validationInt); 
   free(tmpDateString);
   }
}

