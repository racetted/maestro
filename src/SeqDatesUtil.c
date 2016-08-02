/* SeqDatesUtil.c - Basic date functions used in the Maestro sequencer software package.
 * Copyright (C) 2011-2015  Operations division of the Canadian Meteorological Centre
 *                          Environment Canada
 *
 * Maestro is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * version 2.1 of the License.
 *
 * Maestro is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SeqUtil.h"
#include "SeqDatesUtil.h"


/********************************************************************************
 * Returns a copy of baseDatestamp incremented by time_delta or by hour if
 * time_delta is not specified ( NULL or "").
********************************************************************************/
const char * SeqDatesUtil_getIncrementedDatestamp( const char * baseDatestamp,const char * hour,const char * time_delta)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "getIncrementedDatestamp() begin\n");
   const char * incrementedDatestamp = NULL;
   if( time_delta != NULL && strlen(time_delta) > 0 ){
      incrementedDatestamp  = SeqDatesUtil_addTimeDelta( baseDatestamp, time_delta);
   } else if( hour != NULL && strlen(hour) > 0 ) {
      /* calculate relative datestamp based on the current one */
      incrementedDatestamp = SeqDatesUtil_getPrintableDate( baseDatestamp,0, atoi(hour),0,0 );
   } else {
      SeqUtil_TRACE(TL_FULL_TRACE, "getIncrementedDatestamp(): baseDatestamp=%s\n", baseDatestamp);
      incrementedDatestamp = strdup(baseDatestamp);
   }
   SeqUtil_TRACE(TL_FULL_TRACE, "getIncrementedDatestamp() end\n");
   return incrementedDatestamp;
}


/********************************************************************************
 * SeqDatesUtil_addTimeDelta()
 * Returns a datestamp string incremented by the specified time delta.  The
 * units are
 *    d for days,
 *    h for hours,
 *    m for minutes,
 *    s for seconds.
 * If the first character is a '-', then the whole delta will be negative.
 *
 * The inted format is something like 1d2h3m4s or -1d2h3m4s but the parsing
 * scheme actually allows for numbers to be followed by any number of non-digit
 * characters as long as the first letter is d,h,m or s.  So
 * -1day,2hours,3minutes,4seconds will work.
 *
 * Also note that if days are specified twice, no error will be raised, so
 * 1d2m3d will be interpreted as days=3 minutes=2.  Unknown types will be
 * ignored but will cause a TL_ERROR level trace call to show a message.
********************************************************************************/
#define isDigit(a) ('0' <= a && a <= '9')
#define MAX_DIGITS 9
char* SeqDatesUtil_addTimeDelta(const char * datestamp, const char * timeDelta){
   const char * p = timeDelta;
   char buffer[MAX_DIGITS] = {'\0'}, type;
   int sign = ( *p == '-' ? -1 : 1);
   int days = 0, hours = 0, minutes = 0, seconds = 0, digitCounter = 0;
   SeqUtil_TRACE(TL_FULL_TRACE, "SeqDatesUtil_addTimeDelta(): Parsing timeDelta=%s\n", timeDelta);
   while( *p != '\0' && !isDigit(*p) ) p++;

   while( *p != '\0' ){
      while( isDigit(*p) && digitCounter < MAX_DIGITS )
         buffer[digitCounter++] = *p++;
      type = *p++;

      switch(type){
         case 'd': days = sign*atoi(buffer); break;
         case 'h': hours = sign*atoi(buffer); break;
         case 'm': minutes = sign*atoi(buffer); break;
         case 's': seconds = sign*atoi(buffer); break;
         default:
            SeqUtil_TRACE(TL_ERROR,"SeqDatesUtil_addTimeDelta(): Encountered unknown type specifier '%c' after number %s\n", type, buffer);
      }

      while( *p != '\0' && !isDigit(*p) ) p++;
      memset(buffer, '\0', sizeof(buffer));
      digitCounter = 0;
   }

   SeqUtil_TRACE(TL_FULL_TRACE, "TimeDelta=%s : Days=%d, hours=%d, minutes=%d, seconds=%d\n",timeDelta, days,hours,minutes,seconds);
   return SeqDatesUtil_getPrintableDate( datestamp, days, hours, minutes, seconds );
}
#undef MAX_DIGITS

/********************************************************************************
 * Returns incremented date (or decremented)
 * char* printable_date  -- start date
 * int day               -- increment or decrement by x days
 * int hour              -- increment or decrement by x hours
 * int minute            -- increment or decrement by x minutes
 * int second            -- increment or decrement by x seconds
********************************************************************************/
char* SeqDatesUtil_getPrintableDate(const char* printable_date, int day, int hour, int minute, int second )
{
   long long int increment;
   char fulldate[15],buffer[15],four_char_date[5],two_char_date[3];
   long long int stamp;
   int yyyymmdd, hhmmss, i_year=0, i_month=0, i_day=0, i_hour=0, i_minute=0, i_second=0;
  
   memset( fulldate, '\0', sizeof(fulldate) );
   memset( buffer, '\0', sizeof(buffer) );
   memset( four_char_date, '\0', sizeof(four_char_date) );
   memset( two_char_date, '\0', sizeof(two_char_date) );
   if (strlen(printable_date) > 14) {
       fprintf(stderr,"SeqDatesUtil_getPrintableDate received date longer than 14 characters (printable_date=%s), returning NULL", printable_date);   
       return NULL; 
   }
   sprintf(fulldate,"%s",printable_date); 
   increment=FromDaysToSeconds(day,hour,minute,second); 
 
   SeqUtil_TRACE(TL_FULL_TRACE, "SeqDatesUtil_getPrintableDate() input date: %s, increment: %d d %d h %d m %d s = %lli increment (seconds)\n",fulldate, day, hour, minute, second, increment);

   strncpy(four_char_date,fulldate,4);   
   four_char_date[4]='\0';   
   i_year=atoi(four_char_date);   
   strncpy(two_char_date,fulldate+4,2);   
   two_char_date[2]='\0';   
   i_month=atoi(two_char_date);   
   strncpy(two_char_date,fulldate+6,2);   
   two_char_date[2]='\0';   
   i_day=atoi(two_char_date);   
   strncpy(two_char_date,fulldate+8,2);   
   two_char_date[2]='\0';   
   i_hour=atoi(two_char_date);   
   strncpy(two_char_date,fulldate+10,2);   
   two_char_date[2]='\0';   
   i_minute=atoi(two_char_date);   
   strncpy(two_char_date,fulldate+12,2);   
   two_char_date[2]='\0';   
   i_second=atoi(two_char_date);   
    
   stamp = JulianSecond(i_year,i_month,i_day,i_hour,i_minute,i_second); 
   stamp = stamp + increment;
   DateFromJulian(stamp,&yyyymmdd,&hhmmss);
   sprintf( buffer,"%8.8d%6.6d",yyyymmdd,hhmmss);
   SeqUtil_TRACE(TL_FULL_TRACE, "SeqDatesUtil_getPrintableDate() result: yyyymmdd %d hhmmss %d -> buffer=%s \n",yyyymmdd,hhmmss, buffer);
   return strdup( buffer );
}

/*Sakamoto's algorithm for day of week */
int SeqDatesUtil_dow(int y, int m, int d)
{
   static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
   y -= m < 3;
   return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}

int SeqDatesUtil_isDepHourValid(const char* date,const char* hourToCheck){
  SeqUtil_TRACE(TL_FULL_TRACE, "SeqDatesUtil.isDepHourValid() checking hour %s for date %s \n", hourToCheck, date);

  if ((hourToCheck == NULL) || (date == NULL))  {
     SeqUtil_TRACE(TL_ERROR, "SeqDatesUtil.isDepHourValid() input null, return 0\n");
     return 0; 
  }
 
  if (strlen(date) >= 10) {
     if (strncmp(date+8,hourToCheck,2)==0) {
        SeqUtil_TRACE(TL_FULL_TRACE, "SeqDatesUtil.isDepHourValid() comparison done, returning 1 \n");
        return 1;
     } else {
        SeqUtil_TRACE(TL_FULL_TRACE, "SeqDatesUtil.isDepHourValid() comparison done, returning 0 \n");
        return 0;
     }
  } else {
     SeqUtil_TRACE(TL_ERROR, "SeqDatesUtil.isDepHourValid() date string too short to check hour, return 0\n");
     return 0; 
  }
 
}
 
int SeqDatesUtil_isDepDOWValid( const char* date, const char* dowToCheck){
  int dow = 0, dowToCheckInInt=0; 
  char year[5], month[3], day[3];

  SeqUtil_TRACE(TL_FULL_TRACE, "SeqDatesUtil.isDepDOWValid() checking day of week %s for date %s \n", dowToCheck, date);

  if ((dowToCheck == NULL) || (date == NULL))  {
      SeqUtil_TRACE(TL_ERROR, "SeqDatesUtil.isDepDOWValid() input null, return 0\n");
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
  SeqUtil_TRACE(TL_FULL_TRACE, "SeqDatesUtil.isDepDOWValid() returning result of comparing %d and %d \n", dowToCheckInInt, dow);
  return (dow == atoi(dowToCheck));
}


/* Hopefully useful routines for C and FORTRAN programming
 * Copyright (C) 2015  Environnement Canada
 *
 * This code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * version 2.1 of the License.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this code; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
 NOTES    - ALGORITHM FROM "COMMUNICATIONS OF THE ACM" (1968), PAGE 657.
          - IT COVERS A PERIOD OF 7980 YEARS WITH DAY 1 STARTING
            AT YEAR=-4713, MONTH=11, DAY=25
*/
/* original fortran code
   I, J, K = year, month, day
      JD        = K-32075+1461*(I+4800+(J-14)/12)/4
     1             +367*(J-2-(J-14)/12*12)/12-3
     2             *((I+4900+(J-14)/12)/100)/4
*/
long long JulianSecond(int year, int month, int day, int hour, int minute, int second)
{
  int jd, js;
  long long jsec;
  /* compute julian day */
  jd = day-32075+1461*(year+4800+(month-14)/12)/4
       +367*(month-2-(month-14)/12*12)/12
       -3*((year+4900+(month-14)/12)/100)/4 ;
  js = second + (60*minute) + (3600*hour) ;
  jsec = jd ;
  jsec = jsec * (24*3600) + js;

  return jsec; 
}
/* original fortran code
   I, J, K = year, month, day
      L= JD+68569
      N= 4*L/146097
      L= L-(146097*N+3)/4
      I= 4000*(L+1)/1461001
      L= L-1461*I/4+31
      J= 80*L/2447
      K= L-2447*J/80
      L= J/11
      J= J+2-12*L
      I= 100*(N-49)+I+L
*/
void DateFromJulian (long long jsec, int *yyyymmdd, int *hhmmss)
{
  int js;
  long long jr8;
  int h, m, s;
  int yr, mth, day;
  int temp1, temp2, jday;

  jr8 = jsec / (24*3600) ;           /* julian day */
  jday = jr8;

  js  = jsec - (jr8 * 24 * 3600) ;   /* seconds in day */
  h = js / 3600 ;                    /* hours */
  m = (js - h * 3600) / 60 ;         /* minutes */
  s = js - (h * 3600) - (m *  60) ;  /* seconds */
  *hhmmss = s + (m * 100) + (h * 10000);

  temp1= jday+68569 ;
  temp2= 4*temp1/146097 ;
  temp1= temp1-(146097*temp2+3)/4 ;
  yr= 4000*(temp1+1)/1461001 ;
  temp1= temp1-1461*yr/4+31 ;
  mth= 80*temp1/2447 ;
  day= temp1-2447*mth/80 ;
  temp1= mth/11 ;
  mth= mth+2-12*temp1 ;
  yr= 100*(temp2-49)+yr+temp1 ;
  *yyyymmdd = day + (mth * 100) + (yr * 10000) ;
}

/*
If one wishes to return a value from a total of 6 days 5 hours ago, both day and hour arguments should be negative.
*/

long long FromDaysToSeconds (int day, int hour, int minute, int second)
{
  return day*60*60*24 + hour*60*60 + minute*60 + second;
} 


#if defined(SELF_TEST)
#include <stdio.h>
#include <stdlib.h>
main()
{
  long long stamp;
  int yyyymmdd, hhmmss;

  stamp = JulianSecond(2015,01,31,22,45,15) ; /* jan 31 2015 22:45:15 */
  stamp = stamp + 5415 ;                      /* + 1:30:15   */
  DateFromJulian(stamp,&yyyymmdd,&hhmmss);
  fprintf(stdout,"new date = %8.8d:%6.6d\n",yyyymmdd,hhmmss);
  fprintf(stdout,"expected = %8.8d:%6.6d\n",20150201,1530) ; /* feb 1 2015 00:1530 */

  stamp = JulianSecond(2016,02,28,22,45,15) ; /* feb 28 2016 22:45:15 , leap year test */
  stamp = stamp + 5415 ;                      /* + 1:30:15   */
  DateFromJulian(stamp,&yyyymmdd,&hhmmss);
  fprintf(stdout,"new date = %8.8d:%6.6d\n",yyyymmdd,hhmmss);
  fprintf(stdout,"expected = %8.8d:%6.6d\n",20160229,1530) ; /* feb 1 2015 00:1530 */

  stamp = JulianSecond(2000,02,28,22,45,15) ; /* feb 28 2016 22:45:15 , leap year test */
  stamp = stamp + 5415 ;                      /* + 1:30:15   */
  DateFromJulian(stamp,&yyyymmdd,&hhmmss);
  fprintf(stdout,"new date = %8.8d:%6.6d\n",yyyymmdd,hhmmss);
  fprintf(stdout,"expected = %8.8d:%6.6d\n",20000229,1530) ; /* feb 1 2000 00:1530 */

  stamp = JulianSecond(2100,02,28,22,45,15) ; /* feb 28 2100 22:45:15 , leap year test */
  stamp = stamp + 5415 ;                      /* + 1:30:15   */
  DateFromJulian(stamp,&yyyymmdd,&hhmmss);
  fprintf(stdout,"new date = %8.8d:%6.6d\n",yyyymmdd,hhmmss);
  fprintf(stdout,"expected = %8.8d:%6.6d\n",21000301,1530) ; /* feb 1 2015 00:1530 */
}
#endif



