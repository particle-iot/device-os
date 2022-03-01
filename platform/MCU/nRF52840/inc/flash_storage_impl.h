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

#pragma once

#include "flash_mal.h"
#include "string.h"

/**
 * Implements access to the internal flash, providing the interface expected by dcd.h
 */
class InternalFlashStore
{
public:
    static int eraseSector(unsigned address)
    {
        return erase(address, 16*1024);
    }

    static int erase(unsigned address, unsigned size) {
        return (FLASH_EraseMemory(FLASH_INTERNAL, address, size) ? 0 : -1);
    }

    static int write(const unsigned offset, const void* data, const unsigned size)
    {
        int status = hal_flash_write(offset, (const uint8_t *)data, size);

        if (status == 0)
        {
            return (memcmp(dataAt(offset), data, size)) ? -1 : 0;
        }
        else
        {
        	return -1;
        }
    }

    static const uint8_t* dataAt(unsigned address)
    {
        return (const uint8_t*)address;
    }

    static int read(unsigned offset, void* data, unsigned size)
    {
        memcpy(data, dataAt(offset), size);
        return 0;
    }
};

