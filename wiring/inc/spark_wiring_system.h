/*
 * @file    spark_wiring_system.h
 * @author  Satish Nair, Zachary Crockett, Matthew McGowan
 *
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

#ifndef SPARK_WIRING_SYSTEM_H
#define SPARK_WIRING_SYSTEM_H

#include "spark_wiring_ticks.h"
#include "spark_wiring_string.h"
#include "spark_wiring_platform.h"
#include "spark_wiring_version.h"
#include "system_mode.h"
#include "system_update.h"
#include "system_sleep.h"
#include "system_cloud.h"
#include "system_event.h"
#include "interrupts_hal.h"
#include "core_hal.h"
#include "system_user.h"
#include "system_version.h"
#include "spark_wiring_flags.h"
#include <chrono>
#include <limits>
#include <mutex>
#include "spark_wiring_system_power.h"
#include "system_sleep_configuration.h"
#include "system_control.h"
#include "spark_wiring_vector.h"
#include "enumclass.h"
#include "system_control.h"
#include "scope_guard.h"
#include "underlying_type.h"
#include "deviceid_hal.h"
#include "platform_ncp.h"
#include "backup_ram_hal.h"
#include "spark_wiring_asset.h"

using spark::Vector;

#if defined(SPARK_PLATFORM) && PLATFORM_ID != PLATFORM_GCC
#define SYSTEM_HW_TICKS 1
#else
#define SYSTEM_HW_TICKS 0
#endif

#if SYSTEM_HW_TICKS
#include "hw_ticks.h"
#endif

class Stream;

struct SleepOptionFlagType; // Tag type for System.sleep() flags
typedef particle::Flags<SleepOptionFlagType, uint32_t> SleepOptionFlags;
typedef SleepOptionFlags::FlagType SleepOptionFlag;

const SleepOptionFlag SLEEP_NETWORK_OFF(System_Sleep_Flag::SYSTEM_SLEEP_FLAG_NETWORK_OFF);
const SleepOptionFlag SLEEP_NETWORK_STANDBY(System_Sleep_Flag::SYSTEM_SLEEP_FLAG_NETWORK_STANDBY);
const SleepOptionFlag SLEEP_DISABLE_WKP_PIN(System_Sleep_Flag::SYSTEM_SLEEP_FLAG_DISABLE_WKP_PIN);
const SleepOptionFlag SLEEP_NO_WAIT(System_Sleep_Flag::SYSTEM_SLEEP_FLAG_NO_WAIT);

struct SystemResetFlagType; // Tag type for System.reset() flags
typedef particle::Flags<SystemResetFlagType, uint32_t> SystemResetFlags;
typedef SystemResetFlags::FlagType SystemResetFlag;

const SystemResetFlag RESET_NO_WAIT(system_reset_flag::SYSTEM_RESET_FLAG_NO_WAIT);
const SystemResetFlag RESET_PERSIST_DFU(system_reset_flag::SYSTEM_RESET_FLAG_PERSIST_DFU);

#if Wiring_LogConfig
enum LoggingFeature {
    FEATURE_CONFIGURABLE_LOGGING = 1
};
#endif

enum WakeupReason {
    WAKEUP_REASON_NONE = 0,
    WAKEUP_REASON_PIN = 1,
    WAKEUP_REASON_RTC = 2,
    WAKEUP_REASON_PIN_OR_RTC = 3,
    WAKEUP_REASON_UNKNOWN = 4
};

enum class SystemControlRequestAclAction {
    NONE = 0,
    ACCEPT = 1,
    DENY = 2
};

class SystemControlRequestAcl {
public:
    SystemControlRequestAcl(ctrl_request_type type, SystemControlRequestAclAction action) {
        type_ = type;
        action_ = action;
    };
    ctrl_request_type type() const {
        return type_;
    }
    SystemControlRequestAclAction action() const {
        return action_;
    }

private:
    ctrl_request_type type_;
    SystemControlRequestAclAction action_;
};

/**
 * Firmware update status.
 */
enum class UpdateStatus: int {
    /**
     * The system will check for firmware updates when the device connects to the Cloud.
     */
    UNKNOWN = SYSTEM_UPDATE_STATUS_UNKNOWN,
    /**
     * No firmware update available.
     */
    NOT_AVAILABLE = SYSTEM_UPDATE_STATUS_NOT_AVAILABLE,
    /**
     * A firmware update is available.
     */
    PENDING = SYSTEM_UPDATE_STATUS_PENDING,
    /**
     * A firmware update is in progress.
     */
    IN_PROGRESS = SYSTEM_UPDATE_STATUS_IN_PROGRESS
};

PARTICLE_DEFINE_ENUM_COMPARISON_OPERATORS(UpdateStatus)

struct SleepResult {
    SleepResult() {}
    SleepResult(WakeupReason r, system_error_t e, hal_pin_t p = std::numeric_limits<hal_pin_t>::max());
    SleepResult(int ret, const hal_pin_t* pins, size_t pinsSize);

    WakeupReason reason() const;
    bool wokenUpByRtc() const;
    bool wokenUpByPin() const;

    hal_pin_t pin() const;
    bool rtc() const;
    system_error_t error() const;

private:
    WakeupReason reason_ = WAKEUP_REASON_NONE;
    system_error_t err_ = SYSTEM_ERROR_NONE;
    hal_pin_t pin_ = std::numeric_limits<hal_pin_t>::max();
};

inline SleepResult::SleepResult(WakeupReason r, system_error_t e, hal_pin_t p)
    : reason_(r),
      err_(e),
      pin_(p) {
}

enum class SystemSleepWakeupReason: uint16_t {
    UNKNOWN = HAL_WAKEUP_SOURCE_TYPE_UNKNOWN,
    BY_GPIO = HAL_WAKEUP_SOURCE_TYPE_GPIO,
    BY_ADC = HAL_WAKEUP_SOURCE_TYPE_ADC,
    BY_DAC = HAL_WAKEUP_SOURCE_TYPE_DAC,
    BY_RTC = HAL_WAKEUP_SOURCE_TYPE_RTC,
    BY_LPCOMP = HAL_WAKEUP_SOURCE_TYPE_LPCOMP,
    BY_USART = HAL_WAKEUP_SOURCE_TYPE_USART,
    BY_I2C = HAL_WAKEUP_SOURCE_TYPE_I2C,
    BY_SPI = HAL_WAKEUP_SOURCE_TYPE_SPI,
    BY_TIMER = HAL_WAKEUP_SOURCE_TYPE_TIMER,
    BY_CAN = HAL_WAKEUP_SOURCE_TYPE_CAN,
    BY_USB = HAL_WAKEUP_SOURCE_TYPE_USB,
    BY_BLE = HAL_WAKEUP_SOURCE_TYPE_BLE,
    BY_NFC = HAL_WAKEUP_SOURCE_TYPE_NFC,
    BY_NETWORK = HAL_WAKEUP_SOURCE_TYPE_NETWORK,
};

class SystemSleepResult {
public:
    SystemSleepResult()
            : wakeupSource_(nullptr),
              error_(SYSTEM_ERROR_NONE) {
    }

    SystemSleepResult(SleepResult r)
            : wakeupSource_(nullptr),
              error_(SYSTEM_ERROR_NONE),
              compatResult_(r) {
    }

    SystemSleepResult(hal_wakeup_source_base_t* source, system_error_t error)
            : SystemSleepResult() {
        copyWakeupSource(source);
        error_ = error;
    }

    SystemSleepResult(system_error_t error)
            : SystemSleepResult() {
        error_ = error;
    }

    // Copy constructor
    SystemSleepResult(const SystemSleepResult& result)
            : SystemSleepResult() {
        error_ = result.error_;
        compatResult_ = result.compatResult_;
        copyWakeupSource(result.wakeupSource_);
    }

    SystemSleepResult& operator=(const SystemSleepResult& result) {
        error_ = result.error_;
        compatResult_ = result.compatResult_;
        copyWakeupSource(result.wakeupSource_);
        return *this;
    }

    // Move constructor
    SystemSleepResult(SystemSleepResult&& result)
            : SystemSleepResult() {
        error_ = result.error_;
        compatResult_ = result.compatResult_;
        freeWakeupSourceMemory();
        if (result.wakeupSource_) {
            wakeupSource_ = result.wakeupSource_;
            result.wakeupSource_ = nullptr;
        }
    }

    SystemSleepResult& operator=(SystemSleepResult&& result) {
        error_ = result.error_;
        compatResult_ = result.compatResult_;
        freeWakeupSourceMemory();
        if (result.wakeupSource_) {
            wakeupSource_ = result.wakeupSource_;
            result.wakeupSource_ = nullptr;
        }
        return *this;
    }

    ~SystemSleepResult() {
        freeWakeupSourceMemory();
    }

    void setError(system_error_t error, bool clear = false) {
        error_ = error;
        if (clear) {
            freeWakeupSourceMemory();
        }
    }

    hal_wakeup_source_base_t** halWakeupSource() {
        return &wakeupSource_;
    }

    SystemSleepWakeupReason wakeupReason() const {
        if (wakeupSource_) {
            return static_cast<SystemSleepWakeupReason>(wakeupSource_->type);
        } else {
            return SystemSleepWakeupReason::UNKNOWN;
        }
    }

    hal_pin_t wakeupPin() const {
        if (wakeupReason() == SystemSleepWakeupReason::BY_GPIO) {
            return reinterpret_cast<hal_wakeup_source_gpio_t*>(wakeupSource_)->pin;
        } else {
            return std::numeric_limits<hal_pin_t>::max();
        }
    }

    system_error_t error() const {
        return error_;
    }

    SleepResult toSleepResult() {
        if (error_ || wakeupSource_) {
            switch (wakeupReason()) {
                case SystemSleepWakeupReason::BY_GPIO: {
                    compatResult_ = SleepResult(WAKEUP_REASON_PIN, error(), wakeupPin());
                    break;
                }
                case SystemSleepWakeupReason::BY_RTC: {
                    compatResult_ = SleepResult(WAKEUP_REASON_RTC, error());
                    break;
                }
                default: {
                    compatResult_ = SleepResult(WAKEUP_REASON_UNKNOWN, error());
                    break;
                }
            }
        }
        return compatResult_;
    }

    operator SleepResult() {
        return toSleepResult();
    }

private:
    void freeWakeupSourceMemory() {
        if (wakeupSource_) {
            free(wakeupSource_);
            wakeupSource_ = nullptr;
        }
    }

    int copyWakeupSource(hal_wakeup_source_base_t* source) {
        freeWakeupSourceMemory();
        if (source) {
            wakeupSource_ = (hal_wakeup_source_base_t*)malloc(source->size);
            if (wakeupSource_) {
                memcpy(wakeupSource_, source, source->size);
            } else {
                return SYSTEM_ERROR_NO_MEMORY;
            }
        }
        return SYSTEM_ERROR_NONE;
    }

    hal_wakeup_source_base_t* wakeupSource_;
    system_error_t error_;
    SleepResult compatResult_;
};

class SystemEventSubscription {
public:
    SystemEventSubscription(system_event_t events = (system_event_t)0, void* callable = nullptr)
            : events_(events),
              callable_(callable) {
    }

    operator bool() const {
        return callable_ != nullptr;
    }

    void* getCallable() const {
        return callable_;
    }

    system_event_t getEvents() const {
        return events_;
    }

private:
    system_event_t events_;
    void* callable_;
};

struct SystemHardwareInfo {
    SystemHardwareInfo()
            : info_{},
              error_(SYSTEM_ERROR_INVALID_STATE) {
        memset(&info_, 0xff, sizeof(info_));
        info_.size = sizeof(info_);
        info_.version = 0;
        memset(info_.ncp, 0xff, sizeof(info_.ncp));
        error_ = hal_get_device_hw_info(&info_, nullptr);
    }

    SystemHardwareInfo(const SystemHardwareInfo&) = default;
    SystemHardwareInfo(SystemHardwareInfo&&) = default;
    SystemHardwareInfo& operator=(const SystemHardwareInfo&) = default;
    SystemHardwareInfo& operator=(SystemHardwareInfo&&) = default;

    uint32_t revision() const {
        return info_.revision;
    }

    uint32_t model() const {
        return info_.model;
    }

    uint32_t variant() const {
        return info_.variant;
    }

    uint32_t featureFlags() const {
        return info_.features;
    }

    bool isValid() const {
        return error_ == SYSTEM_ERROR_NONE;
    }

    spark::Vector<PlatformNCPIdentifier> ncp() const {
        spark::Vector<PlatformNCPIdentifier> ncps;
        for (const auto& ncpId: info_.ncp) {
            if (ncpId != PLATFORM_NCP_UNKNOWN) {
                ncps.append((PlatformNCPIdentifier)ncpId);
            }
        }
        return ncps;
    }

    operator bool() const {
        return isValid();
    }

private:
    hal_device_hw_info info_;
    int error_;
};

class SystemClass {
public:

    SystemClass(System_Mode_TypeDef mode = DEFAULT) {
        set_system_mode(mode);
    }

    static System_Mode_TypeDef mode(void) {
        return system_mode();
    }

    static bool firmwareUpdate(Stream *serialObj) {
        return system_firmwareUpdate(serialObj);
    }

    static void factoryReset(SystemResetFlags flags = SystemResetFlags());
    static void dfu(SystemResetFlags flags = SystemResetFlags());
    static void dfu(bool persist);
    static void reset();
    static void reset(SystemResetFlags flags);
    static void reset(uint32_t data, SystemResetFlags flags = SystemResetFlags());

    static void enterSafeMode(SystemResetFlags flags = SystemResetFlags());

#if HAL_PLATFORM_SYSTEM_HW_TICKS
    static inline uint32_t ticksPerMicrosecond() {
        return SYSTEM_US_TICKS;
    }

    static inline uint32_t ticks() {
        return SYSTEM_TICK_COUNTER;
    }
#else
    static inline uint32_t ticksPerMicrosecond() {
        return 1;
    }

    static inline uint32_t ticks() {
        return HAL_Timer_Get_Micro_Seconds();
    }
#endif // HAL_PLATFORM_SYSTEM_HW_TICKS

    static inline void ticksDelay(uint32_t duration) {
        uint32_t start = ticks();
        while ((ticks() - start) < duration) {}
    }

    static SystemSleepResult sleep(const particle::SystemSleepConfiguration& config);
    static SleepResult sleep(Spark_Sleep_TypeDef sleepMode, long seconds = 0, SleepOptionFlags flag = SLEEP_NETWORK_OFF);

    inline static SleepResult sleep(Spark_Sleep_TypeDef sleepMode, std::chrono::seconds s, SleepOptionFlags flag = SLEEP_NETWORK_OFF) {
        return sleep(sleepMode, s.count(), flag);
    }

    inline static SleepResult sleep(Spark_Sleep_TypeDef sleepMode, SleepOptionFlags flag, long seconds = 0) {
        return sleep(sleepMode, seconds, flag);
    }

    inline static SleepResult sleep(Spark_Sleep_TypeDef sleepMode, SleepOptionFlags flag, std::chrono::seconds s) {
        return sleep(sleepMode, flag, s.count());
    }

    inline static SleepResult sleep(long seconds) {
        return sleep(SLEEP_MODE_WLAN, seconds);
    }

    inline static SleepResult sleep(std::chrono::seconds s) {
        return sleep(s.count());
    }

    inline static SleepResult sleep(uint16_t wakeUpPin, InterruptMode edgeTriggerMode, long seconds = 0, SleepOptionFlags flag = SLEEP_NETWORK_OFF) {
        return sleepPinImpl(&wakeUpPin, 1, &edgeTriggerMode, 1, seconds, flag);
    }

    inline static SleepResult sleep(uint16_t wakeUpPin, InterruptMode edgeTriggerMode, std::chrono::seconds s, SleepOptionFlags flag = SLEEP_NETWORK_OFF) {
        return sleep(wakeUpPin, edgeTriggerMode, s.count(), flag);
    }

    inline static SleepResult sleep(uint16_t wakeUpPin, InterruptMode edgeTriggerMode, SleepOptionFlags flag, long seconds = 0) {
        return sleep(wakeUpPin, edgeTriggerMode, seconds, flag);
    }

    inline static SleepResult sleep(uint16_t wakeUpPin, InterruptMode edgeTriggerMode, SleepOptionFlags flag, std::chrono::seconds s) {
        return sleep(wakeUpPin, edgeTriggerMode, flag, s.count());
    }

    /*
     * wakeup pins: std::initializer_list<hal_pin_t>
     * trigger mode: single InterruptMode
     */
    inline static SleepResult sleep(std::initializer_list<hal_pin_t> pins, InterruptMode edgeTriggerMode, long seconds = 0, SleepOptionFlags flag = SLEEP_NETWORK_OFF) {
        // This will only work in C++14
        // static_assert(pins.size() > 0, "Provided pin list is empty");
        return sleepPinImpl(pins.begin(), pins.size(), &edgeTriggerMode, 1, seconds, flag);
    }

    inline static SleepResult sleep(std::initializer_list<hal_pin_t> pins, InterruptMode edgeTriggerMode, std::chrono::seconds s, SleepOptionFlags flag = SLEEP_NETWORK_OFF) {
        return sleep(pins, edgeTriggerMode, s.count(), flag);
    }

    inline static SleepResult sleep(std::initializer_list<hal_pin_t> pins, InterruptMode edgeTriggerMode, SleepOptionFlags flag, long seconds = 0) {
        return sleep(pins, edgeTriggerMode, seconds, flag);
    }

    inline static SleepResult sleep(std::initializer_list<hal_pin_t> pins, InterruptMode edgeTriggerMode, SleepOptionFlags flag, std::chrono::seconds s) {
        return sleep(pins, edgeTriggerMode, flag, s.count());
    }

    /*
     * wakeup pins: std::initializer_list<hal_pin_t>
     * trigger mode: std::initializer_list<InterruptMode>
     */
    inline static SleepResult sleep(std::initializer_list<hal_pin_t> pins, std::initializer_list<InterruptMode> edgeTriggerMode, long seconds = 0, SleepOptionFlags flag = SLEEP_NETWORK_OFF) {
        // This will only work in C++14
        // static_assert(pins.size() > 0, "Provided pin list is empty");
        // static_assert(edgeTriggerMode.size() > 0, "Provided InterruptMode list is empty");
        return sleepPinImpl(pins.begin(), pins.size(), edgeTriggerMode.begin(), edgeTriggerMode.size(), seconds, flag);
    }

    inline static SleepResult sleep(std::initializer_list<hal_pin_t> pins, std::initializer_list<InterruptMode> edgeTriggerMode, std::chrono::seconds s, SleepOptionFlags flag = SLEEP_NETWORK_OFF) { 
        return sleep(pins, edgeTriggerMode, s.count(), flag);
    }

    inline static SleepResult sleep(std::initializer_list<hal_pin_t> pins, std::initializer_list<InterruptMode> edgeTriggerMode, SleepOptionFlags flag, long seconds = 0) {
        return sleep(pins, edgeTriggerMode, seconds, flag);
    }

    inline static SleepResult sleep(std::initializer_list<hal_pin_t> pins, std::initializer_list<InterruptMode> edgeTriggerMode, SleepOptionFlags flag, std::chrono::seconds s) {
        return sleep(pins, edgeTriggerMode, flag, s.count());
    }

    /*
     * wakeup pins: hal_pin_t* + size_t
     * trigger mode: single InterruptMode
     */
    inline static SleepResult sleep(const hal_pin_t* pins, size_t pinsSize, InterruptMode edgeTriggerMode, long seconds = 0, SleepOptionFlags flag = SLEEP_NETWORK_OFF) {
        return sleepPinImpl(pins, pinsSize, &edgeTriggerMode, 1, seconds, flag);
    }

    inline static SleepResult sleep(const hal_pin_t* pins, size_t pinsSize, InterruptMode edgeTriggerMode, std::chrono::seconds s, SleepOptionFlags flag = SLEEP_NETWORK_OFF) {
        return sleep(pins, pinsSize, edgeTriggerMode, s.count(), flag);
    }

    inline static SleepResult sleep(const hal_pin_t* pins, size_t pinsSize, InterruptMode edgeTriggerMode, SleepOptionFlags flag, long seconds = 0) {
        return sleep(pins, pinsSize, edgeTriggerMode, seconds, flag);
    }

    inline static SleepResult sleep(const hal_pin_t* pins, size_t pinsSize, InterruptMode edgeTriggerMode, SleepOptionFlags flag, std::chrono::seconds s) {
        return sleep(pins, pinsSize, edgeTriggerMode, flag, s.count());
    }

    /*
     * wakeup pins: hal_pin_t* + size_t
     * trigger mode: InterruptMode* + size_t
     */
    inline static SleepResult sleep(const hal_pin_t* pins, size_t pinsSize, const InterruptMode* edgeTriggerMode, size_t edgeTriggerModeSize, long seconds = 0, SleepOptionFlags flag = SLEEP_NETWORK_OFF) {
        return sleepPinImpl(pins, pinsSize, edgeTriggerMode, edgeTriggerModeSize, seconds, flag);
    }

    inline static SleepResult sleep(const hal_pin_t* pins, size_t pinsSize, const InterruptMode* edgeTriggerMode, size_t edgeTriggerModeSize, std::chrono::seconds s, SleepOptionFlags flag = SLEEP_NETWORK_OFF) {
        return sleep(pins, pinsSize, edgeTriggerMode, edgeTriggerModeSize, s.count(), flag);
    }

    inline static SleepResult sleep(const hal_pin_t* pins, size_t pinsSize, const InterruptMode* edgeTriggerMode, size_t edgeTriggerModeSize, SleepOptionFlags flag, long seconds = 0) {
        return sleep(pins, pinsSize, edgeTriggerMode, edgeTriggerModeSize, seconds, flag);
    }

    inline static SleepResult sleep(const hal_pin_t* pins, size_t pinsSize, const InterruptMode* edgeTriggerMode, size_t edgeTriggerModeSize, SleepOptionFlags flag, std::chrono::seconds s) {
        return sleep(pins, pinsSize, edgeTriggerMode, edgeTriggerModeSize, flag, s.count());
    }

    static String deviceID(void) {
        return spark_deviceID();
    }

    static uint16_t buttonPushed(uint8_t button = 0) {
        return system_button_pushed_duration(button, nullptr);
    }

    template<typename T>
    static SystemEventSubscription on(system_event_t events, void (T::*handler)(system_event_t, int, void*), T* instance) {
        using namespace std::placeholders;
        auto callback = std::bind(handler, instance, _1, _2, _3);
        auto wrapper = [callback](system_event_t event, int data, void* pointer) {
            callback(event, data, pointer);
        };
        return on(events, wrapper);
    }

    template<typename T>
    static SystemEventSubscription on(system_event_t events, void (T::*handler)(system_event_t, int), T* instance) {
        using namespace std::placeholders;
        auto callback = std::bind(handler, instance, _1, _2);
        auto wrapper = [callback](system_event_t event, int data, void* pointer) {
            callback(event, data);
        };
        return on(events, wrapper);
    }

    template<typename T>
    static SystemEventSubscription on(system_event_t events, void (T::*handler)(system_event_t), T* instance) {
        using namespace std::placeholders;
        auto callback = std::bind(handler, instance, _1);
        auto wrapper = [callback](system_event_t event, int data, void* pointer) {
            callback(event);
        };
        return on(events, wrapper);
    }

    template<typename T>
    static SystemEventSubscription on(system_event_t events, void (T::*handler)(), T* instance) {
        auto callback = std::bind(handler, instance);
        auto wrapper = [callback](system_event_t event, int data, void* pointer) {
            callback();
        };
        return on(events, wrapper);
    }

    static SystemEventSubscription on(system_event_t events, std::function<void(system_event_t, int)> handler) {
        auto wrapper = [handler](system_event_t events, int data, void* pointer) {
            handler(events, data);
        };
        return on(events, wrapper);
    }

    static SystemEventSubscription on(system_event_t events, std::function<void(system_event_t)> handler) {
        auto wrapper = [handler](system_event_t events, int data, void* pointer) {
            handler(events);
        };
        return on(events, wrapper);
    }

    static SystemEventSubscription on(system_event_t events, std::function<void()> handler) {
        auto wrapper = [handler](system_event_t events, int data, void* pointer) {
            handler();
        };
        return on(events, wrapper);
    }

    using SystemOnThreeArgumentFuncPtr = void(system_event_t, int, void*);
    template <typename F, std::enable_if_t<!std::is_assignable<SystemOnThreeArgumentFuncPtr*&, F>::value &&
            std::is_convertible<F, std::function<SystemOnThreeArgumentFuncPtr>>::value, bool> = true>
    static SystemEventSubscription on(system_event_t events, F handler) {
        return on(events, std::function<SystemOnThreeArgumentFuncPtr>(handler));
    }

    // All the above System.on() methods delegate to this overload which allocates
    // a wrapper callable on heap.
    static SystemEventSubscription on(system_event_t events, std::function<SystemOnThreeArgumentFuncPtr> handler) {
        // Copy the function object to heap
        SystemEventSubscription sub;
        SystemEventContext context = {};
        context.version = SYSTEM_EVENT_CONTEXT_VERSION;
        context.size = sizeof(context);

        auto wrapper = new std::function<void(system_event_t, int, void*)>(handler);
        if (!wrapper) {
            return sub;
        }
        context.callable = wrapper;
        context.destructor = [](void* callable) -> void {
            auto callableWrapper = reinterpret_cast<decltype(wrapper)>(callable);
            delete callableWrapper;
        };
        auto r = system_subscribe_event(events, subscribedEventHandler, &context);
        if (r) {
            delete wrapper;
        } else {
            sub = SystemEventSubscription(events, context.callable);
        }
        return sub;
    }

    // This overload takes non-capturing lambdas or C function pointers, which do not require a wrapper.
    // It exists for compatibility with System.off() that takes a C function pointer as an argument
    template <typename F, std::enable_if_t<std::is_assignable<SystemOnThreeArgumentFuncPtr*&, F>::value, bool> = true>
    static SystemEventSubscription on(system_event_t events, F&& handler) {
        SystemEventSubscription sub;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress"
#pragma GCC diagnostic ignored "-Wnonnull-compare"
        if (!handler) {
            return sub;
        }
#pragma GCC diagnostic pop
        auto callable = static_cast<SystemOnThreeArgumentFuncPtr*>(handler);
        SystemEventContext context = {};
        context.version = SYSTEM_EVENT_CONTEXT_VERSION;
        context.size = sizeof(context);
        context.callable = (void*)callable;
        auto r = system_subscribe_event(events, subscribedEventHandlerCompat, &context);
        if (!r) {
            sub = SystemEventSubscription(events, context.callable);
        }
        return sub;
    }

    __attribute__((deprecated("Use System.off(const SystemEventSubscription&)")))
    static void off(void(*handler)(system_event_t, int, void*)) {
        if (!handler) {
            return;
        }
        SystemEventContext context = {};
        context.version = SYSTEM_EVENT_CONTEXT_VERSION;
        context.size = sizeof(context);
        context.callable = (void*)handler;
        system_unsubscribe_event(all_events, nullptr, &context);
    }

    static void off(system_event_t events) {
        system_unsubscribe_event(events, nullptr, nullptr);
    }

    __attribute__((deprecated("Use System.off(const SystemEventSubscription&)")))
    static void off(system_event_t events, void(*handler)(system_event_t, int, void*)) {
        SystemEventContext context = {};
        context.version = SYSTEM_EVENT_CONTEXT_VERSION;
        context.size = sizeof(context);
        context.callable = (void*)handler;
        if (handler) {
            system_unsubscribe_event(events, nullptr, &context);
        } else {
            system_unsubscribe_event(events, nullptr, nullptr);
        }
    }


    static void off(const SystemEventSubscription& subscription) {
        SystemEventContext context = {};
        context.version = SYSTEM_EVENT_CONTEXT_VERSION;
        context.size = sizeof(context);
        context.callable = subscription.getCallable();
        system_unsubscribe_event(subscription.getEvents(), nullptr, &context);
    }

    static uint32_t freeMemory();

    template<typename Condition, typename While>
    static bool waitConditionWhile(Condition _condition, While _while) {
        while (_while() && !_condition()) {
            spark_process();
        }
        return _condition();
    }

    template<typename Condition>
    static bool waitCondition(Condition _condition) {
        return waitConditionWhile(_condition, []{ return true; });
    }

    template<typename Condition>
    static bool waitCondition(Condition _condition, system_tick_t timeout) {
        const system_tick_t start = millis();
        return waitConditionWhile(_condition, [=]{ return (millis()-start)<timeout; });
    }

    bool set(hal_system_config_t config_type, const void* data, unsigned length) {
        return HAL_Set_System_Config(config_type, data, length)>=0;
    }

    bool set(hal_system_config_t config_type, const char* data) {
        return set(config_type, data, strlen(data));
    }

    inline bool featureEnabled(HAL_Feature feature) {
        if (feature == FEATURE_WARM_START) {
            return __backup_ram_was_valid();
        }
        return HAL_Feature_Get(feature);
    }

    inline int enableFeature(HAL_Feature feature) {
        return HAL_Feature_Set(feature, true);
    }

    inline int disableFeature(HAL_Feature feature) {
        return HAL_Feature_Set(feature, false);
    }

#if Wiring_LogConfig
    bool enableFeature(LoggingFeature feature);
#endif

    String version() {
        SystemVersionInfo info = {};
        info.size = sizeof(SystemVersionInfo);

        system_version_info(&info, nullptr);
        return String(info.versionString);
    }

    uint32_t versionNumber() {
        SystemVersionInfo info = {};
        info.size = sizeof(SystemVersionInfo);

        system_version_info(&info, nullptr);
        return info.versionNumber;
    }

    SystemHardwareInfo hardwareInfo() {
        return SystemHardwareInfo();
    }

    inline void enableUpdates() {
        set_flag(SYSTEM_FLAG_OTA_UPDATE_ENABLED, true);
    }

    inline void disableUpdates() {
        set_flag(SYSTEM_FLAG_OTA_UPDATE_ENABLED, false);
    }

    inline uint8_t updatesPending() {
        return get_flag(SYSTEM_FLAG_OTA_UPDATE_PENDING) != 0;
    }

    inline uint8_t updatesEnabled() {
        return get_flag(SYSTEM_FLAG_OTA_UPDATE_ENABLED) != 0;
    }

    inline uint8_t updatesForced() {
    	return get_flag(SYSTEM_FLAG_OTA_UPDATE_FORCED) != 0;
    }

    /**
     * Get the firmware update status.
     *
     * @return A value defined by the `UpdateStatus` enum or a negative result code in case of
     *         an error.
     */
    int updateStatus() {
        return system_get_update_status(nullptr /* reserved */);
    }

    inline void enableReset() {
        set_flag(SYSTEM_FLAG_RESET_ENABLED, true);
    }

    inline void disableReset() {
        set_flag(SYSTEM_FLAG_RESET_ENABLED, false);
    }

    inline uint8_t resetEnabled() {
        return get_flag(SYSTEM_FLAG_RESET_ENABLED) != 0;
    }

    inline uint8_t resetPending() {
        return get_flag(SYSTEM_FLAG_RESET_PENDING) != 0;
    }

    inline void enable(system_flag_t flag) {
    		set_flag(flag, true);
    }

    inline void disable(system_flag_t flag) {
		set_flag(flag, false);
    }

    inline bool enabled(system_flag_t flag) const {
        return get_flag(flag) != 0;
    }

    inline int resetReason() const {
        int reason = RESET_REASON_NONE;
        HAL_Core_Get_Last_Reset_Info(&reason, nullptr, nullptr);
        return reason;
    }

    inline uint32_t resetReasonData() const {
        uint32_t data = 0;
        HAL_Core_Get_Last_Reset_Info(nullptr, &data, nullptr);
        return data;
    }

    inline WakeupReason wakeUpReason() {
        return sleepResult().reason();
    }

    inline bool wokenUpByPin() {
        return sleepResult().wokenUpByPin();
    }

    inline bool wokenUpByRtc() {
        return sleepResult().wokenUpByRtc();
    }

    inline hal_pin_t wakeUpPin() {
        return sleepResult().pin();
    }

    // FIXME: SystemSleepResult
    SleepResult sleepResult() {
        // FIXME: __once_proxy, std::get_once_mutex, std::set_once_functor_lock_ptr
        // static std::once_flag f;
        // std::call_once(f, [&]() {
        //     if (resetReason() == RESET_REASON_POWER_MANAGEMENT) {
        //         // Woken up from standby mode
        //         sleepResult_ = SleepResult(WAKEUP_REASON_PIN_OR_RTC, SYSTEM_ERROR_NONE, WKP);
        //     }
        // });
        static bool f = false;
        if (!f) {
            f = true;
            if (resetReason() == RESET_REASON_POWER_MANAGEMENT) {
                // Woken up from standby mode
                systemSleepResult_ = SystemSleepResult(SleepResult(WAKEUP_REASON_PIN_OR_RTC, SYSTEM_ERROR_NONE, WKP));
            }
        }
        return systemSleepResult_;
    }

    inline system_error_t sleepError() {
        return sleepResult().error();
    }

    void buttonMirror(hal_pin_t pin, InterruptMode mode, bool bootloader = false) const {
        HAL_Core_Button_Mirror_Pin(pin, mode, (uint8_t)bootloader, 0, nullptr);
    }

    void disableButtonMirror(bool bootloader = true) const {
        HAL_Core_Button_Mirror_Pin_Disable((uint8_t)bootloader, 0, nullptr);
    }

    // This function is similar to the global millis() but returns a 64-bit value
    static uint64_t millis() {
        return hal_timer_millis(nullptr);
    }

    static unsigned uptime() {
        return (hal_timer_millis(nullptr) / 1000);
    }

#if HAL_PLATFORM_POWER_MANAGEMENT
    int setPowerConfiguration(const particle::SystemPowerConfiguration& conf) {
        return system_power_management_set_config(conf.config(), nullptr);
    }

    particle::SystemPowerConfiguration getPowerConfiguration() const {
        hal_power_config config = {};
        config.size = sizeof(config);
        system_power_management_get_config(&config, nullptr);
        return particle::SystemPowerConfiguration(config);
    }

    int powerSource() const {
        particle::AbstractIntegerDiagnosticData::IntType val;
        const auto r = particle::AbstractIntegerDiagnosticData::get(DIAG_ID_SYSTEM_POWER_SOURCE, val);
        if (r < 0) {
            return r;
        }
        return val;
    }

    int batteryState() const {
        particle::AbstractIntegerDiagnosticData::IntType val;
        const auto r = particle::AbstractIntegerDiagnosticData::get(DIAG_ID_SYSTEM_BATTERY_STATE, val);
        if (r < 0) {
            return r;
        }
        return val;
    }

    float batteryCharge() const {
        // XXX: we could potentially simply call FuelGauge::getNormalizedSoC(),
        // however in order to exactly match the vitals values sent to the cloud we are going to use
        // diagnostic source as well.
        particle::AbstractIntegerDiagnosticData::IntType val;
        int r = particle::AbstractIntegerDiagnosticData::get(DIAG_ID_SYSTEM_BATTERY_CHARGE, val);
        if (r) {
            return -1.0f;
        }

        using SocFixedPointT = particle::FixedPointUQ<8, 8>;

        SocFixedPointT soc(static_cast<typename SocFixedPointT::type>(val));
        return soc.toFloat();
    }

#endif // HAL_PLATFORM_POWER_MANAGEMENT

    int setControlRequestFilter(SystemControlRequestAclAction defaultAction, Vector<SystemControlRequestAcl> filters = {}) {
		// Add requests and their actions to the control filter
        system_ctrl_acl default_action = (system_ctrl_acl)static_cast<typename std::underlying_type<SystemControlRequestAclAction>::type>(defaultAction);
        system_ctrl_filter* first = nullptr;
        system_ctrl_filter* current = nullptr;

        NAMED_SCOPE_GUARD(cleanup, {
            while (first) {
                auto tmp = first->next;
                free(first);
                first = tmp;
            }
        });

        for (const auto& filter: filters) {
            auto f = (system_ctrl_filter*)malloc(sizeof(system_ctrl_filter));
            if (!f) {
                 return SYSTEM_ERROR_NO_MEMORY;
            }
            memset(f, 0, sizeof(system_ctrl_filter));
            f->size = sizeof(system_ctrl_filter);
            f->req = filter.type();
            f->action = (system_ctrl_acl)static_cast<typename std::underlying_type<SystemControlRequestAclAction>::type>(filter.action());
            if (!current) {
                first = current = f;
            } else {
                current->next = f;
                current = f;
            }
        }

        auto ret = system_ctrl_set_request_filter(default_action, first, nullptr);
        if (!ret) {
            cleanup.dismiss();
        }
        return ret;
    }


    int backupRamSync() {
#if HAL_PLATFORM_BACKUP_RAM_NEED_SYNC
        return hal_backup_ram_sync(nullptr);
#else
        return SYSTEM_ERROR_NONE;
#endif
    }

#if HAL_PLATFORM_ASSETS
    /**
     * Get list of assets required by currently running application binary.
     * 
     * @return spark::Vector<ApplicationAsset> 
     */
    static spark::Vector<ApplicationAsset> assetsRequired();

    /**
     * Get list of assets currently available in asset storage.
     * 
     * @return spark::Vector<ApplicationAsset> 
     */
    static spark::Vector<ApplicationAsset> assetsAvailable();

    /**
     * Mark all currently available assets as 'handled'. Suspends execution of `System.onAssetOta()` callback
     * until a change in assets happens (e.g. application gets updated and depends on a different set of assets).
     * 
     * @param state (optional) `true` to mark assets as handled (default), `false` to mark assets as unhandled
     * @return int `0` on success, `system_error_t` error code on errors.
     */
    static int assetsHandled(bool state = true);

    /**
     * Set a callback to be executed early on boot before `setup()` but after global constructors if current
     * list of available assets has changed and has not been marked as 'handled' with `System.assetsHandled()`.
     * 
     * @param cb Callback.
     * @param context Callback context.
     * @return int `0` on success, `system_error_t` error code otherwise.
     */
    static int onAssetOta(OnAssetOtaCallback cb, void* context = nullptr);

    /**
     * Set a callback to be executed early on boot before `setup()` but after global constructors if current
     * list of available assets has changed and has not been marked as 'handled' with `System.assetsHandled()`.
     * 
     * @param cb Callback.
     * @return int `0` on success, `system_error_t` error code otherwise.
     */
    static int onAssetOta(OnAssetOtaStdFunc cb);

    /**
     * Set a callback to be executed early on boot before `setup()` but after global constructors if current
     * list of available assets has changed and has not been marked as 'handled' with `System.assetsHandled()`.
     * 
     * @tparam T Class type.
     * @param callback Class member method as callback.
     * @param instance Class instance.
     * @return int `0` on success, `system_error_t` error code otherwise. 
     */
    template <typename T>
    static int onAssetOta(void(T::*callback)(spark::Vector<ApplicationAsset> assets), T* instance) {
        return onAssetOta((callback && instance) ? std::bind(callback, instance, std::placeholders::_1) : (OnAssetOtaCallback)nullptr);
    }

    /**
     * OTA -> Ota aliases.
     * 
     */
    ///@{
    static int onAssetOTA(OnAssetOtaCallback cb, void* context = nullptr) {
        return onAssetOta(cb, context);
    }
    static int onAssetOTA(OnAssetOtaStdFunc cb) {
        return onAssetOta(cb);
    }
    template <typename T>
    static int onAssetOTA(void(T::*callback)(spark::Vector<ApplicationAsset> assets), T* instance) {
        return onAssetOTA((callback && instance) ? std::bind(callback, instance, std::placeholders::_1) : (OnAssetOtaCallback)nullptr);
    }
    ///@}
#endif // HAL_PLATFORM_ASSETS

private:
    SystemSleepResult systemSleepResult_;

    static inline uint8_t get_flag(system_flag_t flag) {
        uint8_t value = 0;
        system_get_flag(flag, &value, nullptr);
        return value;
    }

    static inline void set_flag(system_flag_t flag, uint8_t value) {
        system_set_flag(flag, value, nullptr);
    }

    static SleepResult sleepPinImpl(const uint16_t* pins, size_t pins_count, const InterruptMode* modes, size_t modes_count, long seconds, SleepOptionFlags flags);

    static void subscribedEventHandler(system_event_t events, int data, void* pointer, void* context) {
        if (!context) {
            return;
        }
        auto pContext = static_cast<const SystemEventContext*>(context);
        if (!pContext->callable) {
            return;
        }
        std::function<void(system_event_t, int, void*)>* handler = reinterpret_cast< std::function<void(system_event_t, int, void*)>* >(pContext->callable);
        if (!(*handler)) {
            return;
        }
        (*handler)(events, data, pointer);
    }

    static void subscribedEventHandlerCompat(system_event_t events, int data, void* pointer, void* context) {
        if (!context) {
            return;
        }
        auto pContext = static_cast<const SystemEventContext*>(context);
        if (!pContext->callable) {
            return;
        }
        auto handler = reinterpret_cast<void(*)(system_event_t, int, void*)>(pContext->callable);
        if (!handler) {
            return;
        }
        handler(events, data, pointer);
    }
};

extern SystemClass System;

#define SYSTEM_MODE(mode)  SystemClass SystemMode(mode);

#define SYSTEM_THREAD(state) STARTUP(system_thread_set_state(spark::feature::state, nullptr));

#define waitFor(condition, timeout) System.waitCondition([]{ return (condition)(); }, (timeout))
#define waitUntil(condition) System.waitCondition([]{ return (condition)(); })
#define waitForNot(condition, timeout) System.waitCondition([]{ return !(condition)(); }, (timeout))
#define waitUntilNot(condition) System.waitCondition([]{ return !(condition)(); })

#endif /* SPARK_WIRING_SYSTEM_H */

