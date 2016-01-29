/**
 ******************************************************************************
 * @file    hal_dynalib_btstack.h
 * @authors mat
 * @date    04 March 2015
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */
#ifndef HAL_DYNALIB_BTSTACK_H
#define HAL_DYNALIB_BTSTACK_H

#if PLATFORM_ID == 88 // Duo

#include "dynalib.h"
#include "usb_config_hal.h"

#ifdef DYNALIB_EXPORT
#include "btstack_hal.h"
#endif

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_btstack)

DYNALIB_FN(hal_btstack,hal_btstack_init)
DYNALIB_FN(hal_btstack,hal_btstack_deInit)
DYNALIB_FN(hal_btstack,hal_btstack_loop_execute)

DYNALIB_FN(hal_btstack,hal_btstack_setTimer)
DYNALIB_FN(hal_btstack,hal_btstack_setTimerHandler)
DYNALIB_FN(hal_btstack,hal_btstack_addTimer)
DYNALIB_FN(hal_btstack,hal_btstack_removeTimer)
DYNALIB_FN(hal_btstack,hal_btstack_getTimeMs)

DYNALIB_FN(hal_btstack,hal_btstack_debugLogger)
DYNALIB_FN(hal_btstack,hal_btstack_debugError)
DYNALIB_FN(hal_btstack,hal_btstack_enablePacketLogger)

DYNALIB_FN(hal_btstack,hal_btstack_getAdvertisementAddr)
DYNALIB_FN(hal_btstack,hal_btstack_setRandomAddressMode)
DYNALIB_FN(hal_btstack,hal_btstack_setRandomAddr)
DYNALIB_FN(hal_btstack,hal_btstack_setPublicBdAddr)
DYNALIB_FN(hal_btstack,hal_btstack_setLocalName)
DYNALIB_FN(hal_btstack,hal_btstack_setAdvParams)
DYNALIB_FN(hal_btstack,hal_btstack_setAdvData)
DYNALIB_FN(hal_btstack,hal_btstack_startAdvertising)
DYNALIB_FN(hal_btstack,hal_btstack_stopAdvertising)

DYNALIB_FN(hal_btstack,hal_btstack_setConnectedCallback)
DYNALIB_FN(hal_btstack,hal_btstack_setDisconnectedCallback)

DYNALIB_FN(hal_btstack,hal_btstack_disconnect)


DYNALIB_FN(hal_btstack,hal_btstack_attServerCanSend)
DYNALIB_FN(hal_btstack,hal_btstack_attServerSendNotify)
DYNALIB_FN(hal_btstack,hal_btstack_attServerSendIndicate)

DYNALIB_FN(hal_btstack,hal_btstack_setGattCharsRead)
DYNALIB_FN(hal_btstack,hal_btstack_setGattCharsWrite)

DYNALIB_FN(hal_btstack,hal_btstack_addServiceUUID16bits)
DYNALIB_FN(hal_btstack,hal_btstack_addServiceUUID128bits)
DYNALIB_FN(hal_btstack,hal_btstack_addCharsUUID16bits)
DYNALIB_FN(hal_btstack,hal_btstack_addCharsUUID128bits)
DYNALIB_FN(hal_btstack,hal_btstack_addCharsDynamicUUID16bits)
DYNALIB_FN(hal_btstack,hal_btstack_addCharsDynamicUUID128bits)


DYNALIB_FN(hal_btstack,hal_btstack_startScanning)
DYNALIB_FN(hal_btstack,hal_btstack_stopScanning)

DYNALIB_FN(hal_btstack,hal_btstack_setBLEAdvertisementCallback)


DYNALIB_END(hal_btstack)

#endif

#endif /* HAL_DYNALIB_BTSTACK_H */
