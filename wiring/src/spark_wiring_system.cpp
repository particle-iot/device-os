
#include "core_hal.h"
#include "rtc_hal.h"
#include "rgbled.h"
#include "spark_wiring_wifi.h"
#include "spark_wiring_cloud.h"
#include "spark_wiring_logging.h"
#include "system_task.h"
#include "system_control.h"
#include "system_network.h"
#include "spark_wiring_wifitester.h"

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

int SystemClass::sleep(const SystemSleepConfiguration& config) {
    int ret;
#ifndef SPARK_NO_CLOUD
    bool cloudAutoConnect;
#endif
    bool wifiEnabled;
    bool wifiConnect;

    if (config.sleepMode() == SystemSleepMode::NONE || !config.halCoreSleepConfig()->wakeup_sources) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if ((config.sleepMode() == SystemSleepMode::STOP && !config.wakeupSourceFeatured(HAL_CORE_WAKEUP_SOURCE_TYPE_NETWORK)) ||
         config.sleepMode() == SystemSleepMode::HIBERNATE ||
         config.sleepMode() == SystemSleepMode::SHUTDOWN ||) {
        // save the current state so it can be restored on wakeup
#ifndef SPARK_NO_CLOUD
        cloudConnect = spark_cloud_flag_auto_connect();
#endif
        wifiEnabled = !SPARK_WLAN_SLEEP;
        wifiConnect = cloudConnect || network_ready(0, 0, NULL) || network_connecting(0, 0, NULL);
        // Disconnect the cloud and the network
        network_disconnect(0, NETWORK_DISCONNECT_REASON_SLEEP, NULL);
#ifndef SPARK_NO_CLOUD
        // Clear the auto connect status
        spark_cloud_flag_disconnect();
#endif
        network_off(0, 0, 0, NULL);
    }

    // Now enter sleep mode
    ret = hal_core_sleep(config.halCoreSleepConfig(), nullptr);

    if (config.sleepMode() == SystemSleepMode::STOP && !config.wakeupSourceFeatured(HAL_CORE_WAKEUP_SOURCE_TYPE_NETWORK) {
        // Set the system flags that triggers the wifi/cloud reconnection in the background loop
        if (wifiConnect || wifiEnabled) {
           // at present, no way to get the background loop to only turn on wifi.
            SPARK_WLAN_SLEEP = 0;
        }
        if (cloudConnect) {
            spark_cloud_flag_connect();
        }
    }
    return ret;
}

SleepResult SystemClass::sleep(Spark_Sleep_TypeDef sleepMode, long seconds, SleepOptionFlags flags)
{
    int ret = system_sleep(sleepMode, seconds, flags.value(), NULL);
    System.sleepResult_ = SleepResult(WAKEUP_REASON_NONE, static_cast<system_error_t>(ret));
    return System.sleepResult_;
}

SleepResult SystemClass::sleepPinImpl(const uint16_t* pins, size_t pins_count, const InterruptMode* modes, size_t modes_count, long seconds, SleepOptionFlags flags) {
    int ret = system_sleep_pins(pins, pins_count, modes, modes_count, seconds, flags.value(), nullptr);
    System.sleepResult_ = SleepResult(ret, pins, pins_count);
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
