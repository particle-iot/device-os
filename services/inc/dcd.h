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

#include <stddef.h>
#include <stdint.h>

/**
 * Emulates rewritable storage using two flash blocks.
 */

template <typename Store, unsigned sectorSize, unsigned DCD1, unsigned DCD2>
class DCD
{
public:
    using Address = unsigned;
    using Data = uint8_t*;
    using Result = int;
    using Sector = uint8_t;

    enum Errors
    {
        DCD_SUCCESS,
        DCD_INVALID_OFFSET,
        DCD_INVALID_LENGTH,
    };

    Store store;

    struct Header
    {
        static const uint32_t WATERMARK = 0x1E1C279A;   // 9A271C1E
        static const uint32_t SEAL_INIT = 0xFFFFFFFF;
        static const uint32_t SEAL_VALID = 0xEDA15E00;  // 5EA1ED;
        static const uint32_t SEAL_CLEARED = 0;

        uint32_t watermark;
        uint32_t seal;

        bool isValid() const
        {
            return watermark==WATERMARK && seal==SEAL_VALID;
        }

        void initialize()
        {
            watermark = WATERMARK;
            seal = SEAL_INIT;
        }

        void make_valid()
        {
            watermark = WATERMARK;
            seal = SEAL_VALID;
        }

        void make_invalid()
        {
            watermark = WATERMARK;
            seal = SEAL_CLEARED;
        }
    };

    static const Sector Sector_0 = 0;
    static const Sector Sector_1 = 1;

    const Address Length = sectorSize-sizeof(Header);

private:
    inline Address addressOf(Sector sector)
    {
        return sector==Sector_0 ? DCD1 : DCD2;
    }

    bool requiresErase(Address offset)
    {
        const uint8_t* test = store.dataAt(offset);
        const uint8_t* end = test+sectorSize;

        while (test!=end)
        {
            if (*test++!=0xFF)
                return true;
        }
        return false;
    }

    Result erase(Sector sector)
    {
        Result result = 0;
        Address offset = addressOf(sector);
        if (requiresErase(offset)) {
            result = store.eraseSector(offset);
        }
        return result;
    }

    bool isValid(Sector sector)
    {
        const Header& header = *(const Header*)store.dataAt(addressOf(sector));
        return header.isValid();
    }

    void write(Sector sector, const Header& header)
    {
        store.write(addressOf(sector), &header, sizeof(header));
    }

    void initialize(Sector sector)
    {
        erase(sector);
        Header header;
        header.make_valid();
        write(sector, header);
    }

    Sector currentSector()
    {
        Sector result = Sector_0;
        if (isValid(Sector_1))
            result = Sector_1;
        else {
            if (!isValid(Sector_0))
                initialize(Sector_0);
        }
        return result;
    }

    Sector alternateSectorTo(Sector sector)
    {
        return sector==Sector_0 ? Sector_1 : Sector_0;
    }

public:
    DCD() = default;


    bool isInitialized()
    {
        return isValid(Sector_1) || isValid(Sector_0);
    }

    /**
     * Erase all data in this DCD.
     * @return
     */
    Result erase()
    {
        Result error = erase(0);
        if (!error)
            error = erase(1);
        return error;
    }

    /**
     * Retrieve a pointer to the data in the DCD.
     * @param offset
     * @return
     */
    const uint8_t* read(const Address offset)
    {
        Address location = addressOf(currentSector())+sizeof(Header)+offset;
        return store.dataAt(location);
    }

    /**
     * Write data to the DCD.
     * @param data      The data to write
     * @param offset    The logical offset in the DCD region to write to.
     * @param length    The number of bytes of data to write.
     * @return
     */
    Result write(const Address offset, const void* data, size_t length)
    {
        if (offset >= Length)
            return DCD_INVALID_OFFSET;
        if (offset+length > Length)
            return DCD_INVALID_LENGTH;
        if (!length)
            return DCD_SUCCESS;

        const Sector current = currentSector();
        Sector newSector = alternateSectorTo(current);
        const uint8_t* existing = store.dataAt(addressOf(current));
        Result error = this->writeSector(offset, data, length, existing, newSector);
        if (error) return error;

        Header header;
        header.make_invalid();
        error = store.write(addressOf(current), &header, sizeof(header));
        return error;
    }

    Result writeSector(const Address offset, const void* data, size_t length, const uint8_t* existing, Sector newSector)
    {
        Result error = erase(newSector);
        if (error) return error;

        Address destination = addressOf(newSector);
        Address writeOffset = sizeof(Header);        // skip writing the header until the end

        if (existing) {
            error = store.write(destination+writeOffset, existing+writeOffset, offset);
            if (error) return error;
            writeOffset += offset;
        }

        error = store.write(destination+writeOffset, data, length);
        if (error) return error;
        writeOffset += length;

        if (existing) {
            error = store.write(destination+writeOffset, existing+writeOffset, sectorSize-writeOffset);
            if (error) return error;
        }

        Header header;
        header.make_valid();
        error = store.write(destination, &header, sizeof(header));
        return error;
    }

};
