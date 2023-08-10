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
#include "flash_hal.h"
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
#define INTERNAL_FLASH_START        ((uint32_t)0x00000000)
#endif

//Bootloader firmware at the start of internal flash
#define USB_DFU_ADDRESS             INTERNAL_FLASH_START
//Main firmware begin address after 256KB from start of flash
#define CORE_FW_ADDRESS             ((uint32_t)0x00030000)
#define APP_START_MASK              ((uint32_t)0x2FFC0000)

#define EXTERNAL_FLASH_XIP_BASE     (0x12000000)
#if PLATFORM_ID == PLATFORM_TRACKER || PLATFORM_ID == PLATFORM_B5SOM || PLATFORM_ID == PLATFORM_ESOMX
#    define EXTERNAL_FLASH_XIP_LENGTH   (0x800000)
#else
#    define EXTERNAL_FLASH_XIP_LENGTH   (0x400000)
#endif
#define EXTERNAL_FLASH_XIP_ADDRESS_END (EXTERNAL_FLASH_XIP_BASE + EXTERNAL_FLASH_XIP_LENGTH)

/* Internal Flash page size */
#define INTERNAL_FLASH_PAGE_SIZE    ((uint32_t)0x1000) //4K (256 sectors of 4K each used by main firmware)

#ifndef USE_SERIAL_FLASH
#    error "USE_SERIAL_FLASH not defined"
#else
#    define EXTERNAL_FLASH_SIZE                         (sFLASH_PAGESIZE * sFLASH_PAGECOUNT)

#    if PLATFORM_ID == PLATFORM_TRACKER || PLATFORM_ID == PLATFORM_B5SOM || PLATFORM_ID == PLATFORM_ESOMX
#        define EXTERNAL_FLASH_SYSTEM_STORE             (0x400000)          // Lower 4M for filesystem
#        define EXTERNAL_FLASH_RESERVED1_ADDRESS        (EXTERNAL_FLASH_SYSTEM_STORE)
#        define EXTERNAL_FLASH_RESERVED1_XIP_ADDRESS    (EXTERNAL_FLASH_RESERVED1_ADDRESS + EXTERNAL_FLASH_XIP_BASE)
#        define EXTERNAL_FLASH_RESERVED1_LENGTH         (2048*1024)
#        define EXTERNAL_FLASH_FAC_ADDRESS              (EXTERNAL_FLASH_RESERVED1_ADDRESS + EXTERNAL_FLASH_RESERVED1_LENGTH)
#        define EXTERNAL_FLASH_ASSET_STORAGE_FIRST_PAGE ((EXTERNAL_FLASH_RESERVED1_ADDRESS) / sFLASH_PAGESIZE)
#        define EXTERNAL_FLASH_ASSET_STORAGE_PAGE_COUNT ((EXTERNAL_FLASH_RESERVED1_LENGTH) / sFLASH_PAGESIZE) // 2MB
#        define EXTERNAL_FLASH_ASSET_STORAGE_SIZE       (EXTERNAL_FLASH_ASSET_STORAGE_PAGE_COUNT * sFLASH_PAGESIZE)  // 2MB
#        define EXTERNAL_FLASH_RESERVED_LENGTH          (292*1024)
#    else
#        define EXTERNAL_FLASH_SYSTEM_STORE             (0x200000)          // Lower 2M for filesystem
#        define EXTERNAL_FLASH_FAC_ADDRESS              (EXTERNAL_FLASH_SYSTEM_STORE)
#        define EXTERNAL_FLASH_RESERVED_LENGTH          (48*1024) // Just for MBR bootloader updates
#        define EXTERNAL_FLASH_ASSET_STORAGE_FIRST_PAGE ((EXTERNAL_FLASH_SIZE) / sFLASH_PAGESIZE - 288)
#        define EXTERNAL_FLASH_ASSET_STORAGE_PAGE_COUNT (288)  // 1.125MB
#        define EXTERNAL_FLASH_ASSET_STORAGE_SIZE       (EXTERNAL_FLASH_ASSET_STORAGE_PAGE_COUNT * sFLASH_PAGESIZE)  // 1.125MB
#    endif // PLATFORM_ID == PLATFORM_TRACKER || PLATFORM_ID == PLATFORM_B5SOM || PLATFORM_ID == PLATFORM_ESOMX

#    define EXTERNAL_FLASH_FAC_XIP_ADDRESS              (EXTERNAL_FLASH_FAC_ADDRESS + EXTERNAL_FLASH_XIP_BASE)
#    define EXTERNAL_FLASH_FAC_LENGTH                   (256*1024)

#    define EXTERNAL_FLASH_RESERVED_ADDRESS             (EXTERNAL_FLASH_FAC_ADDRESS + EXTERNAL_FLASH_FAC_LENGTH)
#    define EXTERNAL_FLASH_RESERVED_XIP_ADDRESS         (EXTERNAL_FLASH_RESERVED_ADDRESS + EXTERNAL_FLASH_XIP_BASE)

     /* External Flash memory address where Factory programmed core firmware is located */
     /* External Flash memory address where OTA upgraded core firmware will be saved */
#    define EXTERNAL_FLASH_OTA_LENGTH                   (1500*1024) // NOTE: on older Gen 3 platforms this overlaps asset filesystem
#    define EXTERNAL_FLASH_OTA_ADDRESS                  ((uint32_t)(EXTERNAL_FLASH_RESERVED_ADDRESS + EXTERNAL_FLASH_RESERVED_LENGTH))
#    define EXTERNAL_FLASH_OTA_XIP_ADDRESS              (EXTERNAL_FLASH_OTA_ADDRESS + EXTERNAL_FLASH_XIP_BASE)

#    if (PLATFORM_ID == PLATFORM_TRACKER || PLATFORM_ID == PLATFORM_B5SOM || PLATFORM_ID == PLATFORM_ESOMX)
#         define EXTERNAL_FLASH_OTA_SAFE_LENGTH (EXTERNAL_FLASH_OTA_LENGTH)
#    else
#         define EXTERNAL_FLASH_OTA_SAFE_LENGTH (606208) // 592KB
#    endif // (PLATFORM_ID == PLATFORM_TRACKER || PLATFORM_ID == PLATFORM_B5SOM || PLATFORM_ID == PLATFORM_ESOMX)

#    if PLATFORM_ID == PLATFORM_TRACKER || PLATFORM_ID == PLATFORM_B5SOM || PLATFORM_ID == PLATFORM_ESOMX
         static_assert((EXTERNAL_FLASH_SYSTEM_STORE + EXTERNAL_FLASH_RESERVED1_LENGTH + EXTERNAL_FLASH_FAC_LENGTH + EXTERNAL_FLASH_RESERVED_LENGTH + EXTERNAL_FLASH_OTA_SAFE_LENGTH) == EXTERNAL_FLASH_SIZE, "External flash size is incorrect!");
#    else
         static_assert((EXTERNAL_FLASH_SYSTEM_STORE + EXTERNAL_FLASH_FAC_LENGTH + EXTERNAL_FLASH_RESERVED_LENGTH + (EXTERNAL_FLASH_ASSET_STORAGE_PAGE_COUNT * sFLASH_PAGESIZE) + EXTERNAL_FLASH_OTA_SAFE_LENGTH) == EXTERNAL_FLASH_SIZE, "External flash size is incorrect!");
#    endif // PLATFORM_ID == PLATFORM_TRACKER || PLATFORM_ID == PLATFORM_B5SOM || PLATFORM_ID == PLATFORM_ESOMX
#endif /* USE_SERIAL_FLASH */

#ifdef MODULAR_FIRMWARE
#    define FACTORY_RESET_MODULE_FUNCTION MODULE_FUNCTION_USER_PART
#    ifndef USER_FIRMWARE_IMAGE_SIZE
#        error USER_FIRMWARE_IMAGE_SIZE not defined
#    else
#        define FIRMWARE_IMAGE_SIZE  USER_FIRMWARE_IMAGE_SIZE
#    endif

#    ifndef USER_FIRMWARE_IMAGE_LOCATION
#        error USER_FIRMWARE_IMAGE_LOCATION not defined
#    endif
#    define USER_FIRMWARE_IMAGE_LOCATION_COMPAT (INTERNAL_FLASH_START + 0xd4000)
#else /* MODULAR_FIRMWARE */

#    define FACTORY_RESET_MODULE_FUNCTION MODULE_FUNCTION_MONO_FIRMWARE
#    define USER_FIRMWARE_IMAGE_LOCATION CORE_FW_ADDRESS
#    ifdef USE_SERIAL_FLASH
#        define FIRMWARE_IMAGE_SIZE    0x000c4000 // 784K (see module_user_mono.maximum_size)
#    else
        // this was "true" mono firmware. We are now using a hybrid where the firmware payload is 512k
#       define FIRMWARE_IMAGE_SIZE     0x60000 //384K (monolithic firmware size)
#    endif

     /* Internal Flash memory address where Factory programmed monolithic core firmware is located */
#    define INTERNAL_FLASH_FAC_ADDRESS  ((uint32_t)(USER_FIRMWARE_IMAGE_LOCATION + FIRMWARE_IMAGE_SIZE))
     /* Internal Flash memory address where monolithic core firmware will be saved for backup/restore */
//#    define INTERNAL_FLASH_BKP_ADDRESS  ((uint32_t)(USER_FIRMWARE_IMAGE_LOCATION + FIRMWARE_IMAGE_SIZE))
     /* Internal Flash memory address where OTA upgraded monolithic core firmware will be saved */
#    define INTERNAL_FLASH_OTA_ADDRESS  ((uint32_t)(USER_FIRMWARE_IMAGE_LOCATION + FIRMWARE_IMAGE_SIZE))
#endif /* MODULAR_FIRMWARE */

#if FIRMWARE_IMAGE_SIZE > INTERNAL_FLASH_SIZE
#   error "FIRMWARE_IMAGE_SIZE too large to fit into internal flash"
#endif

/* Bootloader Flash regions that needs to be protected: 0x08000000 - 0x08003FFF */
#define BOOTLOADER_FLASH_PAGES      (OB_WRP_Sector_0)

void FLASH_WriteProtection_Enable(uint32_t FLASH_Sectors);
void FLASH_WriteProtection_Disable(uint32_t FLASH_Sectors);
uint32_t FLASH_PagesMask(uint32_t imageSize, uint32_t pageSize);


#include "flash_access.h"

#ifdef __cplusplus
}
#endif


#endif  /*__FLASH_MAL_H*/
