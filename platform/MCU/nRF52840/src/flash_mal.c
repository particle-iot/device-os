/**
 ******************************************************************************
 * @file    flash_mal.c
  * @authors Eugene
  * @version V1.0.0
  * @date    23-4-2018
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
//#include <string.h>
#include "hw_config.h"
#include "module_info.h"
#include "module_info_hal.h"
#include "dct.h"
#include "flash_mal.h"
#include "flash_hal.h"
#include "exflash_hal.h"


#define CEIL_DIV(A, B)        (((A) + (B) - 1) / (B))

#define MAX_COPY_LENGTH     256


/* Private functions ---------------------------------------------------------*/


bool FLASH_CheckValidAddressRange(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length)
{
    uint32_t endAddress = startAddress + length - 1;

    if (flashDeviceID == FLASH_INTERNAL)
    {
        return (startAddress >= 0x00000000 && endAddress <= 0x100000) ||
               (startAddress >= 0x12000000 && endAddress <= 0x20000000);
    }
    else if (flashDeviceID == FLASH_SERIAL)
    {
#ifdef USE_SERIAL_FLASH
        return startAddress >= 0x00000000 && endAddress <= 0x400000;
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

bool FLASH_EraseMemory(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length)
{
    uint32_t numSectors;

    if (FLASH_CheckValidAddressRange(flashDeviceID, startAddress, length) != true)
    {
        return false;
    }

    if (flashDeviceID == FLASH_INTERNAL)
    {
        numSectors = CEIL_DIV(length, INTERNAL_FLASH_PAGE_SIZE);

        if (hal_flash_erase_sector(startAddress, numSectors) != 0)
        {
            return false;
        }

        return true;
    }
#ifdef USE_SERIAL_FLASH
    else if (flashDeviceID == FLASH_SERIAL)
    {
        numSectors = CEIL_DIV(length, sFLASH_PAGESIZE);

        if (hal_exflash_erase_sector(startAddress, numSectors) != 0)
        {
            return false;
        }

        return true;
    }
#endif

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

    if (flags & MODULE_VERIFY_MASK)
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

    return FLASH_ACCESS_RESULT_OK;
}

int FLASH_CopyMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                     flash_device_t destinationDeviceID, uint32_t destinationAddress,
                     uint32_t length, uint8_t module_function, uint8_t flags)
{
    uint32_t endAddress = sourceAddress + length;

    if (FLASH_CheckCopyMemory(sourceDeviceID, sourceAddress, destinationDeviceID, destinationAddress, length, module_function, flags) != FLASH_ACCESS_RESULT_OK)
    {
        return FLASH_ACCESS_RESULT_BADARG;
    }

    // erase sectors
    uint16_t sector_num;
    if (destinationDeviceID == FLASH_SERIAL)
    {
        sector_num = CEIL_DIV(length, sFLASH_PAGESIZE);
        if (hal_exflash_erase_sector(destinationAddress, sector_num))
        {
            return FLASH_ACCESS_RESULT_ERROR;
        }
    }
    else if (destinationDeviceID == FLASH_INTERNAL)
    {
        sector_num = CEIL_DIV(length, INTERNAL_FLASH_PAGE_SIZE);
        if (hal_flash_erase_sector(destinationAddress, sector_num))
        {
            return FLASH_ACCESS_RESULT_ERROR;
        }
    }
    else
    {
        return FLASH_ACCESS_RESULT_BADARG;
    }

    uint8_t data_buf[MAX_COPY_LENGTH];
    uint32_t copy_len = 0;

    /* Program source to destination */
    while (sourceAddress < endAddress)
    {
        copy_len = (endAddress - sourceAddress) >= MAX_COPY_LENGTH ? MAX_COPY_LENGTH : (endAddress - sourceAddress);

        // Read data from source memory address
        if (sourceDeviceID == FLASH_SERIAL)
        {
            if (hal_exflash_read(sourceAddress, data_buf, copy_len))
            {
                return FLASH_ACCESS_RESULT_ERROR;
            }
        }
        else
        {
            if (hal_flash_read(sourceAddress, data_buf, copy_len))
            {
                return FLASH_ACCESS_RESULT_ERROR;
            }
        }

        // Program data to destination memory address
        if (destinationDeviceID == FLASH_SERIAL)
        {
            if (hal_exflash_write(destinationAddress, data_buf, copy_len))
            {
                return FLASH_ACCESS_RESULT_ERROR;
            }
        }
        else
        {
            if (hal_flash_write(destinationAddress, data_buf, copy_len))
            {
                return FLASH_ACCESS_RESULT_ERROR;
            }
        }

        sourceAddress += copy_len;
        destinationAddress += copy_len;
    }

    return FLASH_ACCESS_RESULT_OK;
}

bool FLASH_CompareMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                         flash_device_t destinationDeviceID, uint32_t destinationAddress,
                         uint32_t length)
{
    uint32_t endAddress = sourceAddress + length;

    // check address range
    if (FLASH_CheckValidAddressRange(sourceDeviceID, sourceAddress, length) != true)
    {
        return false;
    }

    if (FLASH_CheckValidAddressRange(destinationDeviceID, destinationAddress, length) != true)
    {
        return false;
    }

    uint8_t src_buf[MAX_COPY_LENGTH];
    uint8_t dest_buf[MAX_COPY_LENGTH];
    uint32_t copy_len = 0;

    /* Program source to destination */
    while (sourceAddress < endAddress)
    {
        copy_len = (endAddress - sourceAddress) >= MAX_COPY_LENGTH ? MAX_COPY_LENGTH : (endAddress - sourceAddress);

        // Read data from source memory address
        if (sourceDeviceID == FLASH_SERIAL)
        {
            if (hal_exflash_read(sourceAddress, src_buf, copy_len))
            {
                return false;
            }
        }
        else
        {
            if (hal_flash_read(sourceAddress, src_buf, copy_len))
            {
                return false;
            }
        }

        // Read data from destination memory address
        if (sourceDeviceID == FLASH_SERIAL)
        {
            if (hal_exflash_read(destinationAddress, dest_buf, copy_len))
            {
                return false;
            }
        }
        else
        {
            if (hal_flash_read(destinationAddress, dest_buf, copy_len))
            {
                return false;
            }
        }

        if (memcmp(src_buf, dest_buf, copy_len))
        {
            /* Failed comparison check */
            return false;
        }

        sourceAddress += copy_len;
        destinationAddress += copy_len;
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
    const size_t magic_num_offs = DCT_FLASH_MODULES_OFFSET + sizeof(platform_flash_modules_t) * FAC_RESET_SLOT + \
            offsetof(platform_flash_modules_t, magicNumber);

    const uint16_t magic_num = 0xffff;
    return (dct_write_app_data(&magic_num, magic_num_offs, sizeof(magic_num)) == 0);
    return 0;
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
#ifdef USE_SERIAL_FLASH
    static module_info_t ex_module_info; // FIXME: This is not thread-safe
#endif

    if(flashDeviceID == FLASH_INTERNAL)
    {
        if (((*(__IO uint32_t*)startAddress) & APP_START_MASK) == 0x20000000)
        {
            startAddress += 0x200;
        }

        const module_info_t* module_info = (const module_info_t*)startAddress;

        return module_info;
    }
#ifdef USE_SERIAL_FLASH
    else if(flashDeviceID == FLASH_SERIAL)
    {
        uint8_t serialFlashData[4];
        uint32_t first_word = 0;

        hal_exflash_read(startAddress, serialFlashData, 4);
        first_word = (uint32_t)(serialFlashData[0] | (serialFlashData[1] << 8) | (serialFlashData[2] << 16) | (serialFlashData[3] << 24));
        if ((first_word & APP_START_MASK) == 0x20000000)
        {
            startAddress += 0x200;
        }

        memset((uint8_t *)&ex_module_info, 0x00, sizeof(module_info_t));
        hal_exflash_read(startAddress, (uint8_t *)&ex_module_info, sizeof(module_info_t));

        return &ex_module_info;
#endif
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

uint16_t FLASH_ModuleVersion(uint8_t flashDeviceID, uint32_t startAddress)
{
    const module_info_t* module_info = FLASH_ModuleInfo(flashDeviceID, startAddress);

    if (module_info != NULL)
    {
        return module_info->module_version;
    }

    return 0;
}

bool FLASH_isUserModuleInfoValid(uint8_t flashDeviceID, uint32_t startAddress, uint32_t expectedAddress)
{
    const module_info_t* module_info = FLASH_ModuleInfo(flashDeviceID, startAddress);

    return (module_info != NULL
            && ((uint32_t)(module_info->module_start_address) == expectedAddress)
            && ((uint32_t)(module_info->module_end_address)<=0x100000)
            && (module_info->platform_id==PLATFORM_ID));
}

bool FLASH_VerifyCRC32(uint8_t flashDeviceID, uint32_t startAddress, uint32_t length)
{
    if(flashDeviceID == FLASH_INTERNAL && length > 0)
    {
        uint32_t expectedCRC = __REV((*(__IO uint32_t*) (startAddress + length)));
        uint32_t computedCRC = Compute_CRC32((uint8_t*)startAddress, length, NULL);

        if (expectedCRC == computedCRC)
        {
            return true;
        }
    }
#ifdef USE_SERIAL_FLASH
    else if(flashDeviceID == FLASH_SERIAL && length > 0)
    {
        uint8_t serialFlashData[4]; // FIXME: Use XIP or a larger buffer
        hal_exflash_read((startAddress + length), serialFlashData, 4);
        uint32_t expectedCRC = (uint32_t)(serialFlashData[3] | (serialFlashData[2] << 8) | (serialFlashData[1] << 16) | (serialFlashData[0] << 24));

        uint32_t endAddress = startAddress + length;
        uint32_t len = 0;
        uint32_t computedCRC = 0;

        do {
            len = endAddress - startAddress;
            if (len > sizeof(serialFlashData)) {
                len = sizeof(serialFlashData);
            }
            hal_exflash_read(startAddress, serialFlashData, len);

            computedCRC = Compute_CRC32(serialFlashData, len, &computedCRC);

            startAddress += len;
        } while (startAddress < endAddress);

        if (expectedCRC == computedCRC)
        {
            return true;
        }
#endif
    }

    return false;
}

void FLASH_ClearFlags(void)
{
    return;
}

void FLASH_WriteProtection_Enable(uint32_t FLASH_Sectors)
{
    // TBD
    return;
}

void FLASH_WriteProtection_Disable(uint32_t FLASH_Sectors)
{
    // TBD
    return;
}

void FLASH_Erase(void)
{
    // This is too dangerous.
    //FLASH_EraseMemory(FLASH_INTERNAL, CORE_FW_ADDRESS, FIRMWARE_IMAGE_SIZE);
    return;
}

void FLASH_Backup(uint32_t FLASH_Address)
{
    // TBD
    return;
}

void FLASH_Restore(uint32_t FLASH_Address)
{
#ifdef USE_SERIAL_FLASH
    //CRC verification Disabled by default
    //FLASH_CopyMemory(FLASH_SERIAL, FLASH_Address, FLASH_INTERNAL, CORE_FW_ADDRESS, FIRMWARE_IMAGE_SIZE, 0, 0);
#else
    //commented below since FIRMWARE_IMAGE_SIZE != Actual factory firmware image size
    //FLASH_CopyMemory(FLASH_INTERNAL, FLASH_Address, FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION, FIRMWARE_IMAGE_SIZE, true);
    //FLASH_AddToFactoryResetModuleSlot() is now called in HAL_Core_Config() in core_hal.c
#endif
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
    int ret = -1;
#ifdef USE_SERIAL_FLASH
    ret = hal_exflash_write(address, pBuffer, bufferSize);
#else
    ret = hal_flash_write(address, pBuffer, bufferSize);
#endif
    return ret;
}

void FLASH_End(void)
{
    //FLASH_AddToNextAvailableModulesSlot() should be called in system_update.cpp
}
