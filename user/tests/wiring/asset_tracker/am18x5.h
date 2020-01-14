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

#ifndef RTC_IMPL_H
#define RTC_IMPL_H

#include "Particle.h"
#include "ext_rtc.h"

#define AM18X5_I2C_ADDRESS                      (0x69)


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

class Am18x5 : public ExtRtcBase {
public:
    int begin(uint8_t address);
    int end();
    int sleep();
    int wakeup();

    int getPartNumber(uint16_t* id);

    static Am18x5& getInstance();

private:
    const uint8_t INVALID_I2C_ADDRESS = 0x7F;

    Am18x5();
    ~Am18x5();

    int writeRegister(const Am18x5Register reg, const uint8_t val);
    int readRegister(const Am18x5Register reg, uint8_t* const val);

    uint8_t address_;
    bool initialized_;
    static RecursiveMutex mutex_;
}; // class Am18x5

#define AM18X5 Am18x5::getInstance()

} // namespace particle

#endif // RTC_IMPL_H