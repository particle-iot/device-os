/*
 * Copyright (c) 2025 Particle Industries, Inc.  All rights reserved.
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
#include "system_cache.h"

#if HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL

// #define LOG_CHECKED_ERRORS 1

#include <memory>
#include "check.h"
#include "system_error.h"
#include "bcd_to_dec.h"
#include "interrupts_hal.h"
#include "gpio_hal.h"
#include "system_cache.h"
#include "scope_guard.h"
#include "core_hal.h"
#include "bytes2hexbuf.h"
#include "am18x5_defines.h"
#include "am18x5.h"

using namespace particle;
using namespace particle::services;

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
          disableI2cOnEnded_(false),
          alarmYear_(0),
          alarmHandler_(nullptr),
          alarmHandlerContext_(nullptr),
          exRtcWorkerThread_(nullptr),
          exRtcWorkerSemaphore_(nullptr),
          exRtcWorkerThreadExit_(false)  {
    config_ = {};
    config_.version = HAL_AM18X5_CONFIG_VERSION;
    config_.size = sizeof(hal_am18x5_config_t);
}

Am18x5::~Am18x5() {

}

Am18x5& Am18x5::getInstance() {
    static Am18x5 am18x5;
    return am18x5;
}

int Am18x5::setConfig(const hal_am18x5_config_t* config) {
    CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);
    if (config->size == 0 || config->i2c_if >= HAL_PLATFORM_I2C_NUM) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    hal_am18x5_config_t tempData = {};
    tempData.size = sizeof(hal_am18x5_config_t);
    tempData.version = HAL_AM18X5_CONFIG_VERSION;
    int result = SystemCache::instance().get(SystemCacheKey::EXRTC_CONFIG_DATA, (uint8_t*)&tempData, sizeof(tempData));
    if (result != std::min(tempData.size, config->size) ||
            memcmp(&tempData, config, std::min(tempData.size, config->size)) != 0) {
        result = SystemCache::instance().set(SystemCacheKey::EXRTC_CONFIG_DATA, (uint8_t*)config, config->size);
        if (result < 0) {
            return result;
        }
#if HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
        if (config->default_rtc && !HAL_Feature_Get(FEATURE_EXRTC_DETECTION)) {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
#endif
        if (config->default_rtc || initialized_) {
            CHECK(begin());
        }
    }
    return SYSTEM_ERROR_NONE;
}

int Am18x5::getConfig(hal_am18x5_config_t* config) {
    CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);
    hal_am18x5_config_t tempData = {};
    tempData.size = sizeof(hal_am18x5_config_t);
    tempData.version = HAL_AM18X5_CONFIG_VERSION;
    int result = SystemCache::instance().get(SystemCacheKey::EXRTC_CONFIG_DATA, (uint8_t*)&tempData, sizeof(tempData));
    if (result != std::min(tempData.size, config->size)) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    memcpy(config, &tempData, std::min(tempData.size, config->size));
    return SYSTEM_ERROR_NONE;
}

int Am18x5::detect() {
    Am18x5Lock lock;
    uint16_t partNumber = 0;
    CHECK(readContinuousRegisters(Am18x5Register::ID0, ids_, AM18X5_ID_COUNT));
    partNumber = ((uint16_t)ids_[0]) << 8;
    partNumber |= (uint16_t)ids_[1];
    CHECK_TRUE(partNumber == AM1805_PART_NUMBER, SYSTEM_ERROR_NOT_FOUND);
    return SYSTEM_ERROR_NONE;
}

bool Am18x5::isDefault() const {
    return (initialized_ && config_.default_rtc);
}

int Am18x5::begin() {
    Am18x5Lock lock;
    int ret = SYSTEM_ERROR_INTERNAL;
    SCOPE_GUARD ({
        if (ret != SYSTEM_ERROR_NONE) {
            end();
        }
    });
    CHECK(getConfig(&config_));
    if (!initialized_) {
        if (!hal_i2c_is_enabled(config_.i2c_if, nullptr)) {
            CHECK(hal_i2c_init(config_.i2c_if, nullptr));
            hal_i2c_begin(config_.i2c_if, I2C_MODE_MASTER, 0x00, nullptr);
            // Make sure to reset the I2C bus to avoid potentially corrupting the AM18x5 configuration if
            // we start communication with it during an ongoing write transaction (which may happen e.g. after a hard reset)
            hal_i2c_reset(config_.i2c_if, 0, nullptr);
            disableI2cOnEnded_ = true;
        }
        Am18x5Lock lock;
        // NOTE: acquire lock only after initializing the I2C peripheral, as this will actually call into
        // hal_i2c_lock/hal_i2c_unlock, which do not function unless hal_i2c_init is called.
        CHECK(detect());
        if (config_.wdi_pin != PIN_INVALID) {
            hal_gpio_mode(config_.wdi_pin, OUTPUT);
            hal_gpio_write(config_.wdi_pin, 1);
        }
        if (config_.int_pin != PIN_INVALID) {
            hal_gpio_mode(config_.int_pin, INPUT_PULLUP);
            hal_interrupt_extra_configuration_t extra = {};
            extra.version = HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION_1;
            CHECK(hal_interrupt_attach(config_.int_pin, exRtcInterruptHandler, this, FALLING, &extra));
            // The semaphore must be created before starting the thread
            if (os_semaphore_create(&exRtcWorkerSemaphore_, 1, 0)) {
                LOG(ERROR, "os_semaphore_create() failed");
                return ret = SYSTEM_ERROR_INTERNAL;
            }
            if (os_thread_create(&exRtcWorkerThread_, "exrtc", OS_THREAD_PRIORITY_NETWORK, exRtcInterruptHandleThread, this, 1536)) {
                LOG(ERROR, "os_thread_create() failed");
                return ret = SYSTEM_ERROR_INTERNAL;
            }
        }
        initialized_ = true;
    }
    return ret = applyConfig();
}

int Am18x5::end() {
    Am18x5Lock lock;
    int ret = SYSTEM_ERROR_NONE;
    SCOPE_GUARD ({
        initialized_ = false;
    });
    if (config_.wdi_pin != PIN_INVALID) {
        hal_gpio_mode(config_.wdi_pin, INPUT);
    }
    if (config_.int_pin != PIN_INVALID) {
        exRtcWorkerThreadExit_ = true;
        if (exRtcWorkerThread_) {
            os_semaphore_give(exRtcWorkerSemaphore_, false);
            os_thread_join(exRtcWorkerThread_);
            os_thread_cleanup(exRtcWorkerThread_);
        }
        if (exRtcWorkerSemaphore_) {
            os_semaphore_destroy(exRtcWorkerSemaphore_);
        }
        exRtcWorkerThreadExit_ = false;
        exRtcWorkerThread_ = nullptr;
        exRtcWorkerSemaphore_ = nullptr;
        hal_interrupt_detach(config_.int_pin);
        hal_gpio_mode(config_.int_pin, INPUT);
    }
    if (initialized_) {
        ret = reset();
    }
    if (disableI2cOnEnded_) {
        hal_i2c_end(config_.i2c_if, nullptr);
        disableI2cOnEnded_ = false;
    }
    return ret;
}

int Am18x5::reset() {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return writeRegister(Am18x5Register::CONFIG_KEY, CONFIG_KEY_SOFTWARE_RESET);
}

int Am18x5::applyConfig() {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_NONE);

    uint8_t oscControl = 0x00;
    CHECK(readRegister(Am18x5Register::OSC_CONTROL, &oscControl));
    if (config_.osc_src == Am18x5Oscillator::INTERNAL_RC) {
        oscControl |= OSC_CONTROL_OSEL_MASK;
    } else {
        oscControl &= ~OSC_CONTROL_OSEL_MASK;
    }
    oscControl &= ~OSC_CONTROL_ACAL_MASK;
    oscControl |= (static_cast<uint8_t>(config_.auto_calibration) << OSC_CONTROL_ACAL_SHIFT);
    if (config_.rc_on_battery) {
        oscControl |= OSC_CONTROL_AOS_MASK;
    } else {
        oscControl &= ~OSC_CONTROL_AOS_MASK;
    }
    if (config_.rc_fallback) {
        oscControl |= OSC_CONTROL_FOS_MASK;
    } else {
        oscControl &= ~OSC_CONTROL_FOS_MASK;
    }
    CHECK(writeRegister(Am18x5Register::CONFIG_KEY, CONFIG_KEY_OSC_CONTROL));
    CHECK(writeRegister(Am18x5Register::OSC_CONTROL, oscControl));

    // Digital calibration to improve accuracy.
    xtOscillatorDigitalCalibration(config_.osc_cal_xt);

    // Enable square wave output on the CLKOUT pin
    if (config_.clk_out_en) {
        CHECK(enableClkOut(config_.clk_out_freq));
    } else {
        CHECK(disableClkOut());
    }

    // Automatically clear interrupt flags after reading the the status register.
    CHECK(writeRegister(Am18x5Register::CONTROL1, 1, false, true, CONTROL1_ARST_MASK, CONTROL1_ARST_SHIFT));
    return SYSTEM_ERROR_NONE;
}

int Am18x5::lock() {
    return hal_i2c_lock(config_.i2c_if, nullptr);
}

int Am18x5::unlock() {
    return hal_i2c_unlock(config_.i2c_if, nullptr);
}

int Am18x5::getIdString(char* id, size_t len) const {
    Am18x5Lock lock;
    CHECK_TRUE(id && (len >= HAL_EXRTC_ID_STR_LEN), SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    bytes2hexbuf(ids_, 2, id); // Part Number in BCD format
    id += 4;
    uint8_t major = ((ids_[2] >> 3) & 0x1F);
    bytes2hexbuf(&major, 1, id); // Revision major
    id += 2;
    uint8_t minor = (ids_[2] & 0x07);
    bytes2hexbuf(&minor, 1, id); // Revision minor
    id += 2;
    uint8_t mfgLotHi = ((ids_[4] & 0x80) ? 0x02 : 0x00); // Lot[9]
    mfgLotHi |= ((ids_[6] & 0x80) ? 0x01 : mfgLotHi); // Lot[8]
    bytes2hexbuf(&mfgLotHi, 1, id);
    id += 2;
    uint8_t mfgLotLo = ids_[3]; // Lot[7:0]
    bytes2hexbuf(&mfgLotLo, 1, id);
    id += 2;
    uint8_t mfgWafer = ((ids_[6] >> 2) & 0x1F); // Wafer[4:0]
    bytes2hexbuf(&mfgWafer, 1, id);
    id += 2;
    uint8_t idHi = (ids_[4] & 0x7F); // ID[14:8]
    bytes2hexbuf(&idHi, 1, id);
    id += 2;
    uint8_t idLo = ids_[5]; // ID[7:0]
    bytes2hexbuf(&idLo, 1, id);
    return SYSTEM_ERROR_NONE;
}

int Am18x5::setTime(const struct timeval* tv) const {
    struct tm calendar;
    CHECK(timevalToCalendar(tv, &calendar));

    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t buff[8] = {0};
    auto tv_usec = tv->tv_usec;
    if (tv_usec > 99) {
        // In most cases, tv_usec is not set in user app, which leaves it being an invalid value.
        tv_usec = 0;
    }
    buff[0] = CHECK(decToBcd(tv_usec / MICROS_IN_HUNDREDTH));
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

    Am18x5Lock lock;
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

bool Am18x5::isTimeValid(struct timeval* tv) const {
    struct timeval tempTv = {};
    if (getTime(&tempTv) == SYSTEM_ERROR_NONE) {
        if (tempTv.tv_sec > UNIX_TIME_20180101000000) {
            if (tv) {
                *tv = tempTv;
            }
            return true;
        }
    }
    return false;
}

int Am18x5::setAlarm(bool enable, uint32_t flags, const struct timeval* tv, AlarmHandler handler, void* context) {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    if (enable) {
        if (tv) {
            struct timeval alarm = *tv;
            struct timeval now;
            CHECK(getTime(&now));
            if (flags & HAL_RTC_ALARM_FLAG_IN) {
                timeradd(tv, &now, &alarm);
            }

            if (alarm.tv_sec <= now.tv_sec) {
                // Too late to set such an alarm
                return SYSTEM_ERROR_TIMEOUT;
            }

            struct tm calendar;
            CHECK(timevalToCalendar(&alarm, &calendar));
            uint8_t buff[7] = {0};
            if (alarm.tv_usec > 99) {
                // In most cases, tv_usec is not set in user app, which leaves it being an invalid value.
                alarm.tv_usec = 0;
            }
            buff[0] = CHECK(decToBcd(alarm.tv_usec / MICROS_IN_HUNDREDTH));
            buff[1] = CHECK(decToBcd(calendar.tm_sec));
            buff[2] = CHECK(decToBcd(calendar.tm_min));
            buff[3] = CHECK(decToBcd(calendar.tm_hour));
            buff[4] = CHECK(decToBcd(calendar.tm_mday));
            buff[5] = CHECK(decToBcd(calendar.tm_mon + 1)); // Month in tm structure ranges from 0 - 11.
            alarmYear_ = calendar.tm_year - UNIX_TIME_YEAR_BASE;
            buff[6] = CHECK(decToBcd(calendar.tm_wday));
            CHECK(writeContinuousRegisters(Am18x5Register::HUNDREDTHS_ALARM, buff, sizeof(buff)));
        }
        alarmHandler_ = handler;
        alarmHandlerContext_ = context;
        // Hundredths, seconds, minutes, hours, date and month match (once per year)
        // TODO: allow configuring repeat mode
        CHECK(writeRegister(Am18x5Register::TIMER_CONTROL, 1, false, true, TIMER_CONTROL_RPT_MASK, TIMER_CONTROL_RPT_SHIFT));
    } else {
        CHECK(writeRegister(Am18x5Register::TIMER_CONTROL, 0, false, true, TIMER_CONTROL_RPT_MASK, TIMER_CONTROL_RPT_SHIFT));
    }
    return writeRegister(Am18x5Register::INT_MASK, enable, false, true, INTERRUPT_AIE_MASK, INTERRUPT_AIE_SHIFT);
}

int Am18x5::getAlarm(struct timeval* tv) const {
    Am18x5Lock lock;
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
    CHECK(readRegister(Am18x5Register::TIMER_CONTROL, &alm, false, TIMER_CONTROL_RPT_MASK, TIMER_CONTROL_RPT_SHIFT));
    return alm;
}

int Am18x5::enableWatchdog(system_tick_t ms) {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t value; // Maximum 31.
    Am18x5WatchdogFrequency frequency;
    if (ms < 1937) { // 31 * 1000 / 16
        frequency = Am18x5WatchdogFrequency::HZ_16;
        value = ms * 16 / 1000;
    } else if (ms <= 7750) {
        frequency = Am18x5WatchdogFrequency::HZ_4;
        value = ms * 4 / 1000;
    } else if (ms <= 31000) {
        frequency = Am18x5WatchdogFrequency::HZ_1;
        value = ms / 1000;
    } else if (ms <= 124000) {
        frequency = Am18x5WatchdogFrequency::HZ_1_4;
        value = ms / 4 / 1000;
    } else {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    CHECK_TRUE(value > 0 && value < 32, SYSTEM_ERROR_INVALID_ARGUMENT);
    uint8_t regValue = WDT_REGISTER_WDS_MASK; // Generate a reset when it times out
    regValue |= (value << WDT_REGISTER_BMB_SHIFT);
    regValue |= static_cast<uint8_t>(frequency);
    watchdogValue_ = regValue;
    return writeRegister(Am18x5Register::WDT, regValue);
}

int Am18x5::disableWatchdog() const {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return writeRegister(Am18x5Register::WDT, 0, false, true, WDT_REGISTER_BMB_MASK, WDT_REGISTER_BMB_SHIFT);
}

int Am18x5::feedWatchdog() const {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);

    if (config_.wdi_pin == PIN_INVALID) {
        return writeRegister(Am18x5Register::WDT, watchdogValue_);
    }
    if (hal_gpio_read(config_.wdi_pin) == 1) {
        hal_gpio_write(config_.wdi_pin, 0);
    } else {
        hal_gpio_write(config_.wdi_pin, 1);
    }
    return SYSTEM_ERROR_NONE;
}

void Am18x5::getWatchdogLimits(system_tick_t* low, system_tick_t* high) const {
    if (low) {
        *low = 63; // round(Am18x5WatchdogFrequency::HZ_16 * 1)
    }
    if (high) {
        *high = 124000; // // round(Am18x5WatchdogFrequency::HZ_1_4 * 31)
    }
}

bool Am18x5::isWatchdogStarted() const {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, false);
    uint8_t bmb = 0;
    readRegister(Am18x5Register::WDT, &bmb, false, WDT_REGISTER_BMB_MASK, WDT_REGISTER_BMB_SHIFT);
    return (bmb != 0);
}

int Am18x5::enableClkOut(Am18x5SqwFrequency freq) {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    if (to_underlying(freq) > 0x1F) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    CHECK(writeRegister(Am18x5Register::SQW, to_underlying(freq), false, true, SQW_SQFS_MASK, SQW_SQFS_SHIFT));
    return writeRegister(Am18x5Register::SQW, 1, false, true, SQW_SQWE_MASK, SQW_SQWE_SHIFT);
}

int Am18x5::disableClkOut() {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return writeRegister(Am18x5Register::SQW, 0, false, true, SQW_SQWE_MASK, SQW_SQWE_SHIFT);
}

int Am18x5::setPsw(bool val) const {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK(writeRegister(Am18x5Register::OSC_STATUS, 0, false, true, OSC_STATUS_LKO2_MASK, OSC_STATUS_LKO2_SHIFT));
    CHECK(writeRegister(Am18x5Register::CONTROL1, val, false, true, CONTROL1_OUTB_MASK, CONTROL1_OUTB_SHIFT));
    LOG(TRACE, "PSW set to %d", val);
    return SYSTEM_ERROR_NONE;
}

int Am18x5::sleep(const hal_am18x5_sleep_config_t* config) {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);
    if ((config->exti_polarity == Am18x5ExtiPolarity::NONE) && (config->duration == 0)) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if ((config->exti_polarity == Am18x5ExtiPolarity::NONE) && (config->duration > 0)) {
        // Only the Alarm can wake up the system
        uint8_t oscControl = 0x00;
        CHECK(readRegister(Am18x5Register::OSC_CONTROL, &oscControl));
        if (!(oscControl & OSC_CONTROL_OSEL_MASK) && !(oscControl & OSC_CONTROL_FOS_MASK)) {
            // If we're using XTAL and RC fallback is not enabled, we will end up in infinite sleep if OSC failure occurs.
            return SYSTEM_ERROR_INVALID_STATE;
        }
    }

    // CONFIG_KEY_PRIMARY enables access to BATMODE_IO and OUTPUT_CTRL registers
    CHECK(writeRegister(Am18x5Register::CONFIG_KEY, CONFIG_KEY_PRIMARY));
    // If 1, the AM18X5 will not disable the I/O interface even if VCC goes away and VBAT is still present.
    // This allows external access while the AM18X5 is powered by VBAT.
    CHECK(writeRegister(Am18x5Register::BATMODE_IO, 0x00));

    uint8_t outputCtrl = 0x00;
    uint8_t intMask = 0xE0;

    // 1, the WDI input is disabled when the AM18X5 is in Sleep Mode
    outputCtrl |= OUTPUT_CTRL_WDDS_MASK;

    if (config->exti_polarity != Am18x5ExtiPolarity::NONE) {
        // 1, the EXTI input is enabled when the AM18X5 is powered from VBAT
        outputCtrl |= OUTPUT_CTRL_EXBM_MASK;
        // 0, the EXTI input is enabled when the AM18X5 is in Sleep Mode
        outputCtrl &= ~OUTPUT_CTRL_EXDS_MASK;
        intMask |= INTERRUPT_EX1E_MASK;
        if (config->exti_polarity == Am18x5ExtiPolarity::RISING) {
            CHECK(writeRegister(Am18x5Register::SLEEP_CONTROL, 1, false, true, SLEEP_CONTROL_EX1P_MASK, SLEEP_CONTROL_EX1P_SHIFT));
        } else {
            CHECK(writeRegister(Am18x5Register::SLEEP_CONTROL, 0, false, true, SLEEP_CONTROL_EX1P_MASK, SLEEP_CONTROL_EX1P_SHIFT));
        }
    } else {
        // 1, the EXTI input is disabled when the AM18X5 is in Sleep Mode
        outputCtrl |= OUTPUT_CTRL_EXDS_MASK;
        intMask &= ~INTERRUPT_EX1E_MASK;
    }

    // CONFIG_KEY resets on each write, redo for OUTPUT_CTRL
    CHECK(writeRegister(Am18x5Register::CONFIG_KEY, CONFIG_KEY_PRIMARY));
    // Configure RTC pins to minimize power leakage
    CHECK(writeRegister(Am18x5Register::OUTPUT_CTRL, outputCtrl));

    // Set PSW/nIRQ2 to be working in SLEEP mode
    CHECK(writeRegister(Am18x5Register::CONTROL2, 6, false, true, CONTROL2_OUT2S_MASK, CONTROL2_OUT2S_SHIFT));
    // When 1, the I/O interface will be disabled when the power switch is active and disabled 
    CHECK(writeRegister(Am18x5Register::CONFIG_KEY, CONFIG_KEY_OSC_CONTROL));
    CHECK(writeRegister(Am18x5Register::OSC_CONTROL, 1, false, true, OSC_CONTROL_PWGT_MASK, OSC_CONTROL_PWGT_SHIFT));

    bool watchdogWasStarted = isWatchdogStarted();
    if (watchdogWasStarted) {
        CHECK(disableWatchdog());
    }

    struct timeval alarmTv = {};
    int enabled = CHECK(getAlarm(&alarmTv));

    // This function should not return if succeeded.
    SCOPE_GUARD ({
        // Restore alarm config
        setAlarm(enabled > 0 ? true : false, 0, &alarmTv, alarmHandler_, alarmHandlerContext_);
        // Disable EXTI interrupt, as it is only used for sleep wakeup for now
        intMask &= ~INTERRUPT_EX1E_MASK;
        writeRegister(Am18x5Register::INT_MASK, intMask);
        if (watchdogWasStarted) {
            writeRegister(Am18x5Register::WDT, watchdogValue_);
        }
    });

    if (config->duration > 0) {
        struct timeval tv = {
            .tv_sec = config->duration,
            .tv_usec = 0
        };
        CHECK(setAlarm(true, HAL_RTC_ALARM_FLAG_IN, &tv, nullptr, nullptr));
        intMask |= INTERRUPT_AIE_MASK;
    } else {
        intMask &= ~INTERRUPT_AIE_MASK;
    }

    // Read status to clear the interrupt flags
    uint8_t status = 0x00;
    CHECK(readRegister(Am18x5Register::STATUS, &status));
    // Enable interrupt
    CHECK(writeRegister(Am18x5Register::INT_MASK, intMask));

    if (config->exti_trigger_latched && config->exti_polarity != Am18x5ExtiPolarity::NONE) {
        uint8_t exin;
        CHECK(readRegister(Am18x5Register::EXTENSION_RAM_ADDRESS, &exin));
        bool isExtiHigh = exin & EXTENSION_RAM_EXIN_MASK;
        if((config->exti_polarity == Am18x5ExtiPolarity::RISING && isExtiHigh) ||
            (config->exti_polarity == Am18x5ExtiPolarity::FALLING && !isExtiHigh)) {
            return SYSTEM_ERROR_ABORTED;
        }
    }

    // Transfer to SLEEP state without any delay
    CHECK(writeRegister(Am18x5Register::SLEEP_CONTROL, 1, false, true, SLEEP_CONTROL_SLP_MASK, SLEEP_CONTROL_SLP_SHIFT));
    /*
     * In addition, SLP cannot be set if there is an interrupt pending. Software should read the SLP bit after
     * attempting to set it. If SLP is not asserted, the attempt to set SLP was unsuccessful either because a
     * correct trigger was not enabled or because an interrupt was already pending. Once SLP is set, software
     * should continue to poll it until the Sleep actually occurs, in order to handle the case where a trigger occurs
     * before the AM18X5AB18XX enters Sleep Mode.
     */
    uint8_t isSlpSet = 0;
    CHECK(readRegister(Am18x5Register::SLEEP_CONTROL, &isSlpSet, false, SLEEP_CONTROL_SLP_MASK, SLEEP_CONTROL_SLP_SHIFT));
    if (!isSlpSet) {
        return SYSTEM_ERROR_ABORTED;
    }
    do {
        CHECK(readRegister(Am18x5Register::SLEEP_CONTROL, &isSlpSet, false, SLEEP_CONTROL_SLP_MASK, SLEEP_CONTROL_SLP_SHIFT));
    } while (isSlpSet);

    return SYSTEM_ERROR_ABORTED;
}

int Am18x5::setHundredths(uint8_t hundredths) const {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return writeRegister(Am18x5Register::HUNDREDTHS, hundredths, true);
}

int Am18x5::getHundredths() const {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t hundredths = 0;
    CHECK(readRegister(Am18x5Register::HUNDREDTHS, &hundredths, true));
    return hundredths;
}

int Am18x5::setSeconds(uint8_t seconds) const {
    Am18x5Lock lock;
    CHECK_TRUE(seconds <= 59, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return writeRegister(Am18x5Register::SECONDS, seconds, true, false, SECONDS_MASK);
}

int Am18x5::getSeconds() const {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t seconds = 0;
    CHECK(readRegister(Am18x5Register::SECONDS, &seconds, true, SECONDS_MASK));
    return seconds;
}

int Am18x5::setMinutes(uint8_t minutes) const {
    Am18x5Lock lock;
    CHECK_TRUE(minutes <= 59, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return writeRegister(Am18x5Register::MINUTES, minutes, true, false, MINUTES_MASK);
}

int Am18x5::getMinutes() const {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t minutes = 0;
    CHECK(readRegister(Am18x5Register::MINUTES, &minutes, true, MINUTES_MASK));
    return minutes;
}

int Am18x5::setHours(uint8_t hours, HourFormat format) const {
    Am18x5Lock lock;
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
        return writeRegister(Am18x5Register::HOURS, hours, false, false);
    }
}

int Am18x5::getHours(HourFormat* format) const {
    Am18x5Lock lock;
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
    Am18x5Lock lock;
    CHECK_TRUE(date > 0 && date <= 31, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return writeRegister(Am18x5Register::DATE, date, true, false, DATE_MASK);
}

int Am18x5::getDate() const {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t date = 0;
    CHECK(readRegister(Am18x5Register::DATE, &date, true, DATE_MASK));
    return date;
}

int Am18x5::setMonths(uint8_t months) const {
    Am18x5Lock lock;
    CHECK_TRUE(months > 0 && months <= 12, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return writeRegister(Am18x5Register::MONTHS, months, true, false, MONTHS_MASK);
}

int Am18x5::getMonths() const {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t months = 0;
    CHECK(readRegister(Am18x5Register::MONTHS, &months, true, MONTHS_MASK));
    return months;
}

int Am18x5::setYears(uint8_t years) const {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return writeRegister(Am18x5Register::YEARS, years, true, false);
}

int Am18x5::getYears() const {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t years = 0;
    CHECK(readRegister(Am18x5Register::YEARS, &years, true));
    return years;
}

int Am18x5::setWeekday(uint8_t weekday) const {
    Am18x5Lock lock;
    CHECK_TRUE(weekday > 0 && weekday <= 7, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    return writeRegister(Am18x5Register::WEEKDAY, weekday, true, false, WEEKDAY_MASK);
}

int Am18x5::getWeekday() const {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t weekday = 0;
    CHECK(readRegister(Am18x5Register::WEEKDAY, &weekday, true, WEEKDAY_MASK));
    return weekday;
}

int Am18x5::xtOscillatorDigitalCalibration(int adjVal) const {
    Am18x5Lock lock;
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
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK(writeRegister(Am18x5Register::CONFIG_KEY, CONFIG_KEY_OSC_CONTROL));
    uint8_t val = 0;
    if (oscillator == Am18x5Oscillator::INTERNAL_RC) {
        val = 1;
    }
    return writeRegister(Am18x5Register::OSC_CONTROL, val, false, true, OSC_CONTROL_OSEL_MASK, OSC_CONTROL_OSEL_SHIFT);
}

int Am18x5::enableAutoSwitchOnBattery(bool enable) const {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK(writeRegister(Am18x5Register::CONFIG_KEY, CONFIG_KEY_OSC_CONTROL));
    return writeRegister(Am18x5Register::OSC_CONTROL, enable, false, true, OSC_CONTROL_AOS_MASK, OSC_CONTROL_AOS_SHIFT);
}

int Am18x5::writeRegister(const Am18x5Register reg, uint8_t val, bool bcd, bool rw, uint8_t mask, uint8_t shift) const {
    Am18x5Lock lock;
    uint8_t currValue = 0x00;
    if (rw) {
        CHECK(readRegister(reg, &currValue));
    }
    currValue &= ~mask;
    if (bcd) {
        CHECK(val = decToBcd(val));
    }
    currValue |= (val << shift);
    hal_i2c_begin_transmission(config_.i2c_if, AM18X5_I2C_ADDR, nullptr);
    hal_i2c_write(config_.i2c_if, static_cast<uint8_t>(reg), nullptr);
    hal_i2c_write(config_.i2c_if, currValue, nullptr);
    hal_i2c_end_transmission(config_.i2c_if, true, nullptr);
    return SYSTEM_ERROR_NONE;
}

int Am18x5::writeContinuousRegisters(const Am18x5Register start_reg, const uint8_t* buff, size_t len) const {
    Am18x5Lock lock;
    hal_i2c_begin_transmission(config_.i2c_if, AM18X5_I2C_ADDR, nullptr);
    hal_i2c_write(config_.i2c_if, static_cast<uint8_t>(start_reg), nullptr);
    for (size_t i = 0; i < len; i++) {
        hal_i2c_write(config_.i2c_if, buff[i], nullptr);
    }
    hal_i2c_end_transmission(config_.i2c_if, true, nullptr);
    return len;
}

int Am18x5::readRegister(const Am18x5Register reg, uint8_t* const val, bool bcd, uint8_t mask, uint8_t shift) const {
    Am18x5Lock lock;
    hal_i2c_begin_transmission(config_.i2c_if, AM18X5_I2C_ADDR, nullptr);
    hal_i2c_write(config_.i2c_if, static_cast<uint8_t>(reg), nullptr);
    hal_i2c_end_transmission(config_.i2c_if, false, nullptr);
    if (hal_i2c_request(config_.i2c_if, AM18X5_I2C_ADDR, 1, true, nullptr) == 0) {
        return SYSTEM_ERROR_INTERNAL;
    }
    *val = hal_i2c_read(config_.i2c_if, nullptr);
    *val &= mask;
    *val >>= shift;
    if (bcd) {
        CHECK(*val = bcdToDec(*val));
    }
    return SYSTEM_ERROR_NONE;
}

int Am18x5::readContinuousRegisters(const Am18x5Register start_reg, uint8_t* buff, size_t len) const {
    Am18x5Lock lock;
    hal_i2c_begin_transmission(config_.i2c_if, AM18X5_I2C_ADDR, nullptr);
    hal_i2c_write(config_.i2c_if, static_cast<uint8_t>(start_reg), nullptr);
    hal_i2c_end_transmission(config_.i2c_if, false, nullptr);
    if (hal_i2c_request(config_.i2c_if, AM18X5_I2C_ADDR, len, true, nullptr) == 0) {
        return SYSTEM_ERROR_INTERNAL;
    }
    int32_t size = hal_i2c_available(config_.i2c_if, nullptr);
    if (size <= 0) {
        return size;
    }
    size = std::min((size_t)size, len);
    for (int32_t i = 0; i < size; i++) {
        buff[i] = hal_i2c_read(config_.i2c_if, nullptr);
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
    while (!instance->exRtcWorkerThreadExit_) {
        os_semaphore_take(instance->exRtcWorkerSemaphore_, CONCURRENT_WAIT_FOREVER, false);
        {
            if (instance->exRtcWorkerThreadExit_) {
                break;
            }
            Am18x5Lock lock;

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
                    // One-shot alarm
                    instance->setAlarm(false);
                    instance->alarmHandler_(instance->alarmHandlerContext_);
                }
            }
        }
    }
    os_thread_exit(instance->exRtcWorkerThread_);
}

#endif // HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL