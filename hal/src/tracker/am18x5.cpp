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

// #define LOG_CHECKED_ERRORS 1

#include <memory>
#include "check.h"
#include "system_error.h"
#include "bcd_to_dec.h"
#include "interrupts_hal.h"
#include "gpio_hal.h"

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
#define CONTROL1_ARST_SHIFT         (2)
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
#define INTERRUPT_AIE_SHIFT         (2)
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
#define TIMER_CONTROL_RPT_SHIFT     (2)
#define TIMER_CONTROL_TFS_MASK      0x03

// WDT Bits Mask
#define WDT_REGISTER_WDS_MASK       0x80
#define WDT_REGISTER_BMB_MASK       0x7C
#define WDT_REGISTER_WRB_MASK       0x03

//outcontrol sleep mode value
// #define CCTRL_SLEEP_MODE_MASK 0xC0 //清除 WDDS,EXDS,RSEN,O4EN,O3EN,O1EN

// OSC Control Bits Mask
#define OSC_CONTROL_OSEL_MASK       0x80
#define OSC_CONTROL_OSEL_SHIFT      (7)
#define OSC_CONTROL_ACAL_MASK       0x60
#define OSC_CONTROL_AOS_MASK        0x10
#define OSC_CONTROL_AOS_SHIFT       (4)
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


void exRtcInterruptHandler(void* data) {
    auto instance = static_cast<Am18x5*>(data);
    instance->sync();
}

} // anonymous namespace

Am18x5::Am18x5()
        : initialized_(false),
          address_(HAL_PLATFORM_EXTERNAL_RTC_I2C_ADDR),
          wire_(HAL_PLATFORM_EXTERNAL_RTC_I2C),
          alarmYear_(0),
          alarmHandler_(nullptr),
          alarmHandlerContext_(nullptr),
          exRtcWorkerThread_(nullptr),
          exRtcWorkerSemaphore_(nullptr),
          exRtcWorkerThreadExit_(false)  {
    begin();
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
    
    if (!HAL_I2C_Is_Enabled(wire_, nullptr)) {
        CHECK(HAL_I2C_Init(wire_, nullptr));
        HAL_I2C_Set_Speed(wire_, CLOCK_SPEED_400KHZ, nullptr);
        HAL_I2C_Begin(wire_, I2C_MODE_MASTER, 0x00, nullptr);
    }

    if (os_semaphore_create(&exRtcWorkerSemaphore_, 1, 0)) {
        exRtcWorkerSemaphore_ = nullptr;
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }
    if (os_thread_create(&exRtcWorkerThread_, "IO Expander Thread", OS_THREAD_PRIORITY_NETWORK, exRtcInterruptHandleThread, this, 512)) {
        os_semaphore_destroy(exRtcWorkerSemaphore_);
        exRtcWorkerSemaphore_ = nullptr;
        LOG(ERROR, "os_thread_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }

    HAL_Pin_Mode(RTC_INT, INPUT_PULLUP);
    HAL_InterruptExtraConfiguration extra = {0};
    extra.version = HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION_1;
    CHECK(HAL_Interrupts_Attach(RTC_INT, exRtcInterruptHandler, this, FALLING, &extra));

    initialized_ = true;

    uint16_t partNumber = 0;
    CHECK(getPartNumber(&partNumber));
    CHECK_TRUE(partNumber == PART_NUMBER, SYSTEM_ERROR_INTERNAL);

    // Automatically switch to internal RC oscillator when using VBAT as the power supply.
    CHECK(enableAutoSwitchOnBattery(true));

    // Automatically clear interrupt flags after reading the the status register.
    CHECK(writeRegister(Am18x5Register::CONTROL1, 1, false, true, CONTROL1_ARST_MASK, CONTROL1_ARST_SHIFT));

    return SYSTEM_ERROR_NONE;
}

int Am18x5::end() {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_NONE);

    exRtcWorkerThreadExit_ = true;
    os_thread_join(exRtcWorkerThread_);
    os_thread_cleanup(exRtcWorkerThread_);
    os_semaphore_destroy(exRtcWorkerSemaphore_);
    exRtcWorkerThreadExit_ = false;
    exRtcWorkerThread_ = nullptr;
    exRtcWorkerSemaphore_ = nullptr;

    initialized_ = false;
    return SYSTEM_ERROR_NONE;
}

int Am18x5::lock() {
    return HAL_I2C_Acquire(wire_, nullptr);
}

int Am18x5::unlock() {
    return HAL_I2C_Release(wire_, nullptr);
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

int Am18x5::setCalendar(const struct tm* calendar) const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(calendar, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(calendar->tm_year >= UNIX_TIME_YEAR_BASE, SYSTEM_ERROR_INVALID_ARGUMENT);
    uint8_t buff[7] = {0};
    CHECK(buff[0] = decToBcd(calendar->tm_sec));
    CHECK(buff[1] = decToBcd(calendar->tm_min));
    CHECK(buff[2] = decToBcd(calendar->tm_hour));
    CHECK(buff[3] = decToBcd(calendar->tm_mday));
    CHECK(buff[4] = decToBcd(calendar->tm_mon + 1)); // Month in tm structure ranges from 0 - 11.
    CHECK(buff[5] = decToBcd(calendar->tm_year - UNIX_TIME_YEAR_BASE));
    CHECK(buff[6] = decToBcd(calendar->tm_wday));
    CHECK(writeContinuouseRegisters(Am18x5Register::SECONDS, buff, sizeof(buff)));
    return SYSTEM_ERROR_NONE;
}

int Am18x5::getCalendar(struct tm* calendar) const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(calendar, SYSTEM_ERROR_INVALID_ARGUMENT);
    uint8_t buff[7] = {0};
    CHECK(readContinuouseRegisters(Am18x5Register::SECONDS, buff, sizeof(buff)));
    CHECK(calendar->tm_sec = bcdToDec(buff[0]));
    CHECK(calendar->tm_min = bcdToDec(buff[1]));
    CHECK(calendar->tm_hour = bcdToDec(buff[2]));
    CHECK(calendar->tm_mday = bcdToDec(buff[3]));
    CHECK(calendar->tm_mon = bcdToDec(buff[4]));
    calendar->tm_mon -= 1;
    CHECK(calendar->tm_year = bcdToDec(buff[5]));
    calendar->tm_year += UNIX_TIME_YEAR_BASE;
    CHECK(calendar->tm_wday = bcdToDec(buff[6]));
    return SYSTEM_ERROR_NONE;
}

int Am18x5::setAlarm(const struct tm* calendar) {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(calendar, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(calendar->tm_year >= UNIX_TIME_YEAR_BASE, SYSTEM_ERROR_INVALID_ARGUMENT);
    uint8_t buff[6] = {0};
    CHECK(buff[0] = decToBcd(calendar->tm_sec));
    CHECK(buff[1] = decToBcd(calendar->tm_min));
    CHECK(buff[2] = decToBcd(calendar->tm_hour));
    CHECK(buff[3] = decToBcd(calendar->tm_mday));
    CHECK(buff[4] = decToBcd(calendar->tm_mon + 1)); // Month in tm structure ranges from 0 - 11.
    alarmYear_ = calendar->tm_year - UNIX_TIME_YEAR_BASE;
    CHECK(buff[5] = decToBcd(calendar->tm_wday));
    CHECK(writeContinuouseRegisters(Am18x5Register::SECONDS_ALARM, buff, sizeof(buff)));
    return SYSTEM_ERROR_NONE;
}

int Am18x5::enableAlarm(bool enable, Am18x5::AlarmHandler handler, void* context) {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    alarmHandler_ = handler;
    alarmHandlerContext_ = context;
    CHECK(writeRegister(Am18x5Register::TIMER_CONTROL, 1, false, true, TIMER_CONTROL_RPT_MASK, TIMER_CONTROL_RPT_SHIFT));
    return writeRegister(Am18x5Register::INT_MASK, enable, false, true, INTERRUPT_AIE_MASK, INTERRUPT_AIE_SHIFT);
}

int Am18x5::setHundredths(uint8_t hundredths) const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return writeRegister(Am18x5Register::HUNDREDTHS, hundredths, true);
}

int Am18x5::getHundredths() const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t hundredths = 0;
    CHECK(readRegister(Am18x5Register::HUNDREDTHS, &hundredths, true));
    return hundredths;
}

int Am18x5::setSeconds(uint8_t seconds) const {
    Am18x5Lock lock();
    CHECK_TRUE(seconds <= 59, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return writeRegister(Am18x5Register::SECONDS, seconds, true, false, SECONDS_MASK);
}

int Am18x5::getSeconds() const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t seconds = 0;
    CHECK(readRegister(Am18x5Register::SECONDS, &seconds, true, SECONDS_MASK));
    return seconds;
}

int Am18x5::setMinutes(uint8_t minutes) const {
    Am18x5Lock lock();
    CHECK_TRUE(minutes <= 59, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return writeRegister(Am18x5Register::MINUTES, minutes, true, false, MINUTES_MASK);
}

int Am18x5::getMinutes() const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t minutes = 0;
    CHECK(readRegister(Am18x5Register::MINUTES, &minutes, true, MINUTES_MASK));
    return minutes;
}

int Am18x5::setHours(uint8_t hours, HourFormat format) const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    if (format == HourFormat::HOUR24) {
        CHECK_TRUE(hours <= 23, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK(writeRegister(Am18x5Register::CONTROL1, 0, false, true, CONTROL1_1224_MASK, CONTROL1_1224_SHIFT));
        return writeRegister(Am18x5Register::HOURS, hours, true, false, HOURS_24_MASK);
    } else {
        CHECK_TRUE(hours <= 11, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK(writeRegister(Am18x5Register::CONTROL1, 1, false, true, CONTROL1_1224_MASK, CONTROL1_1224_SHIFT));
        CHECK(hours = decToBcd(hours));
        if (format == HourFormat::HOUR12_PM) {
            hours |= HOURS_AM_PM_MASK;
        }
        return writeRegister(Am18x5Register::HOURS_ALARM, hours, false, false);
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

int Am18x5::setDate(uint8_t date) const {
    Am18x5Lock lock();
    CHECK_TRUE(date > 0 && date <= 31, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return writeRegister(Am18x5Register::DATE, date, true, false, DATE_MASK);
}

int Am18x5::getDate() const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t date = 0;
    CHECK(readRegister(Am18x5Register::DATE, &date, true, DATE_MASK));
    return date;
}

int Am18x5::setMonths(uint8_t months) const {
    Am18x5Lock lock();
    CHECK_TRUE(months > 0 && months <= 12, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return writeRegister(Am18x5Register::MONTHS, months, true, false, MONTHS_MASK);
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

int Am18x5::setWeekday(uint8_t weekday) const {
    Am18x5Lock lock();
    CHECK_TRUE(weekday > 0 && weekday <= 7, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return writeRegister(Am18x5Register::WEEKDAY, weekday, true, false, WEEKDAY_MASK);
}

int Am18x5::getWeekday() const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t weekday = 0;
    CHECK(readRegister(Am18x5Register::WEEKDAY, &weekday, true, WEEKDAY_MASK));
    return weekday;
}

int Am18x5::selectOscillator(Am18x5Oscillator oscillator) const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t val = 0;
    if (oscillator == Am18x5Oscillator::INTERNAL_RC) {
        val = 1;
    }
    return writeRegister(Am18x5Register::OSC_CONTROL, val, false, true, OSC_CONTROL_OSEL_MASK, OSC_CONTROL_OSEL_SHIFT);
}

int Am18x5::enableAutoSwitchOnBattery(bool enable) const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return writeRegister(Am18x5Register::OSC_CONTROL, enable, false, true, OSC_CONTROL_AOS_MASK, OSC_CONTROL_AOS_SHIFT);
}

int Am18x5::writeRegister(const Am18x5Register reg, uint8_t val, bool bcd, bool rw, uint8_t mask, uint8_t shift) const {
    Am18x5Lock lock();
    uint8_t currValue = 0x00;
    if (rw) {
        CHECK(readRegister(reg, &currValue));
    }
    currValue &= ~mask;
    if (bcd) {
        CHECK(val = decToBcd(val));
    }
    currValue |= (val << shift);
    HAL_I2C_Begin_Transmission(wire_, address_, nullptr);
    HAL_I2C_Write_Data(wire_, static_cast<uint8_t>(reg), nullptr);
    HAL_I2C_Write_Data(wire_, currValue, nullptr);
    HAL_I2C_End_Transmission(wire_, true, nullptr);
    return SYSTEM_ERROR_NONE;
}

int Am18x5::writeContinuouseRegisters(const Am18x5Register start_reg, const uint8_t* buff, size_t len) const {
    Am18x5Lock lock();
    HAL_I2C_Begin_Transmission(wire_, address_, nullptr);
    HAL_I2C_Write_Data(wire_, static_cast<uint8_t>(start_reg), nullptr);
    for (size_t i = 0; i < len; i++) {
        HAL_I2C_Write_Data(wire_, buff[i], nullptr);
    }
    HAL_I2C_End_Transmission(wire_, true, nullptr);
    return len;
}

int Am18x5::readRegister(const Am18x5Register reg, uint8_t* const val, bool bcd, uint8_t mask, uint8_t shift) const {
    Am18x5Lock lock();
    HAL_I2C_Begin_Transmission(wire_, address_, nullptr);
    HAL_I2C_Write_Data(wire_, static_cast<uint8_t>(reg), nullptr);
    HAL_I2C_End_Transmission(wire_, false, nullptr);
    if (HAL_I2C_Request_Data(wire_, address_, 1, true, nullptr) == 0) {
        return SYSTEM_ERROR_INTERNAL;
    }
    *val = HAL_I2C_Read_Data(wire_, nullptr);
    *val &= mask;
    *val >>= shift;
    if (bcd) {
        CHECK(*val = bcdToDec(*val));
    }
    return SYSTEM_ERROR_NONE;
}

int Am18x5::readContinuouseRegisters(const Am18x5Register start_reg, uint8_t* buff, size_t len) const {
    Am18x5Lock lock();
    HAL_I2C_Begin_Transmission(wire_, address_, nullptr);
    HAL_I2C_Write_Data(wire_, static_cast<uint8_t>(start_reg), nullptr);
    HAL_I2C_End_Transmission(wire_, false, nullptr);
    if (HAL_I2C_Request_Data(wire_, address_, len, true, nullptr) == 0) {
        return SYSTEM_ERROR_INTERNAL;
    }
    int32_t size = HAL_I2C_Available_Data(wire_, nullptr);
    if (size <= 0) {
        return size;
    }
    size = std::min((size_t)size, len);
    for (int32_t i = 0; i < size; i++) {
        buff[i] = HAL_I2C_Read_Data(wire_, nullptr);
    }
    return size;
}

int Am18x5::sync() {
    if (exRtcWorkerSemaphore_) {
        os_semaphore_give(exRtcWorkerSemaphore_, false);
    }
    return SYSTEM_ERROR_NONE;
}

os_thread_return_t Am18x5::exRtcInterruptHandleThread(void* param) {
    auto instance = static_cast<Am18x5*>(param);
    while(!instance->exRtcWorkerThreadExit_) {
        os_semaphore_take(instance->exRtcWorkerSemaphore_, CONCURRENT_WAIT_FOREVER, false);
        {
            Am18x5Lock lock();

            uint8_t alm = 0;
            if (instance->readRegister(Am18x5Register::STATUS, &alm, false, STATUS_ALM_MASK) != SYSTEM_ERROR_NONE) {
                continue;
            }
            if (alm) {
                int currYear = instance->getYears();
                if (currYear < 0) {
                    continue;
                }
                if (instance->alarmYear_ == currYear && instance->alarmHandler_) {
                    instance->alarmHandler_(instance->alarmHandlerContext_);
                }
            }
        }
    }
    os_thread_exit(instance->exRtcWorkerThread_);
}

constexpr uint16_t Am18x5::UNIX_TIME_YEAR_BASE;
constexpr uint16_t Am18x5::PART_NUMBER;

#endif // HAL_PLATFORM_EXTERNAL_RTC
