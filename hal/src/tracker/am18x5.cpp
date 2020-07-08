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
#include "am18x5_defines.h"

using namespace particle;

namespace {

void exRtcInterruptHandler(void* data) {
    auto instance = static_cast<Am18x5*>(data);
    instance->sync();
}

const long MICROS_IN_HUNDREDTH = 10000;
const auto UNIX_TIME_YEAR_BASE = 118; // 2018 - 1900

int timevalToCalendar(const struct timeval* tv, struct tm* calendar) {
    CHECK_TRUE(tv, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(gmtime_r(&tv->tv_sec, calendar), SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(calendar->tm_year >= UNIX_TIME_YEAR_BASE, SYSTEM_ERROR_INVALID_ARGUMENT);
    return 0;
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
    
    if (!hal_i2c_is_enabled(wire_, nullptr)) {
        CHECK(hal_i2c_init(wire_, nullptr));
        hal_i2c_set_speed(wire_, CLOCK_SPEED_400KHZ, nullptr);
        hal_i2c_begin(wire_, I2C_MODE_MASTER, 0x00, nullptr);
    }

    if (os_semaphore_create(&exRtcWorkerSemaphore_, 1, 0)) {
        exRtcWorkerSemaphore_ = nullptr;
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }
    if (os_thread_create(&exRtcWorkerThread_, "exrtc", OS_THREAD_PRIORITY_NETWORK, exRtcInterruptHandleThread, this, 512)) {
        os_semaphore_destroy(exRtcWorkerSemaphore_);
        exRtcWorkerSemaphore_ = nullptr;
        LOG(ERROR, "os_thread_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }

    HAL_Pin_Mode(RTC_WDI, OUTPUT);
    HAL_GPIO_Write(RTC_WDI, 1);

    HAL_Pin_Mode(RTC_INT, INPUT_PULLUP);
    HAL_InterruptExtraConfiguration extra = {0};
    extra.version = HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION_1;
    CHECK(HAL_Interrupts_Attach(RTC_INT, exRtcInterruptHandler, this, FALLING, &extra));

    initialized_ = true;

    uint16_t partNumber = 0;
    CHECK(getPartNumber(&partNumber));
    CHECK_TRUE(partNumber == PART_NUMBER, SYSTEM_ERROR_INTERNAL);

    // Automatically switch to internal RC oscillator when using VBAT as the power supply.
    // CHECK(enableAutoSwitchOnBattery(true));

    // Digital calibration to improve accuracy.
    xtOscillatorDigitalCalibration(HAL_PLATFORM_EXTERNAL_RTC_CAL_XT);

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
    return hal_i2c_lock(wire_, nullptr);
}

int Am18x5::unlock() {
    return hal_i2c_unlock(wire_, nullptr);
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

int Am18x5::setTime(const struct timeval* tv) const {
    struct tm calendar;
    CHECK(timevalToCalendar(tv, &calendar));

    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t buff[8] = {0};
    buff[0] = CHECK(decToBcd(tv->tv_usec / MICROS_IN_HUNDREDTH));
    buff[1] = CHECK(decToBcd(calendar.tm_sec));
    buff[2] = CHECK(decToBcd(calendar.tm_min));
    buff[3] = CHECK(decToBcd(calendar.tm_hour));
    buff[4] = CHECK(decToBcd(calendar.tm_mday));
    buff[5] = CHECK(decToBcd(calendar.tm_mon + 1)); // Month in tm structure ranges from 0 - 11.
    buff[6] = CHECK(decToBcd(calendar.tm_year - UNIX_TIME_YEAR_BASE));
    buff[7] = CHECK(decToBcd(calendar.tm_wday));
    CHECK(writeContinuousRegisters(Am18x5Register::HUNDREDTHS, buff, sizeof(buff)));
    return SYSTEM_ERROR_NONE;
}

int Am18x5::getTime(struct timeval* tv) const {
    CHECK_TRUE(tv, SYSTEM_ERROR_INVALID_ARGUMENT);

    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t buff[8] = {0};
    CHECK(readContinuousRegisters(Am18x5Register::HUNDREDTHS, buff, sizeof(buff)));

    struct tm calendar = {};
    calendar.tm_sec = CHECK(bcdToDec(buff[1]));
    calendar.tm_min = CHECK(bcdToDec(buff[2]));
    calendar.tm_hour = CHECK(bcdToDec(buff[3]));
    calendar.tm_mday = CHECK(bcdToDec(buff[4]));
    calendar.tm_mon = CHECK(bcdToDec(buff[5]));
    calendar.tm_mon -= 1;
    calendar.tm_year = CHECK(bcdToDec(buff[6]));
    calendar.tm_year += UNIX_TIME_YEAR_BASE;
    calendar.tm_wday = CHECK(bcdToDec(buff[7]));
    calendar.tm_isdst = -1;

    tv->tv_sec = mktime(&calendar);
    tv->tv_usec = (long)buff[0] * MICROS_IN_HUNDREDTH;
    return SYSTEM_ERROR_NONE;
}

int Am18x5::setAlarm(const struct timeval* tv) {
    struct tm calendar;
    CHECK(timevalToCalendar(tv, &calendar));

    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t buff[7] = {0};
    buff[0] = CHECK(decToBcd(tv->tv_usec / MICROS_IN_HUNDREDTH));
    buff[1] = CHECK(decToBcd(calendar.tm_sec));
    buff[2] = CHECK(decToBcd(calendar.tm_min));
    buff[3] = CHECK(decToBcd(calendar.tm_hour));
    buff[4] = CHECK(decToBcd(calendar.tm_mday));
    buff[5] = CHECK(decToBcd(calendar.tm_mon + 1)); // Month in tm structure ranges from 0 - 11.
    alarmYear_ = calendar.tm_year - UNIX_TIME_YEAR_BASE;
    buff[6] = CHECK(decToBcd(calendar.tm_wday));
    CHECK(writeContinuousRegisters(Am18x5Register::HUNDREDTHS_ALARM, buff, sizeof(buff)));

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

int Am18x5::getAlarm(struct timeval* tv) const {
    if (tv) {
        uint8_t buff[7] = {};
        CHECK(readContinuousRegisters(Am18x5Register::HUNDREDTHS_ALARM, buff, sizeof(buff)));

        struct tm calendar = {};
        calendar.tm_sec = CHECK(bcdToDec(buff[1]));
        calendar.tm_min = CHECK(bcdToDec(buff[2]));
        calendar.tm_hour = CHECK(bcdToDec(buff[3]));
        calendar.tm_mday = CHECK(bcdToDec(buff[4]));
        calendar.tm_mon = CHECK(bcdToDec(buff[5]));
        calendar.tm_mon -= 1;
        // NOTE: alarmYear_ needs to be valid
        calendar.tm_year = alarmYear_ + UNIX_TIME_YEAR_BASE;
        calendar.tm_wday = CHECK(bcdToDec(buff[6]));
        calendar.tm_isdst = -1;

        tv->tv_sec = mktime(&calendar);
        tv->tv_usec = (long)buff[0] * MICROS_IN_HUNDREDTH;
    }
    uint8_t alm;
    CHECK(readRegister(Am18x5Register::STATUS, &alm, false, STATUS_ALM_MASK));
    return (int)alm;
}

int Am18x5::enableWatchdog(uint8_t value, Am18x5WatchdogFrequency frequency) const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(value < 32, SYSTEM_ERROR_INVALID_ARGUMENT);
    uint8_t regValue = WDT_REGISTER_WDS_MASK;
    regValue |= (value << WDT_REGISTER_BMB_SHIFT);
    regValue |= static_cast<uint8_t>(frequency);
    return writeRegister(Am18x5Register::WDT, regValue);
}

int Am18x5::disableWatchdog() const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return writeRegister(Am18x5Register::WDT, 0, false, true, WDT_REGISTER_BMB_MASK, WDT_REGISTER_BMB_SHIFT);
}

int Am18x5::feedWatchdog() const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    if (HAL_GPIO_Read(RTC_WDI) == 1) {
        HAL_GPIO_Write(RTC_WDI, 0);
    } else {
        HAL_GPIO_Write(RTC_WDI, 1);
    }
    return SYSTEM_ERROR_NONE;
}

int Am18x5::sleep(uint8_t ticks, Am18x5TimerFrequency frequency) const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    // Enable to access the BATMODE_IO and OUTPUT_CTRL registers
    CHECK(writeRegister(Am18x5Register::CONFIG_KEY, 0x9D));
    // Configure RTC pins to minimize power leakage
    CHECK(writeRegister(Am18x5Register::BATMODE_IO, 0x00));
    CHECK(writeRegister(Am18x5Register::OUTPUT_CTRL, 0x30));
    // Stop the count down timer just in case
    CHECK(writeRegister(Am18x5Register::TIMER_CONTROL, 0, false, true, TIMER_CONTROL_TE_MASK, TIMER_CONTROL_TE_SHIFT));
    // Set PSW/nIRQ2 to be working in SLEEP mode
    CHECK(writeRegister(Am18x5Register::CONTROL2, 6, false, true, CONTROL2_OUT2S_MASK, CONTROL2_OUT2S_SHIFT));
    // Enable count down timer interrupt
    CHECK(writeRegister(Am18x5Register::INT_MASK, 1, false, true, INTERRUPT_TIE_MASK, INTERRUPT_TIE_SHIFT));
    // Set count down timer's current value
    CHECK(writeRegister(Am18x5Register::TIMER, ticks));
    // Configure and start the count down timer
    uint8_t newValue = 0x00;
    CHECK(readRegister(Am18x5Register::TIMER_CONTROL, &newValue));
    newValue |= TIMER_CONTROL_TE_MASK; // Enable the count down timer
    newValue &= ~TIMER_CONTROL_TRPT_MASK; // Do not repeat
    newValue &= ~TIMER_CONTROL_TFS_MASK;
    newValue |= static_cast<uint8_t>(frequency); // Set the count down timer frequency
    CHECK(writeRegister(Am18x5Register::TIMER_CONTROL, newValue));
    // Transfer to SLEEP state without any delay
    return writeRegister(Am18x5Register::SLEEP_CONTROL, 1, false, true, SLEEP_CONTROL_SLP_MASK, SLEEP_CONTROL_SLP_SHIFT);
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

int Am18x5::xtOscillatorDigitalCalibration(int adjVal) const {
    Am18x5Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t xtcal, cmdx;
    int offsetx;
    if (adjVal < -320 || adjVal >= 128) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (adjVal < -256) {
        xtcal = 3;
        cmdx = 1;
        offsetx = (adjVal + 192) / 2;
    } else if (adjVal < -192) {
        xtcal = 3;
        cmdx = 0;
        offsetx = adjVal + 192;
    } else if (adjVal < -128) {
        xtcal = 2;
        cmdx = 0;
        offsetx = adjVal + 128;
    } else if (adjVal < -64) {
        xtcal = 1;
        cmdx = 0;
        offsetx = adjVal + 64;
    } else if (adjVal < 64) {
        xtcal = 0;
        cmdx = 0;
        offsetx = adjVal;
    } else {
        xtcal = 0;
        cmdx = 1;
        offsetx = adjVal / 2;
    }
    CHECK(writeRegister(Am18x5Register::OSC_STATUS, xtcal, false, true, OSC_STATUS_XTCAL_MASK, OSC_STATUS_XTCAL_SHIFT));
    uint8_t calibration = (uint8_t)offsetx & 0x7F;
    if (cmdx) {
        calibration |= 0x80;
    }
    CHECK(writeRegister(Am18x5Register::CAL_XT, calibration));
    return SYSTEM_ERROR_NONE;
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
    hal_i2c_begin_transmission(wire_, address_, nullptr);
    hal_i2c_write(wire_, static_cast<uint8_t>(reg), nullptr);
    hal_i2c_write(wire_, currValue, nullptr);
    hal_i2c_end_transmission(wire_, true, nullptr);
    return SYSTEM_ERROR_NONE;
}

int Am18x5::writeContinuousRegisters(const Am18x5Register start_reg, const uint8_t* buff, size_t len) const {
    Am18x5Lock lock();
    hal_i2c_begin_transmission(wire_, address_, nullptr);
    hal_i2c_write(wire_, static_cast<uint8_t>(start_reg), nullptr);
    for (size_t i = 0; i < len; i++) {
        hal_i2c_write(wire_, buff[i], nullptr);
    }
    hal_i2c_end_transmission(wire_, true, nullptr);
    return len;
}

int Am18x5::readRegister(const Am18x5Register reg, uint8_t* const val, bool bcd, uint8_t mask, uint8_t shift) const {
    Am18x5Lock lock();
    hal_i2c_begin_transmission(wire_, address_, nullptr);
    hal_i2c_write(wire_, static_cast<uint8_t>(reg), nullptr);
    hal_i2c_end_transmission(wire_, false, nullptr);
    if (hal_i2c_request(wire_, address_, 1, true, nullptr) == 0) {
        return SYSTEM_ERROR_INTERNAL;
    }
    *val = hal_i2c_read(wire_, nullptr);
    *val &= mask;
    *val >>= shift;
    if (bcd) {
        CHECK(*val = bcdToDec(*val));
    }
    return SYSTEM_ERROR_NONE;
}

int Am18x5::readContinuousRegisters(const Am18x5Register start_reg, uint8_t* buff, size_t len) const {
    Am18x5Lock lock();
    hal_i2c_begin_transmission(wire_, address_, nullptr);
    hal_i2c_write(wire_, static_cast<uint8_t>(start_reg), nullptr);
    hal_i2c_end_transmission(wire_, false, nullptr);
    if (hal_i2c_request(wire_, address_, len, true, nullptr) == 0) {
        return SYSTEM_ERROR_INTERNAL;
    }
    int32_t size = hal_i2c_available(wire_, nullptr);
    if (size <= 0) {
        return size;
    }
    size = std::min((size_t)size, len);
    for (int32_t i = 0; i < size; i++) {
        buff[i] = hal_i2c_read(wire_, nullptr);
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

constexpr uint16_t Am18x5::PART_NUMBER;

#endif // HAL_PLATFORM_EXTERNAL_RTC
