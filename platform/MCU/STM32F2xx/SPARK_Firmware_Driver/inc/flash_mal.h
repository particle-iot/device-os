/**
 ******************************************************************************
 * @file    flash_mal.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    30-Jan-2015
 * @brief   Header for flash media access layer
 ******************************************************************************
  Copyright (c) 2015 Spark Labs, Inc.  All rights reserved.

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

/* Includes ------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/* Exported types ------------------------------------------------------------*/
typedef enum
{
    FLASH_INTERNAL = 0, FLASH_SERIAL = 1
} FlashDevice_TypeDef;

/* Exported constants --------------------------------------------------------*/

/* Exported macros ------------------------------------------------------------*/
#ifndef INTERNAL_FLASH_SIZE
#   error "INTERNAL_FLASH_SIZE not defined"
#endif

#ifndef FIRMWARE_IMAGE_SIZE
#define FIRMWARE_IMAGE_SIZE         (0x7E000) //504K
#endif

#if FIRMWARE_IMAGE_SIZE > INTERNAL_FLASH_SIZE
#   error "FIRMWARE_IMAGE_SIZE too large to fit into internal flash"
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

#define INTERNAL_FLASH_END_ADDRESS  ((uint32_t)CORE_FW_ADDRESS+FIRMWARE_IMAGE_SIZE)    //For 1MB Internal Flash
/* Internal Flash page size */
#define INTERNAL_FLASH_PAGE_SIZE    ((uint32_t)0x20000) //128K (7 sectors of 128K each used by main firmware)

#ifdef USE_SERIAL_FLASH
/* External Flash block size allocated for firmware storage */
#define EXTERNAL_FLASH_BLOCK_SIZE   ((uint32_t)FIRMWARE_IMAGE_SIZE)
/* External Flash memory address where Factory programmed core firmware is located */
#define EXTERNAL_FLASH_FAC_ADDRESS  ((uint32_t)0x4000)
/* External Flash memory address where core firmware will be saved for backup/restore */
#define EXTERNAL_FLASH_BKP_ADDRESS  ((uint32_t)EXTERNAL_FLASH_FAC_ADDRESS)
/* External Flash memory address where OTA upgraded core firmware will be saved */
#define EXTERNAL_FLASH_OTA_ADDRESS  ((uint32_t)(EXTERNAL_FLASH_BLOCK_SIZE + EXTERNAL_FLASH_BKP_ADDRESS))
#endif

/* Bootloader Flash regions that needs to be protected: 0x08000000 - 0x08003FFF */
#define BOOTLOADER_FLASH_PAGES      (OB_WRP_Sector_0) //Sector 0

/* MAL access layer for Internal/Serial Flash Routines */
//New routines specific for BM09/BM14 flash usage
uint16_t FLASH_SectorToErase(uint8_t flashDeviceID, uint32_t startAddress);
bool FLASH_CheckValidAddressRange(uint8_t flashDeviceID, uint32_t startAddress, uint32_t length);
bool FLASH_EraseMemory(uint8_t flashDeviceID, uint32_t startAddress, uint32_t length);
bool FLASH_CopyMemory(uint8_t sourceDeviceID, uint32_t sourceAddress,
                      uint8_t destinationDeviceID, uint32_t destinationAddress,
                      uint32_t length);
bool FLASH_CompareMemory(uint8_t sourceDeviceID, uint32_t sourceAddress,
                         uint8_t destinationDeviceID, uint32_t destinationAddress,
                         uint32_t length);
void FLASH_UpdateModules(void (*flashModulesCallback)(bool isUpdating));

//Old routines with same signature both for Core and Photon
void FLASH_ClearFlags(void);
void FLASH_WriteProtection_Enable(uint32_t FLASH_Sectors);
void FLASH_WriteProtection_Disable(uint32_t FLASH_Sectors);
void FLASH_Erase(void);
void FLASH_Backup(uint32_t FLASH_Address);
void FLASH_Restore(uint32_t FLASH_Address);
uint32_t FLASH_PagesMask(uint32_t imageSize, uint32_t pageSize);
void FLASH_Begin(uint32_t FLASH_Address, uint32_t imageSize);
uint16_t FLASH_Update(uint8_t *pBuffer, uint32_t bufferSize);
void FLASH_End(void);

#ifdef __cplusplus
}
#endif


#endif  /*__FLASH_MAL_H*/
