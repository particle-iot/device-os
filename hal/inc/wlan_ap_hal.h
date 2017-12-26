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

#ifndef WLAN_AP_HAL_H
#define WLAN_AP_HAL_H

#include "wlan_hal.h"

wlan_result_t wlan_ap_has_credentials(void* reserved);
wlan_result_t wlan_ap_set_credentials(WLanCredentials* wlan_creds, void* reserved);
wlan_result_t wlan_ap_get_credentials(WiFiAccessPoint* wap, void* reserved);
wlan_result_t wlan_ap_get_state(uint8_t* state, void* reserved);
int wlan_ap_enabled(uint8_t enabled, void* reserved);

#endif /* WLAN_AP_HAL_H */
