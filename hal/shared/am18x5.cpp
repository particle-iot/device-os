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
#undef LOG_COMPILE_TIME_LEVEL
#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_ALL

#include "hal_platform.h"

#if HAL_PLATFORM_EXTERNAL_RTC

// #define LOG_CHECKED_ERRORS 1

#include <memory>
#include <iterator>
#include <new>
#include "check.h"
#include "system_error.h"
#include "bcd_to_dec.h"
#include "interrupts_hal.h"
#include "gpio_hal.h"
#include "scope_guard.h"
#include "core_hal.h"
#include "bytes2hexbuf.h"
#include "am18x5_defines.h"
#include "am18x5.h"
#include "rtc_hal.h"
#include "exrtc_hal_internal.h"
#include "system_cache.h"

using namespace particle;
namespace {

const uint32_t AM18X5_SUPPORTED_CAPS =
        HAL_EXRTC_CAPS_POWER_GATE |
        HAL_EXRTC_CAPS_CLOCK_SOURCE |
        HAL_EXRTC_CAPS_CLOCK_OUTPUT |
        HAL_EXRTC_CAPS_AUTO_CALIBRATION |
        HAL_EXRTC_CAPS_AUTO_CLOCK_SOURCE_INTERNAL_ON_BATTERY |
        HAL_EXRTC_CAPS_AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL |
        HAL_EXRTC_CAPS_SLEEP |
        HAL_EXRTC_CAPS_EXTI |
        HAL_EXRTC_CAPS_EXTI_LEVEL_TRIGGER |
        HAL_EXRTC_CAPS_WATCHDOG;

const size_t AM18X5_ID_STR_LEN = 18;

void exRtcInterruptHandler(void* data) {
    auto instance = static_cast<Am18x5*>(data);
    instance->sync();
}

const long MICROS_IN_HUNDREDTH = 10000;
const long MICROS_IN_SECOND = 1000000;
const auto TM_CALENDAR_YEAR_2000 = 2000 - 1900; // for leap years we need to be in correct epoch

#define CEIL_DIV(A, B) (((A) + (B) - 1) / (B))

int timevalToCalendar(const struct timeval* tv, struct tm* calendar) {
    CHECK_TRUE(tv, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(gmtime_r(&tv->tv_sec, calendar), SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(calendar->tm_year >= TM_CALENDAR_YEAR_2000, SYSTEM_ERROR_INVALID_ARGUMENT);
    return 0;
}


int canonicalizeTimeval(struct timeval* tv) {
    CHECK_TRUE(tv, SYSTEM_ERROR_INVALID_ARGUMENT);

    if (tv->tv_usec >= MICROS_IN_SECOND) {
        tv->tv_sec += tv->tv_usec / MICROS_IN_SECOND;
        tv->tv_usec %= MICROS_IN_SECOND;
    }

    tv->tv_usec = CEIL_DIV(tv->tv_usec, MICROS_IN_HUNDREDTH) * MICROS_IN_HUNDREDTH;

    if (tv->tv_usec >= MICROS_IN_SECOND) {
        ++tv->tv_sec;
        tv->tv_usec -= MICROS_IN_SECOND;
    }

    return SYSTEM_ERROR_NONE;
}

} // anonymous namespace

Am18x5::Am18x5()
        : initialized_(false),
          alarmYear_(0),
          alarmHandler_(nullptr),
          alarmHandlerContext_(nullptr),
          exRtcWorkerThread_(nullptr),
          exRtcWorkerSemaphore_(nullptr),
          exRtcWorkerThreadExit_(false),
          exrtcConfigFlags_(0),
          subscribedOscEvents_(0),
          oscEventHandler_(nullptr),
          oscEventHandlerContext_(nullptr) {
    config_ = {};
    config_.version = HAL_EXRTC_API_VERSION;
    config_.size = sizeof(am18x5_config_t);
    config_.wdi_pin = PIN_INVALID;
    config_.int_pin = PIN_INVALID;
    config_.i2c_if = HAL_I2C_INTERFACE1;
    config_.osc_src = Am18x5Oscillator::EXTERNAL_CRYSTAL;
    config_.clk_out_freq = Am18x5SqwFrequency::HZ_32768;
    config_.mfg_magic = HAL_EXRTC_MFG_MAGIC;
}

Am18x5::~Am18x5() {

}

Am18x5& Am18x5::getInstance() {
    static Am18x5 am18x5;
    return am18x5;
}

int Am18x5::loadLegacyMfgOscCalibration(int32_t* cal) const {
    CHECK_TRUE(cal, SYSTEM_ERROR_INVALID_ARGUMENT);
    am18x5_config_t legacy = {};
    int r = particle::services::SystemCache::instance().get(particle::services::SystemCacheKey::AM18X5_MANUFACTURING_CONFIG, &legacy, sizeof(legacy));
    if (r == SYSTEM_ERROR_NOT_FOUND) {
        return r;
    }
    CHECK(r);
    if (legacy.version >= 2 && legacy.mfg_magic == HAL_EXRTC_MFG_MAGIC) {
        *cal = legacy.mfg_osc_cal_xt;
    } else {
        *cal = legacy.osc_cal_xt;
    }
    return SYSTEM_ERROR_NONE;
}

int Am18x5::loadMfgOscCalibration(int32_t* cal) const {
    CHECK_TRUE(cal, SYSTEM_ERROR_INVALID_ARGUMENT);
    hal_exrtc_calibration_data_t stored = {};
    int r = particle::services::SystemCache::instance().get(particle::services::SystemCacheKey::EXRTC_MFG_XTAL_CALIBRATION, &stored, sizeof(stored));
    if (r == SYSTEM_ERROR_NONE) {
        CHECK_TRUE(stored.size >= sizeof(uint16_t) * 2, SYSTEM_ERROR_BAD_DATA);
        *cal = stored.value;
        return SYSTEM_ERROR_NONE;
    }
    CHECK_TRUE(r == SYSTEM_ERROR_NOT_FOUND, r);
    return loadLegacyMfgOscCalibration(cal);
}

int Am18x5::bind(const hal_exrtc_binding_t* binding) {
    if (isPresent()) {
        CHECK(end());
    }
    CHECK_TRUE(binding && binding->device && binding->config, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(binding->device->type == HAL_EXRTC_TYPE_AM18X5, SYSTEM_ERROR_NOT_SUPPORTED);
    CHECK_TRUE(binding->device->transport == HAL_EXRTC_TRANSPORT_I2C, SYSTEM_ERROR_NOT_SUPPORTED);
    CHECK_TRUE(binding->device->i2c.address == HAL_EXRTC_TYPE_AM18X5_DEFAULT_ADDRESS, SYSTEM_ERROR_NOT_SUPPORTED);

    am18x5_config_t config = {};
    config.version = HAL_EXRTC_API_VERSION;
    config.size = sizeof(config);
    config.default_rtc = !!(binding->config->flags & HAL_EXRTC_CONFIG_USE_AS_MAIN_RTC);
    exrtcConfigFlags_ = binding->config->flags;
    config.wdi_pin = binding->device->i2c.pins[0];
    config.int_pin = binding->device->i2c.pin_int;
    config.i2c_if = binding->device->i2c.interface;
    config.rc_fallback = !!(binding->config->caps_enable & HAL_EXRTC_CAPS_AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL);
    config.rc_on_battery = !!(binding->config->caps_enable & HAL_EXRTC_CAPS_AUTO_CLOCK_SOURCE_INTERNAL_ON_BATTERY);
    config.osc_src = binding->config->clock_source == HAL_EXRTC_CLOCK_SOURCE_INTERNAL ?
            Am18x5Oscillator::INTERNAL_RC : Am18x5Oscillator::EXTERNAL_CRYSTAL;
    config.clk_out_en = !!(binding->config->caps_enable & HAL_EXRTC_CAPS_CLOCK_OUTPUT);
    config.clk_out_freq = Am18x5SqwFrequency::HZ_32768;
    config.auto_calibration = (binding->config->caps_enable & HAL_EXRTC_CAPS_AUTO_CALIBRATION) ?
            Am18x5AutoCalibration::AUTO_CAL_EVERY_1024_SEC : Am18x5AutoCalibration::AUTO_CAL_DISABLE;
    config.mfg_magic = HAL_EXRTC_MFG_MAGIC;

    int32_t legacyMfgCal = 0;
    bool haveLegacyMfgCal = (loadMfgOscCalibration(&legacyMfgCal) == SYSTEM_ERROR_NONE);
    if (haveLegacyMfgCal) {
        CHECK_TRUE(legacyMfgCal >= INT8_MIN && legacyMfgCal <= INT8_MAX, SYSTEM_ERROR_BAD_DATA);
        config.mfg_osc_cal_xt = static_cast<int8_t>(legacyMfgCal);
    }

    if (binding->vendor) {
        CHECK_TRUE(binding->vendor->type == HAL_EXRTC_TYPE_AM18X5, SYSTEM_ERROR_INVALID_ARGUMENT);
        auto vendor = reinterpret_cast<const hal_exrtc_vendor_config_am18x5_t*>(binding->vendor);
        if (vendor->xtal_calibration_set) {
            config.osc_cal_xt = vendor->xtal_calibration;
        }
    }
    if (!haveLegacyMfgCal) {
        config.mfg_osc_cal_xt = config.osc_cal_xt;
    }

    CHECK(setConfig(&config));
    return begin();
}

int Am18x5::command(hal_exrtc_command_t cmd, void* arg, uint32_t arg1) {
    (void)arg1;
    switch (cmd) {
        case HAL_EXRTC_COMMAND_SLEEP: {
            auto params = static_cast<hal_exrtc_sleep_config_t*>(arg);
            CHECK_TRUE(params && params->size >= sizeof(uint16_t) * 2, SYSTEM_ERROR_INVALID_ARGUMENT);
            am18x5_sleep_config_t config = {};
            config.version = HAL_EXRTC_API_VERSION;
            config.size = sizeof(config);
            config.duration = params->duration;
            switch (params->exti_mode) {
                case CHANGE:
                    config.exti_polarity = Am18x5ExtiPolarity::NONE;
                    break;
                case FALLING:
                    config.exti_polarity = Am18x5ExtiPolarity::FALLING;
                    break;
                case RISING:
                    config.exti_polarity = Am18x5ExtiPolarity::RISING;
                    break;
                default:
                    return SYSTEM_ERROR_INVALID_ARGUMENT;
            }
            return sleep(&config);
        }
        default:
            return SYSTEM_ERROR_NOT_SUPPORTED;
    }
}

int Am18x5::getDevice(hal_exrtc_device_t* device) const {
    CHECK_TRUE(device && device->size >= sizeof(uint16_t) * 2, SYSTEM_ERROR_INVALID_ARGUMENT);
    hal_exrtc_device_t dev = {};
    dev.size = sizeof(dev);
    dev.version = HAL_EXRTC_API_VERSION;
    dev.type = HAL_EXRTC_TYPE_AM18X5;
    dev.transport = HAL_EXRTC_TRANSPORT_I2C;
    dev.i2c.interface = config_.i2c_if;
    dev.i2c.address = HAL_EXRTC_TYPE_AM18X5_DEFAULT_ADDRESS;
    dev.i2c.pin_int = config_.int_pin;
    dev.i2c.pins[0] = config_.wdi_pin;
    std::fill(std::begin(dev.i2c.pins) + 1, std::end(dev.i2c.pins), PIN_INVALID);
    memcpy(device, &dev, std::min<size_t>(device->size, sizeof(dev)));
    return SYSTEM_ERROR_NONE;
}

int Am18x5::getConfig(hal_exrtc_config_t* config, hal_exrtc_vendor_config_t* vendor) const {
    CHECK_TRUE(config && config->size >= sizeof(uint16_t) * 2, SYSTEM_ERROR_INVALID_ARGUMENT);

    hal_exrtc_config_t exrtcConfig = {};
    exrtcConfig.size = sizeof(exrtcConfig);
    exrtcConfig.version = HAL_EXRTC_API_VERSION;
    if (config_.default_rtc) {
        exrtcConfig.flags |= HAL_EXRTC_CONFIG_USE_AS_MAIN_RTC;
    }
    if (exrtcConfigFlags_ & HAL_EXRTC_CONFIG_SLEEP_EXTI_CHECK) {
        exrtcConfig.flags |= HAL_EXRTC_CONFIG_SLEEP_EXTI_CHECK;
    }
    exrtcConfig.clock_source = config_.osc_src == Am18x5Oscillator::INTERNAL_RC ?
            HAL_EXRTC_CLOCK_SOURCE_INTERNAL : HAL_EXRTC_CLOCK_SOURCE_EXTERNAL;
    if (config_.rc_on_battery) {
        exrtcConfig.caps_enable |= HAL_EXRTC_CAPS_AUTO_CLOCK_SOURCE_INTERNAL_ON_BATTERY;
    }
    if (config_.rc_fallback) {
        exrtcConfig.caps_enable |= HAL_EXRTC_CAPS_AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL;
    }
    if (config_.auto_calibration != Am18x5AutoCalibration::AUTO_CAL_DISABLE) {
        exrtcConfig.caps_enable |= HAL_EXRTC_CAPS_AUTO_CALIBRATION;
    }
    if (config_.clk_out_en) {
        exrtcConfig.caps_enable |= HAL_EXRTC_CAPS_CLOCK_OUTPUT;
    }
    memcpy(config, &exrtcConfig, std::min<size_t>(config->size, sizeof(exrtcConfig)));

    if (vendor) {
        CHECK_TRUE(vendor->size >= sizeof(hal_exrtc_vendor_config_t), SYSTEM_ERROR_INVALID_ARGUMENT);
        hal_exrtc_vendor_config_am18x5_t amVendor = {};
        amVendor.base.size = sizeof(amVendor);
        amVendor.base.version = HAL_EXRTC_API_VERSION;
        amVendor.base.type = HAL_EXRTC_TYPE_AM18X5;
        amVendor.xtal_calibration_set = true;
        amVendor.xtal_calibration = config_.osc_cal_xt;
        memcpy(vendor, &amVendor, std::min<size_t>(vendor->size, sizeof(amVendor)));
    }

    return SYSTEM_ERROR_NONE;
}

int Am18x5::getStatus(hal_exrtc_status_t* status) const {
    CHECK_TRUE(status && status->size >= sizeof(uint16_t) * 2, SYSTEM_ERROR_INVALID_ARGUMENT);

    hal_exrtc_status_t exrtcStatus = {};
    exrtcStatus.size = sizeof(exrtcStatus);
    exrtcStatus.version = HAL_EXRTC_API_VERSION;
    exrtcStatus.type = HAL_EXRTC_TYPE_AM18X5;
    exrtcStatus.status = HAL_EXRTC_STATUS_BOUND;
    exrtcStatus.caps_supported = AM18X5_SUPPORTED_CAPS;
    exrtcStatus.caps_enabled = 0;
    if (config_.rc_on_battery) {
        exrtcStatus.caps_enabled |= HAL_EXRTC_CAPS_AUTO_CLOCK_SOURCE_INTERNAL_ON_BATTERY;
    }
    if (config_.rc_fallback) {
        exrtcStatus.caps_enabled |= HAL_EXRTC_CAPS_AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL;
    }
    if (config_.auto_calibration != Am18x5AutoCalibration::AUTO_CAL_DISABLE) {
        exrtcStatus.caps_enabled |= HAL_EXRTC_CAPS_AUTO_CALIBRATION;
    }
    if (config_.clk_out_en) {
        exrtcStatus.caps_enabled |= HAL_EXRTC_CAPS_CLOCK_OUTPUT;
    }
    if (initialized_) {
        exrtcStatus.status |= HAL_EXRTC_STATUS_PRESENT | HAL_EXRTC_STATUS_READY;
        Am18x5Oscillator source = Am18x5Oscillator::EXTERNAL_CRYSTAL;
        if (!const_cast<Am18x5*>(this)->getOscillatorSource(&source)) {
            exrtcStatus.clock_source = source == Am18x5Oscillator::INTERNAL_RC ?
                    HAL_EXRTC_CLOCK_SOURCE_INTERNAL : HAL_EXRTC_CLOCK_SOURCE_EXTERNAL;
        }
    } else {
        exrtcStatus.clock_source = config_.osc_src == Am18x5Oscillator::INTERNAL_RC ?
                HAL_EXRTC_CLOCK_SOURCE_INTERNAL : HAL_EXRTC_CLOCK_SOURCE_EXTERNAL;
    }
    memcpy(status, &exrtcStatus, std::min<size_t>(status->size, sizeof(exrtcStatus)));
    return SYSTEM_ERROR_NONE;
}

// Version takes the precedence in case of upgrading/downgrading DVOS firmware
// The size of the config should keep/increase on version bumped
int Am18x5::setConfig(const am18x5_config_t* config) {
    CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(config->size > 0 && config->i2c_if < HAL_PLATFORM_I2C_NUM, SYSTEM_ERROR_INVALID_ARGUMENT);
    am18x5_config_t updated = config_;
    memcpy(&updated, config, std::min<size_t>(sizeof(updated), config->size));
    updated.version = HAL_EXRTC_API_VERSION;
    updated.size = sizeof(updated);
    if (updated.mfg_magic != HAL_EXRTC_MFG_MAGIC) {
        updated.mfg_magic = HAL_EXRTC_MFG_MAGIC;
        updated.mfg_osc_cal_xt = updated.osc_cal_xt;
    }
    config_ = updated;
    return SYSTEM_ERROR_NONE;
}

int Am18x5::getConfig(am18x5_config_t* config) {
    CHECK_TRUE(config && (config->size > 0), SYSTEM_ERROR_INVALID_ARGUMENT);
    memcpy(config, &config_, std::min<size_t>(config_.size, config->size));
    LOG(INFO, "Get EXRTC config version: %d, size: %d", config_.version, config_.size);
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
    int ret = SYSTEM_ERROR_INTERNAL;
    SCOPE_GUARD ({
        if (ret != SYSTEM_ERROR_NONE) {
            end();
        }
    });
    CHECK_TRUE(config_.size >= sizeof(am18x5_config_t), SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(config_.i2c_if < HAL_PLATFORM_I2C_NUM, SYSTEM_ERROR_INVALID_STATE);
    if (!initialized_) {
        if (!hal_i2c_is_enabled(config_.i2c_if, nullptr)) {
            hal_i2c_init(config_.i2c_if, nullptr);
            hal_i2c_begin(config_.i2c_if, I2C_MODE_MASTER, 0x00, nullptr);
            CHECK_TRUE(hal_i2c_is_enabled(config_.i2c_if, nullptr), SYSTEM_ERROR_INTERNAL);
            // Make sure to reset the I2C bus to avoid potentially corrupting the AM18x5 configuration if
            // we start communication with it during an ongoing write transaction (which may happen e.g. after a hard reset)
            CHECK(hal_i2c_reset(config_.i2c_if, 0, nullptr));
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
    CHECK(applyConfig());
    CHECK(updateEventHandlers());
    return ret = SYSTEM_ERROR_NONE;
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
    if (oscEventHandler_ && (config_.osc_src == Am18x5Oscillator::EXTERNAL_CRYSTAL) && (subscribedOscEvents_ & Am18x5OscEvent::XT_OSC_FAILURE)) {
        oscControl |= OSC_CONTROL_OFIE_MASK;
        // Clear the oscillator failure flag
        CHECK(writeRegister(Am18x5Register::OSC_STATUS, 0, false, true, OSC_STATUS_OF_MASK, OSC_STATUS_OF_SHIFT));
    } else {
        oscControl &= ~OSC_CONTROL_OFIE_MASK;
    }
    CHECK(writeRegister(Am18x5Register::CONFIG_KEY, CONFIG_KEY_OSC_CONTROL));
    CHECK(writeRegister(Am18x5Register::OSC_CONTROL, oscControl));

    // Digital calibration to improve accuracy.
    CHECK(xtOscillatorDigitalCalibration(config_.osc_cal_xt));

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

int Am18x5::getOscillatorSource(Am18x5Oscillator* source) {
    Am18x5Lock lock;
    CHECK_TRUE(source, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t oscStatus = 0;
    CHECK(readRegister(Am18x5Register::OSC_STATUS, &oscStatus));
    *source = (oscStatus & OSC_STATUS_OMODE_MASK) ? Am18x5Oscillator::INTERNAL_RC : Am18x5Oscillator::EXTERNAL_CRYSTAL;
    return SYSTEM_ERROR_NONE;
}

int Am18x5::onOscillatorEvent(uint8_t events, Am18x5OscEventHandler handler, void* context) {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t oscControl = 0x00;
    CHECK(readRegister(Am18x5Register::OSC_CONTROL, &oscControl));
    if (handler && (config_.osc_src == Am18x5Oscillator::EXTERNAL_CRYSTAL) && (events & Am18x5OscEvent::XT_OSC_FAILURE)) {
        oscControl |= OSC_CONTROL_OFIE_MASK;
    } else {
        oscControl &= ~OSC_CONTROL_OFIE_MASK;
    }
    if (handler && (events & Am18x5OscEvent::AUTO_CAL_FAILURE)) {
        oscControl |= OSC_CONTROL_ACIE_MASK;
    } else {
        oscControl &= ~OSC_CONTROL_ACIE_MASK;
    }
    // Clear the oscillator failure flag
    CHECK(writeRegister(Am18x5Register::OSC_STATUS, 0, false, true, OSC_STATUS_OF_MASK | OSC_STATUS_ACF_MASK, 0));
    CHECK(writeRegister(Am18x5Register::CONFIG_KEY, CONFIG_KEY_OSC_CONTROL));
    CHECK(writeRegister(Am18x5Register::OSC_CONTROL, oscControl));
    subscribedOscEvents_ = events;
    oscEventHandler_ = handler;
    oscEventHandlerContext_ = context;
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
    CHECK_TRUE(id && (len >= AM18X5_ID_STR_LEN), SYSTEM_ERROR_INVALID_ARGUMENT);
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
    id += 2;
    *id = '\0';
    return SYSTEM_ERROR_NONE;
}

void Am18x5::exRtcOscEventHandler(uint8_t events, void* context) {
    auto self = static_cast<Am18x5*>(context);
    if (!self) {
        return;
    }
    uint32_t exrtcEvents = HAL_EXRTC_EVENT_NONE;
    if (events & Am18x5OscEvent::XT_OSC_FAILURE) {
        exrtcEvents |= HAL_EXRTC_EVENT_CLOCK_SOURCE_EXTERNAL_FAILURE;
    }
    if (events & Am18x5OscEvent::AUTO_CAL_FAILURE) {
        exrtcEvents |= HAL_EXRTC_EVENT_CALIBRATION_FAILURE;
    }
    if (!exrtcEvents) {
        return;
    }
    for (auto h = self->eventHandlers_.front(); h; h = h->next) {
        if (h->handler) {
            h->handler(exrtcEvents, nullptr, h->context);
        }
    }
}

void* Am18x5::addEventHandler(hal_exrtc_event_handler_t handler, void* context, hal_exrtc_event_cleanup_handler_t cleanup) {
    CHECK_TRUE(handler, nullptr);
    auto h = new(std::nothrow) ExRtcEventHandler();
    CHECK_TRUE(h, nullptr);
    h->handler = handler;
    h->context = context;
    h->cleanup = cleanup;
    eventHandlers_.pushFront(h);
    if (updateEventHandlers()) {
        eventHandlers_.popFront();
        if (h->cleanup) {
            h->cleanup(h->context);
        }
        delete h;
        return nullptr;
    }
    return h;
}

int Am18x5::removeEventHandler(void* cookie) {
    CHECK_TRUE(cookie, SYSTEM_ERROR_INVALID_ARGUMENT);
    for (auto h = eventHandlers_.front(), prev = static_cast<ExRtcEventHandler*>(nullptr); h; prev = h, h = h->next) {
        if (h == cookie) {
            eventHandlers_.pop(h, prev);
            if (h->cleanup) {
                h->cleanup(h->context);
            }
            delete h;
            return updateEventHandlers();
        }
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int Am18x5::clearEventHandlers() {
    while (auto h = eventHandlers_.popFront()) {
        if (h->cleanup) {
            h->cleanup(h->context);
        }
        delete h;
    }
    return updateEventHandlers();
}

int Am18x5::updateEventHandlers() {
    const uint8_t events = eventHandlers_.front() ?
            (Am18x5OscEvent::XT_OSC_FAILURE | Am18x5OscEvent::AUTO_CAL_FAILURE) : 0;
    if (!initialized_) {
        return SYSTEM_ERROR_NONE;
    }
    return onOscillatorEvent(events, events ? exRtcOscEventHandler : nullptr, this);
}

int Am18x5::setTime(const struct timeval* tv) const {
    struct tm calendar;
    struct timeval canonical = *tv;
    CHECK(canonicalizeTimeval(&canonical));
    CHECK(timevalToCalendar(&canonical, &calendar));

    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    uint8_t buff[8] = {0};
    buff[0] = CHECK(decToBcd(canonical.tv_usec / MICROS_IN_HUNDREDTH));
    buff[1] = CHECK(decToBcd(calendar.tm_sec));
    buff[2] = CHECK(decToBcd(calendar.tm_min));
    buff[3] = CHECK(decToBcd(calendar.tm_hour));
    buff[4] = CHECK(decToBcd(calendar.tm_mday));
    buff[5] = CHECK(decToBcd(calendar.tm_mon + 1)); // Month in tm structure ranges from 0 - 11.
    buff[6] = CHECK(decToBcd(calendar.tm_year % 100));
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
    calendar.tm_year += TM_CALENDAR_YEAR_2000;
    calendar.tm_wday = CHECK(bcdToDec(buff[7]));
    calendar.tm_isdst = -1;

    tv->tv_sec = mktime(&calendar);
    tv->tv_usec = CHECK(bcdToDec(buff[0])) * MICROS_IN_HUNDREDTH;
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

int Am18x5::setAlarm(bool enable, uint32_t flags, const struct timeval* tv, AlarmHandler handler, void* context, bool requireInterruptPin) {
    Am18x5Lock lock;
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    if (enable) {
        if (requireInterruptPin) {
            CHECK_TRUE(config_.int_pin != PIN_INVALID, SYSTEM_ERROR_NOT_SUPPORTED);
        }
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
            CHECK(canonicalizeTimeval(&alarm));
            CHECK(timevalToCalendar(&alarm, &calendar));
            uint8_t buff[7] = {0};
            buff[0] = CHECK(decToBcd(alarm.tv_usec / MICROS_IN_HUNDREDTH));
            buff[1] = CHECK(decToBcd(calendar.tm_sec));
            buff[2] = CHECK(decToBcd(calendar.tm_min));
            buff[3] = CHECK(decToBcd(calendar.tm_hour));
            buff[4] = CHECK(decToBcd(calendar.tm_mday));
            buff[5] = CHECK(decToBcd(calendar.tm_mon + 1)); // Month in tm structure ranges from 0 - 11.
            alarmYear_ = calendar.tm_year % 100;
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
        calendar.tm_year = alarmYear_ + TM_CALENDAR_YEAR_2000;
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

int Am18x5::sleep(const am18x5_sleep_config_t* config) {
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

    uint8_t control1 = 0x00;
    CHECK(readRegister(Am18x5Register::CONTROL1, &control1));
    bool pwr2Enabled = control1 & CONTROL1_PWR2_MASK;

    // Set PSW/nIRQ2 to be working in SLEEP mode as the power switch output.
    CHECK(writeRegister(Am18x5Register::CONTROL2, 6, false, true, CONTROL2_OUT2S_MASK, CONTROL2_OUT2S_SHIFT));
    CHECK(writeRegister(Am18x5Register::CONTROL1, 1, false, true, CONTROL1_PWR2_MASK, 1));
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
        setAlarm(enabled > 0 ? true : false, 0, &alarmTv, alarmHandler_, alarmHandlerContext_, false);
        // Disable EXTI interrupt, as it is only used for sleep wakeup for now
        intMask &= ~INTERRUPT_EX1E_MASK;
        writeRegister(Am18x5Register::INT_MASK, intMask);
        writeRegister(Am18x5Register::CONTROL1, pwr2Enabled ? 1 : 0, false, true, CONTROL1_PWR2_MASK, 1);
        if (watchdogWasStarted) {
            writeRegister(Am18x5Register::WDT, watchdogValue_);
        }
    });

    if (config->duration > 0) {
        struct timeval tv = {
            .tv_sec = config->duration,
            .tv_usec = 0
        };
        CHECK(setAlarm(true, HAL_RTC_ALARM_FLAG_IN, &tv, nullptr, nullptr, false));
        intMask |= INTERRUPT_AIE_MASK;
    } else {
        intMask &= ~INTERRUPT_AIE_MASK;
    }

    // Read status to clear the interrupt flags
    uint8_t status = 0x00;
    CHECK(readRegister(Am18x5Register::STATUS, &status));
    // Enable interrupt
    CHECK(writeRegister(Am18x5Register::INT_MASK, intMask));

    // Optionally require the EXTI wake input to be inactive before arming sleep.
    if ((exrtcConfigFlags_ & HAL_EXRTC_CONFIG_SLEEP_EXTI_CHECK) && config->exti_polarity != Am18x5ExtiPolarity::NONE) {
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

            if (!(instance->subscribedOscEvents_) || !(instance->oscEventHandler_)) {
                continue;
            }
            uint8_t oscControl = 0x00;
            if (instance->readRegister(Am18x5Register::OSC_CONTROL, &oscControl) != SYSTEM_ERROR_NONE) {
                continue;
            }
            if (!(oscControl & OSC_CONTROL_OFIE_MASK) && !(oscControl & OSC_CONTROL_ACIE_MASK)) {
                // No oscillator failure interrupts enabled
                continue;
            }
            uint8_t oscStatus = 0x00;
            if (instance->readRegister(Am18x5Register::OSC_STATUS, &oscStatus) != SYSTEM_ERROR_NONE) {
                continue;
            }
            uint8_t events = 0x00;
            if (instance->subscribedOscEvents_ & Am18x5OscEvent::XT_OSC_FAILURE) {
                uint8_t stopped;
                if (instance->readRegister(Am18x5Register::CONTROL1, &stopped, false, CONTROL1_STOP_MASK, CONTROL1_STOP_SHIFT) != SYSTEM_ERROR_NONE) {
                    continue;
                }
                // When the STOP bit is set or the OSEL bit is set to 1 to select the RC Oscillator,
                // OF will always be set
                if (!stopped && !(oscControl & OSC_CONTROL_OSEL_MASK)) {
                    if (oscStatus & OSC_STATUS_OF_MASK) {
                        events |= Am18x5OscEvent::XT_OSC_FAILURE;
                    }
                }
            }
            if (instance->subscribedOscEvents_ & Am18x5OscEvent::AUTO_CAL_FAILURE) {
                if (oscStatus & OSC_STATUS_ACF_MASK) {
                    events |= Am18x5OscEvent::AUTO_CAL_FAILURE;
                }
            }
            if (oscStatus & (OSC_STATUS_OF_MASK | OSC_STATUS_ACF_MASK)) {
                // Clear the oscillator failure flag
                instance->writeRegister(Am18x5Register::OSC_STATUS, 0, false, true, OSC_STATUS_OF_MASK | OSC_STATUS_ACF_MASK, 0);
            }
            if (events) {
                instance->oscEventHandler_(events, instance->oscEventHandlerContext_);
            }
        }
    }
    os_thread_exit(instance->exRtcWorkerThread_);
}

#endif // HAL_PLATFORM_EXTERNAL_RTC
