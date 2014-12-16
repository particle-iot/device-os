/**
 ******************************************************************************
 * @file    memory_hal.h
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    25-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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

#ifndef OTA_FLASH_HAL_H
#define	OTA_FLASH_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "static_assert.h"


#ifdef	__cplusplus
extern "C" {
#endif
    
// TODO - this is temporary to get a working hal.
// A C++ MemoryDeviceRegion will be used so that callers can incrementally
// write to that. This decouples writing to memory    

uint32_t HAL_OTA_FlashAddress();
uint32_t HAL_OTA_FlashLength();

void HAL_FLASH_Begin(uint32_t sFLASH_Address, uint32_t fileSize);
uint16_t HAL_FLASH_Update(uint8_t *pBuffer, uint32_t bufferSize);
void HAL_FLASH_End(void);

// todo - the status is not hardware dependent. It's just the current code has
// status updates and flash writing intermixed. No time to refactor into something cleaner.
bool HAL_OTA_Flashed_GetStatus(void);
void HAL_OTA_Flashed_ResetStatus(void);



typedef enum
{
  IP_ADDRESS = 0, DOMAIN_NAME = 1, INVALID_INTERNET_ADDRESS = 0xff
} Internet_Address_TypeDef;


typedef struct __attribute__((__packed__)) ServerAddress {
  Internet_Address_TypeDef addr_type;
  uint8_t length;
  union {
    char domain[127];
    uint32_t ip;
  };
} ServerAddress;

STATIC_ASSERT(Internet_Address_is_2_bytes, sizeof(Internet_Address_TypeDef)==1);
STATIC_ASSERT(ServerAddress_ip_offset, offsetof(ServerAddress, ip)==2);
STATIC_ASSERT(ServerAddress_domain_offset, offsetof(ServerAddress, ip)==2);



/* Length in bytes of DER-encoded 2048-bit RSA public key */
#define EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH		(294)
/* Length in bytes of DER-encoded 1024-bit RSA private key */
#define EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH		(612)

void HAL_FLASH_Read_ServerAddress(ServerAddress *server_addr);
void HAL_FLASH_Read_ServerPublicKey(uint8_t *keyBuffer);
void HAL_FLASH_Read_CorePrivateKey(uint8_t *keyBuffer);


#ifdef	__cplusplus
}
#endif

#endif	/* OTA_FLASH_HAL_H */

