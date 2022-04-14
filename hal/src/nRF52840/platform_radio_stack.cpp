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

int platform_radio_stack_fetch_module_info(hal_module_t* module) {
    module_info_t* info = &module->info;
    info->module_version = SD_FWID_GET(MBR_SIZE);
    info->platform_id = PLATFORM_ID;
    info->module_function = MODULE_FUNCTION_RADIO_STACK;

    // FIXME: assuming that all checks passed for now
    module->validity_checked = MODULE_VALIDATION_RANGE | MODULE_VALIDATION_DEPENDENCIES |
            MODULE_VALIDATION_PLATFORM | MODULE_VALIDATION_INTEGRITY;
    module->validity_result = module->validity_checked;

    // IMPORTANT: a valid suffix with SHA is required for the communication layer to detect a change
    // in the SYSTEM DESCRIBE state and send a HELLO after the SoftDevice update to
    // cause the DS to request new DESCRIBE info
    module_info_suffix_t* suffix = &module->suffix;
    memset(suffix, 0, sizeof(module_info_suffix_t));

    // Use a unique SoftDevice string in place of an SHA
    auto addr = SD_UNIQUE_STR_ADDR_GET(MBR_SIZE);
    memcpy(suffix->sha, addr, SD_UNIQUE_STR_SIZE);

    module->module_info_offset = 0;

    return 0;
}
