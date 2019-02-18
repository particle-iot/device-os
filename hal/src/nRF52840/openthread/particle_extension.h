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

#pragma once

#include <openthread/instance.h>
#include <openthread/coap.h>
#include <openthread/ip6.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef void(*otExtParticleReceiveDeviceIdGetCallback)(otError err, otCoapCode coapResult,
        const uint8_t* deviceId, size_t length, const otMessageInfo* msgInfo, void* ctx);

otError otExtParticleSetReceiveDeviceIdGetCallback(otInstance* instance,
        otExtParticleReceiveDeviceIdGetCallback cb, void* ctx);
otError otExtParticleSendDeviceIdGet(otInstance* instance, const otIp6Address* dest);

#ifdef __cplusplus
}
#endif // __cplusplus
