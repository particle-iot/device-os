#include <stdint.h>
#include "dfu_hal.h"

#ifndef APP_START_MASK
#define APP_START_MASK              ((uint32_t)0x2FF10000)
#endif /* APP_START_MASK */

uint8_t is_application_valid(uint32_t address)
{
#ifdef FLASH_UPDATE_MODULES
    return FLASH_isUserModuleInfoValid(FLASH_INTERNAL, address, address);
#else
    return (((*(volatile uint32_t*)address) & APP_START_MASK) == 0x20000000);
#endif
}

void HAL_DFU_USB_Init(void) {
    while(1);
}

void DFU_Check_Reset(void) {
}
