
#include "ota_flash_hal.h"
#include "hw_config.h"
#include <string.h>

uint32_t HAL_OTA_FlashAddress()
{
    return EXTERNAL_FLASH_OTA_ADDRESS;
}

#define FLASH_MAX_SIZE          (int32_t)(INTERNAL_FLASH_END_ADDRESS - CORE_FW_ADDRESS)

uint32_t HAL_OTA_FlashLength()
{
    return FLASH_MAX_SIZE;
}
    

void HAL_FLASH_Begin(uint32_t sFLASH_Address, uint32_t fileSize) 
{
    FLASH_Begin(sFLASH_Address, fileSize);
}

uint16_t HAL_FLASH_Update(uint8_t *pBuffer, uint32_t bufferSize) 
{
    return FLASH_Update(pBuffer, bufferSize);
}

void HAL_FLASH_End(void) 
{
    FLASH_End();
}


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
    FLASH_Read_ServerAddress_Data(buf);
    parseServerAddressData(server_addr, buf);
}


bool HAL_OTA_Flashed_GetStatus(void) 
{
    return OTA_Flashed_GetStatus();
}

void HAL_OTA_Flashed_ResetStatus(void)
{
    OTA_Flashed_ResetStatus();
}

void HAL_FLASH_Read_ServerPublicKey(uint8_t *keyBuffer)
{
    FLASH_Read_ServerPublicKey(keyBuffer);
}

void HAL_FLASH_Read_CorePrivateKey(uint8_t *keyBuffer)
{
    FLASH_Read_CorePrivateKey(keyBuffer);    
}


