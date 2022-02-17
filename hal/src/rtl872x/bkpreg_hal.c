/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHAN'TABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "core_hal.h"
#include "platform_system_flags.h"
#include "hw_config.h"
#include "rtl8721d.h"

const size_t HAL_BACKUP_REGISTER_NUM = 10;
static uint32_t* hal_backup_reg_location = (uint32_t*)((uintptr_t)&system_flags + sizeof(system_flags));

// NOTE: we still have an option of using actual backup registers available on RTL872X
// We made sure to free up all of them except for BKUP_REG0
int32_t HAL_Core_Backup_Register(uint32_t reg) {
    if ((reg == 0) || (reg > HAL_BACKUP_REGISTER_NUM)) {
        return -1;
    }

    return reg - 1;
}

void HAL_Core_Write_Backup_Register(uint32_t reg, uint32_t data) {
    int32_t idx = HAL_Core_Backup_Register(reg);
    if (idx != -1) {
        hal_backup_reg_location[idx] = data;
    }
}

uint32_t HAL_Core_Read_Backup_Register(uint32_t reg) {
    int32_t idx = HAL_Core_Backup_Register(reg);
    if (idx != -1) {
        return hal_backup_reg_location[idx];
    }
    return 0xffffffff;
}
