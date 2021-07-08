#include <stdint.h>
#include "dfu_hal.h"
#include "flash_mal.h"
#include "platform_config.h"

#ifndef APP_START_MASK
#define APP_START_MASK              ((uint32_t)0x2FFC0000)
#endif /* APP_START_MASK */

uint8_t is_application_valid(uint32_t address, uint32_t* entry)
{
    bool ret = false;
#ifdef FLASH_UPDATE_MODULES
    if (FLASH_isUserModuleInfoValid(FLASH_INTERNAL, address, address) &&
           FLASH_VerifyCRC32(FLASH_INTERNAL, address, FLASH_ModuleLength(FLASH_INTERNAL, address))) {
        ret = true;
    }
#else
    if ((((*(volatile uint32_t*)address) & APP_START_MASK) == 0x20000000)) {
        ret = true;
    }
#endif
    if (ret && entry != NULL) {
        *entry = address + sizeof(module_info_t);
    }
    return ret;
}
