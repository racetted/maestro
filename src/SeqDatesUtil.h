#ifndef _SEQ_DATES_UTIL
#define _SEQ_DATES_UTIL

/* function - getPrintableDate
 *  * given a date(dat1), time (tim1), and increment (nhours 6 or -6)
 *   * in hours, compute resulting date and hour and  return printable format
 *    */
char* SeqDatesUtil_getPrintableDate( char* printable_date, int increment );
int SeqDatesUtil_dow(int y, int m, int d);
int SeqDatesUtil_isDepHourValid( char* date, char* hourToCheck); 
int SeqDatesUtil_isDepDOWValid( char* date, char* dowToCheck); 


#endif
