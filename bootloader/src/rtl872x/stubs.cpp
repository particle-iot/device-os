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

#include <cstdint>
#include "button_hal.h"
#include "flash_access.h"
#include "dfu_hal.h"
#include "dct_hal.h"

bool FLASH_IsFactoryResetAvailable(void) {
    return false;
}

int FLASH_UpdateModules(void (*flashModulesCallback)(bool isUpdating)) {
    return FLASH_ACCESS_RESULT_OK;
}

void HAL_DFU_USB_Init(void) {
}

bool FLASH_isUserModuleInfoValid(uint8_t flashDeviceID, uint32_t startAddress, uint32_t expectedAddress) {
    return false;
}

uint32_t FLASH_ModuleLength(flash_device_t flashDeviceID, uint32_t startAddress) {
    return 0;
}

bool FLASH_VerifyCRC32(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length) {
    return false;
}

void DFU_Check_Reset(void) {

}
