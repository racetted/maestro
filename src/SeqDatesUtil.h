#ifndef _SEQ_DATES_UTIL
#define _SEQ_DATES_UTIL

char* SeqDatesUtil_getPrintableDate( char* printable_date, int day, int hour, int minute, int second );
int SeqDatesUtil_dow(int y, int m, int d);
int SeqDatesUtil_isDepHourValid( char* date, char* hourToCheck); 
int SeqDatesUtil_isDepDOWValid( char* date, char* dowToCheck); 

long long FromDaysToSeconds (int day, int hour, int minute, int second);
void DateFromJulian (long long jsec, int *yyyymmdd, int *hhmmss);
long long JulianSecond(int year, int month, int day, int hour, int minute, int second);

#endif
