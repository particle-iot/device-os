#include "memory_hal.h"

#include "sst25vf_spi.h"
#include "hw_config.h"


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

    virtual bool erasePage(flash_addr_t address) {
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
    virtual bool writePage(const void* data, flash_addr_t address, page_size_t length) {
        // TODO: SPI interface shouldn't need mutable data buffer to write?
        sFLASH_WriteBuffer(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(data)), address, length);
        return true;
    }

    virtual bool readPage(void* data, flash_addr_t address, page_size_t length) const {
        sFLASH_ReadBuffer((uint8_t*) data, address, length);
        return true;
    }

};

/*
 * The external flash. 
 */
static ExternalFlashDevice& externalFlash;


#define EXTERNAL_FLASH_FAC_ADDRESS	((uint32_t)EXTERNAL_FLASH_BLOCK_SIZE)
/* External Flash memory address where core firmware will be saved for backup/restore */
#define EXTERNAL_FLASH_BKP_ADDRESS	((uint32_t)(EXTERNAL_FLASH_BLOCK_SIZE + EXTERNAL_FLASH_FAC_ADDRESS))
/* External Flash memory address where OTA upgraded core firmware will be saved */
#define EXTERNAL_FLASH_OTA_ADDRESS	((uint32_t)(EXTERNAL_FLASH_BLOCK_SIZE + EXTERNAL_FLASH_BKP_ADDRESS))

void MemoryDevices::internalFirmware(MemoryDeviceRegion& region) const 
{
    // todo
}

void MemoryDevices::factoryDefaultFirmware(MemoryDeviceRegion& region) const
{
    region.set(externalFlash, EXTERNAL_FLASH_FAC_ADDRESS, EXTERNAL_FLASH_FAC_ADDRESS+EXTERNAL_FLASH_BLOCK_SIZE);    
}

void MemoryDevices::backupFirmware(MemoryDeviceRegion& region) const
{
    region.set(externalFlash, EXTERNAL_FLASH_BKP_ADDRESS, EXTERNAL_FLASH_BKP_ADDRESS+EXTERNAL_FLASH_BLOCK_SIZE);
}

void MemoryDevices::OTAFlashFirmware(MemoryDeviceRegion& region) const
{
    region.set(externalFlash, EXTERNAL_FLASH_OTA_ADDRESS, EXTERNAL_FLASH_OTA_ADDRESS+EXTERNAL_FLASH_BLOCK_SIZE);
}
