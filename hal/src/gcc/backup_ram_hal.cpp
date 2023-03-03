/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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

#include "hal_platform.h"

#if HAL_PLATFORM_BACKUP_RAM_NEED_SYNC

int hal_backup_ram_init(void) {
    return SYSTEM_ERROR_NOT_SUPPORTED;;
}

int hal_backup_ram_sync(void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;;
}

int hal_backup_ram_routine(void) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

#endif
