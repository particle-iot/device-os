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

void hal_button_init_ext() {
}

uint8_t hal_button_is_pressed(hal_button_t button) {
    return 0;
}

bool FLASH_IsFactoryResetAvailable(void) {
    return false;
}

uint16_t hal_button_get_pressed_time(hal_button_t button) {
    return 0;
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

int dct_read_app_data_copy(uint32_t offset, void* ptr, size_t size) {
    return -1;
}

void Set_RGB_LED_Values(uint16_t r, uint16_t g, uint16_t b) {
}

void Get_RGB_LED_Values(uint16_t* values) {
}

void Set_User_LED(uint8_t state) {
}

uint16_t Get_RGB_LED_Max_Value(void) {
    return 0;
}