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

DYNALIB_FN(0, hal_btstack, hal_btstack_init, void(void))
DYNALIB_FN(1, hal_btstack, hal_btstack_deInit, void(void))
DYNALIB_FN(2, hal_btstack, hal_btstack_loop_execute, void(void))

DYNALIB_FN(3, hal_btstack, hal_btstack_setTimer, void(btstack_timer_source_t*, uint32_t))
DYNALIB_FN(4, hal_btstack, hal_btstack_setTimerHandler, void(btstack_timer_source_t*, btstack_timer_handler_t))
DYNALIB_FN(5, hal_btstack, hal_btstack_addTimer, void(btstack_timer_source_t*))
DYNALIB_FN(6, hal_btstack, hal_btstack_removeTimer, int(btstack_timer_source_t*))
DYNALIB_FN(7, hal_btstack, hal_btstack_getTimeMs, uint32_t(void))

DYNALIB_FN(8, hal_btstack, hal_btstack_Log_info, void(uint8_t))
DYNALIB_FN(9, hal_btstack, hal_btstack_error_info, void(uint8_t))
DYNALIB_FN(10, hal_btstack, hal_btstack_enable_packet_info, void(void))


DYNALIB_FN(11, hal_btstack, hal_btstack_setRandomAddressMode, void(gap_random_address_type_t))
DYNALIB_FN(12, hal_btstack, hal_btstack_setRandomAddr, void(bd_addr_t))
DYNALIB_FN(13, hal_btstack, hal_btstack_setPublicBdAddr, void(bd_addr_t))
DYNALIB_FN(14, hal_btstack, hal_btstack_getLocalBdAddr, void(bd_addr_t))
DYNALIB_FN(15, hal_btstack, hal_btstack_getAddrOfAdvertisement, void(uint8_t*, bd_addr_t))
DYNALIB_FN(16, hal_btstack, hal_btstack_setLocalName, void(const char*))
DYNALIB_FN(17, hal_btstack, hal_btstack_setAdvertisementParams, void(uint16_t, uint16_t, uint8_t, uint8_t, bd_addr_t, uint8_t, uint8_t))
DYNALIB_FN(18, hal_btstack, hal_btstack_setAdvertisementData, void(uint16_t, uint8_t*))
DYNALIB_FN(19, hal_btstack, hal_btstack_setScanResponseData, void(uint16_t, uint8_t*))
DYNALIB_FN(20, hal_btstack, hal_btstack_startAdvertising, void(void))
DYNALIB_FN(21, hal_btstack, hal_btstack_stopAdvertising, void(void))

DYNALIB_FN(22, hal_btstack, hal_btstack_setConnectedCallback, void(bleConnectedCallback_t))
DYNALIB_FN(23, hal_btstack, hal_btstack_setDisconnectedCallback, void(bleDisconnectedCallback_t))

DYNALIB_FN(24, hal_btstack, hal_btstack_disconnect, void(uint16_t))
DYNALIB_FN(25, hal_btstack, hal_btstack_connect, uint8_t(bd_addr_t, bd_addr_type_t))

DYNALIB_FN(26, hal_btstack, hal_btstack_setConnParamsRange, void(le_connection_parameter_range_t))

DYNALIB_FN(27, hal_btstack, hal_btstack_startScanning, void(void))
DYNALIB_FN(28, hal_btstack, hal_btstack_stopScanning, void(void))

DYNALIB_FN(29, hal_btstack, hal_btstack_setScanParams, void(uint8_t, uint16_t, uint16_t))
DYNALIB_FN(30, hal_btstack, hal_btstack_setBLEAdvertisementCallback, void(bleAdvertismentCallback_t))

DYNALIB_FN(31, hal_btstack, hal_btstack_attServerCanSend, int(void))
DYNALIB_FN(32, hal_btstack, hal_btstack_attServerSendNotify, int(uint16_t, uint8_t*, uint16_t))
DYNALIB_FN(33, hal_btstack, hal_btstack_attServerSendIndicate, int(uint16_t, uint8_t*, uint16_t))

DYNALIB_FN(34, hal_btstack, hal_btstack_setGattCharsRead, void(gattReadCallback_t))
DYNALIB_FN(35, hal_btstack, hal_btstack_setGattCharsWrite, void(gattWriteCallback_t))

DYNALIB_FN(36, hal_btstack, hal_btstack_addServiceUUID16bits, void(uint16_t))
DYNALIB_FN(37, hal_btstack, hal_btstack_addServiceUUID128bits, void(uint8_t*))
DYNALIB_FN(38, hal_btstack, hal_btstack_addCharsUUID16bits, uint16_t(uint16_t, uint16_t, uint8_t*, uint16_t))
DYNALIB_FN(39, hal_btstack, hal_btstack_addCharsUUID128bits, uint16_t(uint8_t*, uint16_t, uint8_t*, uint16_t))
DYNALIB_FN(40, hal_btstack, hal_btstack_addCharsDynamicUUID16bits, uint16_t(uint16_t, uint16_t, uint8_t*, uint16_t))
DYNALIB_FN(41, hal_btstack, hal_btstack_addCharsDynamicUUID128bits, uint16_t(uint8_t*, uint16_t, uint8_t*, uint16_t))


DYNALIB_FN(42, hal_btstack, hal_btstack_setGattServiceDiscoveredCallback, void(gattServicesDiscoveredCallback_t))
DYNALIB_FN(43, hal_btstack, hal_btstack_setGattCharsDiscoveredCallback, void(gattCharsDiscoveredCallback_t))
DYNALIB_FN(44, hal_btstack, hal_btstack_setGattDescriptorsDiscoveredCallback, void(gattDescriptorsDiscoveredCallback_t))

DYNALIB_FN(45, hal_btstack, hal_btstack_setGattCharacteristicReadCallback, void(gattCharacteristicReadCallback_t))
DYNALIB_FN(46, hal_btstack, hal_btstack_setGattCharacteristicWrittenCallback, void(gattCharacteristicWrittenCallback_t))

DYNALIB_FN(47, hal_btstack, hal_btstack_setGattDescriptorReadCallback, void(gattDescriptorReadCallback_t))
DYNALIB_FN(48, hal_btstack, hal_btstack_setGattDescriptorWrittenCallback, void(gattDescriptorWrittenCallback_t))

DYNALIB_FN(49, hal_btstack, hal_btstack_setGattWriteCCCDCallback, void(gattWriteCCCDCallback_t))
DYNALIB_FN(50, hal_btstack, hal_btstack_setGattNotifyUpdateCallback, void(gattNotifyUpdateCallback_t))
DYNALIB_FN(51, hal_btstack, hal_btstack_setGattIndicateUpdateCallback, void(gattIndicateUpdateCallback_t))

DYNALIB_FN(52, hal_btstack, hal_btstack_discoverPrimaryServices, uint8_t(uint16_t))
DYNALIB_FN(53, hal_btstack, hal_btstack_discoverPrimaryServicesByUUID16, uint8_t(uint16_t, uint16_t))
DYNALIB_FN(54, hal_btstack, hal_btstack_discoverPrimaryServicesByUUID128, uint8_t(uint16_t, const uint8_t*))

DYNALIB_FN(55, hal_btstack, hal_btstack_discoverCharsForService, uint8_t(uint16_t, gatt_client_service_t*))
DYNALIB_FN(56, hal_btstack, hal_btstack_discoverCharsForHandleRangeByUUID16, uint8_t(uint16_t, uint16_t, uint16_t, uint16_t))
DYNALIB_FN(57, hal_btstack, hal_btstack_discoverCharsForHandleRangeByUUID128, uint8_t(uint16_t, uint16_t, uint16_t, uint8_t*))
DYNALIB_FN(58, hal_btstack, hal_btstack_discoverCharsForServiceByUUID16, uint8_t(uint16_t, gatt_client_service_t*, uint16_t))
DYNALIB_FN(59, hal_btstack, hal_btstack_discoverCharsForServiceByUUID128, uint8_t(uint16_t, gatt_client_service_t*, uint8_t*))

DYNALIB_FN(60, hal_btstack, hal_btstack_discoverCharsDescriptors, uint8_t(uint16_t, gatt_client_characteristic_t*))

DYNALIB_FN(61, hal_btstack, hal_btstack_readValueOfCharacteristic, uint8_t(uint16_t, gatt_client_characteristic_t*))
DYNALIB_FN(62, hal_btstack, hal_btstack_readValueOfCharacteristicUsingValueHandle, uint8_t(uint16_t, uint16_t))
DYNALIB_FN(63, hal_btstack, hal_btstack_readValueOfCharacteristicByUUID16, uint8_t(uint16_t, uint16_t, uint16_t, uint16_t))
DYNALIB_FN(64, hal_btstack, hal_btstack_readValueOfCharacteristicByUUID128, uint8_t(uint16_t, uint16_t, uint16_t, uint8_t*))
DYNALIB_FN(65, hal_btstack, hal_btstack_readLongValueOfCharacteristic, uint8_t(uint16_t, gatt_client_characteristic_t*))
DYNALIB_FN(66, hal_btstack, hal_btstack_readLongValueOfCharacteristicUsingValueHandle, uint8_t(uint16_t, uint16_t))
DYNALIB_FN(67, hal_btstack, hal_btstack_readLongValueOfCharacteristicUsingValueHandleWithOffset, uint8_t(uint16_t, uint16_t, uint16_t))
DYNALIB_FN(68, hal_btstack, hal_btstack_writeValueOfChracteristicWithoutResponse, uint8_t(uint16_t, uint16_t, uint16_t, uint8_t*))
DYNALIB_FN(69, hal_btstack, hal_btstack_writeValueOfCharacteristic, uint8_t(uint16_t, uint16_t, uint16_t, uint8_t*))
DYNALIB_FN(70, hal_btstack, hal_btstack_writeLongValueOfCharacteristic, uint8_t(uint16_t, uint16_t, uint16_t, uint8_t*))
DYNALIB_FN(71, hal_btstack, hal_btstack_writeLongValueOfCharacteristicWithOffset, uint8_t(uint16_t, uint16_t, uint16_t, uint16_t, uint8_t*))

DYNALIB_FN(72, hal_btstack, hal_btstack_readCharacteristicDescriptor, uint8_t(uint16_t, gatt_client_characteristic_descriptor_t*))
DYNALIB_FN(73, hal_btstack, hal_btstack_readCharacteristicDescriptorUsingDescriptorHandle, uint8_t(uint16_t, uint16_t))
DYNALIB_FN(74, hal_btstack, hal_btstack_readLongCharacteristicDescriptor, uint8_t(uint16_t, gatt_client_characteristic_descriptor_t*))
DYNALIB_FN(75, hal_btstack, hal_btstack_readLongCharacteristicDescriptorUsingDescriptorHandle, uint8_t(uint16_t, uint16_t))
DYNALIB_FN(76, hal_btstack, hal_btstack_readLongCharacteristicDescriptorUsingDescriptorHandleWithOffset, uint8_t(uint16_t, uint16_t, uint16_t))

DYNALIB_FN(77, hal_btstack, hal_btstack_writeCharacteristicDescriptor, uint8_t(uint16_t, gatt_client_characteristic_descriptor_t*, uint16_t, uint8_t*))
DYNALIB_FN(78, hal_btstack, hal_btstack_writeCharacteristicDescriptorUsingDescriptorHandle, uint8_t(uint16_t, uint16_t, uint16_t, uint8_t*))
DYNALIB_FN(79, hal_btstack, hal_btstack_writeLongCharacteristicDescriptor, uint8_t(uint16_t, gatt_client_characteristic_descriptor_t*, uint16_t, uint8_t*))
DYNALIB_FN(80, hal_btstack, hal_btstack_writeLongCharacteristicDescriptorUsingDescriptorHandle, uint8_t(uint16_t, uint16_t, uint16_t, uint8_t*))
DYNALIB_FN(81, hal_btstack, hal_btstack_writeLongCharacteristicDescriptorUsingDescriptorHandleWithOffset, uint8_t(uint16_t, uint16_t, uint16_t, uint16_t, uint8_t*))

DYNALIB_FN(82, hal_btstack, hal_btstack_WriteClientCharacteristicConfiguration, uint8_t(uint16_t, gatt_client_characteristic_t*, uint16_t))
DYNALIB_FN(83, hal_btstack, hal_btstack_listenForCharacteristicValueUpdates, void(gatt_client_notification_t*, uint16_t, gatt_client_characteristic_t*))


DYNALIB_END(hal_btstack)

#endif

#endif /* HAL_DYNALIB_BTSTACK_H */
