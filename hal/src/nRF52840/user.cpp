/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include "core_hal.h"
#include "flash_mal.h"
#include "user.h"
#include "module_info.h"
#include "user_hal.h"
#include "ota_flash_hal_impl.h"
#include "system_error.h"

#define USER_ADDR (module_user.start_address)


#if defined(INCLUDE_APP)

static bool user_module_present_and_valid()
{
    return FLASH_isUserModuleInfoValid(FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION,
            USER_FIRMWARE_IMAGE_LOCATION) &&
            FLASH_VerifyCRC32(FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION,
                FLASH_ModuleLength(FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION));
}

int user_update_if_needed(void)
{
    int updated = SYSTEM_ERROR_INVALID_STATE;

    // Update only if there is no user part at all or it doesn't pass integrity validation
    if (!user_module_present_and_valid()) {
        uint32_t user_image_size = 0;
        const uint8_t* user_image = HAL_User_Image(&user_image_size, nullptr);

        updated = FLASH_CopyMemory(FLASH_INTERNAL, (uint32_t)user_image,
                FLASH_INTERNAL, USER_ADDR, user_image_size, MODULE_FUNCTION_USER_PART,
                MODULE_VERIFY_DESTINATION_IS_START_ADDRESS|MODULE_VERIFY_CRC|MODULE_VERIFY_FUNCTION);
        if (updated == FLASH_ACCESS_RESULT_OK) {
            updated = SYSTEM_ERROR_NONE;
        } else {
            updated = SYSTEM_ERROR_UNKNOWN;
        }
    }
    return updated;
}

#else

int user_update_if_needed(void)
{
    return SYSTEM_ERROR_NOT_SUPPORTED;
}
#endif // INCLUDE_APP
