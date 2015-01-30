/**
 ******************************************************************************
 * @file    flash_mal.c
 * @author  Satish Nair
 * @version V1.0.0
 * @date    30-Jan-2015
 * @brief   Media access layer for platform dependent flash interfaces
 ******************************************************************************
  Copyright (c) 2015 Spark Labs, Inc.  All rights reserved.

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
#include "dct.h"
#include <string.h>

/* Private variables ---------------------------------------------------------*/

uint32_t Internal_Flash_Address = 0;
uint32_t Internal_Flash_Data = 0;
uint16_t Flash_Update_Index = 0;
volatile FLASH_Status FLASHStatus = FLASH_COMPLETE;
#ifdef USE_SERIAL_FLASH
uint32_t External_Flash_Address = 0;
uint8_t External_Flash_Data[4];
static uint32_t External_Flash_Start_Address = 0;
#endif

/* Private functions ---------------------------------------------------------*/

uint16_t FLASH_SectorToErase(uint8_t flashDeviceID, uint32_t startAddress)
{
    uint16_t flashSector = 0xFFFF;//Invalid sector

    if (flashDeviceID != FLASH_INTERNAL)
    {
        return flashSector;
    }

    if (startAddress < 0x08004000)
    {
        flashSector = FLASH_Sector_0;
    }
    else if (startAddress < 0x08008000)
    {
        flashSector = FLASH_Sector_1;
    }
    else if (startAddress < 0x0800C000)
    {
        flashSector = FLASH_Sector_2;
    }
    else if (startAddress < 0x08010000)
    {
        flashSector = FLASH_Sector_3;
    }
    else if (startAddress < 0x08020000)
    {
        flashSector = FLASH_Sector_4;
    }
    else if (startAddress < 0x08040000)
    {
        flashSector = FLASH_Sector_5;
    }
    else if (startAddress < 0x08060000)
    {
        flashSector = FLASH_Sector_6;
    }
    else if (startAddress < 0x08080000)
    {
        flashSector = FLASH_Sector_7;
    }
    else if (startAddress < 0x080A0000)
    {
        flashSector = FLASH_Sector_8;
    }
    else if (startAddress < 0x080C0000)
    {
        flashSector = FLASH_Sector_9;
    }
    else if (startAddress < 0x080E0000)
    {
        flashSector = FLASH_Sector_10;
    }
    else if (startAddress < 0x08100000)
    {
        flashSector = FLASH_Sector_11;
    }

    return flashSector;
}

bool FLASH_CheckValidAddressRange(uint8_t flashDeviceID, uint32_t startAddress, uint32_t length)
{
    uint32_t endAddress = startAddress + length - 1;

    if (flashDeviceID == FLASH_INTERNAL)
    {
        if (startAddress < 0x08020000 || endAddress >= 0x08100000)
        {
            return false;
        }
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

bool FLASH_EraseMemory(uint8_t flashDeviceID, uint32_t startAddress, uint32_t length)
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
        uint16_t flashSector = FLASH_SectorToErase(FLASH_INTERNAL, startAddress);

        if (flashSector > FLASH_Sector_11)
        {
            return false;
        }

        /* Unlock the Flash Program Erase Controller */
        FLASH_Unlock();

        /* Define the number of Internal Flash pages to be erased */
        numPages = FLASH_PagesMask(length, INTERNAL_FLASH_PAGE_SIZE);

        /* Clear All pending flags */
        FLASH_ClearFlags();

        /* Erase the Internal Flash pages */
        for (eraseCounter = 0; (eraseCounter < numPages); eraseCounter++)
        {
            FLASHStatus = FLASH_EraseSector(flashSector + (8 * eraseCounter), VoltageRange_3);

            /* If erase operation fails, return Failure */
            if (FLASHStatus != FLASH_COMPLETE)
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

bool FLASH_CopyMemory(uint8_t sourceDeviceID, uint32_t sourceAddress,
                      uint8_t destinationDeviceID, uint32_t destinationAddress,
                      uint32_t length)
{
#ifdef USE_SERIAL_FLASH
    uint8_t serialFlashData[4];
#endif
    uint32_t internalFlashData = 0;
    uint32_t endAddress = sourceAddress + length - 1;

    if (FLASH_CheckValidAddressRange(sourceDeviceID, sourceAddress, length) != true)
    {
        return false;
    }

    if (FLASH_CheckValidAddressRange(destinationDeviceID, destinationAddress, length) != true)
    {
        return false;
    }

    if (FLASH_EraseMemory(destinationDeviceID, destinationAddress, length) != true)
    {
        return false;
    }

    if (sourceDeviceID == FLASH_SERIAL)
    {
#ifdef USE_SERIAL_FLASH
        /* Initialize SPI Flash */
        sFLASH_Init();
#endif
    }

    if (destinationDeviceID == FLASH_INTERNAL)
    {
        /* Unlock the internal flash program erase controller */
        FLASH_Unlock();
    }

    /* Program source to destination */
    while (sourceAddress < endAddress)
    {
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
            FLASHStatus = FLASH_ProgramWord(destinationAddress, internalFlashData);

            /* If program operation fails, return Failure */
            if (FLASHStatus != FLASH_COMPLETE)
            {
                return false;
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

    return true;
}

bool FLASH_CompareMemory(uint8_t sourceDeviceID, uint32_t sourceAddress,
                         uint8_t destinationDeviceID, uint32_t destinationAddress,
                         uint32_t length)
{
#ifdef USE_SERIAL_FLASH
    uint8_t serialFlashData[4];
#endif
    uint32_t sourceDeviceData = 0;
    uint32_t destinationDeviceData = 0;
    uint32_t endAddress = sourceAddress + length - 1;

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

//This function called in bootloader to perform the memory update process
void FLASH_UpdateModules(void)
{
    platform_flash_modules_t flash_modules[FLASH_MODULES_MAX];
    uint8_t flash_module_index = 0;
    uint8_t updateStatusFlags = 0x0;

    //Fill the Flash modules info data read from the dct area
    const void* dct_app_data = dct_read_app_data(DCT_FLASH_MODULES_OFFSET);
    memcpy(flash_modules, dct_app_data, sizeof(flash_modules));

    for (flash_module_index = 0; flash_module_index < FLASH_MODULES_MAX; flash_module_index++)
    {
        if(flash_modules[flash_module_index].statusFlag == 0x1)
        {
            //Copy memory from source to destination based on flash device id
            bool copy_result = FLASH_CopyMemory(flash_modules[flash_module_index].sourceDeviceID,
                                                flash_modules[flash_module_index].sourceAddress,
                                                flash_modules[flash_module_index].destinationDeviceID,
                                                flash_modules[flash_module_index].destinationAddress,
                                                flash_modules[flash_module_index].length);

            if(copy_result != false)
            {
                updateStatusFlags |= flash_modules[flash_module_index].statusFlag;
                flash_modules[flash_module_index].statusFlag = 0x0; //Reset statusFlag
            }
        }
    }

    if(updateStatusFlags == 0x1)
    {
        //Only update DCT if required
        dct_write_app_data(flash_modules, DCT_FLASH_MODULES_OFFSET, sizeof(flash_modules));
    }
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

        /* Generate System Reset (not mandatory on F2 series ???) */
        NVIC_SystemReset();
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

        /* Generate System Reset (not mandatory on F2 series ???) */
        NVIC_SystemReset();
    }
}

void FLASH_Erase(void)
{
    FLASH_EraseMemory(FLASH_INTERNAL, CORE_FW_ADDRESS, FIRMWARE_IMAGE_SIZE);
}

void FLASH_Backup(uint32_t FLASH_Address)
{
#ifdef USE_SERIAL_FLASH
    FLASH_CopyMemory(FLASH_INTERNAL, CORE_FW_ADDRESS, FLASH_SERIAL, FLASH_Address, FIRMWARE_IMAGE_SIZE);
#endif
}

void FLASH_Restore(uint32_t FLASH_Address)
{
#ifdef USE_SERIAL_FLASH
    FLASH_CopyMemory(FLASH_SERIAL, FLASH_Address, FLASH_INTERNAL, CORE_FW_ADDRESS, FIRMWARE_IMAGE_SIZE);
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
#ifdef USE_SERIAL_FLASH
    system_flags.OTA_FLASHED_Status_SysFlag = 0x0000;
    Save_SystemFlags();

    Flash_Update_Index = 0;
    External_Flash_Start_Address = FLASH_Address;
    External_Flash_Address = External_Flash_Start_Address;

    FLASH_EraseMemory(FLASH_SERIAL, External_Flash_Start_Address, imageSize);
#endif
}

uint16_t FLASH_Update(uint8_t *pBuffer, uint32_t bufferSize)
{
#ifdef USE_SERIAL_FLASH
    uint8_t *writeBuffer = pBuffer;
    uint8_t readBuffer[bufferSize];

    /* Write Data Buffer to SPI Flash memory */
    sFLASH_WriteBuffer(writeBuffer, External_Flash_Address, bufferSize);

    /* Read Data Buffer from SPI Flash memory */
    sFLASH_ReadBuffer(readBuffer, External_Flash_Address, bufferSize);

    /* Is the Data Buffer successfully programmed to SPI Flash memory */
    if (0 == memcmp(writeBuffer, readBuffer, bufferSize))
    {
        External_Flash_Address += bufferSize;
        Flash_Update_Index += 1;
    }
    else
    {
        /* Erase the problematic SPI Flash pages and back off the chunk index */
        External_Flash_Address = ((uint32_t)(External_Flash_Address / sFLASH_PAGESIZE)) * sFLASH_PAGESIZE;
        sFLASH_EraseSector(External_Flash_Address);
        Flash_Update_Index = (uint16_t)((External_Flash_Address - External_Flash_Start_Address) / bufferSize);
    }
#endif

    return Flash_Update_Index;
}

void FLASH_End(void)
{
#ifdef USE_SERIAL_FLASH
    system_flags.FLASH_OTA_Update_SysFlag = 0x0005;
    Save_SystemFlags();

    RTC_WriteBackupRegister(RTC_BKP_DR10, 0x0005);

    USB_Cable_Config(DISABLE);

    NVIC_SystemReset();
#endif
}
