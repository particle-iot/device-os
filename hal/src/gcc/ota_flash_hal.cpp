#include "ota_flash_hal.h"
#include <string.h>
#include <cstdio>

#include "filesystem.h"

uint32_t HAL_OTA_FlashAddress()
{
    return 0;
}

uint32_t HAL_OTA_FlashLength()
{
    return 0;
}
    
uint16_t HAL_OTA_ChunkSize()
{
    return 0;
}

bool HAL_FLASH_Begin(uint32_t sFLASH_Address, uint32_t fileSize, void* reserved) 
{    
    return false;
}

uint16_t HAL_FLASH_Update(uint8_t *pBuffer, uint32_t address, uint32_t bufferSize, void* reserved) 
{
    return 0;
}

 hal_update_complete_t HAL_FLASH_End(void* reserved) 
{    
     return HAL_UPDATE_ERROR;
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


void HAL_FLASH_Read_ServerAddress(ServerAddress* server_addr)
{
    uint8_t buf[EXTERNAL_FLASH_SERVER_DOMAIN_LENGTH];
    read_file("server_address", buf, EXTERNAL_FLASH_SERVER_DOMAIN_LENGTH);    
    parseServerAddressData(server_addr, buf);
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
    read_file("public_key", keyBuffer, PUBLIC_KEY_LEN);
}

void HAL_FLASH_Read_CorePrivateKey(uint8_t *keyBuffer)
{
    read_file("private_key", keyBuffer, PRIVATE_KEY_LEN);
}



