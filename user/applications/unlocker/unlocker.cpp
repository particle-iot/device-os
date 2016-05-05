
#include "application.h"
#include "flash_mal.h"

SYSTEM_MODE(MANUAL)

bool FLASH_WriteProtectMemory(flash_device_t flashDeviceID, uint32_t startAddress, uint32_t length, bool protect);

void setup()
{
    RGB.control(true);
    RGB.color(255,255,0);
    FLASH_WriteProtectMemory(FLASH_INTERNAL, 0x8000000, 0x100000, false);
    RGB.color(0,255,0);
}
