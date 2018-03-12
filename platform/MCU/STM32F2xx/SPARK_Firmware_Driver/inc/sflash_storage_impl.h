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

#include "flash_mal.h"
#include "string.h"

/**
 * Implements access to the internal flash, providing the interface expected by dcd.h
 */
class SerialFlashStore
{
public:
    void init(void)
    {
        sFLASH_Init();
    }

    int eraseSector(unsigned address)
    {
        return !FLASH_EraseMemory(FLASH_SERIAL, address, 16384);
    }

    int write(const unsigned offset, const void* data, const unsigned size)
    {
        const uint8_t *ptr = (const uint8_t*)data;
        uint8_t temp;
        unsigned addr = offset, cnt = size;

    	sFLASH_WriteBuffer((const uint8_t*)data, (uint32_t)offset, size);

    	while (cnt > 0)
    	{
    	    sFLASH_ReadBuffer(&temp, addr, 1);
    	    if (temp != *ptr) return -1;
    	    ptr++;
    	    addr++;
    	    cnt--;
    	}

    	return 0;
    }

    void dataAt(unsigned address, const void *buffer, unsigned size)
    {
    	sFLASH_ReadBuffer((unsigned char*)buffer, address, size);
    }

    int read(unsigned offset, void* data, unsigned size)
    {
    	sFLASH_ReadBuffer((unsigned char*)data, offset, size);
        return 0;
    }
};

