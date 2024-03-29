/*
 * Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.
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

#include "system_control.h"

namespace particle {

namespace control {

int startFirmwareUpdateRequest(ctrl_request* req);
void finishFirmwareUpdateRequest(ctrl_request* req);
int cancelFirmwareUpdateRequest(ctrl_request* req);
int firmwareUpdateDataRequest(ctrl_request* req);

int getModuleInfo(ctrl_request* req);

#if HAL_PLATFORM_ASSETS
int getAssetInfo(ctrl_request* req);
#endif // HAL_PLATFORM_ASSETS

} // namespace particle::control

} // namespace particle
