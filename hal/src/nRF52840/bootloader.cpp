
#include <stdint.h>
#include "core_hal.h"
#include "flash_mal.h"
#include "bootloader.h"
#include "module_info.h"
#include "bootloader_hal.h"

#ifdef HAL_REPLACE_BOOTLOADER_OTA
int bootloader_update(const void* bootloader_image, unsigned length)
{
    HAL_Bootloader_Lock(false);
    int result = (FLASH_CopyMemory(FLASH_INTERNAL, (uint32_t)bootloader_image,
        FLASH_INTERNAL, 0x8000000, length, MODULE_FUNCTION_BOOTLOADER,
        MODULE_VERIFY_DESTINATION_IS_START_ADDRESS|MODULE_VERIFY_CRC|MODULE_VERIFY_FUNCTION));
    HAL_Bootloader_Lock(true);
    return result;
}
#else
int bootloader_update(const void*, unsigned)
{
    return FLASH_ACCESS_RESULT_ERROR;
}
#endif // HAL_REPLACE_BOOTLOADER_OTA

#ifdef HAL_REPLACE_BOOTLOADER

/**
 * Manages upgrading the bootloader.
 */

bool bootloader_requires_update(const uint8_t* bootloader_image, uint32_t length)
{
    if ((bootloader_image == nullptr) || length == 0)
        return false;

    const uint32_t VERSION_OFFSET = 0x184+10;

    uint16_t current_version = *(uint16_t*)(0x8000000+VERSION_OFFSET);
    uint16_t available_version = *(uint16_t*)(bootloader_image+VERSION_OFFSET);

    bool requires_update = current_version<available_version;
    return requires_update;
}

bool bootloader_update_if_needed()
{
    bool updated = false;
    uint32_t bootloader_image_size = 0;
    const uint8_t* bootloader_image = HAL_Bootloader_Image(&bootloader_image_size, nullptr);
    if (bootloader_requires_update(bootloader_image, bootloader_image_size)) {
        updated = bootloader_update(bootloader_image, bootloader_image_size);
    }
    return updated;
}

#else

bool bootloader_requires_update()
{
    return false;
}

bool bootloader_update_if_needed()
{
    return false;
}

#endif
