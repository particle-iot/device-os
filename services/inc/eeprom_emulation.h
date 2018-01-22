/**
 ******************************************************************************
 * @file    eeprom_emulation.h
 * @author  Julien Vanier
 ******************************************************************************
  Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.

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

#include <cstring>
#include <memory>
#include <vector>
#include <limits>

/* EEPROM Emulation using Flash memory
 *
 * EEPROM provides reads and writes for single bytes, with a default
 * value of 0xFF for unprogrammed cells.
 *
 * Two pages (sectors) of Flash memory with potentially different sizes
 * are used to store records each containing the value of 1 byte of
 * emulated EEPROM.
 *
 * Each record contain an index (EEPROM cell virtual address), a data
 * byte and a status byte (valid/erased).
 *
 * The maximum number of bytes that can be written is the smallest page
 * size divided by the record size.
 *
 * Since erased Flash starts at 0xFF and bits can only be written as 0,
 * writing a new value of an EEPROM byte involves appending a new record
 * to the list of current records in the active page.
 *
 * Reading involves going through the list of valid records in the
 * active page looking for the last record with a specified index.
 *
 * When writing a new value and there is no more room in the current
 * page to append new records, a page swap occurs as follows:
 * - The alternate page is erased if necessary
 * - Records for all values except the  ones being written are copied to
 *   the alternate page
 * - Records for the changed records are written to the alternate page.
 * - The alternate page is marked active and becomes the new active page
 * - The old active page is marked inactive
 *
 * Any of these steps can be interrupted by a reset and the data will
 * remain consistent because the old page will be used until the very
 * last step (old active page is marked inactive).
 *
 * In order to make application programming easier, it is possible to
 * write multiple bytes in an atomic fashion: either all bytes written
 * will be read back or none will be read back, even in the presence of
 * power failure/controller reset.
 *
 * Atomic writes are implemented as follows:
 * - If any invalid records exist, do a page swap (which is atomic)
 * - Write records backwards from the end
 * - If any of the writes failed or there was not enough room for all
 *   records, do a page swap
 *
 * It is possible for a write to fail verification (reading back the
 * value). This is because of previous marginal writes or marginal
 * erases (reset during writing or erase that leaves Flash cells reading
 * back as 1 but with a true state between 0 and 1).  To protect against
 * this, if a write doesn't read back correctly, a page swap will be
 * done.
 *
 * On the STM32 microcontroller, the Flash memory cannot be read while
 * being programmed which means the application is frozen while writing
 * or erasing the Flash (no interrupts are serviced). Flash writes are
 * fast (~10 us), but erases take 200ms or more (depending on the sector
 * size). To avoid intermittent pauses in the user application due to
 * page erases during the page swap, the hasPendingErase() and
 * performPendingErase() APIs exist to allow the user application to
 * schedule when an old page can be erased. If the user application does
 * not call performPendingErase() before the next page swap, the
 * alternate page will be erased just before the page swap.
 *
 */

template <typename Store, uintptr_t PageBase1, size_t PageSize1, uintptr_t PageBase2, size_t PageSize2>
class EEPROMEmulation
{
public:
    using Address = uintptr_t;
    using Index = uint16_t;
    using Data = uint8_t;

    static constexpr size_t SmallestPageSize = (PageSize1 < PageSize2) ? PageSize1 : PageSize2;

    enum class LogicalPage
    {
        NoPage,
        Page1,
        Page2
    };

    static const uint8_t FLASH_ERASED = 0xFF;

    // Stores the status of a page of emulated EEPROM
    //
    // WARNING: Do not change the size of struct or order of elements since
    // instances of this struct are persisted in the flash memory
    struct __attribute__((packed)) PageHeader
    {
        // These status were selected for compatibility with the old
        // EEPROM emulation code: the lower 16 bits correspond to the
        // old statuses
        static const uint32_t ERASED   = 0xFFFFFFFF;
        static const uint32_t COPY     = 0xFFFFEEEE;
        static const uint32_t ACTIVE   = 0xFFFF0000;
        static const uint32_t INACTIVE = 0xCCCC0000;

        uint32_t status;

        PageHeader(uint32_t status = ERASED) : status(status)
        {
        }
    };

    // A record stores the value of 1 byte in the emulated EEPROM
    //
    // WARNING: Do not change the size of struct or order of elements since
    // instances of this struct are persisted in the flash memory
    struct __attribute__((packed)) Record
    {
        static const Index EMPTY_INDEX = 0xFFFF;
        static const uint8_t VALID = 0;

        Data data;
        uint8_t status;
        Index index;

        Record(Address index, Data data)
            : data(data), status(VALID), index(index)
        {
        }

        Record()
            : data(FLASH_ERASED), status(FLASH_ERASED), index(EMPTY_INDEX)
        {
        }

        bool empty() const
        {
            return index == EMPTY_INDEX &&
                status == FLASH_ERASED &&
                data == FLASH_ERASED;
        }

        bool valid() const
        {
            return index != EMPTY_INDEX && status == VALID;
        }
    };

    /* Public API */

    // Initialize the EEPROM pages
    // Call at boot
    void init()
    {
        updateActivePage();

        if(getActivePage() == LogicalPage::NoPage)
        {
            clear();
        }
    }

    // Read the latest value of a byte of EEPROM in data or 0xFF if the
    // value was not programmed
    void get(Index index, Data &data)
    {
        readRange(index, &data, sizeof(data));
    }

    // Reads the latest valid values of a block of EEPROM into data.
    // Fills data with 0xFF for values that were not programmed
    void get(Index index, void *data, uint16_t length)
    {
        readRange(index, (Data *)data, length);
    }

    // Writes a new value for a byte of EEPROM
    // Performs a page swap (move all valid records to a new page)
    // if the current page is full
    void put(Index index, Data data)
    {
        writeRange(index, &data, sizeof(data));
    }

    // Writes new values for a block of EEPROM
    // The write will be atomic (all or nothing) even if a reset occurs
    // during the write
    //
    // Performs a page swap (move all valid records to a new page)
    // if the current page is full
    void put(Index index, const void *data, uint16_t length)
    {
        writeRange(index, (Data *)data, length);
    }

    // Destroys all the data 💣
    void clear()
    {
        erasePage(LogicalPage::Page1);
        erasePage(LogicalPage::Page2);
        writePageStatus(LogicalPage::Page1, PageHeader::ACTIVE);

        updateActivePage();
    }

    // Returns number of bytes that can be stored in EEPROM
    // The actual capacity is set to 50% of the records that fit in the smallest page
    static constexpr size_t capacity()
    {
        return SmallestPageSize / sizeof(Record) / 2;
    }

    // Check if the old page needs to be erased
    bool hasPendingErase()
    {
        return getPendingErasePage() != LogicalPage::NoPage;
    }

    // Erases the old page after a page swap, if necessary
    // Let the user application call this when convenient since erasing
    // Flash freezes the application for several 100ms.
    void performPendingErase()
    {
        if(hasPendingErase())
        {
            erasePage(getPendingErasePage());
        }
    }

    /* Implementation */

    // Start address of the page
    Address getPageBegin(LogicalPage page)
    {
        switch(page)
        {
            case LogicalPage::Page1: return PageBase1;
            case LogicalPage::Page2: return PageBase2;
            default: return 0;
        }
    }

    // End address (1 past the end) of the page
    Address getPageEnd(LogicalPage page)
    {
        switch(page)
        {
            case LogicalPage::Page1: return PageBase1 + PageSize1;
            case LogicalPage::Page2: return PageBase2 + PageSize2;
            default: return 0;
        }
    }

    // Number of bytes in the page
    size_t getPageSize(LogicalPage page)
    {
        switch(page)
        {
            case LogicalPage::Page1: return PageSize1;
            case LogicalPage::Page2: return PageSize2;
            default: return 0;
        }
    }

    // Figure out which page should currently be read from/written to
    // and which one should be used as the target of the page swap
    void updateActivePage()
    {
        uint32_t status1 = readPageStatus(LogicalPage::Page1);
        uint32_t status2 = readPageStatus(LogicalPage::Page2);

        // Pick the first active page
        if(status1 == PageHeader::ACTIVE)
        {
            activePage = LogicalPage::Page1;
            alternatePage = LogicalPage::Page2;
        }
        else if(status2 == PageHeader::ACTIVE)
        {
            activePage = LogicalPage::Page2;
            alternatePage = LogicalPage::Page1;
        }
        else
        {
            activePage = LogicalPage::NoPage;
            alternatePage = LogicalPage::NoPage;
        }
    }

    // Which page should currently be read from/written to
    LogicalPage getActivePage()
    {
        return activePage;
    }

    // Which page should be used as the target for the next swap
    LogicalPage getAlternatePage()
    {
        return alternatePage;
    }

    // Get the current status of a page (empty, active, being copied, ...)
    uint32_t readPageStatus(LogicalPage page)
    {
        PageHeader *header = (PageHeader *) store.dataAt(getPageBegin(page));
        return header->status;
    }

    // Update the status of a page
    bool writePageStatus(LogicalPage page, uint32_t status)
    {
        PageHeader header = { status };
        return store.write(getPageBegin(page), &header, sizeof(header)) == 0;
    }

    // Iterate through a page to extract the latest value of each address
    void readRange(Index indexBegin, Data *data, uint16_t length)
    {
        std::memset(data, FLASH_ERASED, length);

        Index indexEnd = indexBegin + length;
        forEachValidRecord(getActivePage(), [=](Address address, const Record &record)
        {
            if(record.index >= indexBegin && record.index < indexEnd)
            {
                data[record.index - indexBegin] = record.data;
            }
        });
    }

    // Write each byte in the range if its value has changed.
    void writeRange(Index indexBegin, const Data *data, uint16_t length)
    {
        // don't write anything if index is out of range
        Index indexEnd = indexBegin + length;
        if(indexEnd > capacity())
        {
            return;
        }

        // Read existing values for range
        std::unique_ptr<Data[]> existingData(new Data[length]);
        // don't write anything if memory is full
        if(!existingData)
        {
            return;
        }

        Address writeAddressBegin;

        // Read the data and make sure there are no previous invalid
        // records before starting to write
        bool success = readRangeAndFindEmpty(getActivePage(),
                existingData.get(), indexBegin, length, writeAddressBegin);

        // Write records for all new values
        success = success && writeRangeChanged(writeAddressBegin, indexBegin, data, existingData.get(), length);

        // If any writes failed because the page was full or a marginal
        // write error occured, do a page swap then write all the
        // records
        if(!success)
        {
            swapPagesAndWrite(indexBegin, data, length);
        }
    }

    // Read values and find the address where to write new records
    //
    // Return false if there are invalid records, true if page can be
    // written to
    bool readRangeAndFindEmpty(LogicalPage page, Data *existingData, Index indexBegin,
            uint16_t length, Address &emptyAddress)
    {
        bool hasInvalidRecords = false;
        Index indexEnd = indexBegin + length;

        std::memset(existingData, FLASH_ERASED, length);
        emptyAddress = getPageEnd(page);

        forEachRecord(page, [&](Address address, const Record &record) -> bool
        {
            if(record.empty())
            {
                emptyAddress = address;
                return true;
            }
            else if(record.valid())
            {
                if(record.index >= indexBegin && record.index < indexEnd)
                {
                    existingData[record.index - indexBegin] = record.data;
                }
                return false;
            }
            else
            {
                hasInvalidRecords = true;
                return true;
            }
        });

        return !hasInvalidRecords;
    }


    // Write new records backwards in Flash. This ensures data
    // consistency if writeRange is interrupted by a reset since reads
    // stop at the first non-valid record.
    bool writeRangeChanged(Address writeAddressBegin, Index indexBegin, const uint8_t *data, const uint8_t *existingData, uint16_t length)
    {
        bool success = true;

        // Count changed values
        uint16_t changedCount = 0;
        for(uint16_t i = 0; i < length && success; i++)
        {
            if(existingData[i] != data[i])
            {
                changedCount++;
            }
        }

        // Write all changed values, backwards from the end
        if(changedCount > 0)
        {
            Address writeAddress = writeAddressBegin + changedCount * sizeof(Record);
            Address endAddress = getPageEnd(getActivePage());

            // There must be an empty record after the position where the
            // last record will be written to act as a separator for the
            // valid record detection algorithm to work well
            if(writeAddress < endAddress)
            {
                Record separatorRecord;
                store.read(writeAddress, &separatorRecord, sizeof(separatorRecord));

                success = separatorRecord.empty();
            }

            for(uint16_t i = 0; i < length && success; i++)
            {
                if(existingData[i] != data[i])
                {
                    Index index = indexBegin + i;
                    writeAddress -= sizeof(Record);
                    success = success && writeRecord(
                            writeAddress, endAddress, Record(index, data[i]));
                }
            }
        }

        return success;
    }

    // Write a record to the first empty space available in a page
    //
    // Returns false when write was unsuccessful to protect against
    // marginal erase, true on proper write
    bool writeRecord(Address writeAddress,
            Address endAddress,
            const Record &record)
    {
        // No more room for record
        if(writeAddress + sizeof(Record) > endAddress)
        {
            return false;
        }

        // Write record and return true when write is verified successfully
        return (store.write(writeAddress, &record, sizeof(record)) >= 0);
    }

    // Iterate through a page and yield each record, including valid
    // and invalid records, and the empty record at the end (if there is
    // room)
    template <typename Func>
    void forEachRecord(LogicalPage page, Func f)
    {
        Address address = getPageBegin(page);
        Address endAddress = getPageEnd(page);

        // Skip page header
        address += sizeof(PageHeader);

        // Walk through record list
        while(address < endAddress)
        {
            const Record &record = *(const Record *) store.dataAt(address);

            // Yield record and potentially break early
            if(f(address, record))
            {
                return;
            }

            // Skip over record
            address += sizeof(record);
        }
    }

    // Iterate through a page and yield each valid record,
    // ignoring any records after the first invalid one
    template <typename Func>
    void forEachValidRecord(LogicalPage page, Func f)
    {
        forEachRecord(page, [=](Address address, const Record &record) -> bool
        {
            if(record.valid())
            {
                f(address, record);
                return false;
            }
            else
            {
                return true;
            }
        });
    }

    // Iterate through a page and yield each valid record once, with the
    // latest value
    template <typename Func>
    void forEachUniqueValidRecord(LogicalPage page, Func f)
    {
        // Find latest address of each record in several passes through the page, batching
        // the finds to reduce the number of linear searches through the page.

        // To save heap space, only address offsets are used instead of the full
        // addresses, so make sure offsets fit in the chosen AddressOffset data type
        using AddressOffset = uint16_t;
        static_assert(
            PageSize1 <= std::numeric_limits<AddressOffset>::max() + 1 &&
            PageSize2 <= std::numeric_limits<AddressOffset>::max() + 1,
            "PageSize1 or PageSize2 doesn't fit in AddressOffset. "
            "Make pages smaller or AddressOffset a larger data type"
        );

        // The recordAddresses vector will use up to BatchSize * sizeof(AddressOffset)
        // bytes on the heap.
        std::vector<AddressOffset> recordAddresses;
        const Index BatchSize = 128;

        Address baseAddress = getPageBegin(page);

        Index firstIndex = 0;
        Index lastIndex = BatchSize;
        bool hasMoreRecords = true;

        while(hasMoreRecords) {
            hasMoreRecords = false;

            forEachValidRecord(page, [&](Address address, const Record &record)
            {
                // Save records in this batch
                if(record.index >= firstIndex && record.index < lastIndex) {
                    Index indexOffset = record.index - firstIndex;
                    AddressOffset addressOffset = address - baseAddress;

                    if(indexOffset >= recordAddresses.size())
                    {
                        recordAddresses.resize(indexOffset + 1);
                    }

                    recordAddresses[indexOffset] = addressOffset;
                }

                // More records in a later batch
                if(record.index >= lastIndex) {
                    hasMoreRecords = true;
                }
            });

            // Yield the records in this batch that have a value
            for(auto addressOffset: recordAddresses)
            {
                if(addressOffset != 0) {
                    Address address = baseAddress + addressOffset;
                    const Record &record = *(const Record *) store.dataAt(address);

                    // Yield record
                    f(address, record);
                }
            }

            // Get ready for a new batch
            recordAddresses.clear();
            firstIndex += BatchSize;
            lastIndex += BatchSize;
        }
    }

    // Verify that the entire page is erased to protect against resets
    // during page erase
    bool verifyPage(LogicalPage page)
    {
        const uint8_t *begin = store.dataAt(getPageBegin(page));
        const uint8_t *end = store.dataAt(getPageEnd(page));
        while(begin < end)
        {
            if(*begin++ != FLASH_ERASED)
            {
                return false;
            }
        }

        return true;
    }

    // Reset entire page to 0xFF
    void erasePage(LogicalPage page)
    {
        store.eraseSector(getPageBegin(page));
    }

    // Write all valid records from the active page to the alternate
    // page. Erase the alternate page if it is not already erased.
    // Then write the new record to the alternate page.
    // Then erase the old active page
    bool swapPagesAndWrite(Index indexBegin, const Data *data, uint16_t length)
    {
        LogicalPage sourcePage = getActivePage();
        LogicalPage destinationPage = getAlternatePage();

        // loop protects against marginal erase: if a page was kind of
        // erased and read back as all 0xFF but when values are written
        // some bits written as 1 actually become 0
        for(int tries = 0; tries < 2; tries++)
        {
            bool success = true;
            if(tries > 0 || !verifyPage(destinationPage))
            {
                erasePage(destinationPage);
            }

            Address writeAddress = getPageBegin(destinationPage);

            // Write alternate page as destination for copy
            success = success && writePageStatus(destinationPage, PageHeader::COPY);

            writeAddress += sizeof(PageHeader);

            // Copy records from source to destination
            success = success && copyAllRecordsToPageExcept(sourcePage,
                                                            destinationPage,
                                                            writeAddress,
                                                            indexBegin,
                                                            indexBegin + length);

            Address endAddress = getPageEnd(destinationPage);

            // Write new records to destination directly
            success = success && writeRangeDirect(writeAddress,
                                                  getPageEnd(destinationPage),
                                                  indexBegin,
                                                  data,
                                                  length);

            // Mark new page as active
            success = success && writePageStatus(destinationPage, PageHeader::ACTIVE);
            success = success && writePageStatus(sourcePage, PageHeader::INACTIVE);

            if(success)
            {
                updateActivePage();
                return true;
            }
        }

        return false;
    }

    // Perform the actual copy of records during page swap
    bool copyAllRecordsToPageExcept(LogicalPage sourcePage,
            LogicalPage destinationPage,
            Address &writeAddress,
            Index exceptIndexBegin,
            Index exceptIndexEnd)
    {
        bool success = true;
        Address endAddress = getPageEnd(destinationPage);
        forEachUniqueValidRecord(sourcePage, [&](Address address, const Record &record)
        {
            // Don't copy the records that are being replaced or records that are 0xFF
            if(!(record.index >= exceptIndexBegin && record.index < exceptIndexEnd) &&
                record.data != FLASH_ERASED)
            {
                success = success && writeRecord(writeAddress, endAddress, Record(record.index, record.data));
                writeAddress += sizeof(Record);
            }
        });

        return success;
    }

    // Write a range of valid records starting a specified address
    bool writeRangeDirect(Address writeAddress,
            Address endAddress,
            Index indexBegin,
            const Data *data,
            uint16_t length)
    {
        bool success = true;

        // Write new records to destination directly
        for(uint16_t i = 0; i < length && success; i++)
        {
            // Don't bother writing records that are 0xFF
            if(data[i] != FLASH_ERASED)
            {
                Index index = indexBegin + i;
                success = success && writeRecord(
                        writeAddress, endAddress, Record(index, data[i]));
                writeAddress += sizeof(Record);
            }
        }

        return success;
    }

    // Which page needs to be erased after a page swap.
    LogicalPage getPendingErasePage()
    {
        if(readPageStatus(getAlternatePage()) != PageHeader::ERASED)
        {
            return getAlternatePage();
        }
        else
        {
            return LogicalPage::NoPage;
        }
    }

    // Hardware-dependent interface to read, erase and program memory
    Store store;

protected:
    LogicalPage activePage;
    LogicalPage alternatePage;
};
