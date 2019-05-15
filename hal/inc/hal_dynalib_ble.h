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

DYNALIB_FN(0, hal_ble, hal_ble_stack_init, int(void*))
DYNALIB_FN(1, hal_ble, hal_ble_select_antenna, int(hal_ble_ant_type_t))
DYNALIB_FN(2, hal_ble, hal_ble_set_callback_on_events, int(on_ble_evt_cb_t, void*))
DYNALIB_FN(3, hal_ble, hal_ble_gap_set_device_address, int(const hal_ble_addr_t*))
DYNALIB_FN(4, hal_ble, hal_ble_gap_get_device_address, int(hal_ble_addr_t*))
DYNALIB_FN(5, hal_ble, hal_ble_gap_set_device_name, int(const char*, size_t))
DYNALIB_FN(6, hal_ble, hal_ble_gap_get_device_name, int(char*, size_t))
DYNALIB_FN(7, hal_ble, hal_ble_gap_set_appearance, int(ble_sig_appearance_t))
DYNALIB_FN(8, hal_ble, hal_ble_gap_get_appearance, int(ble_sig_appearance_t*))
DYNALIB_FN(9, hal_ble, hal_ble_gap_set_ppcp, int(const hal_ble_conn_params_t*, void*))
DYNALIB_FN(10, hal_ble, hal_ble_gap_get_ppcp, int(hal_ble_conn_params_t*, void*))
DYNALIB_FN(11, hal_ble, hal_ble_gap_add_whitelist, int(const hal_ble_addr_t*, size_t, void*))
DYNALIB_FN(12, hal_ble, hal_ble_gap_delete_whitelist, int(void*))
DYNALIB_FN(13, hal_ble, hal_ble_gap_set_tx_power, int(int8_t))
DYNALIB_FN(14, hal_ble, hal_ble_gap_get_tx_power, int(int8_t*, void*))
DYNALIB_FN(15, hal_ble, hal_ble_gap_set_advertising_parameters, int(const hal_ble_adv_params_t*, void*))
DYNALIB_FN(16, hal_ble, hal_ble_gap_get_advertising_parameters, int(hal_ble_adv_params_t*, void*))
DYNALIB_FN(17, hal_ble, hal_ble_gap_set_advertising_data, int(const uint8_t*, size_t, void*))
DYNALIB_FN(18, hal_ble, hal_ble_gap_get_advertising_data, ssize_t(uint8_t*, size_t, void*))
DYNALIB_FN(19, hal_ble, hal_ble_gap_set_scan_response_data, int(const uint8_t*, size_t, void*))
DYNALIB_FN(20, hal_ble, hal_ble_gap_get_scan_response_data, ssize_t(uint8_t*, size_t, void*))
DYNALIB_FN(21, hal_ble, hal_ble_gap_start_advertising, int(void*))
DYNALIB_FN(22, hal_ble, hal_ble_gap_set_auto_advertise, int(hal_ble_auto_adv_cfg_t, void*))
DYNALIB_FN(23, hal_ble, hal_ble_gap_get_auto_advertise, hal_ble_auto_adv_cfg_t(void*))
DYNALIB_FN(24, hal_ble, hal_ble_gap_stop_advertising, int(void))
DYNALIB_FN(25, hal_ble, hal_ble_gap_is_advertising, bool(void))
DYNALIB_FN(26, hal_ble, hal_ble_gap_set_scan_parameters, int(const hal_ble_scan_params_t*, void*))
DYNALIB_FN(27, hal_ble, hal_ble_gap_get_scan_parameters, int(hal_ble_scan_params_t*, void*))
DYNALIB_FN(28, hal_ble, hal_ble_gap_start_scan, int(on_ble_scan_result_cb_t, void*, void*))
DYNALIB_FN(29, hal_ble, hal_ble_gap_is_scanning, bool(void))
DYNALIB_FN(30, hal_ble, hal_ble_gap_stop_scan, int(void))
DYNALIB_FN(31, hal_ble, hal_ble_gap_connect, int(const hal_ble_addr_t*, void*))
DYNALIB_FN(32, hal_ble, hal_ble_gap_is_connecting, bool(const hal_ble_addr_t*))
DYNALIB_FN(33, hal_ble, hal_ble_gap_is_connected, bool(const hal_ble_addr_t*))
DYNALIB_FN(34, hal_ble, hal_ble_gap_connect_cancel, int(const hal_ble_addr_t*))
DYNALIB_FN(35, hal_ble, hal_ble_gap_disconnect, int(hal_ble_conn_handle_t, void*))
DYNALIB_FN(36, hal_ble, hal_ble_gap_update_connection_params, int(hal_ble_conn_handle_t, const hal_ble_conn_params_t*, void*))
DYNALIB_FN(37, hal_ble, hal_ble_gap_get_connection_params, int(hal_ble_conn_handle_t, hal_ble_conn_params_t*, void*))
DYNALIB_FN(38, hal_ble, hal_ble_gap_get_rssi, int(hal_ble_conn_handle_t, void*))
DYNALIB_FN(39, hal_ble, hal_ble_gatt_server_add_service, int(uint8_t, const hal_ble_uuid_t*, hal_ble_attr_handle_t*, void*))
DYNALIB_FN(40, hal_ble, hal_ble_gatt_server_add_characteristic, int(const hal_ble_char_init_t*, hal_ble_char_handles_t*, void*))
DYNALIB_FN(41, hal_ble, hal_ble_gatt_server_add_descriptor, int(const hal_ble_desc_init_t*, hal_ble_attr_handle_t*, void*))
DYNALIB_FN(42, hal_ble, hal_ble_gatt_server_set_characteristic_value, ssize_t(hal_ble_attr_handle_t, const uint8_t*, size_t, void*))
DYNALIB_FN(43, hal_ble, hal_ble_gatt_server_get_characteristic_value, ssize_t(hal_ble_attr_handle_t, uint8_t*, size_t, void*))
DYNALIB_FN(44, hal_ble, hal_ble_gatt_client_discover_all_services, int(hal_ble_conn_handle_t, on_ble_disc_service_cb_t, void*, void*))
DYNALIB_FN(45, hal_ble, hal_ble_gatt_client_discover_service_by_uuid, int(hal_ble_conn_handle_t, const hal_ble_uuid_t*, on_ble_disc_service_cb_t, void*, void*))
DYNALIB_FN(46, hal_ble, hal_ble_gatt_client_discover_characteristics, int(hal_ble_conn_handle_t, const hal_ble_svc_t*, on_ble_disc_char_cb_t, void*, void*))
DYNALIB_FN(47, hal_ble, hal_ble_gatt_client_discover_characteristics_by_uuid, int(hal_ble_conn_handle_t, const hal_ble_svc_t*, const hal_ble_uuid_t*, on_ble_disc_char_cb_t, void*, void*))
DYNALIB_FN(48, hal_ble, hal_ble_gatt_client_is_discovering, bool(hal_ble_conn_handle_t))
DYNALIB_FN(49, hal_ble, hal_ble_gatt_client_set_att_mtu, int(size_t, void*))
DYNALIB_FN(50, hal_ble, hal_ble_gatt_client_configure_cccd, int(hal_ble_conn_handle_t, hal_ble_attr_handle_t, ble_sig_cccd_value_t, void*))
DYNALIB_FN(51, hal_ble, hal_ble_gatt_client_write_with_response, ssize_t(hal_ble_conn_handle_t, hal_ble_attr_handle_t, const uint8_t*, size_t, void*))
DYNALIB_FN(52, hal_ble, hal_ble_gatt_client_write_without_response, ssize_t(hal_ble_conn_handle_t, hal_ble_attr_handle_t, const uint8_t*, size_t, void*))
DYNALIB_FN(53, hal_ble, hal_ble_gatt_client_read, ssize_t(hal_ble_conn_handle_t, hal_ble_attr_handle_t, uint8_t*, size_t, void*))

DYNALIB_END(hal_ble)

#endif /* HAL_PLATFORM_BLE */

#endif /* HAL_DYNALIB_BLE_H */
