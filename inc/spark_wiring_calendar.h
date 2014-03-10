/**
 ******************************************************************************
 * @file    spark_wiring_calendar.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    3-March-2014
 * @brief   Header for spark_wiring_calendar.cpp module
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#ifndef __SPARK_WIRING_CALENDAR_H
#define __SPARK_WIRING_CALENDAR_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif

void Calendar_Init(void);
void Calendar_DateUpdate(void);

typedef unsigned long time_t;

/*
// Arduino time and date functions
int     hour();            // the hour now
int     hour(time_t t);    // the hour for the given time
int     hourFormat12();    // the hour now in 12 hour format
int     hourFormat12(time_t t); // the hour for the given time in 12 hour format
uint8_t isAM();            // returns true if time now is AM
uint8_t isAM(time_t t);    // returns true the given time is AM
uint8_t isPM();            // returns true if time now is PM
uint8_t isPM(time_t t);    // returns true the given time is PM
int     minute();          // the minute now
int     minute(time_t t);  // the minute for the given time
int     second();          // the second now
int     second(time_t t);  // the second for the given time
int     day();             // the day now
int     day(time_t t);     // the day for the given time
int     weekday();         // the weekday now (Sunday is day 1)
int     weekday(time_t t); // the weekday for the given time
int     month();           // the month now  (Jan is month 1)
int     month(time_t t);   // the month for the given time
int     year();            // the full four digit year: (2009, 2010 etc)
int     year(time_t t);    // the year for the given time

time_t now();              // return the current time as seconds since Jan 1 1970
void    setTime(time_t t);
void    setTime(int hr,int min,int sec,int day, int month, int yr);
void    adjustTime(long adjustment);

//date strings
#define dt_MAX_STRING_LEN 9 // length of longest date string (excluding terminating null)
char* monthStr(uint8_t month);
char* dayStr(uint8_t day);
char* monthShortStr(uint8_t month);
char* dayShortStr(uint8_t day);
*/

#ifdef __cplusplus
}
#endif

#endif
