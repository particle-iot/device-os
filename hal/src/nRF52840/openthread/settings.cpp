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

/**
 * @file
 *   This file implements the OpenThread platform abstraction for non-volatile storage of settings.
 *
 */

#include <stddef.h>
#include <stdlib.h>

#include "service_debug.h"

#include <openthread-core-config.h>
#include <openthread/platform/settings.h>
#include <openthread/instance.h>

#include "tlv_file.h"

namespace {

particle::services::settings::TlvFile s_settingsFile("/sys/openthread.dat");

} /* anonymous */

void otPlatSettingsInit(otInstance* aInstance)
{
    SPARK_ASSERT(s_settingsFile.init() == 0);
}

otError otPlatSettingsBeginChange(otInstance* aInstance) {
    return OT_ERROR_NONE;
}

otError otPlatSettingsCommitChange(otInstance* aInstance) {
    return OT_ERROR_NONE;
}

otError otPlatSettingsAbandonChange(otInstance* aInstance) {
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatSettingsGet(otInstance* aInstance, uint16_t aKey, int aIndex, uint8_t* aValue, uint16_t* aValueLength) {
    int r = s_settingsFile.get(aKey, aValue, *aValueLength, aIndex);
    if (r >= 0) {
        *aValueLength = (uint16_t)r;
        return OT_ERROR_NONE;
    }


    return OT_ERROR_NOT_FOUND;
}

otError otPlatSettingsSet(otInstance* aInstance, uint16_t aKey, const uint8_t* aValue, uint16_t aValueLength) {
    int r = s_settingsFile.set(aKey, aValue, aValueLength, -1);
    return r == 0 ? OT_ERROR_NONE : OT_ERROR_FAILED;
}

otError otPlatSettingsAdd(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength) {
    int r = s_settingsFile.add(aKey, aValue, aValueLength);
    return r == 0 ? OT_ERROR_NONE : OT_ERROR_FAILED;
}

otError otPlatSettingsDelete(otInstance *aInstance, uint16_t aKey, int aIndex) {
    int r = s_settingsFile.del(aKey, aIndex);
    return r == 0 ? OT_ERROR_NONE : OT_ERROR_NOT_FOUND;
}

void otPlatSettingsWipe(otInstance* aInstance) {
    s_settingsFile.purge();
    otPlatSettingsInit(aInstance);
}
