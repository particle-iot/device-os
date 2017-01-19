/**
 * Copyright 2014  Matthew McGowan
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _FLASHEE_EEPROM_H_
#define _FLASHEE_EEPROM_H_

#ifdef SPARK
#include "application.h"
#endif

#include <stdint.h>
#include "string.h"
#include "stdlib.h"

#ifndef FLASHEE_FAT_FS_SUPPORT
#define FLASHEE_FAT_FS_SUPPORT 0
#endif


#if FLASHEE_FAT_FS_SUPPORT
#include "FlashIO.h"
#endif

namespace Flashee {

typedef uint32_t flash_addr_t;
typedef uint32_t page_size_t;
typedef uint32_t page_count_t;

/**
 * Function that performs the data transformation when relocating a page in flash.
 */
typedef void (*TransferHandler)(page_size_t pageOffset, void* data, uint8_t* buf, page_size_t bufSize);

/**
 * Provides an interface to a flash device.
 * Reads/Writes can span multiple pages, and be any arbitrary length or offset.
 */
class FlashDevice {
public:
    virtual ~FlashDevice();

    /**
     * @return The size of each page in this flash device.
     */
    virtual page_size_t pageSize() const = 0;

    /**
     * @return The number of pages in this flash device.
     */
    virtual page_count_t pageCount() const = 0;

    flash_addr_t length() const {
        return pageAddress(pageCount());
    }

    inline bool write(const void* data, flash_addr_t address, page_size_t length) {
        return writeErasePage(data, address, length);
    }

    inline bool read(void* data, flash_addr_t address, page_size_t length) {
        return readPage(data, address, length);
    }

    inline bool writeString(const char* s, flash_addr_t address, bool includeNull=true) {
        return write(s, address, strlen(s)+(includeNull?1:0));
    }

    /**
     * Converts a page index [0,N) into the corresponding read/write address.
     * @param page  The page to convert to an address.
     * @return
     */
    flash_addr_t pageAddress(page_count_t page) const {
        return flash_addr_t(page) * pageSize();
    }

    page_count_t addressPage(flash_addr_t address) const {
        return address/pageSize();
    }

    /**
     * Determines if the given address represents the start of a page.
     */
    bool isPageAddress(flash_addr_t address) const {
        return (address % pageSize()) == 0;
    }

    inline bool isValidAddress(flash_addr_t address, page_size_t extent) const {
        return address + extent <= length() && (extent==0 || (addressPage(address)==addressPage(address+extent-1)));
    }


    bool writeEraseByte(uint8_t data, flash_addr_t address) {
        return writeErasePage(&data, address, 1);
    }

    uint8_t readByte(flash_addr_t address) const {
        uint8_t data = 0xFF;
        readPage(&data, address, 1);
        return data;
    }

    virtual bool erasePage(flash_addr_t address) = 0;

    bool eraseAll() {
        flash_addr_t end = length();
        flash_addr_t size = pageSize();
        bool success = true;
        for (flash_addr_t i = 0; i<end; i+=size) {
            success = success && erasePage(i);
        }
        return success;
    }

    /**
     * Writes directly to the flash. Depending upon the state of the flash, the
     * write may provide the data required or it may not.
     * @param data
     * @param address
     * @param length
     * @return
     */
    virtual bool writePage(const void* data, flash_addr_t address, page_size_t length) = 0;


    virtual bool readPage(void* data, flash_addr_t address, page_size_t length) const = 0;

    /**
     * Writes data to the flash memory, performing an erase beforehand if necessary
     * to ensure the data is written correctly.
     * @param data
     * @param address
     * @param length
     * @return
     */
    virtual bool writeErasePage(const void* data, flash_addr_t address, page_size_t length) = 0;

    /**
     * Internally re-reorganizes the page's storage by passing the page contents via
     * a buffer through a handler function and then writing the buffer back to
     * memory.
     *
     * This is not part of the public API and is for use only by the library.
     *
     * @param address
     * @param handler
     * @param data
     * @param buf
     * @param bufSize
     * @return
     */
    virtual bool copyPage(flash_addr_t address, TransferHandler handler, void* data, uint8_t* buf, page_size_t bufSize) = 0;

};

#include "flashee-eeprom-impl.h"

/**
 * A circular buffer over flash memory. When the writer attempts to overwrite
 * the page that the reader is on, writing fails by returning 0.
 *
 * Regular reads and writes are all or nothing - they will read or write the
 * required amount to the buffer or fail if that is not possible.
 * There are also soft variants of the read/write methods that allow up to the
 * specified number of bytes to be read/written.
 */
class CircularBuffer {
    FlashDevice& flash;
    flash_addr_t write_pointer;
    mutable flash_addr_t read_pointer;
    const flash_addr_t capacity_;
    mutable flash_addr_t size_;

private:
    /**
     * Attempts to write data to the buffer.
     * @param buf       Pointer to the data to write
     * @param length    The maximum number of bytes of data to write.
     * @param hard      When {@code true} all the bytes should be written or none.
     *   When false, write as much as possible.
     * @return The number of bytes actually written. If this is 0, then the buffer
     *  is full.
     */
    page_size_t write_impl(const void* buf, page_size_t length, bool hard) {
        page_size_t space = free();
        if (length>space) {
            if (hard)
                return 0;
            else
                length = space;
        }

        page_size_t blockSize = flash.pageSize();
        page_size_t result = length;
        while (length > 0) {
            page_size_t offset = write_pointer % blockSize;
            page_size_t blockWrite = min(length, blockSize-offset);
            if (!offset)
                flash.erasePage(write_pointer);
            flash.writePage(buf, write_pointer, blockWrite);
            write_pointer += blockWrite;
            if (write_pointer == capacity_)
                write_pointer = 0;
            buf = ((uint8_t*)buf)+blockWrite;
            length -= blockWrite;
        }
        size_ += result;
        return result;
    }

    /**
     * Reads up to {@code length} bytes from the buffer. If data is available
     * then at least one byte will be returned (unless length is 0.)
     * @param buf
     * @param len
     * @return The number of bytes written to the buffer. This will be >0 if there
     * is data available, and <= length specified. If this returns 0, there is no
     * data available in the buffer.
     */
    page_size_t read_impl(void* buf, page_size_t length, bool hard) const {
        if (length>size_) {
            if (hard)
                return 0;
            else
                length = size_;
        }

        page_size_t result = length;
        page_size_t blockSize = flash.pageSize();
        while (length > 0) {
            // write each page
            page_size_t offset = read_pointer % blockSize;
            page_size_t blockRead = min(length, blockSize-offset);
            flash.readPage(buf, read_pointer, blockRead);
            read_pointer += blockRead;
            if (read_pointer == capacity_)
                read_pointer = 0;
            buf = ((uint8_t*)buf)+blockRead;
            length -= blockRead;
        }
        size_ -= result;
        return result;
    }

public:

    /**
     * Create a new circular buffer using the given flash device as storage.
     * The buffer is initially empty, with a capacity equal to the size of the
     * flash device.
     */
    CircularBuffer(FlashDevice& storage)
    : flash(storage), write_pointer(0), read_pointer(0),
            capacity_(flash.length()), size_(0) {
    }

    /**
     * Writes data to the buffer when there is sufficient free space.
     * @return {@code length} when the data has been written, or 0 if there
     *   is not enough room.
     */
    page_size_t write(const void* buf, page_size_t length) {
        return write_impl(buf, length, true);
    }

    /**
     * Writes data to the buffer. If there is insufficient space to write all
     * the data, then as much as will fit is written.
     * @return the amount of data written, which can be <= {@code length}.
     */
    page_size_t write_soft(const void* buf, page_size_t length) {
        return write_impl(buf, length, false);
    }

    /**
     * Reads the given number of bytes from the buffer, or none.
     * @return {@code length} if there was sufficient data in the buffer to read length bytes, or
     * 0 otherwise.
     */
    page_size_t read(void* buf, page_size_t length) const {
        return read_impl(buf, length, true);
    }

    /**
     * Reads up to a given number of bytes from the buffer.
     * @return The number of bytes read, up to {@code length}.
     */
    page_size_t read_soft(void* buf, page_size_t length) const {
        return read_impl(buf, length, false);
    }


    /**
     * Retrieves the maximum number of bytes that can be read from the buffer.
     * @return The maximum number of bytes that can be read from the buffer.
     * As bytes are read, this value will decrease, and will increase when
     * bytes are written.
     */
    page_size_t available() const {
        return size_;
    }

    /**
     * Retrieves the maximum storage capacity of this buffer. This value does
     * not change as bytes are read or written.
     * @param buf
     * @param length
     * @return
     */
    page_size_t capacity() const {
        return this->capacity_;
    }

    /**
     * Retrieves the number of bytes that can be written to the buffer.
     * @return The free space in the buffer. Note that this may not change
     * as data is read from the buffer due to page erase constraints.
     */
    page_size_t free() const {
        // cannot write into the same page that is being read so this space is unavailable
        page_size_t free = capacity_ - size_ - (read_pointer % flash.pageSize());
        return free;
    }
};

#if defined(__GXX_EXPERIMENTAL_CXX0X__) &&  0
#include <muted>

// todo use a condition variable so that reading threads block until there is data, or
// writing threads block until there is space.
class CircularBufferThreadSafe {


    CircularBuffer* forward;
    std::mutex  mutex;

public:
    CircularBufferThreadSafe(CircularBuffer* buffer) : forward(buffer) {]

    ~CircularBufferThreadSafe() {
        delete forward;
    }


    (page_size_t write(const void* buf, page_size_t length) {

        return write(buf, length);
    }

    page_size_t write_soft(const void* buf, page_size_t length) {
        return write_impl(buf, length, false);
    }

    /**
     * Reads the given number of bytes from the buffer, if available.
     * @return {@code length} if there was sufficient data in the buffer, or
     * 0 if it could not be read.
     */
    page_size_t read(void* buf, page_size_t length) const {
        return read_impl(buf, length, true);
    }

    /**
     * Reads up to a given number of characters.
     * @return The number of bytes read, up to {@code length}.
     */
    page_size_t read_upto(void* buf, page_size_t length) const {
        return read_impl(buf, length, false);
    }

    page_size_t free() const {
        return 0;
    }

};
#endif


class FlashStream {

protected:
    FlashDevice& flash;
    flash_addr_t address;

public:

    FlashStream(FlashDevice& device, flash_addr_t start=0) : flash(device), address(start) { }

    void advance(page_count_t amount) { address += amount; }
};

class FlashReader : FlashStream {

public:
    FlashReader(FlashDevice& device, flash_addr_t start=0)
            : FlashStream(device, start) {}

    FlashReader(FlashDevice* device, flash_addr_t start=0)
            : FlashStream(*device, start) {}


    void read(void* buf, page_size_t length) {
        flash.read(buf, address, length);
        advance(length);
    }

    uint8_t read() {
        uint8_t result;
        read(&result, 1);
        return result;
    }

    uint16_t readWord() {
        uint16_t result;
        read(&result, sizeof(result));
        return result;
    }

    uint32_t readInt() {
        uint32_t result;
        read(&result, sizeof(result));
        return result;
    }

    void readString(char* buf) {
        uint16_t actual = readWord();
        read(buf, actual);
    }
};


class FlashWriter : FlashStream {

public:
    FlashWriter(FlashDevice& device, flash_addr_t start=0)
            : FlashStream(device, start) {}

    FlashWriter(FlashDevice* device, flash_addr_t start=0)
            : FlashStream(*device, start) {}

    void write(const void* buf, page_size_t length) {
        flash.write(buf, address, length);
        advance(length);
    }

    void writeString(const char* s) {
        uint16_t len = strlen(s);
        write(&len, sizeof(len));
        write(s, len);
    }

    void write(uint8_t value) {
        write(&value, 1);
    }

    void writeWord(uint16_t value) {
        write(&value, 2);
    }

    void writeInt(uint32_t value) {
        write(&value, sizeof(value));
    }

};




class Devices {
private:
    static FlashDeviceRegion userRegion;

    inline static FlashDevice* createLogicalPageMapper(FlashDevice* flash, page_count_t pageCount) {
        page_count_t count = flash->pageCount();
        return count <= 256 && pageCount < count && pageCount > 1 ? new LogicalPageMapper<>(*flash, pageCount) : NULL;
    }

    inline static FlashDevice* createMultiWrite(FlashDevice* flash) {
        return new MultiWriteFlashStore(*flash);
    }

    inline static FlashDevice* createMultiPageEraseImpl(flash_addr_t startAddress, flash_addr_t endAddress, page_count_t freePageCount) {
        if (endAddress == flash_addr_t(-1))
            endAddress = startAddress + userRegion.pageAddress(256);
        if (freePageCount < 2 || freePageCount >= ((endAddress - startAddress) / userRegion.pageSize()))
            return NULL;
        FlashDevice* userFlash = createUserFlashRegion(startAddress, endAddress);
        if (userFlash==NULL)
            return NULL;
        FlashDevice* mapper = createLogicalPageMapper(userFlash, userFlash->pageCount() - freePageCount);
        return mapper;
    }

public:

    /**
     * Provides access to the user accessible region.
     * The hides the actual location in external flash - so the
     * the first writable address is 0x000000.
     * @return A reference to the user accessible flash.
     */
    static FlashDevice& userFlash() {
        return userRegion;
    }

    static FlashDevice* createUserFlashRegion(flash_addr_t startAddress, flash_addr_t endAddress, page_count_t minPageCount=1) {
        if (((endAddress-startAddress)/userRegion.pageSize())<minPageCount)
            return NULL;
        return userRegion.createSubregion(startAddress, endAddress);
    }


    /**
     * Creates a flash device where each destructive write causes the page to
     * be erased and and internal reserved page to be erased.
     * This should not be used only if the number of destructive writes is known to be
     * less than 10^6 for the lifetime of the unit.
     */
    static FlashDevice* createSinglePageErase(flash_addr_t startAddress, flash_addr_t endAddress) {
        FlashDevice* userFlash = createUserFlashRegion(startAddress, endAddress);
        if (userFlash!=NULL) {
            SinglePageWear* wear = new SinglePageWear(*userFlash);
            return new PageSpanFlashDevice(*wear);
        }
        return NULL;
    }

    /**
     * Creates a flash device where destructive writes cause a page erase, and
     * the page erases are levelled out over the available free pages.
     * @param startAddress
     * @param endAddress
     * @param freePageCount     The number of pages to allocate in the given region.
     *
     * @return
     */
    static FlashDevice* createWearLevelErase(flash_addr_t startAddress = 0, flash_addr_t endAddress = flash_addr_t(-1), page_count_t freePageCount = 2) {
        FlashDevice* mapper = createMultiPageEraseImpl(startAddress, endAddress, freePageCount);
        return mapper == NULL ? NULL : new PageSpanFlashDevice(*mapper);
    }

    /**
     * Creates a flash device where destructive writes do not require a page erase,
     * and when a page erase is required, it is wear-levelled out over the available
     * free pages.
     * @param startAddress
     * @param endAddress
     * @param pageCount
     * @return
     */
    static FlashDevice* createAddressErase(flash_addr_t startAddress = 0, flash_addr_t endAddress = flash_addr_t(-1), page_count_t freePageCount = 2) {
        FlashDevice* mapper = createMultiPageEraseImpl(startAddress, endAddress, freePageCount);
        if (mapper == NULL)
            return NULL;
        FlashDevice* multi = createMultiWrite(mapper);
        return new PageSpanFlashDevice(*multi);
    }

    /**
     * Creates a circular buffer that uses the pages
     */
    static CircularBuffer* createCircularBuffer(flash_addr_t startAddress=0, flash_addr_t endAddress=-1) {
        if (endAddress==flash_addr_t(-1))
            endAddress = userFlash().length();
        FlashDevice* device = createUserFlashRegion(startAddress, endAddress, 2);
        return device ? new CircularBuffer(*device) : NULL;
    }

#if FLASHEE_FAT_FS_SUPPORT
    /**
     * Allocates a region of flash for storing a FAT filesystem. If an existing filesystem
     * has alredy been created elsewhere, that volume is closed. (Only one volume can be
     * accessed at a time.)
     *
     * @param startAddress  The starting address for the allocated region.
     * @param endAddress    The ending address (exclusive) for the allocated region.
     * @param pfs           The address of the FATFS structure for this filesystem.
     *  This is typically statically allocated.
     * @param format        When true, the storage will be formatted.
     */
    static FRESULT createFATRegion(flash_addr_t startAddress, flash_addr_t endAddress,
        FATFS* pfs, FormatCmd formatCmd=FORMAT_CMD_FORMAT_IF_NEEDED);
#endif

};

} // namespace

#endif
