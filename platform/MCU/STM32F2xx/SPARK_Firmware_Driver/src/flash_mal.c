/**
 ******************************************************************************
 * @file    flash_mal.c
 * @author  Satish Nair, Matthew McGowan
 * @version V1.0.0
 * @date    30-Jan-2015
 * @brief   Media access layer for platform dependent flash interfaces
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"
#include "flash_mal.h"
#include "dct.h"
#include "module_info.h"
#include <string.h>

/* Private functions ---------------------------------------------------------*/

static const uint32_t sectorAddresses[] = {
    0x8000000, 0x8004000, 0x8008000, 0x800C000, 0x8010000, 0x8020000
};
static const uint8_t flashSectors[] = {
    FLASH_Sector_0, FLASH_Sector_1, FLASH_Sector_2, FLASH_Sector_3, FLASH_Sector_4,
    FLASH_Sector_5, FLASH_Sector_6, FLASH_Sector_7, FLASH_Sector_8, FLASH_Sector_9,
    FLASH_Sector_10, FLASH_Sector_11
};


/**
 * Retrieves the index into the flashSectors array that contains the
 * flash sector that spans the given address.
 */
uint16_t addressToSectorIndex(uint32_t address)
{
    int i;
    for (i=1; i<6; i++) {
        if (address<sectorAddresses[i])
            return i-1;
    }
    return ((address-0x8020000)>>17)+5;
}

uint32_t sectorIndexToStartAddress(uint16_t sector)
{
	return sector<5 ? sectorAddresses[sector] :
			((sector-5)<<17)+0x8020000;
}

static inline uint16_t InternalSectorToWriteProtect(uint32_t startAddress)
{
    uint16_t OB_WRP_Sector;
	OB_WRP_Sector = 1<<addressToSectorIndex(startAddress);
    return OB_WRP_Sector;
}

inline static uint16_t InternalSectorToErase(uint32_t startAddress)
{
    uint16_t    flashSector = flashSectors[addressToSectorIndex(startAddress)];
    return flashSector;
}

uint16_t FLASH_SectorToWriteProtect(flash_device_t device, uint32_t startAddress)
{
	uint16_t sector = 0;
	if (device==FLASH_INTERNAL)
		sector = InternalSectorToWriteProtect(startAddress);
	return sector;
}

uint16_t FLASH_SectorToErase(flash_device_t device, uint32_t startAddress)
{
	uint16_t sector = 0xFFFF;
	if (device==FLASH_INTERNAL)
		sector = InternalSectorToErase(startAddress);
	return sector;
}

/**
 * Determines the address that is the end of this sector (exclusive - so it's really the start of the next sector.)
 */
uint32_t EndOfFlashSector(flash_device_t device, uint32_t address)
{
	uint32_t end;
	if (device==FLASH_INTERNAL)
	{
		uint16_t sector = addressToSectorIndex(address);
		end = sectorIndexToStartAddress(sector+1);
	}
#ifdef USE_SERIAL_FLASH
	else if (device==FLASH_SERIAL)
	{
		uint16_t sector = address / sFLASH_PAGESIZE;
		end = (sector+1) * sFLASH_PAGESIZE;
	}
#endif
	else
		end = 0;
	return end;
}



bool FLASH_CheckValidAddressRange(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length)
{
    uint32_t endAddress = startAddress + length - 1;

    if (flashDeviceID == FLASH_INTERNAL)
    {
        return startAddress>=0x8000000 && endAddress<=0x8100000;
    }
    else if (flashDeviceID == FLASH_SERIAL)
    {
#ifdef USE_SERIAL_FLASH
        if (startAddress < 0x4000 || endAddress >= 0x100000)
        {
            return false;
        }
#else
        return false;
#endif
    }
    else
    {
        return false;   //Invalid FLASH ID
    }

    return true;
}

bool FLASH_WriteProtectMemory(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length, bool protect)
{
    if (FLASH_CheckValidAddressRange(flashDeviceID, startAddress, length) != true)
    {
        return false;
    }

    if (flashDeviceID == FLASH_INTERNAL)
    {
        /* Get the first OB_WRP_Sector */
        uint16_t OB_WRP_Sector = InternalSectorToWriteProtect(startAddress);
        uint16_t end = InternalSectorToWriteProtect(startAddress+length-1)<<1;

        if (OB_WRP_Sector < OB_WRP_Sector_0)
        {
            return false;
        }

        while (!(OB_WRP_Sector & end))
        {
            if (protect)
            {
                FLASH_WriteProtection_Enable(OB_WRP_Sector);
            }
            else
            {
                FLASH_WriteProtection_Disable(OB_WRP_Sector);
            }
            OB_WRP_Sector <<= 1;
        }

        return true;
    }

    return false;
}

bool FLASH_EraseMemory(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length)
{
    uint32_t eraseCounter = 0;
    uint32_t numPages = 0;

    if (FLASH_CheckValidAddressRange(flashDeviceID, startAddress, length) != true)
    {
        return false;
    }

    if (flashDeviceID == FLASH_INTERNAL)
    {
        /* Check which sector has to be erased */
        uint16_t flashSector = InternalSectorToErase(startAddress);

        if (flashSector > FLASH_Sector_11)
        {
            return false;
        }

        /* Disable memory write protection if any */
        FLASH_WriteProtectMemory(FLASH_INTERNAL, startAddress, length, false);

        /* Unlock the Flash Program Erase Controller */
        FLASH_Unlock();

        /* Define the number of Internal Flash pages to be erased */
        numPages = FLASH_PagesMask(length, INTERNAL_FLASH_PAGE_SIZE);

        /* Clear All pending flags */
        FLASH_ClearFlags();

        /* Erase the Internal Flash pages */
        for (eraseCounter = 0; (eraseCounter < numPages); eraseCounter++)
        {
            FLASH_Status status = FLASH_EraseSector(flashSector + (8 * eraseCounter), VoltageRange_3);

            /* If erase operation fails, return Failure */
            if (status != FLASH_COMPLETE)
            {
                return false;
            }
        }

        /* Locks the FLASH Program Erase Controller */
        FLASH_Lock();

        return true;
    }
    else if (flashDeviceID == FLASH_SERIAL)
    {
#ifdef USE_SERIAL_FLASH
        /* Initialize SPI Flash */
        sFLASH_Init();

        /* Define the number of External Flash pages to be erased */
        numPages = FLASH_PagesMask(length, sFLASH_PAGESIZE);

        /* Erase the SPI Flash pages */
        for (eraseCounter = 0; (eraseCounter < numPages); eraseCounter++)
        {
            sFLASH_EraseSector(startAddress + (sFLASH_PAGESIZE * eraseCounter));
        }

        /* Return Success */
        return true;
#endif
    }

    /* Return Failure */
    return false;
}

int FLASH_CheckCopyMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                      flash_device_t destinationDeviceID, uint32_t destinationAddress,
                      uint32_t length, uint8_t module_function, uint8_t flags)
{
    if (!FLASH_CheckValidAddressRange(sourceDeviceID, sourceAddress, length))
    {
        return FLASH_ACCESS_RESULT_BADARG;
    }

    if (!FLASH_CheckValidAddressRange(destinationDeviceID, destinationAddress, length))
    {
        return FLASH_ACCESS_RESULT_BADARG;
    }

#ifndef USE_SERIAL_FLASH    // this predates the module system (early P1's using external flash for storage)
    if ((sourceDeviceID == FLASH_INTERNAL) && (flags & MODULE_VERIFY_MASK))
    {
        uint32_t moduleLength = FLASH_ModuleLength(sourceDeviceID, sourceAddress);

        if((flags & (MODULE_VERIFY_LENGTH|MODULE_VERIFY_CRC)) && (length < moduleLength+4))
        {
            return FLASH_ACCESS_RESULT_BADARG;
        }

        const module_info_t* info = FLASH_ModuleInfo(sourceDeviceID, sourceAddress);
        if ((info->module_function != MODULE_FUNCTION_RESOURCE) && (info->platform_id != PLATFORM_ID))
        {
            return FLASH_ACCESS_RESULT_BADARG;
        }

        // verify destination address
        if ((flags & MODULE_VERIFY_DESTINATION_IS_START_ADDRESS) && (((uint32_t)info->module_start_address) != destinationAddress))
        {
            return FLASH_ACCESS_RESULT_BADARG;
        }

        if ((flags & MODULE_VERIFY_FUNCTION) && (info->module_function != module_function))
        {
            return FLASH_ACCESS_RESULT_BADARG;
        }

        if ((flags & MODULE_VERIFY_CRC) && !FLASH_VerifyCRC32(sourceDeviceID, sourceAddress, moduleLength))
        {
            return FLASH_ACCESS_RESULT_BADARG;
        }
    }
#endif
    return FLASH_ACCESS_RESULT_OK;
}

bool CopyFlashBlock(flash_device_t sourceDeviceID, uint32_t sourceAddress, flash_device_t destinationDeviceID, uint32_t destinationAddress, uint32_t length)
{
#ifdef USE_SERIAL_FLASH
    uint8_t serialFlashData[4];
#endif

	uint32_t endAddress = sourceAddress+length;

    if (!FLASH_EraseMemory(destinationDeviceID, destinationAddress, length))
    {
        return false;
    }

	if (destinationDeviceID == FLASH_INTERNAL)
	{
	    /* Locks the internal flash program erase controller */
	    FLASH_Unlock();
	}

	bool success = true;

	/* Program source to destination */
	while (sourceAddress < endAddress)
	{
	    uint32_t internalFlashData = 0;

		if (sourceDeviceID == FLASH_INTERNAL)
		{
			/* Read data from internal flash source address */
			internalFlashData = (*(__IO uint32_t*) sourceAddress);
		}
	#ifdef USE_SERIAL_FLASH
		else if (sourceDeviceID == FLASH_SERIAL)
		{
			/* Read data from serial flash source address */
			sFLASH_ReadBuffer(serialFlashData, sourceAddress, 4);
		}
	#endif

		if (destinationDeviceID == FLASH_INTERNAL)
		{
	#ifdef USE_SERIAL_FLASH
			if (sourceDeviceID == FLASH_SERIAL)
			{
				internalFlashData = (uint32_t)(serialFlashData[0] | (serialFlashData[1] << 8) | (serialFlashData[2] << 16) | (serialFlashData[3] << 24));
			}
	#endif

			/* Program data to internal flash destination address */
			FLASH_Status status = FLASH_ProgramWord(destinationAddress, internalFlashData);

			/* If program operation fails, return Failure */
			if (status != FLASH_COMPLETE)
			{
				success = false;
				break;
			}
		}
	#ifdef USE_SERIAL_FLASH
		else if (destinationDeviceID == FLASH_SERIAL)
		{
			if (sourceDeviceID == FLASH_INTERNAL)
			{
				serialFlashData[0] = (uint8_t)(internalFlashData & 0xFF);
				serialFlashData[1] = (uint8_t)((internalFlashData & 0xFF00) >> 8);
				serialFlashData[2] = (uint8_t)((internalFlashData & 0xFF0000) >> 16);
				serialFlashData[3] = (uint8_t)((internalFlashData & 0xFF000000) >> 24);
			}

			/* Program data to serial flash destination address */
			sFLASH_WriteBuffer(serialFlashData, destinationAddress, 4);
		}
	#endif

		sourceAddress += 4;
		destinationAddress += 4;
	}

	if (destinationDeviceID == FLASH_INTERNAL)
	{
	    /* Locks the internal flash program erase controller */
	    FLASH_Lock();
	}

	return success;
}

int FLASH_CopyMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                     flash_device_t destinationDeviceID, uint32_t destinationAddress,
                     uint32_t length, uint8_t module_function, uint8_t flags)
{
    if (FLASH_CheckCopyMemory(sourceDeviceID, sourceAddress, destinationDeviceID, destinationAddress, length, module_function, flags) != FLASH_ACCESS_RESULT_OK)
    {
        return FLASH_ACCESS_RESULT_BADARG;
    }

    if (sourceDeviceID == FLASH_SERIAL)
    {
#ifdef USE_SERIAL_FLASH
        /* Initialize SPI Flash */
        sFLASH_Init();
#endif
    }

    while (length)
    {
        uint32_t endBlockAddress = EndOfFlashSector(destinationDeviceID, destinationAddress);
    		uint32_t blockLength = endBlockAddress-destinationAddress;
    		if (blockLength>length)
    			blockLength = length;
    		if (!CopyFlashBlock(sourceDeviceID, sourceAddress, destinationDeviceID, destinationAddress, blockLength))
    			return FLASH_ACCESS_RESULT_ERROR;
    		length -= blockLength;
    		sourceAddress += blockLength;
    		destinationAddress += blockLength;
    }
    return FLASH_ACCESS_RESULT_OK;
}

bool FLASH_CompareMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                         flash_device_t destinationDeviceID, uint32_t destinationAddress,
                         uint32_t length)
{
	uint32_t endAddress = sourceAddress + length;

    if (FLASH_CheckValidAddressRange(sourceDeviceID, sourceAddress, length) != true)
    {
        return false;
    }

    if (FLASH_CheckValidAddressRange(destinationDeviceID, destinationAddress, length) != true)
    {
        return false;
    }

    if (sourceDeviceID == FLASH_SERIAL || destinationDeviceID == FLASH_SERIAL)
    {
#ifdef USE_SERIAL_FLASH
        /* Initialize SPI Flash */
        sFLASH_Init();
#endif
    }

    /* Program source to destination */
    while (sourceAddress < endAddress)
    {
    		uint32_t sourceDeviceData = 0;
        if (sourceDeviceID == FLASH_INTERNAL)
        {
            /* Read data from internal flash source address */
            sourceDeviceData = (*(__IO uint32_t*) sourceAddress);
        }
#ifdef USE_SERIAL_FLASH
        else if (sourceDeviceID == FLASH_SERIAL)
        {
            /* Read data from serial flash source address */
            sFLASH_ReadBuffer(serialFlashData, sourceAddress, 4);
            sourceDeviceData = (uint32_t)(serialFlashData[0] | (serialFlashData[1] << 8) | (serialFlashData[2] << 16) | (serialFlashData[3] << 24));
        }
#endif

        uint32_t destinationDeviceData = 0;
        if (destinationDeviceID == FLASH_INTERNAL)
        {
            /* Read data from internal flash destination address */
            destinationDeviceData = (*(__IO uint32_t*) destinationAddress);
        }
#ifdef USE_SERIAL_FLASH
        else if (destinationDeviceID == FLASH_SERIAL)
        {
            /* Read data from serial flash destination address */
            sFLASH_ReadBuffer(serialFlashData, destinationAddress, 4);
            destinationDeviceData = (uint32_t)(serialFlashData[0] | (serialFlashData[1] << 8) | (serialFlashData[2] << 16) | (serialFlashData[3] << 24));
        }
#endif

        if (sourceDeviceData != destinationDeviceData)
        {
            /* Failed comparison check */
            return false;
        }

        sourceAddress += 4;
        destinationAddress += 4;
    }

    /* Passed comparison check */
    return true;
}

bool FLASH_AddToNextAvailableModulesSlot(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                                         flash_device_t destinationDeviceID, uint32_t destinationAddress,
                                         uint32_t length, uint8_t function, uint8_t flags)
{
    platform_flash_modules_t flash_modules[MAX_MODULES_SLOT];
    uint8_t flash_module_index = MAX_MODULES_SLOT;

    //Read the flash modules info from the dct area
    dct_read_app_data_copy(DCT_FLASH_MODULES_OFFSET, flash_modules, sizeof(flash_modules));

    //fill up the next available modules slot and return true else false
    //slot 0 is reserved for factory reset module so start from flash_module_index = 1
    for (flash_module_index = GEN_START_SLOT; flash_module_index < MAX_MODULES_SLOT; flash_module_index++)
    {
        if(flash_modules[flash_module_index].magicNumber == 0xABCD)
        {
            continue;
        }
        else
        {
            flash_modules[flash_module_index].sourceDeviceID = sourceDeviceID;
            flash_modules[flash_module_index].sourceAddress = sourceAddress;
            flash_modules[flash_module_index].destinationDeviceID = destinationDeviceID;
            flash_modules[flash_module_index].destinationAddress = destinationAddress;
            flash_modules[flash_module_index].length = length;
            flash_modules[flash_module_index].magicNumber = 0xABCD;
            flash_modules[flash_module_index].module_function = function;
            flash_modules[flash_module_index].flags = flags;

            dct_write_app_data(&flash_modules[flash_module_index],
                               offsetof(application_dct_t, flash_modules[flash_module_index]),
                               sizeof(platform_flash_modules_t));

            return true;
        }
    }

    return false;
}

bool FLASH_AddToFactoryResetModuleSlot(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                                       flash_device_t destinationDeviceID, uint32_t destinationAddress,
                                       uint32_t length, uint8_t function, uint8_t flags)
{
    platform_flash_modules_t flash_module;
    platform_flash_modules_t flash_module_current;

    //Read the flash modules info from the dct area
    dct_read_app_data_copy(DCT_FLASH_MODULES_OFFSET + (FAC_RESET_SLOT * sizeof(platform_flash_modules_t)), &flash_module_current, sizeof(flash_module_current));
    flash_module = flash_module_current;

    flash_module.sourceDeviceID = sourceDeviceID;
    flash_module.sourceAddress = sourceAddress;
    flash_module.destinationDeviceID = destinationDeviceID;
    flash_module.destinationAddress = destinationAddress;
    flash_module.length = length;
    flash_module.magicNumber = 0x0FAC;
    flash_module.module_function = function;
    flash_module.flags = flags;

    if(memcmp(&flash_module, &flash_module_current, sizeof(flash_module)) != 0)
    {
        //Only write dct app data if factory reset module slot is different
        dct_write_app_data(&flash_module, DCT_FLASH_MODULES_OFFSET + (FAC_RESET_SLOT * sizeof(platform_flash_modules_t)), sizeof(platform_flash_modules_t));
    }

    return true;
}

bool FLASH_ClearFactoryResetModuleSlot(void)
{
    // Mark slot as unused
    const size_t magic_num_offs = DCT_FLASH_MODULES_OFFSET + sizeof(platform_flash_modules_t) * FAC_RESET_SLOT +
            offsetof(platform_flash_modules_t, magicNumber);
    const uint16_t magic_num = 0xffff;
    return (dct_write_app_data(&magic_num, magic_num_offs, sizeof(magic_num)) == 0);
}

int FLASH_ApplyFactoryResetImage(copymem_fn_t copy)
{
    platform_flash_modules_t flash_module;
    int restoreFactoryReset = FLASH_ACCESS_RESULT_ERROR;

    //Read the flash modules info from the dct area
    dct_read_app_data_copy(DCT_FLASH_MODULES_OFFSET + (FAC_RESET_SLOT * sizeof(flash_module)), &flash_module, sizeof(flash_module));

    if(flash_module.magicNumber == 0x0FAC)
    {
        //Restore Factory Reset Firmware (slot 0 is factory reset module)
        restoreFactoryReset = copy(flash_module.sourceDeviceID,
                                   flash_module.sourceAddress,
                                   flash_module.destinationDeviceID,
                                   flash_module.destinationAddress,
                                   flash_module.length,
                                   flash_module.module_function,
                                   flash_module.flags);
    }
    else
    {
        // attempt to use the default that the bootloader was built with
        restoreFactoryReset = copy(FLASH_INTERNAL, INTERNAL_FLASH_FAC_ADDRESS, FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION, FIRMWARE_IMAGE_SIZE,
            FACTORY_RESET_MODULE_FUNCTION,
            MODULE_VERIFY_CRC | MODULE_VERIFY_DESTINATION_IS_START_ADDRESS | MODULE_VERIFY_FUNCTION);
    }
    return restoreFactoryReset;

}

bool FLASH_IsFactoryResetAvailable(void)
{
    return !FLASH_ApplyFactoryResetImage(FLASH_CheckCopyMemory);
}

bool FLASH_RestoreFromFactoryResetModuleSlot(void)
{
    return !FLASH_ApplyFactoryResetImage(FLASH_CopyMemory);
}

//This function called in bootloader to perform the memory update process
bool FLASH_UpdateModules(void (*flashModulesCallback)(bool isUpdating))
{
    // Copy module info from DCT before updating any modules, since bootloader might load DCT
    // functions dynamically. FAC_RESET_SLOT is reserved for factory reset module
    const size_t max_module_count = MAX_MODULES_SLOT - GEN_START_SLOT;
    platform_flash_modules_t modules[max_module_count];
    size_t module_offs = DCT_FLASH_MODULES_OFFSET + sizeof(platform_flash_modules_t) * GEN_START_SLOT;
    size_t module_count = 0;
    for (size_t i = 0; i < max_module_count; ++i) {
        const size_t magic_num_offs = module_offs + offsetof(platform_flash_modules_t, magicNumber);
        uint16_t magic_num = 0;
        if (dct_read_app_data_copy(magic_num_offs, &magic_num, sizeof(magic_num)) != 0) {
            return false;
        }
        if (magic_num == 0xabcd) {
            // Copy module info
            if (dct_read_app_data_copy(module_offs, &modules[module_count], sizeof(platform_flash_modules_t)) != 0) {
                return false;
            }
            // Mark slot as unused
            magic_num = 0xffff;
            if (dct_write_app_data(&magic_num, magic_num_offs, sizeof(magic_num)) != 0) {
                return false;
            }
            ++module_count;
        }
        module_offs += sizeof(platform_flash_modules_t);
    }
    for (size_t i = 0; i < module_count; ++i) {
        const platform_flash_modules_t* module = &modules[i];
        // Turn On RGB_COLOR_MAGENTA toggling during flash updating
        if (flashModulesCallback) {
            flashModulesCallback(true);
        }
        // Copy memory from source to destination based on flash device id
        FLASH_CopyMemory(module->sourceDeviceID, module->sourceAddress, module->destinationDeviceID,
                module->destinationAddress, module->length, module->module_function, module->flags);
        // Turn Off RGB_COLOR_MAGENTA toggling
        if (flashModulesCallback) {
            flashModulesCallback(false);
        }
    }
    return true;
}

const module_info_t* FLASH_ModuleInfo(uint8_t flashDeviceID, uint32_t startAddress)
{
    if(flashDeviceID == FLASH_INTERNAL)
    {
        if (((*(__IO uint32_t*)startAddress) & APP_START_MASK) == 0x20000000)
        {
            startAddress += 0x184;
        }

        const module_info_t* module_info = (const module_info_t*)startAddress;

        return module_info;
    }

    return NULL;
}

uint32_t FLASH_ModuleAddress(uint8_t flashDeviceID, uint32_t startAddress)
{
    const module_info_t* module_info = FLASH_ModuleInfo(flashDeviceID, startAddress);

    if (module_info != NULL)
    {
        return (uint32_t)module_info->module_start_address;
    }

    return 0;
}

uint32_t FLASH_ModuleLength(uint8_t flashDeviceID, uint32_t startAddress)
{
    const module_info_t* module_info = FLASH_ModuleInfo(flashDeviceID, startAddress);

    if (module_info != NULL)
    {
        return ((uint32_t)module_info->module_end_address - (uint32_t)module_info->module_start_address);
    }

    return 0;
}

bool FLASH_isUserModuleInfoValid(uint8_t flashDeviceID, uint32_t startAddress, uint32_t expectedAddress)
{
    const module_info_t* module_info = FLASH_ModuleInfo(flashDeviceID, startAddress);

    return (module_info != NULL
            && ((uint32_t)(module_info->module_start_address) == expectedAddress)
            && ((uint32_t)(module_info->module_end_address)<=0x8100000)
            && (module_info->platform_id==PLATFORM_ID));
}

bool FLASH_VerifyCRC32(uint8_t flashDeviceID, uint32_t startAddress, uint32_t length)
{
    if(flashDeviceID == FLASH_INTERNAL && length > 0)
    {
        uint32_t expectedCRC = __REV((*(__IO uint32_t*) (startAddress + length)));
        uint32_t computedCRC = Compute_CRC32((uint8_t*)startAddress, length);

        if (expectedCRC == computedCRC)
        {
            return true;
        }
    }

    return false;
}

void FLASH_ClearFlags(void)
{
    /* Clear All pending flags */
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                    FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
}

void FLASH_WriteProtection_Enable(uint32_t FLASH_Sectors)
{
    /* Get FLASH_Sectors write protection status */
    uint32_t SectorsWRPStatus = FLASH_OB_GetWRP() & FLASH_Sectors;

    if (SectorsWRPStatus != 0)
    {
        //If FLASH_Sectors are not write protected, enable the write protection

        /* Enable the Flash option control register access */
        FLASH_OB_Unlock();

        /* Clear All pending flags */
        FLASH_ClearFlags();

        /* Enable FLASH_Sectors write protection */
        FLASH_OB_WRPConfig(FLASH_Sectors, ENABLE);

        /* Start the Option Bytes programming process */
        if (FLASH_OB_Launch() != FLASH_COMPLETE)
        {
            //Error during Option Bytes programming process
        }

        /* Disable the Flash option control register access */
        FLASH_OB_Lock();

        /* Get FLASH_Sectors write protection status */
        SectorsWRPStatus = FLASH_OB_GetWRP() & FLASH_Sectors;

        /* Check if FLASH_Sectors are write protected */
        if (SectorsWRPStatus == 0)
        {
            //Write Protection Enable Operation is done correctly
        }
    }
}

void FLASH_WriteProtection_Disable(uint32_t FLASH_Sectors)
{
    /* Get FLASH_Sectors write protection status */
    uint32_t SectorsWRPStatus = FLASH_OB_GetWRP() & FLASH_Sectors;

    if (SectorsWRPStatus == 0)
    {
        //If FLASH_Sectors are write protected, disable the write protection

        /* Enable the Flash option control register access */
        FLASH_OB_Unlock();

        /* Clear All pending flags */
        FLASH_ClearFlags();

        /* Disable FLASH_Sectors write protection */
        FLASH_OB_WRPConfig(FLASH_Sectors, DISABLE);

        /* Start the Option Bytes programming process */
        if (FLASH_OB_Launch() != FLASH_COMPLETE)
        {
            //Error during Option Bytes programming process
        }

        /* Disable the Flash option control register access */
        FLASH_OB_Lock();

        /* Get FLASH_Sectors write protection status */
        SectorsWRPStatus = FLASH_OB_GetWRP() & FLASH_Sectors;

        /* Check if FLASH_Sectors write protection is disabled */
        if (SectorsWRPStatus == FLASH_Sectors)
        {
            //Write Protection Disable Operation is done correctly
        }
    }
}

void FLASH_Erase(void)
{
    // This is too dangerous.
    //FLASH_EraseMemory(FLASH_INTERNAL, CORE_FW_ADDRESS, FIRMWARE_IMAGE_SIZE);
}

void FLASH_Backup(uint32_t FLASH_Address)
{
#ifdef USE_SERIAL_FLASH
    FLASH_CopyMemory(FLASH_INTERNAL, CORE_FW_ADDRESS, FLASH_SERIAL, FLASH_Address, FIRMWARE_IMAGE_SIZE, 0, 0);
#else
    //Don't have enough space in Internal Flash to save a Backup copy of the firmware
#endif
}

void FLASH_Restore(uint32_t FLASH_Address)
{
#ifdef USE_SERIAL_FLASH
    //CRC verification Disabled by default
    FLASH_CopyMemory(FLASH_SERIAL, FLASH_Address, FLASH_INTERNAL, CORE_FW_ADDRESS, FIRMWARE_IMAGE_SIZE, 0, 0);
#else
    //commented below since FIRMWARE_IMAGE_SIZE != Actual factory firmware image size
    //FLASH_CopyMemory(FLASH_INTERNAL, FLASH_Address, FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION, FIRMWARE_IMAGE_SIZE, true);
    //FLASH_AddToFactoryResetModuleSlot() is now called in HAL_Core_Config() in core_hal.c
#endif
}

uint32_t FLASH_PagesMask(uint32_t imageSize, uint32_t pageSize)
{
    //Calculate the number of flash pages that needs to be erased
    uint32_t numPages = 0x0;

    if ((imageSize % pageSize) != 0)
    {
        numPages = (imageSize / pageSize) + 1;
    }
    else
    {
        numPages = imageSize / pageSize;
    }

    return numPages;
}

void FLASH_Begin(uint32_t FLASH_Address, uint32_t imageSize)
{
    system_flags.OTA_FLASHED_Status_SysFlag = 0x0000;
    Save_SystemFlags();

#ifdef USE_SERIAL_FLASH
    FLASH_EraseMemory(FLASH_SERIAL, FLASH_Address, imageSize);
#else
    FLASH_EraseMemory(FLASH_INTERNAL, FLASH_Address, imageSize);
#endif
}

int FLASH_Update(const uint8_t *pBuffer, uint32_t address, uint32_t bufferSize)
{
#ifdef USE_SERIAL_FLASH
    const uint8_t *writeBuffer = pBuffer;
    uint8_t readBuffer[bufferSize];

    readBuffer[0] = writeBuffer[0]+1;       // ensure different
    int i;

    for (i=50; i-->0 && memcmp(writeBuffer, readBuffer, bufferSize); )
    {
        /* Write Data Buffer to SPI Flash memory */
        sFLASH_WriteBuffer(writeBuffer, address, bufferSize);

        /* Read Data Buffer from SPI Flash memory */
        sFLASH_ReadBuffer(readBuffer, address, bufferSize);
    }

    return !i;
#else
    uint32_t index = 0;

    /* Unlock the internal flash */
    FLASH_Unlock();

    FLASH_ClearFlags();

    /* Data received are Word multiple */
    for (index = 0; index < (bufferSize & 0xFFFC); index += 4)
    {
        FLASH_ProgramWord(address, *(uint32_t *)(pBuffer + index));
        address += 4;
    }

    if (bufferSize & 0x3) /* Not an aligned data */
    {
        char buf[4];
        memset(buf, 0xFF, 4);

        for (index = bufferSize&3; index -->0; )
        {
            buf[index] = pBuffer[ (bufferSize & 0xFFFC)+index ];
        }
        FLASH_ProgramWord(address, *(uint32_t *)(pBuffer + index));

    }

    /* Lock the internal flash */
    FLASH_Lock();

    return 0;
#endif
}

void FLASH_End(void)
{
#ifdef USE_SERIAL_FLASH
    system_flags.FLASH_OTA_Update_SysFlag = 0x0005;
    Save_SystemFlags();

    RTC_WriteBackupRegister(RTC_BKP_DR10, 0x0005);
#else
    //FLASH_AddToNextAvailableModulesSlot() should be called in system_update.cpp
#endif
}
