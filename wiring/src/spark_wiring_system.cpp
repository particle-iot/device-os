
#include "core_hal.h"
#include "rtc_hal.h"
#include "rgbled.h"
#include "spark_wiring_wifi.h"
#include "spark_wiring_cloud.h"
#include "spark_wiring_logging.h"
#include "system_task.h"
#include "system_control.h"
#include "system_network.h"
#include "scope_guard.h"

#if Wiring_LogConfig
extern void(*log_process_ctrl_request_callback)(ctrl_request* req);
#endif

SystemClass System;

void SystemClass::factoryReset(SystemResetFlags flags)
{
    //This method will work only if the Core is supplied
    //with the latest version of Bootloader
    system_reset(SYSTEM_RESET_MODE_FACTORY, 0, 0, flags.value(), nullptr);
}

void SystemClass::dfu(SystemResetFlags flags)
{
    system_reset(SYSTEM_RESET_MODE_DFU, 0, 0, flags.value(), nullptr);
}

void SystemClass::dfu(bool persist)
{
    // true  - DFU mode persist if firmware upgrade is not completed
    // false - Briefly enter DFU bootloader mode (works with latest bootloader only )
    //         Subsequent reset or power off-on will execute normal firmware
    dfu(persist ? RESET_PERSIST_DFU : SystemResetFlags());
}

void SystemClass::reset()
{
    // We can't simply provide a default value for the argument of reset(SystemResetFlags) because
    // the reference docs show that method used without arguments in the application watchdog example
    reset(SystemResetFlags());
}

void SystemClass::reset(SystemResetFlags flags)
{
    reset(0, flags);
}

void SystemClass::reset(uint32_t data, SystemResetFlags flags)
{
    system_reset(SYSTEM_RESET_MODE_NORMAL, RESET_REASON_USER, data, flags.value(), nullptr);
}

void SystemClass::enterSafeMode(SystemResetFlags flags)
{
    system_reset(SYSTEM_RESET_MODE_SAFE, 0, 0, flags.value(), nullptr);
}

SystemSleepResult SystemClass::sleep(const particle::SystemSleepConfiguration& config) {
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

SleepResult SystemClass::sleep(Spark_Sleep_TypeDef sleepMode, long seconds, SleepOptionFlags flags)
{
    int ret = system_sleep(sleepMode, seconds, flags.value(), NULL);
    System.systemSleepResult_ = SystemSleepResult(SleepResult(WAKEUP_REASON_NONE, static_cast<system_error_t>(ret)));
    return System.systemSleepResult_;
}

SleepResult SystemClass::sleepPinImpl(const uint16_t* pins, size_t pins_count, const InterruptMode* modes, size_t modes_count, long seconds, SleepOptionFlags flags) {
    int ret = system_sleep_pins(pins, pins_count, modes, modes_count, seconds, flags.value(), nullptr);
    System.systemSleepResult_ = SystemSleepResult(SleepResult(ret, pins, pins_count));
    return System.systemSleepResult_;
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

#if HAL_PLATFORM_ASSETS

spark::Vector<ApplicationAsset> SystemClass::assetsRequired() {
    spark::Vector<ApplicationAsset> assets;
    asset_manager_info info = {};
    info.size = sizeof(info);
    int r = asset_manager_get_info(&info, nullptr);
    if (r) {
        return assets;
    }
    SCOPE_GUARD({
        asset_manager_free_info(&info, nullptr);
    });
    for (size_t i = 0; i < info.required_count; i++) {
        asset_manager_asset* a = (asset_manager_asset*)(((uint8_t*)info.required) + info.asset_size * i);
        assets.append(ApplicationAsset(a));
    }
    return assets;
}

spark::Vector<ApplicationAsset> SystemClass::assetsAvailable() {
    spark::Vector<ApplicationAsset> assets;
    asset_manager_info info = {};
    info.size = sizeof(info);
    int r = asset_manager_get_info(&info, nullptr);
    if (r) {
        return assets;
    }
    SCOPE_GUARD({
        asset_manager_free_info(&info, nullptr);
    });
    for (size_t i = 0; i < info.available_count; i++) {
        asset_manager_asset* a = (asset_manager_asset*)(((uint8_t*)info.available) + info.asset_size * i);
        assets.append(ApplicationAsset(a));
    }
    return assets;
}

int SystemClass::assetsHandled(bool state) {
    return asset_manager_set_consumer_state(state ? ASSET_MANAGER_CONSUMER_STATE_HANDLED : ASSET_MANAGER_CONSUMER_STATE_WANT, nullptr);
}

int SystemClass::onAssetOta(OnAssetOtaCallback cb, void* context) {
    OnAssetOtaStdFunc f = [cb, context](spark::Vector<ApplicationAsset> assets) -> void {
        if (cb) {
            cb(assets, context);
        }
    };
    return onAssetOta(f);
}

int SystemClass::onAssetOta(OnAssetOtaStdFunc cb) {
    onAssetOtaHookStorage() = cb;
    asset_manager_set_notify_hook([](void* context) -> void {
        if (onAssetOtaHookStorage()) {
            onAssetOtaHookStorage()(SystemClass::assetsAvailable());
        }
    }, nullptr, nullptr);
    return 0;
}
#endif // HAL_PLATFORM_ASSET

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

hal_pin_t SleepResult::pin() const {
    return pin_;
}

bool SleepResult::rtc() const {
    return wokenUpByRtc();
}

system_error_t SleepResult::error() const {
    return err_;
}