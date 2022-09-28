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
#include "firmware_update.h"
#include "module_info.h"
#include "spark_protocol_functions.h"
#include "string_convert.h"
#include "system_version.h"
#include "spark_macros.h"
#include "system_network_internal.h"
#include "system_threading.h"
#include "check.h"
#include <cstdio>
#if HAL_PLATFORM_DCT
#include "dct.h"
#endif // HAL_PLATFORM_DCT
#include "check.h"

using namespace particle;
using namespace particle::system;

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
volatile uint8_t SPARK_UPDATE_PENDING_EVENT_RECEIVED = 0;

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
    set_ymodem_serial_flash_update_handler(Ymodem_Serial_Flash_Update);
    system_file_transfer_t tx;
    tx.descriptor.store = FileTransfer::Store::FIRMWARE;
    tx.stream = stream;
    return system_fileTransfer(&tx);
}

bool system_fileTransfer(system_file_transfer_t* tx, void* reserved)
{
    bool status = false;
    Stream* serialObj = tx->stream;

    if (Ymodem_Serial_Flash_Update_Handler)
    {
        status = Ymodem_Serial_Flash_Update_Handler(serialObj, tx->descriptor, NULL);
        if (status)
        {
            serialObj->println("Restarting system to apply firmware update...");
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

System_Reset_Reason pendingResetReason = RESET_REASON_UNKNOWN;

int formatOtaUpdateStatusEventData(int result, uint8_t *buf, size_t size)
{
    memset(buf, 0, size);

    BufferAppender appender(buf, size);
    appender.append("{\"r\":");

    char str[12] = {};
    snprintf(str, sizeof(str), "%d", result);

    appender.append(str);
    appender.append("}");

    return 0;
}

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

int Spark_Prepare_For_Firmware_Update(FileTransfer::Descriptor& file, uint32_t flags, void* reserved)
{
    if (file.store != FileTransfer::Store::FIRMWARE) {
        return SYSTEM_ERROR_OTA_UNSUPPORTED_MODULE;
    }
    file.file_address = HAL_OTA_FlashAddress() + file.chunk_address; // Address is relative to the OTA region
    if (!file.chunk_size) { // 0 indicates defaults
        file.chunk_size = HAL_OTA_ChunkSize();
    }
    if (!file.file_length) {
        file.file_length = HAL_OTA_FlashLength();
    }
    FirmwareUpdateFlags f;
    if (flags & 1) { // See ChunkedTransfer::handle_update_begin()
        f |= FirmwareUpdateFlag::VALIDATE_ONLY;
    }
    return FirmwareUpdate::instance()->startUpdate(file.file_length, nullptr /* fileHash */, nullptr /* partialSize */, f);
}

int Spark_Finish_Firmware_Update(FileTransfer::Descriptor& file, uint32_t flags, void* reserved)
{
    FirmwareUpdateFlags f;
    if (!(flags & protocol::UpdateFlag::SUCCESS)) {
        f |= FirmwareUpdateFlag::CANCEL;
    } else if (flags & protocol::UpdateFlag::VALIDATE_ONLY) {
        f |= FirmwareUpdateFlag::VALIDATE_ONLY;
    }
    const int r = FirmwareUpdate::instance()->finishUpdate(f);
    if (!(f & FirmwareUpdateFlag::CANCEL)) {
        if (reserved) {
            const auto buf = (uint8_t*)reserved;
            formatOtaUpdateStatusEventData(r, (uint8_t*)buf, 255 /* Hardcoded in chunked_transfer.cpp :( */);
        }
        if (!(f & FirmwareUpdateFlag::VALIDATE_ONLY)) {
            // Let's keep the original Gen 2 behavior in this compatibility API and reset the device
            // regardless of whether the update has been applied successfully or not
            system_pending_shutdown(RESET_REASON_UPDATE);
        }
    }
    return r;
}

int Spark_Save_Firmware_Chunk(FileTransfer::Descriptor& file, const uint8_t* chunk, void* reserved)
{
    return FirmwareUpdate::instance()->saveChunk((const char*)chunk, file.chunk_size,
            file.chunk_address - file.file_address, 0 /* partialSize */);
}

int system_get_update_status(void* reserved) {
    SYSTEM_THREAD_CONTEXT_SYNC(system_get_update_status(reserved));
    if (FirmwareUpdate::instance()->isRunning()) {
        return SYSTEM_UPDATE_STATUS_IN_PROGRESS;
    }
    if (!SPARK_UPDATE_PENDING_EVENT_RECEIVED) {
        return SYSTEM_UPDATE_STATUS_UNKNOWN;
    }
    uint8_t pending = 0;
    CHECK(system_get_flag(SYSTEM_FLAG_OTA_UPDATE_PENDING, &pending, nullptr /* reserved */));
    if (pending) {
        return SYSTEM_UPDATE_STATUS_PENDING;
    }
    return SYSTEM_UPDATE_STATUS_NOT_AVAILABLE;
}
