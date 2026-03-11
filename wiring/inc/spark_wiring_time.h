/**
 ******************************************************************************
 * @file    spark_wiring_time.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    3-March-2014
 * @brief   Header for spark_wiring_time.cpp module
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

#ifndef __SPARK_WIRING_TIME_H
#define __SPARK_WIRING_TIME_H

#include "spark_wiring_string.h"
#include "time_compat.h"
#include "rtc_hal.h"
#include <time.h>
#include "spark_wiring_platform.h"

extern const char* TIME_FORMAT_DEFAULT;
extern const char* TIME_FORMAT_ISO8601_FULL;

enum class TimeSource : uint8_t {
    DEFAULT = HAL_RTC_SOURCE_DEFAULT,
    INTERNAL = HAL_RTC_SOURCE_INTERNAL,
    EXTERNAL = HAL_RTC_SOURCE_EXTERNAL
};

class TimeClass {
public:
    TimeClass(TimeSource source = TimeSource::DEFAULT);

	// Arduino time and date functions
	int     hour();            			// current hour
	int     hour(time_t t);				// the hour for the given time
	int     hourFormat12();    			// current hour in 12 hour format
	int     hourFormat12(time_t t);		// the hour for the given time in 12 hour format
	uint8_t isAM();            			// returns true if time now is AM
	uint8_t isAM(time_t t);    			// returns true the given time is AM
	uint8_t isPM();            			// returns true if time now is PM
	uint8_t isPM(time_t t);    			// returns true the given time is PM
	int     minute();          			// current minute
	int     minute(time_t t);  			// the minute for the given time
	int     second();          			// current second
	int     second(time_t t);  			// the second for the given time
	int     day();             			// current day
	int     day(time_t t);     			// the day for the given time
	int     weekday();         			// the current weekday
	int     weekday(time_t t); 			// the weekday for the given time
	int     month();           			// current month
	int     month(time_t t);   			// the month for the given time
	int     year();            			// current four digit year
	int     year(time_t t);    			// the year for the given time
	// FIXME: For now using time32_t, until newlib printf %lld/%llu absence is resolved
	// or at least %d/%u crashes with 64-bit arguments
	time32_t  now();              			// return the current time as seconds since Jan 1 1970
	time32_t  local();						// return the time as seconds since Jan 1 1970 in the local timezone.
	void    zone(float GMT_Offset);		// set the time zone (+/-) offset from GMT
	float	   zone();						// retrieve the current timezone
	void    setTime(time_t t);			// set the given time as unix/rtc time

    TimeSource timeSource();             // get RTC time source
    TimeSource getTimeSource() { return timeSource(); }

    operator bool() const;
    bool isValid() const;
    
    /* Retrieve the current DST offset that is added to the current local time when
    * Time.beginDST() has been called.
    * The default is 1 hour.
    */
    float getDSTOffset();
    /* Set a custom DST offset */
    void setDSTOffset(float offset);
    /* Add the offset from getDSTOffset() to the current time */
    void beginDST();
    /* Do not add the offset from getDSTOffset() to the current time */
    void endDST();
    /* Returns true if DST is in effect (beginDST() was called previously) */
    uint8_t isDST();

    /* return string representation of the current time */
    inline String timeStr()
    {
            return timeStr(now());
    }

    /* return string representation for the given time */
    String timeStr(time_t t);

    /**
     * Return a string representation of the given time using strftime().
     * This function takes several kilobytes of flash memory so it's kept separate
     * from `timeStr()` to reduce memory footprint for applications that don't use
     * alternative time formats.
     *
     * @param t
     * @param format_spec
     * @return
     */
    String format(time_t t, const char* format_spec=NULL);

    inline String format(const char* format_spec=NULL)
    {
        return format(now(), format_spec);
    }

    void setFormat(const char* format)
    {
        this->format_spec = format;
    }

    const char* getFormat() const { return format_spec; }

private:
    TimeSource source_;
    const char* format_spec;
    String timeFormatImpl(tm* calendar_time, const char* format, int time_zone);
};

TimeClass& __fetch_global_Time();

#define Time __fetch_global_Time()

#if (HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL)

// Mainly for debug

TimeClass& __fetch_global_InternalTime();
#define InternalTime __fetch_global_InternalTime()

#else

#define InternalTime __fetch_global_Time()

#endif // HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL

#endif
