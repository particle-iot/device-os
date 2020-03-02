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

#include "am18x5.h"

#if HAL_PLATFORM_EXTERNAL_RTC

#include "check.h"
#include "system_error.h"
#include "bcd_to_dec.h"

using namespace particle;

namespace {

// Time and Date Bits Mask
#define SECONDS_MASK                0x7F
#define MINUTES_MASK                0x7F
#define HOURS_12_MASK               0x1F
#define HOURS_AM_PM_MASK            0x20
#define HOURS_24_MASK               0x3F
#define DATE_MASK                   0x3F
#define MONTHS_MASK                 0x1F
#define WEEKDAY_MASK                0x07

// Alarm Bits Mask
#define SECONDS_ALARM_MASK          0x7F
#define MINUTES_ALARM_MASK          0x7F
#define HOURS_12_ALARM_MASK         0x1F
#define HOURS_AM_PM_ALARM_MASK      0x20
#define HOURS_24_ALARM_MASK         0x3F
#define DATE_ALARM_MASK             0x3F
#define MONTHS_ALARM_MASK           0x1F
#define WEEKDAY_ALARM_MASK          0x07

// Status Bit Mask
#define STATUS_CB_MASK              0x80
#define STATUS_BAT_MASK             0x40
#define STATUS_WDT_MASK             0x20
#define STATUS_BL_MASK              0x10
#define STATUS_TIM_MASK             0x08
#define STATUS_ALM_MASK             0x04
#define STATUS_EX2_MASK             0x02
#define STATUS_EX1_MASK             0x01

// Control 1 Bits Mask
#define CONTROL1_STOP_MASK          0x80
#define CONTROL1_1224_MASK          0x40
#define CONTROL1_1224_SHIFT         (6)
#define CONTROL1_OUTB_MASK          0x20
#define CONTROL1_OUT_MASK           0x10
#define CONTROL1_RSP_MASK           0x08
#define CONTROL1_ARST_MASK          0x04
#define CONTROL1_PWR2_MASK          0x02
#define CONTROL1_WRTC_MASK          0x01

// Control 2 Bits Mask
#define CONTROL2_RS1E_MASK          0x20
#define CONTROL2_OUT2S_MASK         0x1C
#define CONTROL2_OUT1S_MASK         0x03

// Interrupt Bits Mask
#define INTERRUPT_CEB_MASK          0x80
#define INTERRUPT_IM_MASK           0x60
#define INTERRUPT_BLIE_MASK         0x10
#define INTERRUPT_TIE_MASK          0x08
#define INTERRUPT_AIE_MASK          0x04
#define INTERRUPT_EX2E_MASK         0x02
#define INTERRUPT_EX1E_MASK         0x01

// SQW Bit Mask
#define SQW_SQWE_MASK               0x80
#define SQW_SQFS_MASK               0x1F

// Calibration Bits Mask
#define CAL_XT_CMDX_MASK            0x80
#define CAL_XT_OFFSETX_MASK         0x7F
#define CAL_RC_HI_CMDR_MASK         0xC0
#define CAL_RC_HI_OFFSETRU_MASK     0x3F

// Sleep Control Bit Mask
#define SLEEP_CONTROL_SLP_MASK      0x80
#define SLEEP_CONTROL_SLRES_MASK    0x40 // When 1, assert nRST low when the Power Control SM is in the SLEEP state.
#define SLEEP_CONTROL_EX2P_MASK     0x20 // When 1, the external interrupt XT2 will trigger on a rising edge of the WDI pin.
#define SLEEP_CONTROL_EX1P_MASK     0x10 // When 1, the external interrupt XT1 will trigger on a rising edge of the EXTI pin.
#define SLEEP_CONTROL_SLST_MASK     0x08 // Set when the AM18X5 enters Sleep Mode
#define SLEEP_CONTROL_SLTO_MASK     0x07 // The number of 7.8 ms periods after SLP is set until the Power Control SM goes into the SLEEP state.

// Countdown Timer Control Bits Mask
#define TIMER_CONTROL_TE_MASK       0x80
#define TIMER_CONTROL_TM_MASK       0x40
#define TIMER_CONTROL_TRPT_MASK     0x20
#define TIMER_CONTROL_RPT_MASK      0x1C
#define TIMER_CONTROL_TFS_MASK      0x03

// WDT Bits Mask
#define WDT_REGISTER_WDS_MASK       0x80
#define WDT_REGISTER_BMB_MASK       0x7C
#define WDT_REGISTER_WRB_MASK       0x03

//outcontrol sleep mode value
// #define CCTRL_SLEEP_MODE_MASK 0xC0 //清除 WDDS,EXDS,RSEN,O4EN,O3EN,O1EN

// OSC Control Bits Mask
#define OSC_CONTROL_OSEL_MASK       0x80
#define OSC_CONTROL_ACAL_MASK       0x60
#define OSC_CONTROL_AOS_MASK        0x10
#define OSC_CONTROL_FOS_MASK        0x08
#define OSC_CONTROL_PWGT_MASK       0x04
#define OSC_CONTROL_OFIE_MASK       0x02
#define OSC_CONTROL_ACIE_MASK       0x01

// OSC Status Bits Mask
#define OSC_STATUS_XTCAL_MASK       0xC0
#define OSC_STATUS_LKO2_MASK        0x20
#define OSC_STATUS_OMODE_MASK       0x10
#define OSC_STATUS_OF_MASK          0x02
#define OSC_STATUS_ACF_MASK         0x01

// Trickle Bits Mask
#define TRICKLE_TCS_MASK            0xF0
#define TRICKLE_DIODE_MASK          0x0C
#define TRICKLE_ROUT_MASK           0x03

// BREF Control Bits Mask
#define BREF_MASK                   0xF0

// Batmode IO Bit Mask
#define BATMODE_IO_MASK             0x80

// Analog Status Bits Mask
#define ANALOG_STATUS_BBOD_MASK     0x80
#define ANALOG_STATUS_BMIN_MASK     0x40
#define ANALOG_STATUS_VINIT_MASK    0x02

// Out Control Bit Mask
#define OUTPUT_CTRL_WDBM_MASK       0x80
#define OUTPUT_CTRL_EXBM_MASK       0x40
#define OUTPUT_CTRL_WDDS_MASK       0x20
#define OUTPUT_CTRL_EXDS_MASK       0x10
#define OUTPUT_CTRL_RSEN_MASK       0x08
#define OUTPUT_CTRL_O4EN_MASK       0x04
#define OUTPUT_CTRL_O3EN_MASK       0x02
#define OUTPUT_CTRL_O1EN_MASK       0x01

} // anonymous namespace

Am18x5::Am18x5()
        : initialized_(false),
          address_(HAL_PLATFORM_EXTERNAL_RTC_I2C_ADDR),
          wire_(HAL_PLATFORM_EXTERNAL_RTC_I2C) {

}

Am18x5::~Am18x5() {

}

Am18x5& Am18x5::getInstance() {
    static Am18x5 am18x5;
    return am18x5;
}

int Am18x5::begin() {
    Am18x5Lock lock();
    CHECK_FALSE(initialized_, SYSTEM_ERROR_NONE);
    CHECK(HAL_I2C_Init(wire_, nullptr));
    HAL_I2C_Set_Speed(wire_, CLOCK_SPEED_400KHZ, nullptr);
    HAL_I2C_Begin(wire_, I2C_MODE_MASTER, 0x00, nullptr);
    return SYSTEM_ERROR_NONE;
}

int Am18x5::end() {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_NONE);
    initialized_ = false;
    return SYSTEM_ERROR_NONE;
}

int Am18x5::lock() {
    return !mutex_.lock();
}

int Am18x5::unlock() {
    return !mutex_.unlock();
}

int Am18x5::getPartNumber(uint16_t* id) const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t val = 0x00;
    CHECK(readRegister(Am18x5Register::ID0, &val));
    *id = ((uint16_t)val) << 8;
    CHECK(readRegister(Am18x5Register::ID1, &val));
    *id |= (uint16_t)val;
    return SYSTEM_ERROR_NONE;
}

int Am18x5::setHundredths(uint8_t hundredths, bool alarm) const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    if (alarm) {
        return writeRegister(Am18x5Register::HUNDREDTHS_ALARM, hundredths, true);
    } else {
        return writeRegister(Am18x5Register::HUNDREDTHS, hundredths, true);
    }
}

int Am18x5::getHundredths() const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t hundredths = 0;
    CHECK(readRegister(Am18x5Register::HUNDREDTHS, &hundredths, true));
    return hundredths;
}

int Am18x5::setSeconds(uint8_t seconds, bool alarm) const {
    Am18x5Lock lock();
    CHECK_TRUE(seconds <= 59, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    if (alarm) {
        return writeRegister(Am18x5Register::SECONDS_ALARM, seconds, true, false, SECONDS_MASK);
    } else {
        return writeRegister(Am18x5Register::SECONDS, seconds, true, false, SECONDS_MASK);
    }
}

int Am18x5::getSeconds() const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t seconds = 0;
    CHECK(readRegister(Am18x5Register::SECONDS, &seconds, true, SECONDS_MASK));
    return seconds;
}

int Am18x5::setMinutes(uint8_t minutes, bool alarm) const {
    Am18x5Lock lock();
    CHECK_TRUE(minutes <= 59, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    if (alarm) {
        return writeRegister(Am18x5Register::MINUTES_ALARM, minutes, true, false, MINUTES_MASK);
    } else {
        return writeRegister(Am18x5Register::MINUTES, minutes, true, false, MINUTES_MASK);
    }
}

int Am18x5::getMinutes() const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t minutes = 0;
    CHECK(readRegister(Am18x5Register::MINUTES, &minutes, true, MINUTES_MASK));
    return minutes;
}

int Am18x5::setHours(uint8_t hours, HourFormat format, bool alarm) const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    if (format == HourFormat::HOUR24) {
        CHECK_TRUE(hours <= 23, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK(writeRegister(Am18x5Register::CONTROL1, 0, false, true, CONTROL1_1224_MASK, CONTROL1_1224_SHIFT));
        if (alarm) {
            return writeRegister(Am18x5Register::HOURS_ALARM, hours, true, false, HOURS_24_MASK);
        } else {
            return writeRegister(Am18x5Register::HOURS, hours, true, false, HOURS_24_MASK);
        }
    } else {
        CHECK_TRUE(hours <= 11, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK(writeRegister(Am18x5Register::CONTROL1, 1, false, true, CONTROL1_1224_MASK, CONTROL1_1224_SHIFT));
        CHECK(hours = decToBcd(hours));
        if (format == HourFormat::HOUR12_PM) {
            hours |= HOURS_AM_PM_MASK;
        }
        if (alarm) {
            return writeRegister(Am18x5Register::HOURS_ALARM, hours, false, false);
        } else {
            return writeRegister(Am18x5Register::HOURS, hours, false, false);
        }
    }
}

int Am18x5::getHours(HourFormat* format) const {
    Am18x5Lock lock();
    CHECK_TRUE(format, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t control1 = 0x00;
    CHECK(readRegister(Am18x5Register::CONTROL1, &control1, false, CONTROL1_1224_MASK));
    uint8_t hours = 0;
    if (control1) {
        // 12 hour mode
        CHECK(readRegister(Am18x5Register::HOURS, &hours, false));
        *format = (hours & HOURS_AM_PM_MASK) ? HourFormat::HOUR12_PM : HourFormat::HOUR12_AM;
        hours &= ~HOURS_AM_PM_MASK;
        CHECK(hours = bcdToDec(hours));
    } else {
        *format = HourFormat::HOUR24;
        CHECK(readRegister(Am18x5Register::HOURS, &hours, true, HOURS_24_MASK));
    }
    return hours;
}

int Am18x5::setDate(uint8_t date, bool alarm) const {
    Am18x5Lock lock();
    CHECK_TRUE(date > 0 && date <= 31, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    if (alarm) {
        return writeRegister(Am18x5Register::DATE_ALARM, date, true, false, DATE_MASK);
    } else {
        return writeRegister(Am18x5Register::DATE, date, true, false, DATE_MASK);
    }
}

int Am18x5::getDate() const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t date = 0;
    CHECK(readRegister(Am18x5Register::DATE, &date, true, DATE_MASK));
    return date;
}

int Am18x5::setMonths(uint8_t months, bool alarm) const {
    Am18x5Lock lock();
    CHECK_TRUE(months > 0 && months <= 12, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    if (alarm) {
        return writeRegister(Am18x5Register::MONTHS_ALARM, months, true, false, MONTHS_MASK);
    } else {
        return writeRegister(Am18x5Register::MONTHS, months, true, false, MONTHS_MASK);
    }
}

int Am18x5::getMonths() const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t months = 0;
    CHECK(readRegister(Am18x5Register::MONTHS, &months, true, MONTHS_MASK));
    return months;
}

int Am18x5::setYears(uint8_t years) const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return writeRegister(Am18x5Register::YEARS, years, true, false);
}

int Am18x5::getYears() const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t years = 0;
    CHECK(readRegister(Am18x5Register::YEARS, &years, true));
    return years;
}

int Am18x5::setWeekday(uint8_t weekday, bool alarm) const {
    Am18x5Lock lock();
    CHECK_TRUE(weekday > 0 && weekday <= 7, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    if (alarm) {
        return writeRegister(Am18x5Register::WEEKDAY_ALARM, weekday, true, false, WEEKDAY_MASK);
    } else {
        return writeRegister(Am18x5Register::WEEKDAY, weekday, true, false, WEEKDAY_MASK);
    }
}

int Am18x5::getWeekday() const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t weekday = 0;
    CHECK(readRegister(Am18x5Register::WEEKDAY, &weekday, true, WEEKDAY_MASK));
    return weekday;
}

int Am18x5::writeRegister(const Am18x5Register reg, uint8_t val, bool bcd, bool rw, uint8_t mask, uint8_t shift) const {
    uint8_t currValue = 0x00;
    if (rw) {
        CHECK(readRegister(reg, &currValue));
    }
    currValue &= ~mask;
    if (bcd) {
        CHECK(val = decToBcd(val));
    }
    val &= mask;
    val <<= shift;
    val |= currValue;
    HAL_I2C_Acquire(wire_, nullptr);
    HAL_I2C_Begin_Transmission(wire_, address_, nullptr);
    HAL_I2C_Write_Data(wire_, static_cast<uint8_t>(reg), nullptr);
    HAL_I2C_Write_Data(wire_, val, nullptr);
    HAL_I2C_End_Transmission(wire_, true, nullptr);
    HAL_I2C_Release(wire_, nullptr);
    return SYSTEM_ERROR_NONE;
}

int Am18x5::readRegister(const Am18x5Register reg, uint8_t* const val, bool bcd, uint8_t mask, uint8_t shift) const {
    HAL_I2C_Acquire(wire_, nullptr);
    HAL_I2C_Begin_Transmission(wire_, address_, nullptr);
    HAL_I2C_Write_Data(wire_, static_cast<uint8_t>(reg), nullptr);
    HAL_I2C_End_Transmission(wire_, false, nullptr);
    if (HAL_I2C_Request_Data(wire_, address_, 1, true, nullptr) == 0) {
        return SYSTEM_ERROR_INTERNAL;
    }
    *val = HAL_I2C_Read_Data(wire_, nullptr);
    HAL_I2C_Release(wire_, nullptr);
    *val &= mask;
    *val >>= shift;
    if (bcd) {
        CHECK(*val = bcdToDec(*val));
    }
    return SYSTEM_ERROR_NONE;
}

StaticRecursiveMutex Am18x5::mutex_;

#endif // HAL_PLATFORM_EXTERNAL_RTC
