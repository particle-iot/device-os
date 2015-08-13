/**
 ******************************************************************************
 * @file    memory_hal.cpp
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

#include "memory_hal.h"

#include "sst25vf_spi.h"
#include "hw_config.h"

MemoryDevice::~MemoryDevice()
{
}

class ExternalFlashDevice : public MemoryDevice {

    /**
     * @return The size of each page in this flash device.
     */
    virtual mem_page_size_t pageSize() const {
        return sFLASH_PAGESIZE;
    }

    /**
     * @return The number of pages in this flash device.
     */
    virtual mem_page_count_t pageCount() const {
        return 512;
    }

    virtual bool erasePage(mem_addr_t address) {
        bool success = false;
        if (address < pageAddress(pageCount()) && (address % pageSize()) == 0) {
            sFLASH_EraseSector(address);
            success = true;
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
    virtual bool write(const void* data, mem_addr_t address, mem_page_size_t length) {
        // TODO: SPI interface shouldn't need mutable data buffer to write?
        sFLASH_WriteBuffer(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(data)), address, length);
        return true;
    }

    virtual bool read(void* data, mem_addr_t address, mem_page_size_t length) const {
        sFLASH_ReadBuffer((uint8_t*) data, address, length);
        return true;
    }

};

/*
 * The external flash.
 */
static ExternalFlashDevice externalFlash;


#define EXTERNAL_FLASH_FAC_ADDRESS	((uint32_t)EXTERNAL_FLASH_BLOCK_SIZE)
/* External Flash memory address where core firmware will be saved for backup/restore */
#define EXTERNAL_FLASH_BKP_ADDRESS	((uint32_t)(EXTERNAL_FLASH_BLOCK_SIZE + EXTERNAL_FLASH_FAC_ADDRESS))
/* External Flash memory address where OTA upgraded core firmware will be saved */
#define EXTERNAL_FLASH_OTA_ADDRESS	((uint32_t)(EXTERNAL_FLASH_BLOCK_SIZE + EXTERNAL_FLASH_BKP_ADDRESS))

void MemoryDevices::internalFirmware(MemoryDeviceRegion& region)
{
    // todo
}

void MemoryDevices::factoryDefaultFirmware(MemoryDeviceRegion& region)
{
    region.set(externalFlash, EXTERNAL_FLASH_FAC_ADDRESS, EXTERNAL_FLASH_FAC_ADDRESS+EXTERNAL_FLASH_BLOCK_SIZE);
}

void MemoryDevices::backupFirmware(MemoryDeviceRegion& region)
{
    region.set(externalFlash, EXTERNAL_FLASH_BKP_ADDRESS, EXTERNAL_FLASH_BKP_ADDRESS+EXTERNAL_FLASH_BLOCK_SIZE);
}

void MemoryDevices::OTAFlashFirmware(MemoryDeviceRegion& region)
{
    region.set(externalFlash, EXTERNAL_FLASH_OTA_ADDRESS, EXTERNAL_FLASH_OTA_ADDRESS+EXTERNAL_FLASH_BLOCK_SIZE);
}
