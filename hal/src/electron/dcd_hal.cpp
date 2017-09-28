/*
 ******************************************************************************
 *  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.
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
 ******************************************************************************
 */

#include "dct_hal.h"
#include "dcd_flash.h"
#include "dcd_flash_impl.h"

UpdateDCD<InternalFlashStore, 16*1024, 0x8004000, 0x8008000>& dcd()
{
	static UpdateDCD<InternalFlashStore, 16*1024, 0x8004000, 0x8008000> dcd;
    return dcd;
}

const void* dct_read_app_data (uint32_t offset)
{
    return dcd().read(offset);
}

int dct_write_app_data(const void* data, uint32_t offset, uint32_t size)
{
    return dcd().write(offset, data, size);
}

void dcd_migrate_data()
{
    dcd().migrate();
}
