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

#include "stm32f10x_flash.h"
#include "string.h"

/**
 * Implements access to the internal flash, providing the interface expected by dcd.h
 */
class InternalFlashStore
{
public:
    int eraseSector(unsigned address)
    {
        FLASH_Unlock();
        FLASH_Status status = FLASH_ErasePage(address);
        FLASH_Lock();
        return status != FLASH_COMPLETE;
    }

    int write(const unsigned offset, const void* data, const unsigned size)
    {
        const uint8_t* data_ptr = (const uint8_t*)data;
        const uint8_t* end_ptr  = data_ptr+size;
        unsigned destination = offset;
        FLASH_Unlock();
        FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

        while (data_ptr < end_ptr)
        {
            FLASH_Status status;
            const int max_tries = 10;
            int tries = 0;

            if ( !(destination & 0x03) && (end_ptr - data_ptr >= 4))  // have a whole word to write
            {
                while ((FLASH_COMPLETE != (status = FLASH_ProgramWord(destination, *(const uint32_t*)data_ptr))) && (tries++ < max_tries));
                destination += 4;
                data_ptr += 4;
            }
            else if ( !(destination & 0x01) && (end_ptr - data_ptr >= 2))  // have a half word to write
            {
                while ((FLASH_COMPLETE != (status = FLASH_ProgramHalfWord(destination, *(const uint16_t*)data_ptr))) && (tries++ < max_tries));
                destination += 2;
                data_ptr += 2;
            }
            else
            {
                // Cannot program single bytes on STM32F1xx
                // Avoid programming unaligned addresses or single bytes
                return -1;
            }
        }
        FLASH_Lock();
        return (memcmp(dataAt(offset), data, size)) ? -1 : 0;
    }

    const uint8_t* dataAt(unsigned address)
    {
        return (const uint8_t*)address;
    }

    int read(unsigned offset, void* data, unsigned size)
    {
        memcpy(data, dataAt(offset), size);
        return 0;
    }
};

