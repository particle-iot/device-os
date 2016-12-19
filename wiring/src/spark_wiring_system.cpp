
#include "core_hal.h"
#include "rtc_hal.h"
#include "rgbled.h"
#include "spark_wiring_wifi.h"
#include "spark_wiring_cloud.h"
#include "spark_wiring_logging.h"
#include "system_task.h"
#include "system_control.h"
#include "system_network.h"

#if Wiring_LogConfig
extern bool(*log_process_config_request_callback)(char*, size_t, size_t, size_t*, DataFormat);
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

void SystemClass::sleep(Spark_Sleep_TypeDef sleepMode, long seconds, SleepNetworkFlag network)
{
    system_sleep(sleepMode, seconds, network.flag(), NULL);
}

void SystemClass::sleep(uint16_t wakeUpPin, InterruptMode edgeTriggerMode, long seconds, SleepNetworkFlag network)
{
    system_sleep_pin(wakeUpPin, edgeTriggerMode, seconds, network.flag(), NULL);
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
    log_process_config_request_callback = spark::logProcessConfigRequest;
    return true;
}
#endif
