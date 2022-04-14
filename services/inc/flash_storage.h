/**
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include <stdint.h>
#include <limits.h>
#include <stdlib.h>

class FlashStorage
{
public:
    int eraseSector(unsigned address);
    int write(unsigned offset, const void* data, unsigned size);
    int read(unsigned offset, void* data, unsigned size);

    const uint8_t* dataAt(unsigned address);
};

/**
 * A mock Flash memory storage. Emulates power failure/reset by discarding
 * writes after a certain number.
 */
template<int Base, int Sectors, int SectorSize>
class RAMFlashStorage
{
    uint8_t memory[Sectors*SectorSize];
    int write_count;
    int erase_count;

public:
    enum Errors
    {
        FLASH_OK,
        FLASH_INVALID_RANGE,
    };

    RAMFlashStorage()
    {
        for (unsigned i=0; i<sizeof(memory); i++) {
            memory[i] = rand();
        }
        write_count = INT_MAX;
        erase_count = 0;
    }

    inline bool isValidRange(unsigned address, unsigned size)
    {
        return address>=Base && (address+size-Base)<=Sectors*SectorSize;
    }

    int eraseSector(unsigned address)
    {
        if (!isValidRange(address,0))
            return FLASH_INVALID_RANGE;

       if (!write_count)
            return -1;
        write_count--;

        address -= Base;
        unsigned start = address-(address % SectorSize);
        unsigned end = start + SectorSize;
        while(start<end)
        {
            memory[start++] = 0xFF;
        }
        erase_count++;
        return 0;
    }

    int write(unsigned offset, const void* _data, unsigned size)
    {
        const uint8_t* data = (const uint8_t*)_data;
        if (!isValidRange(offset,size))
            return FLASH_INVALID_RANGE;

        unsigned start = offset-Base;
        while (size --> 0)
        {
            if (!write_count)
                return -1;
            write_count--;

            memory[start++] &= *data++;
        }
        return 0;
    }

    int read(unsigned offset, void* _data, unsigned size)
    {
        uint8_t *data = (uint8_t *)_data;
        if (!isValidRange(offset,size))
            return FLASH_INVALID_RANGE;

        unsigned start = offset-Base;
        while (size --> 0)
        {
            *data++ = memory[start++];
        }
        return 0;
    }

    const uint8_t* dataAt(unsigned offset)
    {
        if (!isValidRange(offset,0))
            return nullptr;

        return memory+(offset-Base);
    }

    void setWriteCount(int count)
    {
        write_count = count;
    }

    int getEraseCount()
    {
        return erase_count;
    }

    void resetEraseCount()
    {
        erase_count = 0;
    }

    template <typename Func>
    void discardWritesAfter(int count, Func f)
    {
        setWriteCount(count);
        f();
        setWriteCount(INT_MAX);
    }
};
