#include "tictac.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

   FILE *dateFile = NULL;

   SeqUtil_checkExpHome(_expHome);

   checkValidDatestamp(datestamp); 

   SeqUtil_stringAppend( &dateFileName, _expHome );
   SeqUtil_stringAppend( &dateFileName, "/ExpDate" );

   if ((dateFile=fopen(dateFileName,"w+")) != NULL ) {
       printf("Datestamp for experiment %s changed to %s \n", _expHome ,datestamp); 
       fprintf(dateFile,"%s",datestamp);
   } else {
         raiseError("ERROR: Date File %s cannot be written into.\n", dateFileName);
   }

   fclose(dateFile);
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

   char *dateFileName = NULL, *tmpstrtok = NULL;
   /* char dateValue[128], cmd[128]; */
   char dateValue[128], cmd[128];
   char* returnDate = NULL;
   FILE *dateFile = NULL;

   SeqUtil_checkExpHome (_expHome);
   /* first get from env variable if exists */
   if( datestamp != NULL) {
         strcpy( dateValue, datestamp );
   } else {
      if( getenv("SEQ_DATE") != NULL ) {
         strcpy( dateValue, getenv("SEQ_DATE") );
      } else {
         /* read from file */
         SeqUtil_stringAppend( &dateFileName, _expHome );
         SeqUtil_stringAppend( &dateFileName, "/ExpDate" );

         if ( access(dateFileName, R_OK) == 0 ) {
            dateFile=fopen(dateFileName,"r"); 
 
            if( fgets(dateValue, sizeof(dateValue), dateFile) == NULL ) {
               raiseError("ERROR: Date File %s is empty or erroneus.\n", dateFileName);
            }
         } else {
            raiseError("ERROR: Date File %s cannot be read.\n", dateFileName);
         }
         /* remove the newline character at the end of the returned value
          * causing bug when it is used to create lock files */
         if ( strlen(dateValue) > 0 ) {
            if ( dateValue[strlen(dateValue)-1] == '\n' ) {
               dateValue[strlen(dateValue)-1] = '\0';
            }
         }
         fclose(dateFile);
         free (dateFileName);
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

while (dateLength < 14) {
    SeqUtil_stringAppend( &datestamp, "0" );
    dateLength=strlen(datestamp);
}

tmpDateString= (char*) malloc(5);
sprintf(tmpDateString, "%.*s",4,&datestamp[0]);
validationInt = atoi(tmpDateString);

if (validationInt < 0  || validationInt > 3000)
     raiseError("ERROR: Year %d outside set bounds of [0,3000].\n", validationInt); 

free(tmpDateString);

tmpDateString= (char*) malloc(3);
sprintf(tmpDateString, "%.*s",2,&datestamp[4]);
validationInt = atoi(tmpDateString);

if (validationInt < 0  || validationInt > 12)
     raiseError("ERROR: Month %d outside set bounds of [0,12].\n", validationInt); 

free(tmpDateString);

tmpDateString= (char*) malloc(3);
sprintf(tmpDateString, "%.*s",2,&datestamp[6]);
validationInt = atoi(tmpDateString);

if (validationInt < 0  || validationInt > 31)
     raiseError("ERROR: Day %d outside set bounds of [0,31].\n", validationInt); 

free(tmpDateString);

tmpDateString= (char*) malloc(3);
sprintf(tmpDateString, "%.*s",2,&datestamp[8]);
validationInt = atoi(tmpDateString);

if (validationInt < 0  || validationInt > 23)
     raiseError("ERROR: Hour %d outside set bounds of [0,23].\n", validationInt); 

free(tmpDateString);

tmpDateString= (char*) malloc(3);
sprintf(tmpDateString, "%.*s",2,&datestamp[10]);
validationInt = atoi(tmpDateString);

if (validationInt < 0  || validationInt > 59)
     raiseError("ERROR: Minute %d outside set bounds of [0,59].\n", validationInt); 

free(tmpDateString);

tmpDateString= (char*) malloc(3);
sprintf(tmpDateString, "%.*s",2,&datestamp[12]);
validationInt = atoi(tmpDateString);

if (validationInt < 0  || validationInt > 59)
     raiseError("ERROR: Second %d outside set bounds of [0,59].\n", validationInt); 

free(tmpDateString);

}

