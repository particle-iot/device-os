
#include "core_hal.h"
#include "rtc_hal.h"
#include "rgbled.h"
#include "spark_wiring_wifi.h"
#include "spark_wiring_cloud.h"
#include "spark_wiring_logging.h"
#include "spark_wiring_wifi.h"
#include "spark_wiring_cellular.h"
#include "spark_wiring_mesh.h"
#include "spark_wiring_ethernet.h"
#include "system_task.h"
#include "system_control.h"
#include "system_network.h"
#include "spark_wiring_wifitester.h"

using namespace spark;

#if Wiring_LogConfig
extern void(*log_process_ctrl_request_callback)(ctrl_request* req);
#endif

SystemClass System;

void SystemClass::factoryReset(void)
{
    //This method will work only if the Core is supplied
    //with the latest version of Bootloader
    HAL_Core_Factory_Reset();
}

void SystemClass::dfu(bool persist)
{
    // true  - DFU mode persist if firmware upgrade is not completed
    // false - Briefly enter DFU bootloader mode (works with latest bootloader only )
    //         Subsequent reset or power off-on will execute normal firmware
    HAL_Core_Enter_Bootloader(persist);
}

void SystemClass::reset(void)
{
    reset(0);
}

void SystemClass::reset(uint32_t data)
{
    HAL_Core_System_Reset_Ex(RESET_REASON_USER, data, nullptr);
}

SystemSleepResult SystemClass::sleep(const SystemSleepConfiguration& config) {
    if (!config.valid()) {
        LOG(ERROR, "System sleep configuration is invalid.");
        System.systemSleepResult_ = SystemSleepResult(SYSTEM_ERROR_INVALID_ARGUMENT);
    } else {
        SystemSleepResult result;
        int ret = system_sleep_ext(config.halConfig(), result.halWakeupSource(), nullptr);
        result.setError(static_cast<system_error_t>(ret));
        System.systemSleepResult_ = result;
    }
    System.toSleepResult();
    return System.systemSleepResult_;
}

SleepResult SystemClass::sleep(Spark_Sleep_TypeDef sleepMode, long seconds, SleepOptionFlags flags)
{
    if (sleepMode == SLEEP_MODE_WLAN) {
        // Just for backward compatible.
        int ret = system_sleep(sleepMode, seconds, flags.value(), NULL);
        System.sleepResult_ = SleepResult(WAKEUP_REASON_NONE, static_cast<system_error_t>(ret));
        return System.sleepResult_;
    } else {
        SystemSleepConfiguration config;
        // For backward compatibility. This API will make device enter HIBERNATE mode.
        config.mode(SystemSleepMode::HIBERNATE);

        if (seconds > 0) {
            config.duration(seconds * 1000);
        }

        if (sleepMode == SLEEP_MODE_DEEP && (flags.value() & SYSTEM_SLEEP_FLAG_NETWORK_STANDBY)) {
#if HAL_PLATFORM_CELLULAR
            config.network(Cellular);
#endif
#if HAL_PLATFORM_WIFI
            config.network(WiFi);
#endif
#if HAL_PLATFORM_MESH
            config.network(Mesh);
#endif
#if HAL_PLATFORM_ETHERNET
            config.network(Ethernet);
#endif
        }

        if (flags.value() & SYSTEM_SLEEP_FLAG_NO_WAIT) {
            config.wait(SystemSleepWait::NO_WAIT);
        }

        if (!(flags.value() & SYSTEM_SLEEP_FLAG_DISABLE_WKP_PIN)) {
            config.gpio(WKP, RISING);
        }

        System.sleep(config);
        return System.sleepResult_;
    }
}

SleepResult SystemClass::sleepPinImpl(const uint16_t* pins, size_t pins_count, const InterruptMode* modes, size_t modes_count, long seconds, SleepOptionFlags flags) {
    SystemSleepConfiguration config;
    // For backward compatibility. This API will make device enter STOP mode.
    config.mode(SystemSleepMode::STOP);

    if (seconds > 0) {
        config.duration(seconds * 1000);
    }

    for (size_t i = 0; i < pins_count; i++) {
        config.gpio(pins[i], ((i < modes_count) ? modes[i] : modes[modes_count - 1]));
    }

    if (flags.value() & SYSTEM_SLEEP_FLAG_NETWORK_STANDBY) {
#if HAL_PLATFORM_CELLULAR
        config.network(Cellular);
#endif
#if HAL_PLATFORM_WIFI
        config.network(WiFi);
#endif
#if HAL_PLATFORM_MESH
        config.network(Mesh);
#endif
#if HAL_PLATFORM_ETHERNET
        config.network(Ethernet);
#endif
    }

    if (flags.value() & SYSTEM_SLEEP_FLAG_NO_WAIT) {
        config.wait(SystemSleepWait::NO_WAIT);
    }

    if (!(flags.value() & SYSTEM_SLEEP_FLAG_DISABLE_WKP_PIN)) {
        config.gpio(WKP, RISING);
    }

    System.sleep(config);
    return System.sleepResult_;
}

uint32_t SystemClass::freeMemory()
{
    runtime_info_t info;
    memset(&info, 0, sizeof(info));
    info.size = sizeof(info);
    HAL_Core_Runtime_Info(&info, NULL);
    return info.freeheap;
}

#if Wiring_LogConfig
bool SystemClass::enableFeature(LoggingFeature) {
    log_process_ctrl_request_callback = spark::logProcessControlRequest;
    return true;
}
#endif

bool SystemClass::enableFeature(const WiFiTesterFeature feature) {
    WiFiTester::init();
    return true;
}

SleepResult::SleepResult(int ret, const pin_t* pins, size_t pinsSize) {
    if (ret > 0) {
        // pin
        --ret;
        if ((size_t)ret < pinsSize) {
            pin_ = pins[ret];
            reason_ = WAKEUP_REASON_PIN;
            err_ = SYSTEM_ERROR_NONE;
        }
    } else if (ret == 0) {
        reason_ = WAKEUP_REASON_RTC;
        err_ = SYSTEM_ERROR_NONE;
    } else {
        err_ = static_cast<system_error_t>(ret);
    }
}

WakeupReason SleepResult::reason() const {
    return reason_;
}

bool SleepResult::wokenUpByRtc() const {
    return reason_ == WAKEUP_REASON_RTC || reason_ == WAKEUP_REASON_PIN_OR_RTC;
}

bool SleepResult::wokenUpByPin() const {
    return reason_ == WAKEUP_REASON_PIN || reason_ == WAKEUP_REASON_PIN_OR_RTC;
}

pin_t SleepResult::pin() const {
    return pin_;
}

bool SleepResult::rtc() const {
    return wokenUpByRtc();
}

system_error_t SleepResult::error() const {
    return err_;
}
