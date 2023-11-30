/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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

#include "hal_platform.h"

#if HAL_PLATFORM_PROTOBUF

#include <pb.h>
#include <pb_encode.h>

#include "system_info.h"
#include "enumflags.h"

#if HAL_PLATFORM_ASSETS
#include "asset_manager.h"
#endif // HAL_PLATFORM_ASSETS

namespace particle {
namespace system {

class EncodeFirmwareModules {
public:
    enum class Flag {
        NONE = 0,
        FORCE_SHA = 0x01,
        SYSTEM_INFO_CLOUD = 0x02
    };
    using Flags = EnumFlags<Flag>;

    explicit EncodeFirmwareModules(pb_callback_t* cb, Flags flags = Flags(Flag::NONE));
    ~EncodeFirmwareModules();

    hal_system_info_t* sysInfo() {
        return &sysInfo_;
    }

private:
    hal_system_info_t sysInfo_;
    Flags flags_;
};

#if HAL_PLATFORM_ASSETS

class EncodeAssets {
public:
    explicit EncodeAssets(pb_callback_t* cb, const Vector<Asset>& assets);
    ~EncodeAssets() = default;
private:
    Vector<Asset> assets_;
};

#endif // HAL_PLATFORM_ASSETS


} } // particle::system

#endif // HAL_PLATFORM_PROTOBUF
