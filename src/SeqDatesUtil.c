#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rpnmacros.h>
#include "SeqUtil.h"

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
   int timeValue = 0, newTimeValue = 0;
   int dateValue = 0, newDateValue = 0, finalDateValue = 0, timeHourInt = 0;
   int time = 0;
   int cod = 0;
   int finalPrintableDate = 0, finalPrintableTime = 0;
   double doubleInc = (double) increment;
   char dateBuffer[9], timeBuffer[9], buffer[50], timeHour[3];
   char *datePtr = NULL;

   memset( dateBuffer, '\0', sizeof(dateBuffer) );
   memset( timeBuffer, '\0', sizeof(timeBuffer) );
   memset( timeHour, '\0', sizeof(timeHour) );

   if( strlen( printable_date ) != 14 ) {
      return NULL;
   }
   strncpy( dateBuffer, printable_date, 8 );
   datePtr = printable_date;
   datePtr = datePtr + 8;
   strncpy( timeBuffer, datePtr, 6 );
   strncpy( timeHour, datePtr, 2 );
   sprintf( timeBuffer, "%s00", timeBuffer );

   if( (dateValue = atoi( dateBuffer ) ) == 0 ) {
      return NULL;
   }

   timeHourInt = atoi( timeHour );

   /* I don't rely on the printable time that
    * newdate is giving me because if my hour value is 01 or 02,
    * the time returned is 100000 or 200000 which is the same as hour
    * value for hour 10 and 20. So I calculate the hours on my own
    * and only use the newdate to increment the date
    * */
   newTimeValue = timeHourInt + increment;
   if( newTimeValue < 24 && newTimeValue >= 0 ) {
      /* increment a number of hours */
      if( newTimeValue < 10 ) {
         sprintf( buffer, "%s0%d0000", dateBuffer, newTimeValue);
      } else {
         sprintf( buffer, "%s%d0000", dateBuffer, newTimeValue);
      }
   } else {
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

      timeValue = newTimeValue % 24;

      if( timeValue < 0 ) {
         timeValue = timeValue + 24;
      }	

      if( timeValue < 10 ) {
         sprintf( buffer, "%8d0%d0000", finalPrintableDate, timeValue);
      } else {
         sprintf( buffer, "%8d%d0000", finalPrintableDate, timeValue);
      }
   }

   return strdup( buffer );
}


/*Sakamoto's algorithm for day of week */
int SeqDatesUtil_dow(int y, int m, int d)
{
   static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
   y -= m < 3;
   return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}

int SeqDatesUtil_isDepHourValid( char* date, char* hourToCheck){
  SeqUtil_TRACE( "SeqDatesUtil.isDepHourValid() checking hour %s for date %s \n", hourToCheck, date);

  if ((hourToCheck == NULL) || (date == NULL))  {
     SeqUtil_TRACE( "SeqDatesUtil.isDepHourValid() input null, return 0\n");
     return 0; 
  }
 
  if (strlen(date) >= 10) {
     if (strncmp(date+8,hourToCheck,2)==0) {
        SeqUtil_TRACE( "SeqDatesUtil.isDepHourValid() comparison done, returning 1 \n");
        return 1;
     } else {
        SeqUtil_TRACE( "SeqDatesUtil.isDepHourValid() comparison done, returning 0 \n");
        return 0;
     }
  } else {
     SeqUtil_TRACE( "SeqDatesUtil.isDepHourValid() date string too short to check hour, return 0\n");
     return 0; 
  }
 
}
 
int SeqDatesUtil_isDepDOWValid( char* date, char* dowToCheck){
  int dow = 0, dowToCheckInInt=0; 
  char year[5], month[3], day[3];

  SeqUtil_TRACE( "SeqDatesUtil.isDepDOWValid() checking day of week %s for date %s \n", dowToCheck, date);

  if ((dowToCheck == NULL) || (date == NULL))  {
      SeqUtil_TRACE( "SeqDatesUtil.isDepDOWValid() input null, return 0\n");
      return 0; 
  }
  dowToCheckInInt = atoi(dowToCheck) ;
  strncpy(year, date,4); 
  year[4] = '\0';
  strncpy(month, date+4,2); 
  month[2] = '\0';
  strncpy(day, date+6,2); 
  day[2] = '\0';
  dow = SeqDatesUtil_dow(atoi(year), atoi(month), atoi(day));
  SeqUtil_TRACE( "SeqDatesUtil.isDepDOWValid() returning result of comparing %d and %d \n", dowToCheckInInt, dow);
  return (dow == atoi(dowToCheck));
}


