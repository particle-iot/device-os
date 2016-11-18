/**
 ******************************************************************************
 * @file    core_hal.cpp
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    29-Sept-2014
 * @brief
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

#include "core_hal.h"
#include "deviceid_hal.h"
#include "core_msg.h"
#include "debug.h"
#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <csignal>
#include <iostream>
#include "filesystem.h"
#include "service_debug.h"
#include "device_config.h"
#include "hal_platform.h"
#include "interrupts_hal.h"
#include <boost/crc.hpp>  // for boost::crc_32_type
#include <sstream>
#include <iomanip>
#include <thread>
#define NAMESPACE_WPI_PINMODE
#include "wiringPi.h"
#include "gpio_hal.h"

const auto graceful_exit_time = std::chrono::seconds(3);

using std::cerr;

static LoggerOutputLevel log_level = NO_LOG_LEVEL;

void setLoggerLevel(LoggerOutputLevel level)
{
    log_level = level;
}

void log_message_callback(const char *msg, int level, const char *category, uint32_t time, const char *file, int line,
        const char *func, void *reserved)
{
    if (level < log_level) {
        return;
    }
    std::ostringstream strm;
    // Timestamp
    strm << std::setw(10) << std::setfill('0') << time << ' ';
    // Category (optional)
    if (category && category[0]) {
        strm << category << ": ";
    }
    // Source info (optional)
    if (file && func) {
        strm << file << ':' << line << ", ";
        // Strip argument and return types for better readability
        std::string funcName(func);
        const size_t pos = funcName.find(' ');
        if (pos != std::string::npos) {
            funcName = funcName.substr(pos + 1, funcName.find('(') - pos - 1);
        }
        strm << funcName << "(): ";
    }
    // Level
    strm << log_level_name(level, nullptr) << ": ";
    // Message
    strm << msg;
    std::cerr << strm.str() << std::endl;
}

void log_write_callback(const char *data, size_t size, int level, const char *category, void *reserved)
{
    if (level < log_level) {
        return;
    }
    std::cerr.write(data, size);
}

int log_enabled_callback(int level, const char *category, void *reserved)
{
    return (level >= log_level);
}

void core_log(const char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    char buf[2048];
    vsnprintf(buf, 2048, msg, args);
    cerr << buf << std::endl;
    va_end(args);
}

void makePinsInput() {
    for (pin_t pin = 0; pin < TOTAL_PINS; pin++) {
        HAL_Pin_Mode(pin, INPUT);
    }
}

std::thread forceQuitThread;
void quit_gracefully(int signal) {
    // Terminal main app loop
    signal_handler(signal);

    forceQuitThread = std::thread([]{
        std::this_thread::sleep_for(graceful_exit_time);
        makePinsInput();
        std::raise(SIGQUIT);
    });
}

extern "C" int main(int argc, char* argv[])
{
    std::signal(SIGTERM, quit_gracefully);
    std::signal(SIGINT, quit_gracefully);
    log_set_callbacks(log_message_callback, log_write_callback, log_enabled_callback, nullptr);
    if (read_device_config(argc, argv)) {
        app_setup_and_loop();
    }
    return 0;
}

class RPiStartup {
    public:
    RPiStartup() {
        if (geteuid () != 0) {
            std::cerr << "Firmware must run as root. Run again with sudo\n";
            exit(1);
        }

        HAL_Core_Config();
    }
};

static RPiStartup startup;

inline char* concat_nibble(char* result, uint8_t nibble)
{
    char hex_digit = nibble + 48;
    if (57 < hex_digit)
        hex_digit += 39;
    *result++ = hex_digit;
    return result;
}

char* bytes2hex(const uint8_t* buf, char* result, unsigned len)
{
    for (unsigned i = 0; i < len; ++i)
    {
        result = concat_nibble(result, (buf[i] >> 4));
        result = concat_nibble(result, (buf[i] & 0xF));
    }
    return result;
}

/*******************************************************************************
 * Function Name  : HAL_Core_Config.
 * Description    : Called in startup routine, before calling C++ constructors.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_Core_Config(void)
{
    wiringPiSetupGpio();
    // Put pins in a safe state now and at exit
    atexit(makePinsInput);
    makePinsInput();
}

bool HAL_Core_Mode_Button_Pressed(uint16_t pressedMillisDuration)
{
    return false;
}

void HAL_Core_Mode_Button_Reset(void)
{
}

void HAL_Core_System_Reset(void)
{
    exit(0);
}

void HAL_Core_Factory_Reset(void)
{
    MSG("Factory reset not implemented.");
}

void HAL_Core_Enter_Safe_Mode(void* reserved)
{
    MSG("Enter safe mode not implemented.");
}


void HAL_Core_Enter_Bootloader(void)
{
    MSG("Enter bootloader not implemented.");
}

void HAL_Core_Enter_Stop_Mode(uint16_t wakeUpPin, uint16_t edgeTriggerMode, long seconds)
{
    MSG("Stop mode not implemented.");
}

void HAL_Core_Execute_Stop_Mode(void)
{
    MSG("Stop mode not implemented.");
}

void HAL_Core_Enter_Standby_Mode(void)
{
    MSG("Standby mode not implemented.");
}

void HAL_Core_Execute_Standby_Mode(void)
{
    MSG("Standby mode not implemented.");
}

/**
 * @brief  Computes the 32-bit CRC of a given buffer of byte data.
 * @param  pBuffer: pointer to the buffer containing the data to be computed
 * @param  BufferSize: Size of the buffer to be computed
 * @retval 32-bit CRC
 */
uint32_t HAL_Core_Compute_CRC32(const uint8_t *pBuffer, uint32_t bufferSize)
{
	boost::crc_32_type  result;
	result.process_bytes(pBuffer, bufferSize);
	return result.checksum();
}

// todo find a technique that allows accessor functions to be inlined while still keeping
// hardware independence.
bool HAL_watchdog_reset_flagged()
{
    return false;
}

void HAL_Notify_WDT()
{
}

void HAL_Core_Init(void)
{
}

void HAL_Bootloader_Lock(bool lock)
{
}

uint16_t HAL_Bootloader_Get_Flag(BootloaderFlag flag)
{
	if (flag==BOOTLOADER_FLAG_STARTUP_MODE)
		return 0xFF;
    return 0xFFFF;
}

void HAL_Core_Enter_Bootloader(bool persist)
{
}

uint32_t HAL_Core_Runtime_Info(runtime_info_t* info, void* reserved)
{
    return -1;
}

unsigned HAL_Core_System_Clock(HAL_SystemClock clock, void* reserved)
{
    return 1;
}

int HAL_Set_System_Config(hal_system_config_t config_item, const void* data, unsigned length)
{
    return -1;
}

int HAL_Feature_Set(HAL_Feature feature, bool enabled)
{
    return -1;
}

bool HAL_Feature_Get(HAL_Feature feature)
{
    switch (feature)
    {
        case FEATURE_CLOUD_UDP:
        {
        		uint8_t value = false;
#if HAL_PLATFORM_CLOUD_UDP
        		value = (deviceConfig.get_protocol()==PROTOCOL_DTLS);
#endif
        		return value;
        }
    }
    return false;
}

#if HAL_PLATFORM_CLOUD_UDP

#include "dtls_session_persist.h"
SessionPersistDataOpaque session;

int HAL_System_Backup_Save(size_t offset, const void* buffer, size_t length, void* reserved)
{
    if (offset==0 && length==sizeof(SessionPersistDataOpaque))
    {
        memcpy(&session, buffer, length);
        return 0;
    }
    return -1;
}

int HAL_System_Backup_Restore(size_t offset, void* buffer, size_t max_length, size_t* length, void* reserved)
{
    if (offset==0 && max_length>=sizeof(SessionPersistDataOpaque) && session.size==sizeof(SessionPersistDataOpaque))
    {
        *length = sizeof(SessionPersistDataOpaque);
        memcpy(buffer, &session, sizeof(session));
        return 0;
    }
    return -1;
}


#else

int HAL_System_Backup_Save(size_t offset, const void* buffer, size_t length, void* reserved)
{
    return -1;
}

int HAL_System_Backup_Restore(size_t offset, void* buffer, size_t max_length, size_t* length, void* reserved)
{
    return -1;
}

#endif

int32_t HAL_Core_Backup_Register(uint32_t BKP_DR)
{
    return -1;
}

void HAL_Core_Write_Backup_Register(uint32_t BKP_DR, uint32_t Data)
{
}

uint32_t HAL_Core_Read_Backup_Register(uint32_t BKP_DR)
{
    return 0xFFFFFFFF;
}
