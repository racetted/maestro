/* SeqDatesUtil.h - Basic date functions used in the Maestro sequencer software package.
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
