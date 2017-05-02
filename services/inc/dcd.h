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
 *
 * For the data to be recognised as a valid page, the first 4 bytes must match
 * the expected watermark. After the watermark is a 4 byte status word.
 *
 * The v1 layout comprises just the watermark (4 bytes) the status (4 bytes)
 * followed by application data. The status indicates if the page is to be written (SEAL_INIT), has been written
 * (SEAL_VALID) or has been cleared (SEAL_CLEARED).
 *
 * The v2 format looks identical to the v1 format in the header, and application data.
 * It adds a block of 16 bytes at the end of the sector. (From a compatibility perspective,
 * in theory, the application data region is a few bytes smaller, but
 * these offsets were never used in practice, so it's of no consequence.)
 * The last data in the sector is the 4 byte CRC, which covers all data in the sector up to the CRC.
 * (before writing the CRC, the sector is marked as sealed, and then the CRC for the entire sector is computed.)
 * The data preceding the crc is a variable length block. The length is the first two bytes before the CRC.
 * The contents of the block is determined by the version. for v2, there are 16 bytes reserved that are presently unused.
 * (set to 0xFF)
 *
 * When firmware is downgraded from v2 to v1 (or the two must co-exist, such as with a newer bootloader
 * and older firmware), we must be careful the data remains readable to both v1 and v2 versions of the code.
 *
 * When the v2 implementation writes data, it discards the old page using a distinct code that is different
 * from the v1 status of 0x00000000 (it clears just some of the bits in the valid header.) The CRC is computed independently from the header so that it is not invalidated
 * by clearing the header. The v2 implementation will still consider the header as valid (just as if the status were unchanged)
 * and instead relies on the CRC and the counter to determine which sector contains the latest data. The v1 implementation will not consider the old
 * sector, and instead relies on the SEAL_VALID_V1 status being set.
 *
 * The v1 implementation copies the entire sector, which includes the v2 crc, but does this without updating the crc, meaning the
 * v2 crc will be invalid.
 * The v2 implementation can determine if the sector is written by v1, since the non-current sector will
 * have a status flag of 0x00000000. In that case, it ignores the invalid CRC of the current sector
 * since the other sector is explicitly marked invalid, meaning the current sector is the only one we have to use.
 *
 * The sector crc is checked only when the other sector has a seal of SEAL_VALID or SEAL_INVALID_V2, and
 * the current sector also has a seal with these values. This ensures the CRC is only used after one successful write.
 *
 * Todo - work through all the possible states of status and valid/invalid crc for the pair of sectors to
 * determine the one that should be considered valid.
 *
 *
 * Old stuff...
 * The v2 format is designated by the seal with 01 in the lowest byte of the
 * status word. The format comprises the watermark, the status word, and then
 * a 32-bit crc followed by 8-bits of flags and 11 reserved bytes. The crc checksums the data
 * immediately following the crc up to the end of the flash sector. (The data preceeding
 * the crc has already been validated, and this means we do not have to make provisions for
 * excluding the crc word from the crc check, as would be the case if we tried to apply the crc
 * to the entire sector.
 *
 * When the sector format requires upgrading, then `formatInfo()` returns a structure with
 * the `upgradePending` field is set to non-zero value indicating the version of the upgrade. (0-based)
 *
 * The flash pages are erased at the latest opportunity (when data is written, the oldest page is cleared.)
 * The new data is written, followed by the non-changing data, and finally the crc and then the header.
 *
 * On startup, both pages are examined ot determine their current state.
 * - 0: erased: all erased page
 * - 1: v1:  avalid v1 page, with the seal intact
 * - 2: v2: a valid v2 page, with the crc matching the data.
 *
 * both pages are examined and the highest one is considered the current page to read from.
 * If both pages have the same version and are valid, then for v1 pages, the first one is chosen.
 * For v2, the counter values are compared, and if they differ by 1 (modulo 4) then the highest one is chosen
 * This allows for wrap-around. e.g. if the values are 0 and 3, the page with counter value 0 is
 * chosen since hti is 3+1 modulo 4. If both pages differ by more than 1 modulo 4 then the first one is chosen.
 *
 * When upgrading, the current valid v1 sector (if any) is written out to the spare sector
 * and that is used as the current valid sector going forward.
 *
 * After the write operation, the sector is validated. If it is not valid, the write is reattempted up to 3 times.
 * If the write continues to fail a failure code is returned.
 */

template <typename Store, unsigned sectorSize, unsigned DCD1, unsigned DCD2, uint32_t(*calculateCRC)(const void* data, size_t len)>
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

    static const uint8_t latestVersion = 2;

    struct Header
    {
        static const uint32_t WATERMARK = 0x1E1C279A;   // 9A271C1E
        static const uint32_t SEAL_INIT = 0xFFFFFFFF;
        static const uint32_t SEAL_CLEARED = 0;

        static const uint32_t SEAL_VALID = 0xEDA15E00;  // 005EA1ED;
        static const uint32_t SEAL_INVALID_V2 = 0x69A00C00;

        	static_assert((SEAL_VALID & SEAL_INVALID_V2)==SEAL_INVALID_V2, "SEAL_INVALID_V2 should be a subset of the 1 bits of SEAL_VALID");

        /**
         * Identifies the data in each page as a DCD page
         */
        uint32_t watermark;

        /**
         * Marks the sector status.
         */
        uint32_t seal;

        /**
         * Returns the size of the header. It is always 8 bytes.
         */
        size_t size() const {
        		return sizeof(Header);
        }

        /**
         * Determines if the current header is valid.
         */
        bool isValid() const
        {
            return watermark==WATERMARK && (seal==SEAL_VALID || seal==SEAL_INVALID_V2);
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
            seal = SEAL_INVALID_V2;
        }

        uint8_t version() const {
        		if (watermark!=WATERMARK) return 0;
        		switch (seal) {
        		case SEAL_VALID: return 1;
        		case SEAL_INVALID_V2: return 2;
        		return 0;
        		}
        }
    };

    /**
     * This struct is written to the end of the sector.
     */
    struct __attribute__((packed)) Footer {
    		// add new members here.
		uint32_t reserved[4];
		struct Flags {
				uint8_t counter : 2;
				uint8_t reserved : 6;
				uint8_t reserved2[3];
		} flags;

		uint8_t pad;
		uint8_t version_flags;		// currently 0
		uint16_t mysize;				// the size of the data block (end of sector - size is the start of the data block)
		uint32_t crc;

        void make_valid_v2(uint32_t crc, uint8_t counter)
        {
        		memset(this, 0, sizeof(*this));
            this->crc = crc;
            this->mysize = this->size();
            flags.counter = counter;
        }

        size_t size() {
        		return sizeof(*this);
        }
	};

    /**
     * The offset in each sector where the footer is written.
     */
    const size_t footer_offset = sectorSize-sizeof(Footer);

    const Address Length = sectorSize-sizeof(Header)-sizeof(Footer);
    static const Sector Sector_0 = 0;
    static const Sector Sector_1 = 1;
    static const Sector Sector_Unknown = 255;

protected:
    inline Address addressOf(Sector sector)
    {
        return sector==Sector_0 ? DCD1 : DCD2;
    }

    /**
     * Determine if the sector at the address contains any cleared bits
     * that requires erasing.
     */
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

    /**
     * Erase the given sector if it requires it.
     */
    Result erase(Sector sector)
    {
        Result result = 0;
        Address offset = addressOf(sector);
        if (requiresErase(offset)) {
            result = store.eraseSector(offset);
        }
        return result;
    }

    /**
     * Determine the version of the data at the given sector. If the
     * data is not valid a version of 0 is returned.
     */
    uint8_t version(Sector sector) {
    		const Header& header = sectorHeader(sector);
        uint8_t version = header.version();
        if (version==2 && !isCRCValid(header)) {
        		version = 0;
        }
        	return version;
    }

    const Header& sectorHeader(Sector sector) {
		const Header& header = *(const Header*)store.dataAt(addressOf(sector));
		return header;
    }

    const Footer& sectorFooter(Sector sector) {
 		const Footer& footer = *(const Footer*)store.dataAt(addressOf(sector));
 		return footer;
    }

    void write(Sector sector, const Header& header)
    {
        store.write(addressOf(sector), &header, header.size());
    }

    void write(Sector sector, const Footer& footer)
    {
    		store.write(addressOf(sector)+footer_offset, &footer, footer.size());
    }

    void initialize(Sector sector)
    {
        erase(sector);
        _write_header(sector, 0);
    }

    uint32_t computeSectorCRC(Sector sector) {
    		const Header& flash = sectorHeader(sector);
    		return computeCRC(flash);
    }

    Sector currentSector() {
		uint8_t version0 = version(Sector_0);
		uint8_t version1 = version(Sector_1);

        uint8_t counter0=0, counter1=0;
        if (version0>=2 && version1>=2) {
            counter0 = sectorHeader(Sector_0).flags.counter;
            counter1 = sectorHeader(Sector_1).flags.counter;
        }
        return _currentSector(version0, version1, counter0, counter1);
    }

    uint8_t currentVersion() {
    		Sector current = currentSector();
    		return current==Sector_Unknown ? 0 : version(current);
    }

    Sector alternateSectorTo(Sector sector)
    {
        return sector==Sector_0 ? Sector_1 : Sector_0;
    }

    /**
     * Determines the sector that contains the current valid information
     */
    Sector _currentSector(uint8_t version0, uint8_t version1, uint8_t count0=0, uint8_t count1=0)
    {
        if (version0==version1) {
            switch (version0) {
            case 1: return Sector_1;
            case 2: // determine the one that is most recent
                if (((count0+1) & 3)==count1) {
                    return Sector_1;
                }
                if (((count1+1) & 3)==count0) {
                    return Sector_0;
                }
                return Sector_0;	// both are equally valid - could do a 50/50 random choice here
            case 0: default: return Sector_Unknown;
            }
        }
        else {
            return version0 > version1 ? Sector_0 : Sector_1;
        }
    }

public:
    DCD() = default;

    bool isInitialized()
    {
        return isValid(Sector_1) || isValid(Sector_0);
    }

    bool isCRCValid(const Header& header) {
    		uint32_t actual = computeCRC(header);
    		return actual==header.crc;
    }

    uint32_t computeCRC(const Header& header) {
    		const uint8_t* start = reinterpret_cast<const uint8_t*>(&header);
    		const uint32_t preCRCDataSize = Header::crc_data_start;
    		static_assert(offsetof(Header, flags)==preCRCDataSize, "expected flags to be at offset 12");
    		return calculateCRC(start+preCRCDataSize, sectorSize-preCRCDataSize);
    }

    /**
     * Determine if the sector is valid.
     */
    bool isValid(Sector sector)
    {
    		return version(sector) > 0;
    }

    /**
     * Updates the cached sector data from the backing store.
     */
    void sync()
    {

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

    Sector currentValidSector() {
    		Sector current = currentSector();
    		if (current==Sector_Unknown) {
    			current = Sector_1;
    			initialize(current);
    		}
    		return current;
    }

    /**
     * Retrieve a pointer to the data in the DCD.
     * @param offset
     * @return
     */
    const uint8_t* read(const Address offset)
    {
        Sector current = currentValidSector();
        const Header& header = sectorHeader(current);
        Address location = addressOf(current)+header.size()+offset;
        return store.dataAt(location);
    }

    /**
     * Write data to the DCD.
     * @param data      The data to write
     * @param offset    The logical offset in the DCD region to write to.
     * @param length    The number of bytes of data to write.
     * @return	The result of the write operation. DCD_SUCCESS means the data was written
     * successfully. Any other
     */
    Result write(const Address offset, const void* data, size_t length, size_t version=latestVersion)
    {
        if (offset >= Length)
            return DCD_INVALID_OFFSET;
        if (offset+length > Length)
            return DCD_INVALID_LENGTH;
        if (!length)
            return DCD_SUCCESS;

        Sector current = currentValidSector();
        Sector newSector = alternateSectorTo(current);
        const uint8_t* existing = store.dataAt(addressOf(current));
        Result error = this->_writeSector(offset, data, length, existing, newSector, version);
        if (error) return error;

        const Header& previous = sectorHeader(current);
        if (previous.version()==1) {
        		Header header;
        		header.make_invalid();
        		error = store.write(addressOf(current), &header, previous.size());
        }
        return error;
    }

    /**
     * Perform a rewrite of a sector.
     *
     * @param offset	The logical offset of the data being written (0-based)
     * @param data	The data to write
     * @param length	The amount of data to write
     * @param existing	A pointer to the existing sector
     * @param newSector	The new sector to write the data to
     */
    Result _writeSector(const Address offset, const void* data, size_t length, const uint8_t* existing, Sector newSector, uint8_t version=2)
    {
        Result error = erase(newSector);
        if (error) return error;

        const Header& existingHeader = *(reinterpret_cast<const Header*>(existing));
        Address destination = addressOf(newSector);

        Address writeOffset = sizeof(Header);
        Address readOffset = existingHeader.size();

        // write everything before the data that is changed
        if (existing) {
            error = store.write(destination+writeOffset, existing+readOffset, offset);
            if (error) return error;
            writeOffset += offset;
            readOffset += offset;
        }

        error = store.write(destination+writeOffset, data, length);
        if (error) return error;
        writeOffset += length;
        readOffset += length;

        if (existing) {
            error = store.write(destination+writeOffset, existing+readOffset, sectorSize-writeOffset);
            if (error) return error;
        }

        uint8_t counter = 0;
        if (existingHeader.version()>=2) {
        		counter = (existingHeader.flags.counter + 1) & 3;
        }

        return _write_v2_header(newSector, counter);
    }

    Result _write_v2_header(Sector sector, uint8_t counter) {
        Address destination = addressOf(sector);
        Header header;
        header.make_valid_v2(0, counter);
        const auto crc_data_start = Header::crc_data_start;
        // write everything after the crc
        Result error = store.write(destination+crc_data_start, ((const uint8_t*)&header)+crc_data_start, header.size()-crc_data_start);
        if (error) return error;
        header.crc = computeSectorCRC(sector);
        error = store.write(destination, &header, crc_data_start);
        return error;
    }

};

template <typename Store, unsigned sectorSize, unsigned DCD1, unsigned DCD2, uint32_t(*calculateCRC)(const void* data, size_t len)>
const typename DCD<Store, sectorSize, DCD1, DCD2, calculateCRC>::Sector DCD<Store, sectorSize, DCD1, DCD2, calculateCRC>::Sector_0;

template <typename Store, unsigned sectorSize, unsigned DCD1, unsigned DCD2, uint32_t(*calculateCRC)(const void* data, size_t len)>
const typename DCD<Store, sectorSize, DCD1, DCD2, calculateCRC>::Sector DCD<Store, sectorSize, DCD1, DCD2, calculateCRC>::Sector_1;

template <typename Store, unsigned sectorSize, unsigned DCD1, unsigned DCD2, uint32_t(*calculateCRC)(const void* data, size_t len)>
const typename DCD<Store, sectorSize, DCD1, DCD2, calculateCRC>::Sector DCD<Store, sectorSize, DCD1, DCD2, calculateCRC>::Sector_Unknown;

