#include <stdint.h>
#include "core_hal.h"
#include "flash_mal.h"
#include "bootloader.h"
#include "module_info.h"
#include "bootloader_hal.h"
#include "ota_flash_hal_impl.h"
#include "check.h"
#include "stream.h"

#include <memory>

#define BOOTLOADER_ADDR (module_bootloader.start_address)

int bootloader_update(const void* bootloader_image, unsigned length)
{
    HAL_Bootloader_Lock(false);
    int result = (FLASH_CopyMemory(FLASH_SERIAL, (uint32_t)bootloader_image,
        FLASH_INTERNAL, BOOTLOADER_ADDR, length, MODULE_FUNCTION_BOOTLOADER,
        MODULE_VERIFY_DESTINATION_IS_START_ADDRESS|MODULE_VERIFY_CRC|MODULE_VERIFY_FUNCTION));
    HAL_Bootloader_Lock(true);
    return result;
}

#ifdef HAL_REPLACE_BOOTLOADER

#error "This is currently broken due to removal of XIP support"

/**
 * Manages upgrading the bootloader.
 */

bool bootloader_requires_update(const uint8_t* bootloader_image, uint32_t length)
{
    if ((bootloader_image == nullptr) || length == 0)
        return false;

    const uint32_t VERSION_OFFSET = 0x200+10;

    uint16_t current_version = *(uint16_t*)(BOOTLOADER_ADDR+VERSION_OFFSET);
    uint16_t available_version = *(uint16_t*)(bootloader_image+VERSION_OFFSET);

    bool requires_update = current_version<available_version;
    return requires_update;
}

using namespace particle;

/**
 * Decompresses the first 1K of the compressed bootloader image.
 * This contains the module info, which conains the bootloader module version.
 * This is compared against the current bootloader, and if newer,
 */
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

#else // !defined(HAL_REPLACE_BOOTLOADER)

bool bootloader_requires_update()
{
    return false;
}

bool bootloader_update_if_needed()
{
    return false;
}

#endif

uint16_t bootloader_get_version(void)
{
    module_info_t info = {};
    if (FLASH_ModuleInfo(&info, FLASH_INTERNAL, BOOTLOADER_ADDR, nullptr) != SYSTEM_ERROR_NONE) {
        return 0;
    }
    return info.module_version;
}
