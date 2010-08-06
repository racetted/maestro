#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rpnmacros.h>
extern incdatr();  /* rmnlib.a */
extern newdate();  /* rmnlib.a */

/* date must be YYYYMMDD
 * time must be HHMMSShh
 * increment is 6 or -6  or 12 or -12 etc
 * NULL is returned if format error
 *
 * returns date as YYYYMMDDHHMMSShh
 *
 */
char* SeqDatesUtil_getPrintableDate( char* printable_date, int increment )
{
   int timeValue = 0;
   int dateValue = 0, newDateValue = 0, finalDateValue = 0;
   int cod = 0;
   int finalPrintableDate = 0, finalPrintableTime = 0;
   double doubleInc = (double) increment;
   char dateBuffer[9], timeBuffer[9], buffer[50];
   char *datePtr = NULL;

   memset( dateBuffer, '\0', sizeof(dateBuffer) );
   memset( timeBuffer, '\0', sizeof(timeBuffer) );

   if( strlen( printable_date ) != 14 ) {
      return NULL;
   }
   strncpy( dateBuffer, printable_date, 8 );
   datePtr = printable_date;
   datePtr = datePtr + 8;
   strncpy( timeBuffer, datePtr, 6 );
   sprintf( timeBuffer, "%s00", timeBuffer );

   if( (dateValue = atoi( dateBuffer ) ) == 0 ) {
      return NULL;
   }

   timeValue = atoi( timeBuffer );

   /* call rpn library */
   /* get new date */
   cod = 3;

   /* printable to CMCstamp */  
   f77name(newdate)( &newDateValue, &dateValue, &timeValue, &cod);
   
   /* incr date */
   f77name(incdatr)( &finalDateValue, &newDateValue, &doubleInc );

   /* convert back to printable value */
   cod = -3;
   f77name(newdate)( &finalDateValue, &finalPrintableDate, &finalPrintableTime, &cod);

   printf( "finalPrintableTime=%d\n", finalPrintableTime ); 
   sprintf( buffer, "%8d%06d", finalPrintableDate, finalPrintableTime );

   /* I want only the first 14 digits not the whole 16
   don't care about nano seconds */ 
   buffer[14]='\0';
    
   return strdup( buffer );
}
