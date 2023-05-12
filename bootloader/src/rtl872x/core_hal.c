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

#include "core_hal.h"
#include "hw_ticks.h"
#include <limits.h>
#include "rtl8721d.h"
#include "km0_km4_ipc.h"
#include "flash_mal.h"

void HAL_Delay_Microseconds(uint32_t uSec) {
    DelayUs(uSec);
}

void HAL_Core_System_Reset_Ex(int reason, uint32_t data, void *reserved) {
    HAL_Core_System_Reset();
}

int HAL_Core_Enter_Panic_Mode(void* reserved) {
    __disable_irq();
    return 0;
}

uint32_t HAL_Core_Compute_CRC32(const uint8_t *pBuffer, uint32_t bufferSize) {
    uint32_t crc32 = Compute_CRC32(pBuffer, bufferSize, NULL);
    return crc32;
}