/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#pragma once

#include "hw_config.h"
#include "flash_device_hal.h"
#include "module_info.h"
#include "module_info_hal.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  FLASH_ACCESS_RESULT_OK             = 0,
  FLASH_ACCESS_RESULT_BADARG         = 1,
  FLASH_ACCESS_RESULT_ERROR          = 2,
  FLASH_ACCESS_RESULT_RESET_PENDING  = 3
} flash_access_result_t;

/* MAL access layer for Internal/Serial Flash Routines */
//New routines specific for BM09/BM14 flash usage
uint16_t FLASH_SectorToWriteProtect(uint8_t flashDeviceID, uint32_t startAddress);
uint16_t FLASH_SectorToErase(flash_device_t flashDeviceID, uint32_t startAddress);
bool FLASH_CheckValidAddressRange(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length);
bool FLASH_WriteProtectMemory(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length, bool protect);
bool FLASH_EraseMemory(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length);

typedef int (*copymem_fn_t)(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                            flash_device_t destinationDeviceID, uint32_t destinationAddress,
                            uint32_t length, uint8_t module_function, uint8_t flags);

int FLASH_CopyMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                     flash_device_t destinationDeviceID, uint32_t destinationAddress,
                     uint32_t length, uint8_t module_function, uint8_t flags);

bool FLASH_CompareMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                         flash_device_t destinationDeviceID, uint32_t destinationAddress,
                         uint32_t length);

bool FLASH_AddToNextAvailableModulesSlot(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                                         flash_device_t destinationDeviceID, uint32_t destinationAddress,
                                         uint32_t length, uint8_t module_function, uint8_t flags);

bool FLASH_IsFactoryResetAvailable(void);
int FLASH_AddMfgSystemModuleSlot(void);
int FLASH_UpdateModules(void (*flashModulesCallback)(bool isUpdating));

int FLASH_ModuleInfo(module_info_t* const infoOut, uint8_t flashDeviceID, uint32_t startAddress, uint32_t* infoOffset);
int FLASH_ModuleCrcSuffix(module_info_crc_t* crc, module_info_suffix_t* suffix, uint8_t flashDeviceID, uint32_t endAddress);
uint32_t FLASH_ModuleAddress(flash_device_t flashDeviceID, uint32_t startAddress);
uint32_t FLASH_ModuleLength(flash_device_t flashDeviceID, uint32_t startAddress);
uint16_t FLASH_ModuleVersion(flash_device_t flashDeviceID, uint32_t startAddress);
bool FLASH_isUserModuleInfoValid(uint8_t flashDeviceID, uint32_t startAddress, uint32_t expectedAddress);
bool FLASH_VerifyCRC32(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length);

int FLASH_Begin(flash_device_t flashDeviceID, uint32_t FLASH_Address, uint32_t imageSize);
int FLASH_Update(flash_device_t flashDeviceID, const uint8_t *pBuffer, uint32_t address, uint32_t bufferSize);
void FLASH_End(void);

bool enable_rsip_if_disabled(uint32_t address, int* is);
void disable_rsip_if_enabled(bool enabled, int is);

#ifdef __cplusplus
}
#endif
