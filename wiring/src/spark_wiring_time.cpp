/**
 ******************************************************************************
 * @file    spark_wiring_time.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    3-March-2014
 * @brief   Time utility functions to set and get Date/Time using RTC
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

#include "spark_wiring_time.h"
#include "rtc_hal.h"
#include "stdio.h"
#include "stdlib.h"
#include "spark_wiring_system.h"
#include "spark_wiring_cloud.h"
#include "system_mode.h"
#include "system_event.h"

const char* TIME_FORMAT_DEFAULT = "asctime";
const char* TIME_FORMAT_ISO8601_FULL = "%Y-%m-%dT%H:%M:%S%z";


/* The calendar "tm" structure from the standard libray "time.h" has the following definition: */
//struct tm
//{
//	int tm_sec;         /* seconds,  range 0 to 59          */
//	int tm_min;         /* minutes, range 0 to 59           */
//	int tm_hour;        /* hours, range 0 to 23             */
//	int tm_mday;        /* day of the month, range 1 to 31  */
//	int tm_mon;         /* month, range 0 to 11             */
//	int tm_year;        /* The number of years since 1900   */
//	int tm_wday;        /* day of the week, range 0 to 6    */
//	int tm_yday;        /* day in the year, range 0 to 365  */
//	int tm_isdst;       /* daylight saving time             */
//};

struct tm calendar_time_cache;	// a cache of calendar time structure elements
time_t unix_time_cache;  		// a cache of unix_time that was updated
time_t time_zone_cache;			// a cache of the time zone that was set
time_t dst_cache = 3600;        // a cache of the DST offset that was set (default 1hr)
time_t dst_current_cache = 0;   // a cache of the DST offset currently being applied

/* Time utility functions */
static struct tm Convert_UnixTime_To_CalendarTime(time_t unix_time);
//static time_t Convert_CalendarTime_To_UnixTime(struct tm calendar_time);
//static struct tm Get_CalendarTime(void);
//static void Set_CalendarTime(struct tm t);
static void Refresh_UnixTime_Cache(time_t unix_time);

/* Convert Unix/RTC time to Calendar time */
static struct tm Convert_UnixTime_To_CalendarTime(time_t unix_time)
{
	struct tm *calendar_time;
	calendar_time = localtime(&unix_time);
	calendar_time->tm_year += 1900;
	return *calendar_time;
}

/* Convert Calendar time to Unix/RTC time */
/*
static time_t Convert_CalendarTime_To_UnixTime(struct tm calendar_time)
{
	calendar_time.tm_year -= 1900;
	time_t unix_time = mktime(&calendar_time);
	return unix_time;
}
*/

/* Get converted Calendar time */
/*
 static struct tm Get_CalendarTime(void)
{
	time_t unix_time = HAL_RTC_Get_UnixTime();
	unix_time += time_zone_cache;
	struct tm calendar_time = Convert_UnixTime_To_CalendarTime(unix_time);
	return calendar_time;
}
 */

/* Set Calendar time as Unix/RTC time */
/*
static void Set_CalendarTime(struct tm calendar_time)
{
	HAL_RTC_Set_UnixTime(Convert_CalendarTime_To_UnixTime(calendar_time));
}
*/

/* Refresh Unix/RTC time cache */
static void Refresh_UnixTime_Cache(time_t unix_time)
{
    unix_time += time_zone_cache;
    unix_time += dst_current_cache;
    if(unix_time != unix_time_cache)
    {
            calendar_time_cache = Convert_UnixTime_To_CalendarTime(unix_time);
            unix_time_cache = unix_time;
    }
}

const char* TimeClass::format_spec = TIME_FORMAT_DEFAULT;

/* current hour */
int TimeClass::hour()
{
	return hour(now());
}

/* the hour for the given time */
int TimeClass::hour(time_t t)
{
	Refresh_UnixTime_Cache(t);
	return calendar_time_cache.tm_hour;
}

/* current hour in 12 hour format */
int TimeClass::hourFormat12()
{
	return hourFormat12(now());
}

/* the hour for the given time in 12 hour format */
int TimeClass::hourFormat12(time_t t)
{
	Refresh_UnixTime_Cache(t);
	if(calendar_time_cache.tm_hour == 0)
		return 12;	//midnight
	else if( calendar_time_cache.tm_hour > 12)
		return calendar_time_cache.tm_hour - 12 ;
	else
		return calendar_time_cache.tm_hour ;
}

/* returns true if time now is AM */
uint8_t TimeClass::isAM()
{
	return !isPM(now());
}

/* returns true the given time is AM */
uint8_t TimeClass::isAM(time_t t)
{
	return !isPM(t);
}

/* returns true if time now is PM */
uint8_t TimeClass::isPM()
{
	return isPM(now());
}

/* returns true the given time is PM */
uint8_t TimeClass::isPM(time_t t)
{
	return (hour(t) >= 12);
}

/* current minute */
int TimeClass::minute()
{
	return minute(now());
}

/* the minute for the given time */
int TimeClass::minute(time_t t)
{
	Refresh_UnixTime_Cache(t);
	return calendar_time_cache.tm_min;
}

/* current seconds */
int TimeClass::second()
{
	return second(now());
}

/* the second for the given time */
int TimeClass::second(time_t t)
{
	Refresh_UnixTime_Cache(t);
	return calendar_time_cache.tm_sec;
}

/* current day */
int TimeClass::day()
{
	return day(now());
}

/* the day for the given time */
int TimeClass::day(time_t t)
{
	Refresh_UnixTime_Cache(t);
	return calendar_time_cache.tm_mday;
}

/* the current weekday */
int TimeClass::weekday()
{
	return weekday(now());
}

/* the weekday for the given time */
int TimeClass::weekday(time_t t)
{
	Refresh_UnixTime_Cache(t);
	return (calendar_time_cache.tm_wday + 1);//Arduino's weekday representation
}

/* current month */
int TimeClass::month()
{
	return month(now());
}

/* the month for the given time */
int TimeClass::month(time_t t)
{
	Refresh_UnixTime_Cache(t);
	return (calendar_time_cache.tm_mon + 1);//Arduino's month representation
}

/* current four digit year */
int TimeClass::year()
{
	return year(now());
}

/* the year for the given time */
int TimeClass::year(time_t t)
{
	Refresh_UnixTime_Cache(t);
	return calendar_time_cache.tm_year;
}

/* return the current time as seconds since Jan 1 1970 */
time_t TimeClass::now()
{
    (void)isValid();
	return HAL_RTC_Get_UnixTime();
}

time_t TimeClass::local()
{
	return HAL_RTC_Get_UnixTime()+time_zone_cache+dst_current_cache;
}

/* set the time zone (+/-) offset from GMT */
void TimeClass::zone(float GMT_Offset)
{
	if(GMT_Offset < -12 || GMT_Offset > 14)
	{
		return;
	}
	time_zone_cache = GMT_Offset * 3600;
}

float TimeClass::zone()
{
	return time_zone_cache / 3600.0;
}

float TimeClass::getDSTOffset()
{
    return dst_cache / 3600.0;
}

void TimeClass::setDSTOffset(float offset)
{
    if (offset < 0 || offset > 2)
    {
        return;
    }
    dst_cache = offset * 3600;
}

void TimeClass::beginDST()
{
    dst_current_cache = dst_cache;
}

void TimeClass::endDST()
{
    dst_current_cache = 0;
}

uint8_t TimeClass::isDST()
{
    return !(dst_current_cache == 0);
}

/* set the given time as unix/rtc time */
void TimeClass::setTime(time_t t)
{
    HAL_RTC_Set_UnixTime(t);
    system_notify_time_changed((uint32_t)time_changed_manually, nullptr, nullptr);
}

/* return string representation for the given time */
String TimeClass::timeStr(time_t t)
{
	t += time_zone_cache;
    t += dst_current_cache;
	tm* calendar_time = localtime(&t);
        char* ascstr = asctime(calendar_time);
        int len = strlen(ascstr);
        ascstr[len-1] = 0; // remove final newline
	return String(ascstr);
}

String TimeClass::format(time_t t, const char* format_spec)
{
    if (format_spec==NULL)
        format_spec = this->format_spec;

    if (!format_spec || !strcmp(format_spec,TIME_FORMAT_DEFAULT)) {
        return timeStr(t);
    }
    t += time_zone_cache;
    t += dst_current_cache;
    tm* calendar_time = localtime(&t);
    return timeFormatImpl(calendar_time, format_spec, time_zone_cache + dst_current_cache);
}

String TimeClass::timeFormatImpl(tm* calendar_time, const char* format, int time_zone)
{
    char format_str[64];
    strcpy(format_str, format);
    size_t len = strlen(format_str);

    char time_zone_str[16];
    // while we are not using stdlib for managing the timezone, we have to do this manually
    if (!time_zone) {
        strcpy(time_zone_str, "Z");
    }
    else {
        snprintf(time_zone_str, sizeof(time_zone_str), "%+03d:%02u", time_zone/3600, abs(time_zone/60)%60);
    }

    // replace %z with the timezone
    for (size_t i=0; i<len-1; i++)
    {
        if (format_str[i]=='%' && format_str[i+1]=='z')
        {
            size_t tzlen = strlen(time_zone_str);
            memcpy(format_str+i+tzlen, format_str+i+2, len-i-1);    // +1 include the 0 char
            memcpy(format_str+i, time_zone_str, tzlen);
            len = strlen(format_str);
        }
    }

    char buf[50];
    strftime(buf, 50, format_str, calendar_time);
    return String(buf);
}

bool TimeClass::isValid()
{
    bool rtcstate = HAL_RTC_Time_Is_Valid(nullptr);
    if (rtcstate)
        return rtcstate;
    if (System.mode() == AUTOMATIC && system_thread_get_state(nullptr) == spark::feature::DISABLED)
    {
        waitUntil(Particle.syncTimeDone);
        return HAL_RTC_Time_Is_Valid(nullptr);
    }
    return rtcstate;
}

TimeClass::operator bool() const
{
  return isValid();
}


TimeClass Time;
