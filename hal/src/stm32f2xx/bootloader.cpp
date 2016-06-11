
#include <stdint.h>
#include "core_hal.h"
#include "flash_mal.h"
#include "bootloader.h"
#include "module_info.h"

#if !defined(SYSTEM_MINIMAL)
#if PLATFORM_ID==6 || PLATFORM_ID==8
#define HAL_REPLACE_BOOTLOADER
#endif
#if PLATFORM_ID==6 || PLATFORM_ID==8 || PLATFORM_ID==10
#define HAL_REPLACE_BOOTLOADER_OTA
#endif
#endif

#ifdef HAL_REPLACE_BOOTLOADER_OTA
bool bootloader_update(const void* bootloader_image, unsigned length)
{
    HAL_Bootloader_Lock(false);
    bool result =  (FLASH_CopyMemory(FLASH_INTERNAL, (uint32_t)bootloader_image,
        FLASH_INTERNAL, 0x8000000, length, MODULE_FUNCTION_BOOTLOADER,
        MODULE_VERIFY_DESTINATION_IS_START_ADDRESS|MODULE_VERIFY_CRC|MODULE_VERIFY_FUNCTION));
    HAL_Bootloader_Lock(true);
    return result;
}
#else
bool bootloader_update(const void*, unsigned)
{
    return false;
}
#endif // HAL_REPLACE_BOOTLOADER_OTA

#ifdef HAL_REPLACE_BOOTLOADER

/**
 * Manages upgrading the bootloader.
 */

#define CAT2(a,b) a##b
#define CAT(a,b) CAT2(a,b)

#define BOOTLOADER_IMAGE CAT(bootloader_platform_, CAT(PLATFORM_ID,_bin))
#define BOOTLOADER_IMAGE_LEN CAT(bootloader_platform_, CAT(PLATFORM_ID,_bin_len))

extern "C" const unsigned int BOOTLOADER_IMAGE_LEN;
extern "C" const unsigned char BOOTLOADER_IMAGE[];

bool bootloader_requires_update()
{
    const uint32_t VERSION_OFFSET = 0x184+10;

    uint16_t current_version = *(uint16_t*)(0x8000000+VERSION_OFFSET);
    uint16_t available_version = *(uint16_t*)(BOOTLOADER_IMAGE+VERSION_OFFSET);

    bool requires_update = current_version<available_version;
    return requires_update;
}

bool bootloader_update_if_needed()
{
    bool updated = false;
    if (bootloader_requires_update()) {
        updated = bootloader_update(BOOTLOADER_IMAGE, BOOTLOADER_IMAGE_LEN);
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
