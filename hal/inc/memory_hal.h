/**
 ******************************************************************************
 * @file    memory_hal.h
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    25-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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
#pragma once

#include <stdint.h>

typedef uint32_t mem_addr_t;
typedef uint32_t mem_page_size_t;
typedef uint32_t mem_page_count_t;

/**
 * An abstraction of memory storage.
 */
class MemoryDevice
{
public:
    virtual ~MemoryDevice();

    /**
     * @return The size of each page in this memory device.
     */
    virtual mem_page_size_t pageSize() const = 0;

    /**
     * @return The number of pages in this flash device.
     */
    virtual mem_page_count_t pageCount() const = 0;

    virtual bool write(const void* data, mem_addr_t address, mem_page_size_t length)=0;

    virtual bool read(void* data, mem_addr_t address, mem_page_size_t length) const=0;

    virtual bool erasePage(mem_addr_t address) = 0;


    mem_addr_t length() const {
        return pageAddress(pageCount());
    }


    /**
     * Converts a page index [0,N) into the corresponding read/write address.
     * @param page  The page to convert to an address.
     * @return
     */
    mem_addr_t pageAddress(mem_page_count_t page) const {
        return mem_addr_t(page) * pageSize();
    }

    mem_page_count_t addressPage(mem_addr_t address) const {
        return address/pageSize();
    }

    /**
     * Determines if the given address represents the start of a page.
     */
    bool isPageAddress(mem_addr_t address) const {
        return (address % pageSize()) == 0;
    }

    inline bool isValidAddress(mem_addr_t address, mem_page_size_t extent) const {
        return address + extent <= length() && (extent==0 || (addressPage(address)==addressPage(address+extent-1)));
    }

    bool eraseAll() {
        mem_addr_t end = length();
        mem_addr_t size = pageSize();
        bool success = true;
        for (mem_addr_t i = 0; i<end; i+=size) {
            success = success && erasePage(i);
        }
        return success;
    }

};

/**
 * Describes a target device and an address region.
 */
struct MemoryDeviceRegion {
    MemoryDevice& memory;
    mem_addr_t start;
    mem_addr_t end;

    void set(MemoryDevice& memory, mem_addr_t start, mem_addr_t end)
    {
        this->memory = memory;
        this->start = start;
        this->end = end;
    }
};

class MemoryDeviceWriter : public MemoryDeviceRegion {


    void begin(mem_addr_t offset, mem_addr_t length)
    {
        start += offset;
        end = start + length;
    }

    void write(void* buffer, mem_addr_t length)
    {

    }
};

/**
 * Access the various memory store regions in the system.
 */
class MemoryDevices
{
public:

    /**
     * Retrieves the store for the internal program.
     * @return
     */
    static void internalFirmware(MemoryDeviceRegion& region);
    static void factoryDefaultFirmware(MemoryDeviceRegion& region);
    static void backupFirmware(MemoryDeviceRegion& region);
    static void OTAFlashFirmware(MemoryDeviceRegion& region);

};


