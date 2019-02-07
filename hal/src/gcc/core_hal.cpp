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
#include <iostream>
#include "filesystem.h"
#include "service_debug.h"
#include "device_config.h"
#include "hal_platform.h"
#include "interrupts_hal.h"
#include <boost/crc.hpp>  // for boost::crc_32_type
#include <sstream>
#include <iomanip>
#include "system_error.h"

#include "eeprom_file.h"
#include "eeprom_hal.h"

using std::cout;

static LoggerOutputLevel log_level = NO_LOG_LEVEL;

void setLoggerLevel(LoggerOutputLevel level)
{
    log_level = level;
}

void log_message_callback(const char *msg, int level, const char *category, const LogAttributes *attr, void *reserved)
{
    if (level < log_level) {
        return;
    }
    std::ostringstream strm;
    // Timestamp
    if (attr->has_time) {
        strm << std::setw(10) << std::setfill('0') << attr->time << ' ';
    }
    // Category
    if (category) {
        strm << '[' << category << "] ";
    }
    // Source info
    if (attr->has_file && attr->has_line && attr->has_function) {
        // Strip directory path
        std::string fileName(attr->file);
        size_t pos = fileName.rfind('/');
        if (pos != std::string::npos) {
            fileName = fileName.substr(pos + 1);
        }
        strm << fileName << ':' << attr->line << ", ";
        // Strip argument and return types
        std::string funcName(attr->function);
        pos = funcName.find(' ');
        if (pos != std::string::npos) {
            funcName = funcName.substr(pos + 1, funcName.find('(') - pos - 1);
        }
        strm << funcName << "(): ";
    }
    // Level
    strm << log_level_name(level, nullptr) << ": ";
    // Message
    if (msg) {
        strm << msg;
    }
    // Additional attributes
    if (attr->has_code || attr->has_details) {
        strm << " [";
        if (attr->has_code) {
            strm << "code = " << attr->code << ", ";
        }
        if (attr->has_details) {
            strm << "details = " << attr->details << ", ";
        }
        strm.seekp(-2, std::ios_base::end); // Overwrite trailing comma
        strm << "] ";
    }
    std::cout << strm.str() << std::endl;
}

void log_write_callback(const char *data, size_t size, int level, const char *category, void *reserved)
{
    if (level < log_level) {
        return;
    }
    std::cout.write(data, size);
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
    cout << buf << std::endl;
    va_end(args);
}

const char* eeprom_bin = "eeprom.bin";

#ifndef UNIT_TEST

extern "C" int main(int argc, char* argv[])
{
    log_set_callbacks(log_message_callback, log_write_callback, log_enabled_callback, nullptr);
    if (read_device_config(argc, argv)) {
    		// init the eeprom so that a file of size 0 can be used to trigger the save.
    		HAL_EEPROM_Init();
    		if (exists_file(eeprom_bin)) {
    			GCC_EEPROM_Load(eeprom_bin);
    		}
			app_setup_and_loop();
	}
    return 0;
}

class GCCStartup {
    public:
    GCCStartup() {
        HAL_Core_Config();
    }
};

static GCCStartup startup;

#endif // !defined(UNIT_TEST)

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

void HAL_Core_System_Reset_Ex(int reason, uint32_t data, void *reserved)
{
    HAL_Core_System_Reset();
}

int HAL_Core_Get_Last_Reset_Info(int *reason, uint32_t *data, void *reserved)
{
    return -1;
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

int HAL_Core_Enter_Stop_Mode_Ext(const uint16_t* pins, size_t pins_count, const InterruptMode* mode, size_t mode_count, long seconds, void* reserved)
{
    MSG("Stop mode not implemented.");
    return -1;
}

int HAL_Core_Enter_Standby_Mode(uint32_t seconds, uint32_t flags)
{
    MSG("Standby mode not implemented.");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

void HAL_Core_Execute_Standby_Mode(void)
{
    MSG("Standby mode not implemented.");
}


static uint32_t crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t
crc32(uint32_t crc, const uint8_t* buf, size_t size)
{
	const uint8_t *p = buf;
	crc = crc ^ ~0U;

	while (size--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

	return crc ^ ~0U;
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
    //return crc32(0, pBuffer, bufferSize);
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
        default:
            break;
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

void HAL_Core_Button_Mirror_Pin_Disable(uint8_t bootloader, uint8_t button, void* reserved)
{
}

void HAL_Core_Button_Mirror_Pin(uint16_t pin, InterruptMode mode, uint8_t bootloader, uint8_t button, void *reserved)
{
}

void HAL_Core_Led_Mirror_Pin(uint8_t led, pin_t pin, uint32_t flags, uint8_t bootloader, void* reserved)
{
}

void HAL_Core_Led_Mirror_Pin_Disable(uint8_t led, uint8_t bootloader, void* reserved)
{
}

static HAL_Event_Callback eventCallback = nullptr;

void HAL_Set_Event_Callback(HAL_Event_Callback callback, void* reserved) {
    eventCallback = callback;
}

void hal_notify_event(int event, int flags, void* data) {
    if (eventCallback) {
        eventCallback(event, flags, data);
    }
}
