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

#ifndef OTA_FLASH_HAL_H
#define	OTA_FLASH_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "static_assert.h"
#include "flash_device_hal.h"
#include "module_info_hal.h"
#include "module_info.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t maximum_size;      // the maximum allowable size for the entire module image
    uint32_t start_address;     // the designated start address for the module
    uint32_t end_address;       //
    module_function_t module_function;
    uint8_t module_index;
    module_store_t store;

} module_bounds_t;

typedef enum {
    MODULE_VALIDATION_INTEGRITY        = 1<<1,
    MODULE_VALIDATION_DEPENDENCIES     = 1<<2,
    MODULE_VALIDATION_RANGE            = 1<<3,
    MODULE_VALIDATION_PLATFORM         = 1<<4,
    MODULE_VALIDATION_PRODUCT          = 1<<5,
    MODULE_VALIDATION_END = 0x7FFF
} module_validation_flags_t;

typedef struct {
    module_bounds_t bounds;
    const module_info_t* info;      // pointer to the module info in the module, may be NULL
    const module_info_crc_t* crc;
    const module_info_suffix_t* suffix;
    uint16_t validity_checked;    // the flags that were checked
    uint16_t validity_result;     // the result of the checks
} hal_module_t;

typedef struct key_value {
    const char* key;
    char value[32];
} key_value;

typedef struct {
    uint16_t size;
    uint16_t platform_id;
    hal_module_t* modules;      // allocated by HAL_System_Info
    uint16_t module_count;      // number of modules in the array
    key_value* key_values;      // key_values, allocated by HAL_System_Info
    uint16_t key_value_count;   // number of key values
} hal_system_info_t;


/**
 *
 * @param info          The buffer to fill with system info or to reclaim
 * @param construct     {#code true} to fill the buffer, {@code false} to reclaim.
 * @param reserved      set to 0
 */
void HAL_System_Info(hal_system_info_t* info, bool construct, void* reserved);

bool HAL_Verify_User_Dependencies();

// TODO - this is temporary to get a working hal.
// A C++ MemoryDeviceRegion will be used so that callers can incrementally
// write to that. This abstracts the memory regions without needing to expose
// the addresses.

uint32_t HAL_OTA_FlashAddress();

/**
 * Retrieves the maximum user image size that can be flashed to the device.
 * @return The maximum size of the binary image.
 * This image size is for the user image only. (For statically lined images,
 * the user image is the entire image. For dynamically linked images, the
 * user image is just the user portion.
 */
uint32_t HAL_OTA_FlashLength();

uint16_t HAL_OTA_ChunkSize();

flash_device_t HAL_OTA_FlashDevice();

/**
 * Erase a region of flash in preparation for flashing content.
 * @param address   The start address to erase. Must be on a flash boundary.
 * @param length
 */
bool HAL_FLASH_Begin(uint32_t address, uint32_t length, void* reserved);

/**
 * Updates part of the OTA image.
 * @result 0 on success. non-zero on error.
 */
int HAL_FLASH_Update(const uint8_t *pBuffer, uint32_t address, uint32_t length, void* reserved);

typedef enum {
    HAL_UPDATE_ERROR,
    HAL_UPDATE_APPLIED_PENDING_RESTART,
    HAL_UPDATE_APPLIED
} hal_update_complete_t;

hal_update_complete_t HAL_FLASH_End(void* reserved);

uint32_t HAL_FLASH_ModuleAddress(uint32_t address);
uint32_t HAL_FLASH_ModuleLength(uint32_t address);
bool HAL_FLASH_VerifyCRC32(uint32_t address, uint32_t length);

// todo - the status is not hardware dependent. It's just the current code has
// status updates and flash writing intermixed. No time to refactor into something cleaner.
bool HAL_OTA_Flashed_GetStatus(void);
void HAL_OTA_Flashed_ResetStatus(void);

/**
 * Set the claim code for this device.
 * @param code  The claim code to set. If null, clears the claim code.
 * @return 0 on success.
 */
uint16_t HAL_Set_Claim_Code(const char* code);

/**
 * Retrieves the claim code for this device.
 * @param buffer    The buffer to recieve the claim code.
 * @param len       The maximum size of the code to copy to the buffer, including the null terminator.
 * @return          0 on success.
 */
uint16_t HAL_Get_Claim_Code(char* buffer, unsigned len);

typedef enum
{
  IP_ADDRESS = 0, DOMAIN_NAME = 1, INVALID_INTERNET_ADDRESS = 0xff
} Internet_Address_TypeDef;


typedef struct __attribute__ ((__packed__)) ServerAddress_ {
  uint8_t addr_type;
  uint8_t length;
  union __attribute__ ((__packed__)) {
    char domain[64];
    uint32_t ip;
  };
  uint16_t port;
  /**
   * Make up to 128 bytes.
   */
  uint8_t padding[60];
} ServerAddress;

STATIC_ASSERT(ServerAddress_ip_offset, offsetof(ServerAddress, ip)==2);
STATIC_ASSERT(ServerAddress_domain_offset, offsetof(ServerAddress, domain)==2);
STATIC_ASSERT(ServerAddress_size, sizeof(ServerAddress)==128);


/* Length in bytes of DER-encoded 2048-bit RSA public key */
#define EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH		(294)
/* Length in bytes of DER-encoded 1024-bit RSA private key */
#define EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH		(612)

void HAL_FLASH_Read_ServerAddress(ServerAddress *server_addr);
void HAL_FLASH_Read_ServerPublicKey(uint8_t *keyBuffer);

typedef enum {
    /**
     * Retrieve the private key data if it exists but do not generate a new one.
     */
    PRIVATE_KEY_GENERATE_NEVER,

    /**
     * Generates the private key if it is missing.
     */
    PRIVATE_KEY_GENERATE_MISSING,

    /*
     * Generate a new private key even if one is already present.
     */
    PRIVATE_KEY_GENERATE_ALWAYS

} PrivateKeyGeneration;

typedef struct {
    /*[in]*/ size_t size;
    /*[in]*/ PrivateKeyGeneration gen;
    /**
     * Set if a key was found.
     */
    /*[out]*/ bool had_key;
    /*[out]*/ bool generated_key;

} private_key_generation_t;

/**
 * Reads and optionally generates the private key for this device.
 * @param keyBuffer The key buffer. Should be at least 162 bytes in size.
 * @param generation Describes if the key should be generated.
 * @return {@code true} if the key was generated.
 */
int HAL_FLASH_Read_CorePrivateKey(uint8_t *keyBuffer, private_key_generation_t* generation);


#ifdef	__cplusplus
}
#endif

#endif	/* OTA_FLASH_HAL_H */

