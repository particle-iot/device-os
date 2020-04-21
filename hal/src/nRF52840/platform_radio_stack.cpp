/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include "platform_radio_stack.h"
#include "ota_flash_hal_impl.h"
#include "flash_mal.h"
#include "check.h"
#include "nrf_sdm.h"
#include "nrf_mbr.h"

int platform_radio_stack_fetch_module_info(hal_system_info_t* sys_info, bool create) {
    for (int i = 0; i < sys_info->module_count; i++) {
        hal_module_t* module = sys_info->modules + i;
        if (memcmp(&module->bounds, &module_radio_stack, sizeof(module_radio_stack))) {
            continue;
        }

        if (create) {
            auto info = new module_info_t();
            CHECK_TRUE(info, SYSTEM_ERROR_NO_MEMORY);

            info->module_version = SD_FWID_GET(MBR_SIZE);
            info->platform_id = PLATFORM_ID;
            info->module_function = MODULE_FUNCTION_RADIO_STACK;

            // FIXME: assuming that all checks passed for now
            module->validity_result = module->validity_checked;

            // IMPORTANT: a valid suffix with SHA is required for the communication layer to detect a change
            // in the SYSTEM DESCRIBE state and send a HELLO after the SoftDevice update to
            // cause the DS to request new DESCRIBE info
            auto suffix = new module_info_suffix_t();
            if (!suffix) {
                delete info;
                return SYSTEM_ERROR_NO_MEMORY;
            }
            memset(suffix, 0, sizeof(module_info_suffix_t));

            // Use a unique SoftDevice string in place of an SHA
            auto addr = SD_UNIQUE_STR_ADDR_GET(MBR_SIZE);
            memcpy(suffix->sha, addr, SD_UNIQUE_STR_SIZE);

            module->info = info;
            module->suffix = suffix;
            module->module_info_offset = 0;
        } else {
            delete module->info;
            delete ((module_info_suffix_t*)module->suffix);
        }

        break;
    }
    return 0;
}

hal_update_complete_t platform_radio_stack_update_module(const hal_module_t* module) {
    // We are using the standard update mechanism here, but instruct the module will also instruct the bootloader
    // (via MODULE_INFO_FLAG_DROP_MODULE_INFO) to strip out the module header when copying from OTA into its rightful place.
    //
    // NOTE: MODULE_INFO_FLAG_DROP_MODULE_INFO is a new feature, which might not be supported by the bootloader currently
    // on the device, however we won't even attempt to update in this case, because SoftDevice modules we generate
    // have a dependency on a particular bootloader version.

    // Just in case check that MODULE_INFO_FLAG_DROP_MODULE_INFO is present
    CHECK_TRUE(module->info->flags & MODULE_INFO_FLAG_DROP_MODULE_INFO, HAL_UPDATE_ERROR);

    auto r = FLASH_AddToNextAvailableModulesSlot(FLASH_SERIAL, EXTERNAL_FLASH_OTA_ADDRESS, // source
            FLASH_INTERNAL, (uint32_t)(module->info->module_start_address), // destination
            module_length(module->info) + 4, // + 4 to copy the CRC too
            module_function(module->info),
            (MODULE_VERIFY_CRC | MODULE_VERIFY_DESTINATION_IS_START_ADDRESS | MODULE_VERIFY_FUNCTION));

    CHECK_TRUE(r, HAL_UPDATE_ERROR);
    return HAL_UPDATE_APPLIED_PENDING_RESTART;
}
