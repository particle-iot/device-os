/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "rtc_hal.h"
#include "timer_hal.h"
#include "hal_irq_flag.h"
#include "concurrent_hal.h"
#include "service_debug.h"
#include "hal_platform.h"
#include "check.h"
#include "platform_headers.h"
#include <time.h>

extern "C" {
#include "rtl8721d.h"
}

#if HAL_PLATFORM_EXTERNAL_RTC
#include "exrtc_hal.h"
#endif


namespace {

const time_t UNIX_TIME_20180101000000 = 1514764800UL;  // 2018/01/01 00:00:00
const time_t UNIX_TIME_20000101000000 = 946684800UL;   // 2000/01/01 00:00:00

const int CFG_RTC_PRIORITY = 5;

#if HAL_PLATFORM_EXTERNAL_RTC
class ExternalRtc {
public:
    ExternalRtc() = default;
    ~ExternalRtc() = default;

    int init() {
        return hal_exrtc_init(nullptr);
    }

    int deinit() {
        return SYSTEM_ERROR_NONE;
    }

    int getTime(struct timeval* tv) {
        return hal_exrtc_get_time(tv, nullptr);
    }

    int setTime(const struct timeval* tv) {
        return hal_exrtc_set_time(tv, nullptr);
    }

    int setAlarm(const struct timeval* tv, uint32_t flags, hal_rtc_alarm_handler handler, void* context) {
        return hal_exrtc_set_alarm(tv, flags, handler, context, nullptr);
    }

    int cancelAlarm() {
        return hal_exrtc_cancel_alarm(nullptr);
    }

    bool isTimeValid() {
        return hal_exrtc_time_is_valid(nullptr);
    }
};
#endif

class RealtekRtc {
public:
    RealtekRtc() :
            alarmHandler_(nullptr),
            alarmContext_(nullptr) {
    };
    ~RealtekRtc() = default;

    int init() {
        // 0: 32K from SDM 32.768KHz
        // 1: 32K from XTAL
        RCC_PeriphClockSource_RTC(0);
        RTC_InitTypeDef rtcInitStruct;
        RTC_StructInit(&rtcInitStruct);
        rtcInitStruct.RTC_HourFormat = RTC_HourFormat_24;
        RTC_Init(&rtcInitStruct);
        RTC_BypassShadowCmd(ENABLE);
        return SYSTEM_ERROR_NONE;
    }

    int deinit() {
        return SYSTEM_ERROR_NONE;
    }

    uint64_t usUnixtimeFromTimeval(const struct timeval* tv) {
        return (tv->tv_sec * 1000000ULL + tv->tv_usec);
    }

    int getTime(struct timeval* tv) {
        CHECK_TRUE(tv, SYSTEM_ERROR_INVALID_ARGUMENT);

        struct tm time = {};
        memcpy(&time, &timeInfo_, sizeof(struct tm));

        // hour, min, sec get from RTC
        RTC_TimeTypeDef rtcTimeStruct;
        RTC_GetTime(RTC_Format_BIN, &rtcTimeStruct);
        time.tm_sec = rtcTimeStruct.RTC_Seconds;
        time.tm_min = rtcTimeStruct.RTC_Minutes;
        time.tm_hour = rtcTimeStruct.RTC_Hours;

        // calculate how many days later from last time update timeInfo_
        uint32_t deltaDays = rtcTimeStruct.RTC_Days - time.tm_yday;

        // calculate  wday, mday, yday, mon, year
        time.tm_wday += deltaDays;
        if(time.tm_wday >= 7){
            time.tm_wday = time.tm_wday % 7;
        }

        time.tm_yday += deltaDays;
        time.tm_mday += deltaDays;
        
        while(time.tm_mday > daysInMonth(time.tm_mon, time.tm_year)){
            time.tm_mday -= daysInMonth(time.tm_mon, time.tm_year);
            time.tm_mon++;

            if(time.tm_mon >= 12){
                time.tm_mon -= 12;
                time.tm_yday -= isLeapYear(time.tm_year) ? 366 : 365;
                time.tm_year ++;

                /* over one year, update days in RTC_TR */
                rtcTimeStruct.RTC_Days = time.tm_yday;
                RTC_SetTime(RTC_Format_BIN, &rtcTimeStruct);
            }
        }

        /* update timeInfo_ */
        memcpy((void*)&timeInfo_, (void*)&time, sizeof(struct tm));
        
        /* Convert to timestamp(seconds from 1970.1.1 00:00:00)*/
        tv->tv_sec = mktime(&time);
        tv->tv_usec = 0;
        return SYSTEM_ERROR_NONE;
    }

    int setTime(const struct timeval* tv) {
        CHECK_TRUE(tv, SYSTEM_ERROR_INVALID_ARGUMENT);
        struct tm *timeinfo = localtime(&tv->tv_sec);
        RTC_TimeTypeDef rtcTimeStruct;
        rtcTimeStruct.RTC_H12_PMAM = RTC_H12_AM;
        rtcTimeStruct.RTC_Days = timeinfo->tm_yday;
        rtcTimeStruct.RTC_Hours = timeinfo->tm_hour;
        rtcTimeStruct.RTC_Minutes = timeinfo->tm_min;
        rtcTimeStruct.RTC_Seconds = timeinfo->tm_sec;
        CHECK_TRUE(RTC_SetTime(RTC_Format_BIN, &rtcTimeStruct) == 1, SYSTEM_ERROR_INTERNAL);
        memcpy(&timeInfo_, timeinfo, sizeof(struct tm));
        return SYSTEM_ERROR_NONE;
    }

    int setAlarm(const struct timeval* tv, uint32_t flags, hal_rtc_alarm_handler handler, void* context) {
        CHECK_TRUE(tv, SYSTEM_ERROR_INVALID_ARGUMENT);
        struct timeval alarmTv = *tv;
        struct timeval now;
        CHECK(hal_rtc_get_time(&now, nullptr));
        if (flags & HAL_RTC_ALARM_FLAG_IN) {
            timeradd(&alarmTv, &now, &alarmTv);
        }

        auto unixTimeMs = usUnixtimeFromTimeval(&now) / 1000;
        auto alarmTimeMs = usUnixtimeFromTimeval(&alarmTv) / 1000;
        if (alarmTimeMs <= unixTimeMs) {
            // Too late to set such an alarm
            return SYSTEM_ERROR_TIMEOUT;
        }

        alarmHandler_ = handler;
        alarmContext_ = context;

        struct tm* alarm = localtime(&alarmTv.tv_sec);

        /* set alarm */
        RTC_AlarmTypeDef RTC_AlarmStruct;
        RTC_AlarmStructInit(&RTC_AlarmStruct);
        RTC_AlarmStruct.RTC_AlarmTime.RTC_H12_PMAM = RTC_H12_AM;
        RTC_AlarmStruct.RTC_AlarmTime.RTC_Days = alarm->tm_yday;
        RTC_AlarmStruct.RTC_AlarmTime.RTC_Hours = alarm->tm_hour;
        RTC_AlarmStruct.RTC_AlarmTime.RTC_Minutes = alarm->tm_min;
        RTC_AlarmStruct.RTC_AlarmTime.RTC_Seconds = alarm->tm_sec;
        RTC_AlarmStruct.RTC_AlarmMask = RTC_AlarmMask_None;
        RTC_AlarmStruct.RTC_Alarm2Mask = RTC_Alarm2Mask_None;
        RTC_SetAlarm(RTC_Format_BIN, &RTC_AlarmStruct);
        RTC_AlarmCmd(ENABLE);
        InterruptRegister(rtcAlarmHandler, RTC_IRQ, (uint32_t)this, CFG_RTC_PRIORITY);
        InterruptEn(RTC_IRQ, CFG_RTC_PRIORITY);
        return SYSTEM_ERROR_NONE;
    }

    int cancelAlarm() {
        InterruptDis(RTC_IRQ);
        InterruptUnRegister(RTC_IRQ);
        RTC_AlarmCmd(DISABLE);
        alarmHandler_ = nullptr;
        alarmContext_ = nullptr;

        return SYSTEM_ERROR_NONE;
    }

    bool isTimeValid() {
        struct timeval tv = {};
        getTime(&tv);
        return tv.tv_sec > UNIX_TIME_20180101000000;
    }

    bool isTimeInfoValid() {
        time_t tv_sec = mktime(&timeInfo_);
        return tv_sec >= UNIX_TIME_20000101000000;
    }

    void rtcAlarmHandlerImpl() {
        // Clear alarm flag
        RTC_AlarmClear();

        if (alarmHandler_) {
            (*alarmHandler_)(alarmContext_);
        }

        // Disable RTC alarm and function
        cancelAlarm();
    }

    static uint32_t rtcAlarmHandler(void *context) {
        RealtekRtc* rtc = (RealtekRtc*)context;
        rtc->rtcAlarmHandlerImpl();
        return 0;
    }

    /**
     * @brief  This function tells how many days in a month of a year.
     * @param  year: Specified year
     * @param  month: Specified month
     * @retval value: Number of days in the month.
     */
    static uint8_t daysInMonth(uint8_t month, uint8_t year) {
        static const uint8_t dim[12] = {31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        uint8_t days = dim[month];
        if (days == 0) {
            days = isLeapYear (year) ? 29 : 28;
        }
        return days;
    }

    /**
     * @brief  This function is used to tell a year is a leap year or not.
     * @param  year: The year need to be told.
     * @retval value: 
     *             - true: This year is leap year.
     *             - false: This year is not leap year.
     */
    static bool isLeapYear(uint32_t year) {
        uint32_t fullYear = year + 1900;
        return (!(fullYear % 4) && (fullYear % 100)) || !(fullYear % 400);
    }

    /**
     * @brief  This function is used to calculate month and day of month according to year and day of the year.
     * @param  year: years since 1900.
     * @param  yday: day of the year.
     * @param  mon: pointer to the variable which stores month,  the value can be 0-11
     * @param  mday: pointer to the variable which stores day of month, the value can be 1-31
     * @retval value: none
     */
    static void calculateMonthDay(int year, int yday, int* mon, int* mday) {
        int t_mon = -1, t_yday = yday + 1;

        while (t_yday > 0) {
            t_mon++;
            t_yday -= daysInMonth(t_mon, year);
        }

        *mon = t_mon;
        *mday = t_yday + daysInMonth(t_mon, year);
    }

    /**
     * @brief  This function is used to calculate day of week according to date.
     * @param  year: years since 1900.
     * @param  mon: which month of the year
     * @param  mday: pointer to the variable which store day of month
     * @param  wday: pointer to the variable which store day of week, the value can be 0-6, and 0 means Sunday
     * @retval value: none
     */
    static void calculateWeekDay(int year, int mon, int mday, int* wday) {
        int t_year = year + 1900, t_mon = mon + 1;

        if (t_mon == 1 || t_mon == 2) {
            t_year--;
            t_mon += 12;
        }

        int c = t_year / 100;
        int y = t_year % 100;
        int week = (c / 4) - 2 * c + (y + y / 4) + (26 * (t_mon + 1) / 10) + mday - 1;

        while (week < 0) {
            week += 7;
        }
        week %= 7;

        *wday = week;
    }

private:
    static struct tm timeInfo_;
    hal_rtc_alarm_handler alarmHandler_;
    void* alarmContext_;
};

retained_system struct tm RealtekRtc::timeInfo_ = {};

#if HAL_PLATFORM_EXTERNAL_RTC
ExternalRtc rtcInstance;
#else
RealtekRtc rtcInstance;
#endif

} // anonymous

void hal_rtc_init(void) {
    rtcInstance.init();

    /* Wakes up from the hibernate mode. */
    if (HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_DSLP_INFO_SW) == true) {
#if !HAL_PLATFORM_EXTERNAL_RTC
        if (rtcInstance.isTimeInfoValid())
#endif
        {
            return;
        }
    }

    struct timeval tv = {
        .tv_sec = UNIX_TIME_20000101000000,
        .tv_usec = 0
    };
    rtcInstance.setTime(&tv);
}

bool hal_rtc_time_is_valid(void* reserved) {
    return rtcInstance.isTimeValid();
}

int hal_rtc_get_time(struct timeval* tv, void* reserved) {
    return rtcInstance.getTime(tv);
}

int hal_rtc_set_time(const struct timeval* tv, void* reserved) {
    return rtcInstance.setTime(tv);
}

int hal_rtc_set_alarm(const struct timeval* tv, uint32_t flags, hal_rtc_alarm_handler handler, void* context, void* reserved) {
    return rtcInstance.setAlarm(tv, flags, handler, context);
}

void hal_rtc_cancel_alarm(void) {
    rtcInstance.cancelAlarm();
}

// These are deprecated due to time_t size changes
void hal_rtc_set_unixtime_deprecated(time32_t value) {
    struct timeval tv = {
        .tv_sec = value,
        .tv_usec = 0
    };
    hal_rtc_set_time(&tv, nullptr);
}

time32_t hal_rtc_get_unixtime_deprecated(void) {
    struct timeval tv = {};
    hal_rtc_get_time(&tv, nullptr);
    return (time32_t)tv.tv_sec;
}
