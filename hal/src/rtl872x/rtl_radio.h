/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#ifndef RTL_RADIO_H
#define RTL_RADIO_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    RTW_RADIO_NONE = 0x00,
    RTW_RADIO_WIFI = 0x01,
    RTW_RADIO_BLE = 0x02,
    RTW_RADIO_ALL = 0x03
} RtwRadio;

#ifdef __cplusplus
extern "C" {
#endif

void rtwCoexRunDisable(int idx);
void rtwCoexPreventCleanup(int idx);
void rtwCoexRunEnable(int idx);
void rtwCoexCleanup(int idx);
void rtwCoexCleanupMutex(int idx);

void rtwRadioReset();
void rtwRadioAcquire(RtwRadio r);
void rtwRadioRelease(RtwRadio r);
void rtwCoexStop();
void rtwCoexSetWifiConnectedState(bool state);
bool rtwCoexWifiConnectedState();
int rtwCoexSet(uint32_t coex[4], uint8_t tmda[5], bool apply);

#ifdef __cplusplus
}
#endif

#endif // RTL_RADIO_H
