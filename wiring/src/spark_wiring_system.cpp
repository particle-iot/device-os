
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

#if HAL_PLATFORM_SLEEP20
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
    return System.systemSleepResult_;
}
#endif

SleepResult SystemClass::sleep(Spark_Sleep_TypeDef sleepMode, long seconds, SleepOptionFlags flags)
{
    int ret = system_sleep(sleepMode, seconds, flags.value(), NULL);
    System.systemSleepResult_.setError(static_cast<system_error_t>(ret));
    return SleepResult(System.systemSleepResult_);
}

SleepResult SystemClass::sleepPinImpl(const uint16_t* pins, size_t pins_count, const InterruptMode* modes, size_t modes_count, long seconds, SleepOptionFlags flags) {
    int ret = system_sleep_pins(pins, pins_count, modes, modes_count, seconds, flags.value(), nullptr);
    if (ret < 0) {
        System.systemSleepResult_.setError(static_cast<system_error_t>(ret));
    } else {
        if (ret == 0) {
            System.systemSleepResult_.setWakeupRtc();
        } else {
            System.systemSleepResult_.setWakeupPin(pins[ret - 1]);
        }
    }
    return SleepResult(System.systemSleepResult_);
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
