/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#ifndef AM18X5_H
#define AM18X5_H

#include "hal_platform.h"

#if HAL_PLATFORM_EXTERNAL_RTC

#include "static_recursive_mutex.h"
#include "concurrent_hal.h"
#include "i2c_hal.h"


namespace particle {

enum class Am18x5Register : uint8_t {
    // Time and Date Registers
    HUNDREDTHS                      = 0x00,
    SECONDS                         = 0x01,
    MINUTES                         = 0x02,
    HOURS                           = 0x03,
    DATE                            = 0x04,
    MONTHS                          = 0x05,
    YEARS                           = 0x06,
    WEEKDAY                         = 0x07,
    // Alarm Registers
    HUNDREDTHS_ALARM                = 0x08,
    SECONDS_ALARM                   = 0x09,
    MINUTES_ALARM                   = 0x0A,
    HOURS_ALARM                     = 0x0B,
    DATE_ALARM                      = 0x0C,
    MONTHS_ALARM                    = 0x0D,
    WEEKDAY_ALARM                   = 0x0E,
    // Configuration Registers
    STATUS                          = 0x0F,
    CONTROL1                        = 0x10,
    CONTROL2                        = 0x11,
    INT_MASK                        = 0x12,
    SQW                             = 0x13,
    // Calibration Registers
    CAL_XT                          = 0x14,
    CAL_RC_HI                       = 0x15,
    CAL_RC_LO                       = 0x16,
    // Sleep Control Register
    SLEEP_CONTROL                   = 0x17,
    // Timer Registers
    TIMER_CONTROL                   = 0x18,
    TIMER                           = 0x19,
    TIMER_INITIAL                   = 0x1A,
    WDT                             = 0x1B,
    // Oscillator Registers
    OSC_CONTROL                     = 0x1C,
    OSC_STATUS                      = 0x1D,
    // Miscellaneous Registers
    CONFIG_KEY                      = 0x1F,
    // Analog Control Registers
    TRICKLE                         = 0x20,
    BREF_CONTROL                    = 0x21,
    AFCTRL                          = 0x26,
    BATMODE_IO                      = 0x27,
    ANALOG_STATUS                   = 0X2F,
    OUTPUT_CTRL                     = 0x30,
    // ID Registers
    ID0                             = 0x28,
    ID1                             = 0x29,
    ID2                             = 0x2A,
    ID3                             = 0x2B,
    ID4                             = 0x2C,
    ID5                             = 0x2D,
    ID6                             = 0x2E,
    // RAM Registers
    EXTENSION_RAM_ADDRESS           = 0x3F
};

enum class HourFormat {
    HOUR24,
    HOUR12_AM,
    HOUR12_PM
};

enum class Weekday {
    MONDAY = 0,
    TUESDAY = 1,
    WEDNESDAY = 2,
    THURSDAY = 3,
    FRIDAY = 4,
    SATURDAY = 5,
    SUNDAY = 6
};

class Am18x5 {
public:
    typedef void (*AlarmHandler)(void* context);
    int begin();
    int end();
    int sync();

    int setHundredths(uint8_t hundredths, bool alarm = false) const;
    int setSeconds(uint8_t seconds, bool alarm = false) const;
    int setMinutes(uint8_t minutes, bool alarm = false) const;
    int setHours(uint8_t hours, HourFormat format, bool alarm = false) const;
    int setDate(uint8_t date, bool alarm = false) const;
    int setMonths(uint8_t months, bool alarm = false) const;
    int setYears(uint8_t years) const;
    int setWeekday(uint8_t weekday, bool alarm = false) const;

    int enableAlarm(bool enable, AlarmHandler handler, void* context);

    int getHundredths() const;
    int getSeconds() const;
    int getMinutes() const;
    int getHours(HourFormat* format) const;
    int getDate() const;
    int getMonths() const;
    int getYears() const;
    int getWeekday() const;

    int getPartNumber(uint16_t* id) const;

    void alarm() const;

    static Am18x5& getInstance();
    static int lock();
    static int unlock();

private:
    const uint8_t INVALID_I2C_ADDRESS = 0x7F;

    Am18x5();
    ~Am18x5();

    int writeRegister(const Am18x5Register reg, uint8_t val, bool bcd = false, bool rw = false, uint8_t mask = 0xFF, uint8_t shift = 0) const;
    int readRegister(const Am18x5Register reg, uint8_t* const val, bool bcd = false, uint8_t mask = 0xFF, uint8_t shift = 0) const;
    static os_thread_return_t exRtcInterruptHandleThread(void* param);

    bool initialized_;
    uint8_t address_;
    HAL_I2C_Interface wire_;
    AlarmHandler alarmHandler_;
    void* alarmHandlerContext_;
    os_thread_t exRtcWorkerThread_;
    os_queue_t exRtcWorkerQueue_;
    bool exRtcWorkerThreadExit_;
    static StaticRecursiveMutex mutex_;
}; // class Am18x5


class Am18x5Lock {
public:
    Am18x5Lock()
            : locked_(false) {
        lock();
    }

    Am18x5Lock(Am18x5Lock&& lock)
            : locked_(lock.locked_) {
        lock.locked_ = false;
    }

    Am18x5Lock(const Am18x5Lock&) = delete;
    Am18x5Lock& operator=(const Am18x5Lock&) = delete;

    ~Am18x5Lock() {
        if (locked_) {
            unlock();
        }
    }

    void lock() {
        Am18x5::lock();
        locked_ = true;
    }

    void unlock() {
        Am18x5::unlock();
        locked_ = false;
    }

private:
    bool locked_;
};

} // namespace particle

#define AM18X5 particle::Am18x5::getInstance()

#endif // HAL_PLATFORM_EXTERNAL_RTC

#endif // AM18X5_H