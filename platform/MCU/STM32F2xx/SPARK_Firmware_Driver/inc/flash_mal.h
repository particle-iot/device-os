/**
 ******************************************************************************
 * @file    flash_mal.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    30-Jan-2015
 * @brief   Header for flash media access layer
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FLASH_MAL_H
#define __FLASH_MAL_H

#include "hw_config.h"
#include "flash_device_hal.h"
#include "module_info.h"
#include "module_info_hal.h"

/* Includes ------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macros ------------------------------------------------------------*/
#ifndef INTERNAL_FLASH_SIZE
#   error "INTERNAL_FLASH_SIZE not defined"
#endif
       
/* Internal Flash memory address where various firmwares are located */
#ifndef INTERNAL_FLASH_START
#define INTERNAL_FLASH_START        ((uint32_t)0x08000000)
#endif

//Bootloader firmware at the start of internal flash
#define USB_DFU_ADDRESS             INTERNAL_FLASH_START
//Main firmware begin address after 128KB (4 x 16K + 64K) from start of flash
#define CORE_FW_ADDRESS             ((uint32_t)0x08020000)
#define APP_START_MASK              ((uint32_t)0x2FF10000)

/* Internal Flash page size */
#define INTERNAL_FLASH_PAGE_SIZE    ((uint32_t)0x20000) //128K (7 sectors of 128K each used by main firmware)
    
#ifdef MODULAR_FIRMWARE
    #define FACTORY_RESET_MODULE_FUNCTION MODULE_FUNCTION_USER_PART
    #ifndef USER_FIRMWARE_IMAGE_SIZE
    #error USER_FIRMWARE_IMAGE_SIZE not defined
    #else
    #define FIRMWARE_IMAGE_SIZE  USER_FIRMWARE_IMAGE_SIZE    
    #endif
    
    #ifndef USER_FIRMWARE_IMAGE_LOCATION
    #error USER_FIRMWARE_IMAGE_LOCATION not defined
    #endif
    
    #define INTERNAL_FLASH_OTA_ADDRESS (USER_FIRMWARE_IMAGE_LOCATION+FIRMWARE_IMAGE_SIZE)
    #define INTERNAL_FLASH_FAC_ADDRESS (USER_FIRMWARE_IMAGE_LOCATION+FIRMWARE_IMAGE_SIZE+FIRMWARE_IMAGE_SIZE)
    
#else        
    #define FACTORY_RESET_MODULE_FUNCTION MODULE_FUNCTION_MONO_FIRMWARE
    #define USER_FIRMWARE_IMAGE_LOCATION CORE_FW_ADDRESS
    #ifndef FIRMWARE_IMAGE_SIZE
    #ifdef USE_SERIAL_FLASH
    #define FIRMWARE_IMAGE_SIZE     0x7E000 //504K
    #else
    #define FIRMWARE_IMAGE_SIZE     0x60000 //384K (monolithic firmware size)
    #endif
    #endif    
    
    /* Internal Flash memory address where Factory programmed monolithic core firmware is located */
    #define INTERNAL_FLASH_FAC_ADDRESS  ((uint32_t)(USER_FIRMWARE_IMAGE_LOCATION + FIRMWARE_IMAGE_SIZE))
    /* Internal Flash memory address where monolithic core firmware will be saved for backup/restore */
    //#define INTERNAL_FLASH_BKP_ADDRESS  ((uint32_t)(USER_FIRMWARE_IMAGE_LOCATION + FIRMWARE_IMAGE_SIZE))
    /* Internal Flash memory address where OTA upgraded monolithic core firmware will be saved */
    #define INTERNAL_FLASH_OTA_ADDRESS  ((uint32_t)(USER_FIRMWARE_IMAGE_LOCATION + FIRMWARE_IMAGE_SIZE))

    #ifdef USE_SERIAL_FLASH
    /* External Flash memory address where Factory programmed core firmware is located */
    #define EXTERNAL_FLASH_FAC_ADDRESS  ((uint32_t)0x4000)
    /* External Flash memory address where core firmware will be saved for backup/restore */
    #define EXTERNAL_FLASH_BKP_ADDRESS  ((uint32_t)EXTERNAL_FLASH_FAC_ADDRESS)
    /* External Flash memory address where OTA upgraded core firmware will be saved */
    #define EXTERNAL_FLASH_OTA_ADDRESS  ((uint32_t)(EXTERNAL_FLASH_FAC_ADDRESS + FIRMWARE_IMAGE_SIZE))
    #endif
#endif
    
#if FIRMWARE_IMAGE_SIZE > INTERNAL_FLASH_SIZE
#   error "FIRMWARE_IMAGE_SIZE too large to fit into internal flash"
#endif

/* Bootloader Flash regions that needs to be protected: 0x08000000 - 0x08003FFF */
#define BOOTLOADER_FLASH_PAGES      (OB_WRP_Sector_0)

/* MAL access layer for Internal/Serial Flash Routines */
//New routines specific for BM09/BM14 flash usage
uint16_t FLASH_SectorToWriteProtect(uint8_t flashDeviceID, uint32_t startAddress);
uint16_t FLASH_SectorToErase(flash_device_t flashDeviceID, uint32_t startAddress);
bool FLASH_CheckValidAddressRange(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length);
bool FLASH_WriteProtectMemory(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length, bool protect);
bool FLASH_EraseMemory(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length);

/**
 * @param validateDestinationAddress checks if the destination address corresponds with the start address in the module
 */
bool FLASH_CopyMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                      flash_device_t destinationDeviceID, uint32_t destinationAddress,
                      uint32_t length, uint8_t module_function, uint8_t flags);

bool FLASH_CompareMemory(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                         flash_device_t destinationDeviceID, uint32_t destinationAddress,
                         uint32_t length);

bool FLASH_AddToNextAvailableModulesSlot(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                                         flash_device_t destinationDeviceID, uint32_t destinationAddress,
                                         uint32_t length, uint8_t module_function, uint8_t flags);

bool FLASH_AddToFactoryResetModuleSlot(flash_device_t sourceDeviceID, uint32_t sourceAddress,
                                       flash_device_t destinationDeviceID, uint32_t destinationAddress,
                                       uint32_t length, uint8_t module_function, uint8_t flags);

bool FLASH_ClearFactoryResetModuleSlot(void);
bool FLASH_RestoreFromFactoryResetModuleSlot(void);
void FLASH_UpdateModules(void (*flashModulesCallback)(bool isUpdating));

const module_info_t* FLASH_ModuleInfo(uint8_t flashDeviceID, uint32_t startAddress);
uint32_t FLASH_ModuleAddress(flash_device_t flashDeviceID, uint32_t startAddress);
uint32_t FLASH_ModuleLength(flash_device_t flashDeviceID, uint32_t startAddress);
bool FLASH_isUserModuleInfoValid(uint8_t flashDeviceID, uint32_t startAddress, uint32_t expectedAddress);
bool FLASH_VerifyCRC32(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length);

//Old routines with same signature both for Core and Photon
void FLASH_ClearFlags(void);
void FLASH_WriteProtection_Enable(uint32_t FLASH_Sectors);
void FLASH_WriteProtection_Disable(uint32_t FLASH_Sectors);
void FLASH_Erase(void);
void FLASH_Backup(uint32_t FLASH_Address);
void FLASH_Restore(uint32_t FLASH_Address);
uint32_t FLASH_PagesMask(uint32_t imageSize, uint32_t pageSize);
void FLASH_Begin(uint32_t FLASH_Address, uint32_t imageSize);
int FLASH_Update(const uint8_t *pBuffer, uint32_t address, uint32_t bufferSize);
void FLASH_End(void);


#ifdef __cplusplus
}
#endif


#endif  /*__FLASH_MAL_H*/
