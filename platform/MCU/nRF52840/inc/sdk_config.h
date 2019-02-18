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

#pragma once

// Do not define Particle's STATIC_ASSERT() to avoid conflicts with the nRF SDK's own macro
#define NO_STATIC_ASSERT

#include "module_info.h"

#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
#include "sdk_config_bootloader.h"
#else
#include "sdk_config_system.h"
#endif

// Include default configuration files (TODO: Copy to the platform directory?)
#include "nrf5_sdk/modules/nrfx/templates/nRF52840/nrfx_config.h"
#include "nrf5_sdk/config/nrf52840/config/sdk_config.h"
