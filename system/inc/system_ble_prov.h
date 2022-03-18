/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


#if HAL_PLATFORM_BLE

#include "ble_hal.h"

int system_ble_prov_mode(bool enabled, void* reserved);
bool system_ble_prov_get_status(void* reserved);
int system_ble_prov_set_custom_svc_uuid(hal_ble_uuid_t* svcUuid, void* reserved);
int system_ble_prov_set_custom_tx_uuid(hal_ble_uuid_t* txUuid, void* reserved);
int system_ble_prov_set_custom_rx_uuid(hal_ble_uuid_t* rxUuid, void* reserved);
int system_ble_prov_set_custom_ver_uuid(hal_ble_uuid_t* verUuid, void* reserved);
int system_ble_prov_set_company_id(uint16_t companyId, void* reserved);

#endif

#ifdef __cplusplus
}
#endif // __cplusplus
