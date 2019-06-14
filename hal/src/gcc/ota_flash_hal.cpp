#include "ota_flash_hal.h"
#include "device_config.h"
#include <string.h>
#include <cstdio>
#include "service_debug.h"
#include "core_hal.h"
#include "filesystem.h"
#include "bytes2hexbuf.h"

void HAL_System_Info(hal_system_info_t* info, bool create, void* reserved)
{
    info->platform_id = PLATFORM_ID;
    info->module_count = 0;
    info->modules = NULL;
}

uint32_t HAL_OTA_FlashAddress()
{
    return 0;
}

uint32_t HAL_OTA_FlashLength()
{
    return 1024*100;
}

uint16_t HAL_OTA_ChunkSize()
{
    return 512;
}

FILE* output_file;

bool HAL_FLASH_Begin(uint32_t sFLASH_Address, uint32_t fileSize, void* reserved)
{
    output_file = fopen("output.bin", "wb");
    DEBUG("flash started");
    return output_file;
}

int HAL_FLASH_Update(const uint8_t *pBuffer, uint32_t address, uint32_t length, void* reserved)
{
	DEBUG("flash write %d %d", address, length);
	fseek(output_file, address, SEEK_SET);
    fwrite(pBuffer, length, 1, output_file);
    return 0;
}

int HAL_FLASH_OTA_Validate(hal_module_t* mod, bool userDepsOptional, module_validation_flags_t flags, void* reserved)
{
  return 0;
}

 hal_update_complete_t HAL_FLASH_End(hal_module_t* mod)
{
	 fclose(output_file);
	 output_file = NULL;
     return HAL_UPDATE_APPLIED;
}

hal_update_complete_t HAL_FLASH_ApplyPendingUpdate(hal_module_t* module, bool dryRun, void* reserved)
{
    HAL_FLASH_End(module);
    return HAL_UPDATE_APPLIED_PENDING_RESTART;
}

/**
 * Set the claim code for this device.
 * @param code  The claim code to set. If null, clears the claim code.
 * @return 0 on success.
 */
uint16_t HAL_Set_Claim_Code(const char* code)
{
    return 0;
}

/**
 * Retrieves the claim code for this device.
 * @param buffer    The buffer to recieve the claim code.
 * @param len       The maximum size of the code to copy to the buffer, including the null terminator.
 * @return          0 on success.
 */
uint16_t HAL_Get_Claim_Code(char* buffer, unsigned len)
{
    *buffer = 0;
    return 0;
}

bool HAL_IsDeviceClaimed(void* reserved)
{
	return false;
}


#define EXTERNAL_FLASH_SERVER_DOMAIN_LENGTH 128

// todo - duplicate from core, factor this down into a common area
void parseServerAddressData(ServerAddress* server_addr, uint8_t* buf)
{
  // Internet address stored on external flash may be
  // either a domain name or an IP address.
  // It's stored in a type-length-value encoding.
  // First byte is type, second byte is length, the rest is value.

  switch (buf[0])
  {
    case IP_ADDRESS:
      server_addr->addr_type = IP_ADDRESS;
      server_addr->ip = (buf[2] << 24) | (buf[3] << 16) |
                        (buf[4] << 8)  |  buf[5];
      break;

    case DOMAIN_NAME:
      if (buf[1] <= EXTERNAL_FLASH_SERVER_DOMAIN_LENGTH - 2)
      {
        server_addr->addr_type = DOMAIN_NAME;
        memcpy(server_addr->domain, buf + 2, buf[1]);

        // null terminate string
        char *p = server_addr->domain + buf[1];
        *p = 0;
        break;
      }
      // else fall through to default

    default:
      server_addr->addr_type = INVALID_INTERNET_ADDRESS;
  }

}


#define MAXIMUM_CLOUD_KEY_LEN (512)
#define SERVER_ADDRESS_OFFSET (384)
#define SERVER_ADDRESS_OFFSET_EC (192)
#define SERVER_ADDRESS_SIZE (128)
#define SERVER_PUBLIC_KEY_SIZE (294)
#define SERVER_PUBLIC_KEY_EC_SIZE (320)


void HAL_FLASH_Read_ServerAddress(ServerAddress* server_addr)
{
	memset(server_addr, 0, sizeof(ServerAddress));
	int offset = HAL_Feature_Get(FEATURE_CLOUD_UDP) ? SERVER_ADDRESS_OFFSET_EC : SERVER_ADDRESS_OFFSET;
    parseServerAddressData(server_addr, deviceConfig.server_key+offset);
}

void HAL_FLASH_Write_ServerAddress(const uint8_t *buf, bool udp)
{
    int offset = (udp) ? SERVER_ADDRESS_OFFSET_EC : SERVER_ADDRESS_OFFSET;
    memcpy(&deviceConfig.server_key+offset, buf, SERVER_ADDRESS_SIZE);
}

bool HAL_OTA_Flashed_GetStatus(void)
{
    return false;
}

void HAL_OTA_Flashed_ResetStatus(void)
{
}

#define PUBLIC_KEY_LEN 294
#define PRIVATE_KEY_LEN 612

void HAL_FLASH_Read_ServerPublicKey(uint8_t *keyBuffer)
{
    memcpy(keyBuffer, deviceConfig.server_key, PUBLIC_KEY_LEN);
    char buf[PUBLIC_KEY_LEN*2];
    bytes2hexbuf(keyBuffer, PUBLIC_KEY_LEN, buf);
    INFO("server key: %s", buf);
}

void HAL_FLASH_Write_ServerPublicKey(const uint8_t *keyBuffer, bool udp)
{
    if (udp) {
        memcpy(&deviceConfig.server_key, keyBuffer, SERVER_PUBLIC_KEY_EC_SIZE);
    } else {
        memcpy(&deviceConfig.server_key, keyBuffer, SERVER_PUBLIC_KEY_SIZE);
    }
}

int HAL_FLASH_Read_CorePrivateKey(uint8_t *keyBuffer, private_key_generation_t* generation)
{
    memcpy(keyBuffer, deviceConfig.device_key, PRIVATE_KEY_LEN);
    char buf[PRIVATE_KEY_LEN*2];
    bytes2hexbuf(keyBuffer, PRIVATE_KEY_LEN, buf);
    INFO("device key: %s", buf);
    return 0;
}

extern "C" void random_seed_from_cloud(unsigned int value)
{
}

void HAL_OTA_Add_System_Info(hal_system_info_t* info, bool create, void* reserved)
{
}
