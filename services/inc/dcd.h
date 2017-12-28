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
 * Identifying the current valid sector:
 *
 * v1 code:
 * looks for a valid watermark and seal value equal to SEAL_VALID.  On invalidating a sector, the
 * v1 code writes 0x0 to the seal, and v2 writes SEAL_INVALID_V2 - both of which are ignored in v1 code, so v1 code will only see
 * a sector still marked as valid.
 *
 * v2 code:
 * looks for a sector with SEAL_VALID. When the other sector has a seal of SEAL_INVALID then the CRC is not checked (since this sector was written by v1 code) and assumed valid,
 * otherwise the CRC is checked. When the CRC is invalid, the alternative sector is used if it has SEAL_VALID or SEAL_INVALID_V2
 *
 * Since writes are not atomic it is possible that there is more than one sector with SEAL_VALID (when the final write to invalidate the alternate sector does not succeed.)
 * The v2 code checks the CRC of both and the counter and chooses the one that is most recent and valid. v1 code just chooses one, statically (iirc it's the 2nd sector).
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
    static const uint32_t WATERMARK = 0x1E1C279A;   // 9A271C1E

    struct Header
    {
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
         * Return the size of the header. It is always 8 bytes.
         */
        size_t size() const {
        		return sizeof(Header);
        }

        bool isHeader() const {
        		return watermark==WATERMARK;
        }

        /**
         * Determine if the current header is valid. It is valid
         * if the watermark equals WATERMARK and the seal either SEAL_VALID or
         * SEAL_INVALID_V2
         */
        bool isValid() const
        {
            return isHeader() && (seal==SEAL_VALID || seal == SEAL_INVALID_V2);
        }

        void initialize()
        {
            watermark = WATERMARK;
            seal = SEAL_INIT;
        }

        void makeValid()
        {
            watermark = WATERMARK;
            seal = SEAL_VALID;
        }

        void makeInvalid()
        {
            watermark = WATERMARK;
            seal = SEAL_INVALID_V2;
        }
    };

    /**
     * This struct is written to the end of the sector.
     */
    class __attribute__((packed)) Footer {
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
		uint32_t watermark;
		uint32_t crc_;
    public:
		using crc_type = decltype(crc_);

		bool isValid() const {
			return watermark==WATERMARK;
		}

		uint8_t counter() const {
			return flags.counter;
		}

		void counter(uint8_t counter) {
			flags.counter = counter;
		}

		void initialize() {
			watermark = WATERMARK;
			mysize = sizeof(this);
		}

        void makeValidV2(uint32_t crc, uint8_t counter)
        {
        		memset(this, 0, sizeof(*this));
            this->crc_ = crc;
            this->watermark = WATERMARK;
            this->mysize = this->size();
            flags.counter = counter;
        }

        size_t size() const {
        		return sizeof(*this);
        }

        uint32_t crc() const {
        		return crc_;
        }
	};

    /**
     * The offset in each sector where the footer is written.
     */
    const size_t footerOffset = sectorSize-sizeof(Footer);

    /**
     * The logical size of the data that can be stored.
     */
    const Address Length = sectorSize-sizeof(Header)-sizeof(Footer);

    static const Sector Sector_0 = 0;
    static const Sector Sector_1 = 1;
    static const Sector Sector_Unknown = 255;

protected:
    /**
     * Retrieve the address of a given sector.
     */
    inline Address addressOf(Sector sector)
    {
        return sector!=Sector_1 ? DCD1 : DCD2;
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
     * Determine if the given sector is valid. Does not check the CRC by default since this is done elsewhere.
     */
    bool isValid(Sector sector, bool checkCRC=false) {
    		const Header& header = sectorHeader(sector);
		bool valid = header.isValid();
        if (valid && checkCRC) {
        		valid = isCRCValid(sector);
        }
        	return valid;
    }

    const Header& sectorHeader(Sector sector) {
		const Header& header = *reinterpret_cast<const Header*>(store.dataAt(addressOf(sector)));
		return header;
    }

    const Footer& sectorFooter(Sector sector) {
 		const Footer& footer = *reinterpret_cast<const Footer*>(store.dataAt(addressOf(sector))+footerOffset);
 		return footer;
    }

    Result write(Sector sector, const Header& header)
    {
        return store.write(addressOf(sector), &header, header.size());
    }

    Result write(Sector sector, const Footer& footer)
    {
    		return store.write(addressOf(sector)+footerOffset, &footer, footer.size());
    }

    /**
     * Creates the s	ector as a valid sector, with data initialized to 0xFF.
     */
    Result initialize(Sector sector)
    {
        return _writeSector(0, nullptr, 0, nullptr, sector);
    }

    /**
     * Determine the CRC for a sector. The CRC checks all the data from after the header until the CRC, including the footer data.
     */
    uint32_t computeSectorCRC(Sector sector) {
    		const uint8_t* data = store.dataAt(addressOf(sector));
    		return computeCRC(data);
    }

    /**
     * Determines which sector is valid. The CRC is not checked.
     */
    Sector findValidSector() {
		const Header& header0 = sectorHeader(Sector_0);
		const Header& header1 = sectorHeader(Sector_1);

		Sector primary;
		if (header0.isValid()) {
			primary = Sector_0;
		}
		else if (header1.isValid()) {
			primary = Sector_1;
		}
		else {
			primary = Sector_Unknown;
		}
    		return primary;
    }

    /**
     * Determine the sector that contains the latest valid data. This sector is used to read data from and provide the basis of
     * data when writing to the alternate sector.
     *
     */
    Sector currentSector() {
    		const Sector primary = findValidSector();
    		if (primary==Sector_Unknown)
    			return Sector_Unknown;

    		// check the deeper validity of that sector, and see if it is the one
    		// or if the backup sector should be used.
    		const Sector secondary = alternateSectorTo(primary);
    		const Header& secondaryHeader = sectorHeader(secondary);
		const Footer& primaryFooter = sectorFooter(primary);
		const Footer& secondaryFooter = sectorFooter(secondary);
		Sector current = evaluateSectors(primary, secondary, secondaryHeader.seal, primaryFooter.counter(), secondaryFooter.counter());
    		return current;
    }

    /**
     * Determine the current sector.
     * @param primary	The sector that is known to be marked as valid
     * @param secondary	The alternate sector
     * @param secondarySeal	The seal of the secondary sector.
     * @param primaryCounter	the counter of the primary sector. Only used when both primary and secondary sectors are valid and have valid CRCs
     * @param
     */
    Sector evaluateSectors(Sector primary, Sector secondary, uint32_t secondarySeal, uint8_t primaryCounter, uint8_t secondaryCounter) {
		Sector current;
    		if (secondarySeal==Header::SEAL_VALID) {		// prefer the most recent counter if both valid CRC, else the one with the valid CRC else Sector_1
			// choose the one with valid CRC if it exists
			bool crcValidPrimary = isCRCValid(primary);
			bool crcValidSecondary = isCRCValid(secondary);
			if (crcValidPrimary ^ crcValidSecondary) {		// they are different, prefer the valid one
				current = crcValidPrimary ? primary : secondary;
			}
			else if (crcValidPrimary) {				// both are valid
				current = _currentSector(primaryCounter, secondaryCounter, primary, secondary);
			}
			else {
				current = Sector_1;			// can choose either
			}
    		}
		else if (secondarySeal==Header::SEAL_INVALID_V2) {
			// the current sector was written by V2 code so the CRC shoudl be valid.
			// when the current CRC is not valid, then fallback to the secondary (which we assume is valid.)
			// use primary sector if CRC checks out (no need to check the counter)
			current = isCRCValid(primary) ? primary : secondary;
		} else {
			// assume current sector is valid since it was written by v1 code
			current = primary;
			// don't bother checking the CRC since we have nothing to fallback to and cannot be sure it was written by v2 code (maybe be an old v2 sector
			// that was copied by v1 code without updating the CRC.)
		}
    		return current;
    }

    Sector alternateSectorTo(Sector sector)
    {
        return sector==Sector_0 ? Sector_1 : Sector_0;
    }

    /**
     * Determines the sector that contains the current valid information
     */
    Sector _currentSector(uint8_t count0, uint8_t count1, Sector sector0, Sector sector1)
    {
		if (((count0+1) & 3)==count1) {
			return sector1;
		}
		if (((count1+1) & 3)==count0) {
			return sector0;
		}
		return sector0;	// both are equally valid - could do a 50/50 random choice here
    }

public:
    DCD() = default;

    bool isInitialized()
    {
        return isValid(Sector_1) || isValid(Sector_0);
    }

    bool isCRCValid(Sector sector) {
    		return isCRCValid(sector, sectorFooter(sector).crc());
    }

    bool isCRCValid(Sector sector, uint32_t expected) {
        if (sectorFooter(sector).isValid()) {
            uint32_t actual = computeCRC(store.dataAt(addressOf(sector)));
            return actual==expected;
        }
        return false;
    }

    uint32_t computeCRC(const uint8_t* sectorStart) {
    		return calculateCRC(sectorStart+sizeof(Header), sectorSize-sizeof(Header)-sizeof(typename Footer::crc_type));
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

    /**
     * Fetches a valid sector, initializing a sector if there is no valid one.
     */
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
        	Result error = this->_writeSector(offset, data, length, existing, newSector);
        if (error) return error;

		Header header;
		header.makeInvalid();
        error = write(current, header);
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
    Result _writeSector(const Address offset, const void* data, size_t length, const uint8_t* existing, Sector newSector)
    {
        Result error = erase(newSector);
        if (error) return error;

        const Header& existingHeader = *(reinterpret_cast<const Header*>(existing));
        	const Footer& existingFooter = *(reinterpret_cast<const Footer*>(existing+footerOffset));

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

        if (length) {
			error = store.write(destination+writeOffset, data, length);
			if (error) return error;
			writeOffset += length;
			readOffset += length;
        }

        if (existing) {
            error = store.write(destination+writeOffset, existing+readOffset, sectorSize-writeOffset-sizeof(Footer));
            if (error) return error;
        }

        uint8_t counter = 0;
        if (existing && existingFooter.isValid()) {
            counter = uint8_t((existingFooter.counter() + 1) & 3);
        }
        error = _write_v2_footer(newSector, (existing && existingFooter.isValid()) ? &existingFooter : nullptr, counter);
        if (error) return error;
        typename Footer::crc_type crc = computeSectorCRC(newSector);
        writeOffset = sectorSize-sizeof(typename Footer::crc_type);
        error = store.write(destination+writeOffset, &crc, sizeof(crc));
        if (error) return error;
		Header header;
		header.makeValid();
        error = write(newSector, header);
        return error;
    }

    /**
     * Write the footer to the sector, excluding the CRC.
     */
    Result _write_v2_footer(Sector sector, const Footer* existingFooter, uint8_t counter) {
        Address destination = addressOf(sector);
        Footer footer;
        if (existingFooter) {
            footer = *existingFooter;
        } else {
            footer.initialize();
        }
        footer.counter(counter);
        // write the footer
        Result error = store.write(destination+footerOffset, reinterpret_cast<const uint8_t*>(&footer), footer.size()-sizeof(typename Footer::crc_type));
        return error;
    }

};

template <typename Store, unsigned sectorSize, unsigned DCD1, unsigned DCD2, uint32_t(*calculateCRC)(const void* data, size_t len)>
const typename DCD<Store, sectorSize, DCD1, DCD2, calculateCRC>::Sector DCD<Store, sectorSize, DCD1, DCD2, calculateCRC>::Sector_0;

template <typename Store, unsigned sectorSize, unsigned DCD1, unsigned DCD2, uint32_t(*calculateCRC)(const void* data, size_t len)>
const typename DCD<Store, sectorSize, DCD1, DCD2, calculateCRC>::Sector DCD<Store, sectorSize, DCD1, DCD2, calculateCRC>::Sector_1;

template <typename Store, unsigned sectorSize, unsigned DCD1, unsigned DCD2, uint32_t(*calculateCRC)(const void* data, size_t len)>
const typename DCD<Store, sectorSize, DCD1, DCD2, calculateCRC>::Sector DCD<Store, sectorSize, DCD1, DCD2, calculateCRC>::Sector_Unknown;

