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

#ifndef HAL_DYNALIB_BLE_H
#define	HAL_DYNALIB_BLE_H

#include "dynalib.h"
#include "hal_platform.h"

#if HAL_PLATFORM_BLE

#ifdef DYNALIB_EXPORT
#include "ble_hal.h"
#endif

DYNALIB_BEGIN(hal_ble)

DYNALIB_FN(0, hal_ble, ble_stack_is_initialized, bool(void))
DYNALIB_FN(1, hal_ble, ble_stack_init, int(void*))
DYNALIB_FN(2, hal_ble, ble_select_antenna, int(hal_ble_ant_type_t))
DYNALIB_FN(3, hal_ble, ble_set_callback_on_events, int(on_ble_evt_cb_t, void*))
DYNALIB_FN(4, hal_ble, ble_gap_set_device_address, int(const hal_ble_addr_t*))
DYNALIB_FN(5, hal_ble, ble_gap_get_device_address, int(hal_ble_addr_t*))
DYNALIB_FN(6, hal_ble, ble_gap_set_device_name, int(const uint8_t*, uint16_t))
DYNALIB_FN(7, hal_ble, ble_gap_get_device_name, int(uint8_t*, uint16_t*))
DYNALIB_FN(8, hal_ble, ble_gap_set_appearance, int(uint16_t))
DYNALIB_FN(9, hal_ble, ble_gap_get_appearance, int(uint16_t*))
DYNALIB_FN(10, hal_ble, ble_gap_set_ppcp, int(const hal_ble_conn_params_t*, void*))
DYNALIB_FN(11, hal_ble, ble_gap_get_ppcp, int(hal_ble_conn_params_t*, void*))
DYNALIB_FN(12, hal_ble, ble_gap_add_whitelist, int(const hal_ble_addr_t*, uint8_t, void*))
DYNALIB_FN(13, hal_ble, ble_gap_delete_whitelist, int(void*))
DYNALIB_FN(14, hal_ble, ble_gap_set_tx_power, int(int8_t))
DYNALIB_FN(15, hal_ble, ble_gap_get_tx_power, int(int8_t*))
DYNALIB_FN(16, hal_ble, ble_gap_set_advertising_parameters, int(const hal_ble_adv_params_t*, void*))
DYNALIB_FN(17, hal_ble, ble_gap_set_advertising_data, int(const uint8_t*, uint16_t, void*))
DYNALIB_FN(18, hal_ble, ble_gap_set_scan_response_data, int(const uint8_t*, uint16_t, void*))
DYNALIB_FN(19, hal_ble, ble_gap_start_advertising, int(void*))
DYNALIB_FN(20, hal_ble, ble_gap_stop_advertising, int(void))
DYNALIB_FN(21, hal_ble, ble_gap_is_advertising, bool(void))
DYNALIB_FN(22, hal_ble, ble_gap_set_scan_parameters, int(const hal_ble_scan_params_t*, void*))
DYNALIB_FN(23, hal_ble, ble_gap_start_scan, int(void*))
DYNALIB_FN(24, hal_ble, ble_gap_is_scanning, bool(void))
DYNALIB_FN(25, hal_ble, ble_gap_stop_scan, int(void))
DYNALIB_FN(26, hal_ble, ble_gap_connect, int(const hal_ble_addr_t*, const hal_ble_conn_params_t*, void*))
DYNALIB_FN(27, hal_ble, ble_gap_is_connecting, bool(void))
DYNALIB_FN(28, hal_ble, ble_gap_is_connected, bool(void))
DYNALIB_FN(29, hal_ble, ble_gap_connect_cancel, int(void))
DYNALIB_FN(30, hal_ble, ble_gap_disconnect, int(uint16_t, void*))
DYNALIB_FN(31, hal_ble, ble_gap_update_connection_params, int(uint16_t, const hal_ble_conn_params_t*, void*))
DYNALIB_FN(32, hal_ble, ble_gap_get_rssi, int(uint16_t, void*))
DYNALIB_FN(33, hal_ble, ble_gatt_server_add_service, int(uint8_t, const hal_ble_uuid_t*, uint16_t*, void*))
DYNALIB_FN(34, hal_ble, ble_gatt_server_add_characteristic, int(const hal_ble_char_init_t*, hal_ble_char_handles_t*, void*))
DYNALIB_FN(35, hal_ble, ble_gatt_server_add_descriptor, int(const hal_ble_desc_init_t*, uint16_t*, void*))
DYNALIB_FN(36, hal_ble, ble_gatt_server_set_characteristic_value, int(uint16_t, const uint8_t*, uint16_t, void*))
DYNALIB_FN(37, hal_ble, ble_gatt_server_get_characteristic_value, int(uint16_t, uint8_t*, uint16_t*, void*))
DYNALIB_FN(38, hal_ble, ble_gatt_server_notify_characteristic_value, int(uint16_t, const uint8_t*, uint16_t, void*))
DYNALIB_FN(39, hal_ble, ble_gatt_server_indicate_characteristic_value, int(uint16_t, const uint8_t*, uint16_t, void*))
DYNALIB_FN(40, hal_ble, ble_gatt_client_discover_all_services, int(uint16_t, void*))
DYNALIB_FN(41, hal_ble, ble_gatt_client_discover_service_by_uuid, int(uint16_t, const hal_ble_uuid_t*, void*))
DYNALIB_FN(42, hal_ble, ble_gatt_client_discover_characteristics, int(uint16_t, uint16_t, uint16_t, void*))
DYNALIB_FN(43, hal_ble, ble_gatt_client_discover_characteristics_by_uuid, int(uint16_t, uint16_t, uint16_t, const hal_ble_uuid_t*, void*))
DYNALIB_FN(44, hal_ble, ble_gatt_client_discover_descriptors, int(uint16_t, uint16_t, uint16_t, void*))
DYNALIB_FN(45, hal_ble, ble_gatt_client_is_discovering, bool(void))
DYNALIB_FN(46, hal_ble, ble_gatt_client_configure_cccd, int(uint16_t, uint16_t, uint8_t, void*))
DYNALIB_FN(47, hal_ble, ble_gatt_client_write_with_response, int(uint16_t, uint16_t, const uint8_t*, uint16_t, void*))
DYNALIB_FN(48, hal_ble, ble_gatt_client_write_without_response, int(uint16_t, uint16_t, const uint8_t*, uint16_t, void*))
DYNALIB_FN(49, hal_ble, ble_gatt_client_read, int(uint16_t, uint16_t, uint8_t*, uint16_t*, void*))

DYNALIB_END(hal_ble)

#endif /* HAL_PLATFORM_BLE */

#endif /* HAL_DYNALIB_BLE_H */
