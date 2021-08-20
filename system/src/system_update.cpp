/**
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include <stddef.h>
#include "ota_flash_hal.h"
#include "core_hal.h"
#include "delay_hal.h"
#include "system_event.h"
#include "system_update.h"
#include "system_cloud_internal.h"
#include "system_network.h"
#include "system_ymodem.h"
#include "system_task.h"
#include "module_info.h"
#include "spark_protocol_functions.h"
#include "string_convert.h"
#include "system_version.h"
#include "spark_macros.h"
#include "system_network_internal.h"
#include "system_threading.h"
#include <cstdio>
#if HAL_PLATFORM_DCT
#include "dct.h"
#endif // HAL_PLATFORM_DCT
#include "check.h"

using namespace particle;

#ifdef START_DFU_FLASHER_SERIAL_SPEED
static uint32_t start_dfu_flasher_serial_speed = START_DFU_FLASHER_SERIAL_SPEED;
#endif
#ifdef START_YMODEM_FLASHER_SERIAL_SPEED
static uint32_t start_ymodem_flasher_serial_speed = START_YMODEM_FLASHER_SERIAL_SPEED;
#endif

ymodem_serial_flash_update_handler Ymodem_Serial_Flash_Update_Handler = NULL;

// TODO: Use a single state variable instead of SPARK_CLOUD_XXX flags
volatile uint8_t SPARK_CLOUD_SOCKETED;
volatile uint8_t SPARK_CLOUD_CONNECTED;
volatile uint8_t SPARK_CLOUD_HANDSHAKE_PENDING = 0;
volatile uint8_t SPARK_CLOUD_HANDSHAKE_NOTIFY_DONE = 0;
volatile uint8_t SPARK_FLASH_UPDATE;
volatile uint32_t TimingFlashUpdateTimeout;

static_assert(SYSTEM_FLAG_OTA_UPDATE_PENDING==0, "system flag value");
static_assert(SYSTEM_FLAG_OTA_UPDATE_ENABLED==1, "system flag value");
static_assert(SYSTEM_FLAG_RESET_PENDING==2, "system flag value");
static_assert(SYSTEM_FLAG_RESET_ENABLED==3, "system flag value");
static_assert(SYSTEM_FLAG_STARTUP_LISTEN_MODE == 4, "system flag value");
static_assert(SYSTEM_FLAG_WIFITESTER_OVER_SERIAL1 == 5, "system flag value");
static_assert(SYSTEM_FLAG_PUBLISH_RESET_INFO == 6, "system flag value");
static_assert(SYSTEM_FLAG_RESET_NETWORK_ON_CLOUD_ERRORS == 7, "system flag value");
static_assert(SYSTEM_FLAG_PM_DETECTION == 8, "system flag value");
static_assert(SYSTEM_FLAG_OTA_UPDATE_FORCED == 9, "system flag value");
static_assert(SYSTEM_FLAG_MAX == 10, "system flag max value");

volatile uint8_t systemFlags[SYSTEM_FLAG_MAX] = {
    0, 1, // OTA updates pending/enabled
    0, 1, // Reset pending/enabled
    0,    // SYSTEM_FLAG_STARTUP_LISTEN_MODE
    0,    // SYSTEM_FLAG_SETUP_OVER_SERIAL1
    1,    // SYSTEM_FLAG_PUBLISH_RESET_INFO
    1,    // SYSTEM_FLAG_RESET_NETWORK_ON_CLOUD_ERRORS
    0,    // UNUSED (SYSTEM_FLAG_PM_DETECTION)
	0,	  // SYSTEM_FLAG_OTA_UPDATE_FORCED
};

const uint16_t SAFE_MODE_LISTEN = 0x5A1B;

const char* UPDATES_ENABLED_EVENT = "particle/device/updates/enabled";
const char* UPDATES_FORCED_EVENT = "particle/device/updates/forced";

const char* flag_to_string(uint8_t flag) {
    return flag ? "true" : "false";
}

void system_flag_changed(system_flag_t flag, uint8_t oldValue, uint8_t newValue)
{
    if (flag == SYSTEM_FLAG_STARTUP_LISTEN_MODE)
    {
        HAL_Core_Write_Backup_Register(BKP_DR_09, newValue ? SAFE_MODE_LISTEN : 0xFFFF);
    }
#if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
    else if (flag == SYSTEM_FLAG_PM_DETECTION) {
        hal_power_config conf = {};
        conf.size = sizeof(conf);
        int r = hal_power_load_config(&conf, nullptr);
        if (!r) {
            if (newValue) {
                conf.flags |= HAL_POWER_PMIC_DETECTION;
            } else {
                conf.flags &= ~(HAL_POWER_PMIC_DETECTION);
            }
            hal_power_store_config(&conf, nullptr);
        }
    }
#endif // HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
    else if (flag == SYSTEM_FLAG_OTA_UPDATE_ENABLED)
    {
        // publish the firmware enabled event
        spark_send_event(UPDATES_ENABLED_EVENT, flag_to_string(newValue), 60, PUBLISH_EVENT_FLAG_ASYNC|PUBLISH_EVENT_FLAG_PRIVATE, nullptr);
    }
    else if (flag == SYSTEM_FLAG_OTA_UPDATE_FORCED)
    {
        // acknowledge to the cloud that system updates are forced. It helps avoid a race condition where we might try sending firmware before the event has been received.
        spark_send_event(UPDATES_FORCED_EVENT, flag_to_string(newValue), 60, PUBLISH_EVENT_FLAG_ASYNC|PUBLISH_EVENT_FLAG_PRIVATE, nullptr);
    }
    else if (flag == SYSTEM_FLAG_OTA_UPDATE_PENDING)
    {
        if (newValue) {
            system_notify_event(firmware_update_pending, 0, nullptr, nullptr, nullptr);
            // publish an internal system event for pending updates
    	}
	}
}

/**
 * Refreshes the flag by performing the update action.
 */
int system_refresh_flag(system_flag_t flag) {
    uint8_t value;
    int result = system_get_flag(flag, &value, nullptr);
    if (!result) {
        system_flag_changed(flag, value, value);
    }
    return result;
}

int system_set_flag(system_flag_t flag, uint8_t value, void*)
{
    if (flag>=SYSTEM_FLAG_MAX)
        return -1;

    if (systemFlags[flag] != value || flag == SYSTEM_FLAG_STARTUP_LISTEN_MODE || flag == SYSTEM_FLAG_PM_DETECTION) {
        uint8_t oldValue = systemFlags[flag];
        systemFlags[flag] = value;
        system_flag_changed(flag, oldValue, value);
    }
    return 0;
}


int system_get_flag(system_flag_t flag, uint8_t* value, void*)
{
    if (flag>=SYSTEM_FLAG_MAX)
        return -1;
    if (value)
    {
        if (flag == SYSTEM_FLAG_STARTUP_LISTEN_MODE)
        {
            uint16_t reg = HAL_Core_Read_Backup_Register(BKP_DR_09);
            *value = (reg == SAFE_MODE_LISTEN);
            systemFlags[flag] = *value;
        }
#if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
        else if (flag == SYSTEM_FLAG_PM_DETECTION)
        {
            hal_power_config conf = {};
            conf.size = sizeof(conf);
            int r = hal_power_load_config(&conf, nullptr);
            if (!r) {
                *value = (conf.flags & HAL_POWER_PMIC_DETECTION);
            }
        }
#endif // HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
        else
        {
            *value = systemFlags[flag];
        }
    }
    return 0;
}


void set_ymodem_serial_flash_update_handler(ymodem_serial_flash_update_handler handler)
{
    Ymodem_Serial_Flash_Update_Handler = handler;
}

void set_start_dfu_flasher_serial_speed(uint32_t speed)
{
#ifdef START_DFU_FLASHER_SERIAL_SPEED
    start_dfu_flasher_serial_speed = speed;
#endif
}

void set_start_ymodem_flasher_serial_speed(uint32_t speed)
{
#ifdef START_YMODEM_FLASHER_SERIAL_SPEED
    start_ymodem_flasher_serial_speed = speed;
#endif
}

bool system_firmwareUpdate(Stream* stream, void* reserved)
{
#if PLATFORM_ID>2
    set_ymodem_serial_flash_update_handler(Ymodem_Serial_Flash_Update);
#endif
    system_file_transfer_t tx;
    tx.descriptor.store = FileTransfer::Store::FIRMWARE;
    tx.stream = stream;
    return system_fileTransfer(&tx);
}

bool system_fileTransfer(system_file_transfer_t* tx, void* reserved)
{
    bool status = false;
    Stream* serialObj = tx->stream;

    if (NULL != Ymodem_Serial_Flash_Update_Handler)
    {
        status = Ymodem_Serial_Flash_Update_Handler(serialObj, tx->descriptor, NULL);
        SPARK_FLASH_UPDATE = 0;
        TimingFlashUpdateTimeout = 0;

        if (status == true)
        {
            if (tx->descriptor.store==FileTransfer::Store::FIRMWARE) {
                serialObj->println("Restarting system to apply firmware update...");
                HAL_Delay_Milliseconds(100);
                system_pending_shutdown(RESET_REASON_UPDATE);
            }
        }
    }
    else
    {
        serialObj->println("Firmware update using this terminal is not supported!");
        serialObj->println("Add #include \"Ymodem/Ymodem.h\" to your sketch and try again.");
    }
    return status;
}

void system_lineCodingBitRateHandler(uint32_t bitrate)
{
// todo - ideally the system should post a reset pending event before
// resetting. This does mean the application can block entering listening mode

#ifdef START_DFU_FLASHER_SERIAL_SPEED
    if (bitrate == start_dfu_flasher_serial_speed)
    {
        network_connect_cancel(0, 1, 0, 0);
        //Reset device and briefly enter DFU bootloader mode
        System.dfu(false);
    }
#endif
#ifdef START_YMODEM_FLASHER_SERIAL_SPEED
    if (!network_listening(0, 0, NULL) && bitrate == start_ymodem_flasher_serial_speed)
    {
        network_listen(0, 0, 0);
    }
#endif
}

uint32_t timeRemaining(uint32_t start, uint32_t duration)
{
    uint32_t elapsed = HAL_Timer_Milliseconds()-start;
    return (elapsed>=duration) ? 0 : duration-elapsed;
}

void set_flag(void* flag)
{
	volatile uint8_t* p = (volatile uint8_t*)flag;
	*p = true;
}

namespace {
// FIXME: Dirty hack
bool ledIsOverridden = false;
} // namespace

int Spark_Prepare_For_Firmware_Update(FileTransfer::Descriptor& file, uint32_t flags, void* reserved)
{
    if (file.store==FileTransfer::Store::FIRMWARE)
    {
        // address is relative to the OTA region. Normally will be 0.
        file.file_address = HAL_OTA_FlashAddress() + file.chunk_address;

        // chunk_size 0 indicates defaults.
        if (file.chunk_size==0) {
            file.chunk_size = HAL_OTA_ChunkSize();
            file.file_length = HAL_OTA_FlashLength();
        }
    }
    int result = 0;
    if (System.updatesEnabled() || System.updatesForced()) {		// application event is handled asynchronously
        if (flags & 1) {
            // only check address
		}
		else {
            system_set_flag(SYSTEM_FLAG_OTA_UPDATE_PENDING, 0, nullptr);
            // FIXME: use the APIs in system_led_signal.h instead.
            // The RGB may behave weirdly if multi-threading is enabled and user application
            // also wants to control the RGB.
            ledIsOverridden = LED_RGB_IsOverRidden();
            if (!ledIsOverridden) {
                RGB.control(true);
                // Get base color used for the update process indication
                const LEDStatusData* status = led_signal_status(LED_SIGNAL_FIRMWARE_UPDATE, nullptr);
                RGB.color(status ? status->color : RGB_COLOR_MAGENTA);
            }
            SPARK_FLASH_UPDATE = 1;
            TimingFlashUpdateTimeout = 0;
            system_notify_event(firmware_update, firmware_update_begin, &file);
            HAL_FLASH_Begin(file.file_address, file.file_length, NULL);
        }
    }
    else {
    	result = 1;
    }
    return result;
}

namespace {

System_Reset_Reason pendingResetReason = RESET_REASON_UNKNOWN;

} // unnamed

void system_pending_shutdown(System_Reset_Reason reason)
{
    uint8_t was_set = false;
    system_get_flag(SYSTEM_FLAG_RESET_PENDING, &was_set, nullptr);
    if (!was_set) {
        system_set_flag(SYSTEM_FLAG_RESET_PENDING, 1, nullptr);
        pendingResetReason = reason;
        system_notify_event(reset_pending);
    }
}

inline bool canShutdown()
{
    return (System.resetPending() && System.resetEnabled());
}

void system_shutdown_if_enabled(void* data=nullptr)
{
    // shutdown if user initiated poweroff or system reset is allowed
    if (canShutdown())
    {
        if (SYSTEM_POWEROFF) {              // shutdown network module too.
            system_sleep(SLEEP_MODE_SOFTPOWEROFF, 0, 0, NULL);
        }
        else {
            system_reset(SYSTEM_RESET_MODE_NORMAL, pendingResetReason, 0, 0, nullptr);
        }
    }
}

void system_shutdown_if_needed()
{
    static bool in_shutdown = false;
    if (canShutdown() && !in_shutdown)
    {
        in_shutdown = true;
        system_notify_event(reset, 0, nullptr, system_shutdown_if_enabled);

#if PLATFORM_THREADING
        // timeout for 30 seconds. Keep the system thread pumping queue messages and the background task running
        system_tick_t start = millis();
        while (canShutdown() && (millis()-start)<30000)
        {
            // todo - find a more enapsulated way for the SystemThread to take care of re-entranly
            // doing work.
            spark_process();
            SystemThread.process();
        }
        in_shutdown = false;
        system_shutdown_if_enabled();
#endif
    }
}

int Spark_Finish_Firmware_Update(FileTransfer::Descriptor& file, uint32_t flags, void* reserved)
{
    using namespace particle::protocol;
    SPARK_FLASH_UPDATE = 0;
    TimingFlashUpdateTimeout = 0;
    //DEBUG("update finished flags=%d store=%d", flags, file.store);
    int result = SYSTEM_ERROR_UNKNOWN;

    if ((flags & (UpdateFlag::VALIDATE_ONLY | UpdateFlag::SUCCESS)) == (UpdateFlag::VALIDATE_ONLY | UpdateFlag::SUCCESS)) {
        result = HAL_FLASH_OTA_Validate(true, (module_validation_flags_t)(MODULE_VALIDATION_INTEGRITY | MODULE_VALIDATION_DEPENDENCIES_FULL), NULL);
        return result;
    }

    if (flags & UpdateFlag::SUCCESS) {    // update successful
        if (file.store==FileTransfer::Store::FIRMWARE)
        {
            result = HAL_FLASH_End(nullptr);
            system_notify_event(firmware_update, (result < 0) ? firmware_update_failed : firmware_update_complete, &file);

            // always restart for now
            if ((true || result == HAL_UPDATE_APPLIED_PENDING_RESTART) && !(flags & UpdateFlag::DONT_RESET))
            {
                system_pending_shutdown(RESET_REASON_UPDATE);
            }
        }
    }
    else
    {
        system_notify_event(firmware_update, firmware_update_failed, &file);
    }

    // FIXME: use APIs in system_led_signal.h instead
    // It might lease the control that user application just takes over.
    if (!ledIsOverridden) {
        RGB.control(false);
    }

    return result;
}

int Spark_Save_Firmware_Chunk(FileTransfer::Descriptor& file, const uint8_t* chunk, void* reserved)
{
    TimingFlashUpdateTimeout = 0;
    int result = -1;
    system_notify_event(firmware_update, firmware_update_progress, &file);
    if (file.store==FileTransfer::Store::FIRMWARE)
    {
        result = HAL_FLASH_Update(chunk, file.chunk_address, file.chunk_size, NULL);
        // FIXME: use APIs in system_led_signal.h instead
        if (!ledIsOverridden) {
            LED_Toggle(LED_RGB);
        }
    }
    return result;
}
