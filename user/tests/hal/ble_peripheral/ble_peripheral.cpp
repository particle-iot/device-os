/*
 ******************************************************************************
  Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

#include "application.h"
#include "unit-test/unit-test.h"
#include "ble_hal.h"
#include "system_error.h"

Serial1LogHandler log(115200, LOG_LEVEL_ALL);

hal_ble_characteristic_t characteristic1; // Read and Write
hal_ble_characteristic_t characteristic2; // Notify
hal_ble_characteristic_t characteristic3; // Write without response

const char *addrType[4] = {
    "Public",
    "Random Static",
    "Random Private Resolvable",
    "Random Static Non-resolvable"
};

static void ble_on_connected(hal_ble_gap_on_connected_evt_t *event) {
    LOG(TRACE, "BLE connected, connection handle: 0x%04X.", event->conn_handle);
    LOG(TRACE, "Local device role: %d.", event->role);
    if (event->peer_addr.addr_type <= 3) {
        LOG(TRACE, "Peer address type: %s", addrType[event->peer_addr.addr_type]);
    }
    else {
        LOG(TRACE, "Peer address type: Anonymous");
    }
    LOG(TRACE, "Peer address: %02X:%02X:%02X:%02X:%02X:%02X.", event->peer_addr.addr[0], event->peer_addr.addr[1],
                event->peer_addr.addr[2], event->peer_addr.addr[3], event->peer_addr.addr[4], event->peer_addr.addr[5]);
    LOG(TRACE, "Interval: %.2fms, Latency: %d, Timeout: %dms", event->conn_interval*1.25, event->slave_latency, event->conn_sup_timeout*10);
}

static void ble_on_disconnected(hal_ble_gap_on_disconnected_evt_t *event) {
    LOG(TRACE, "BLE disconnected, connection handle: 0x%04X.", event->conn_handle);
    if (event->peer_addr.addr_type <= 3) {
        LOG(TRACE, "Peer address type: %s", addrType[event->peer_addr.addr_type]);
    }
    else {
        LOG(TRACE, "Peer address type: Anonymous");
    }
    LOG(TRACE, "Peer address: %02X:%02X:%02X:%02X:%02X:%02X.", event->peer_addr.addr[0], event->peer_addr.addr[1],
                event->peer_addr.addr[2], event->peer_addr.addr[3], event->peer_addr.addr[4], event->peer_addr.addr[5]);
}

static void ble_on_data_received(hal_ble_gatt_server_on_data_received_evt_t* event) {
    LOG(TRACE, "BLE data received, connection handle: 0x%04X.", event->conn_handle);

    if (event->attr_handle == characteristic1.value_handle) {
        LOG(TRACE, "Write BLE characteristic 1 value:");
    }
    else if (event->attr_handle == characteristic2.cccd_handle) {
        LOG(TRACE, "Configure BLE characteristic 2 CCCD:");
    }
    else if (event->attr_handle == characteristic3.value_handle) {
        LOG(TRACE, "Write BLE characteristic 3 value:");
    }
    else {
        LOG(TRACE, "BLE received data, attribute handle: %d.", event->attr_handle);
    }

    for (uint8_t i = 0; i < event->data_len; i++) {
        Serial1.printf("0x%02X,", event->data[i]);
    }
    Serial1.print("\r\n");
}

test(01_BleStackReinitializationShouldFail) {
    int ret;

    // First time to initialize BLE stack
    ble_stack_init(NULL);

    ret = ble_stack_init(NULL);
    assertNotEqual(ret, 0);

    ble_gap_set_callback_on_connected(ble_on_connected);
    ble_gap_set_callback_on_disconnected(ble_on_disconnected);
    ble_gatt_server_set_callback_on_data_received(ble_on_data_received);
}

test(02_BleSetDeviceAddressShouldBePublicOrRandomStatic) {
    int ret;
    hal_ble_address_t setAddr;
    hal_ble_address_t getAddr;
    uint8_t macAddr[6] = {0x10, 0x23, 0x35, 0x46, 0x57, 0x66};

    // Set a public device address
    setAddr.addr_type = BLE_SIG_ADDR_TYPE_PUBLIC;
    memcpy(setAddr.addr, macAddr, 6);
    ret = ble_gap_set_device_address(&setAddr);
    assertEqual(ret, 0);

    // Get device address
    ret = ble_gap_get_device_address(&getAddr);
    assertEqual(ret, 0);
    assertEqual(getAddr.addr_type, BLE_SIG_ADDR_TYPE_PUBLIC);
    ret = memcmp(setAddr.addr, getAddr.addr, 6);
    assertEqual(ret, 0);

    // Set a random device address
    setAddr.addr_type = BLE_SIG_ADDR_TYPE_RANDOM_STATIC;
    memcpy(setAddr.addr, macAddr, 6);
    ret = ble_gap_set_device_address(&setAddr);
#if !HAL_PLATFORM_NRF52840
    assertEqual(ret, 0);
#else
    // nRF52840 must set the random static address to be as the same
    // as the populated random address during IC manufacturing. Otherwise, it will fail.
    // Upon BLE stack being initialized, the device address of type is random static already.
    assertNotEqual(ret, 0);
#endif

    // Get device address
    ret = ble_gap_get_device_address(&getAddr);
    assertEqual(ret, 0);
#if !HAL_PLATFORM_NRF52840
    assertEqual(getAddr.addr_type, BLE_SIG_ADDR_TYPE_RANDOM_STATIC);
#else
    assertEqual(getAddr.addr_type, BLE_SIG_ADDR_TYPE_PUBLIC);
#endif
}

test(03_BleSetDeviceNameWithoutNullTerminated) {
    int ret;

    // Set device name
    const char* devName = "01234567890123456789";
    ret = ble_gap_set_device_name((const unsigned char*)devName, strlen(devName));
    assertEqual(ret, 0);

    // Get device name
    uint8_t devName1[31];
    uint16_t devName1Len = sizeof(devName1);
    ret = ble_gap_get_device_name(devName1, &devName1Len);
    assertEqual(ret, 0);
    assertEqual(devName1Len, 20);
    ret = memcmp(devName1, devName, strlen(devName));
    assertEqual(ret, 0);
}

test(04_BleSetAppearance) {
    int ret;
    uint16_t appearance;

    // Set BLE appearance
    ret = ble_gap_set_appearance(BLE_SIG_APPEARANCE_GENERIC_THERMOMETER);
    assertEqual(ret, 0);

    // Get BLE appearance
    ret = ble_gap_get_appearance(&appearance);
    assertEqual(ret, 0);
    assertEqual(appearance, BLE_SIG_APPEARANCE_GENERIC_THERMOMETER);
}

test(05_BleSetPpcpShouldBeWithinValidRange) {
    int ret;
    hal_ble_connection_parameters_t setConnParams;
    hal_ble_connection_parameters_t getConnParams;

    // Set invalid PPCP
    setConnParams.min_conn_interval = BLE_SIG_CP_MIN_CONN_INTERVAL_MAX + 1;
    setConnParams.max_conn_interval = BLE_SIG_CP_MAX_CONN_INTERVAL_NONE;
    setConnParams.slave_latency     = 1;
    setConnParams.conn_sup_timeout  = 400;
    ret = ble_gap_set_ppcp(&setConnParams);
    assertNotEqual(ret, 0);

    setConnParams.min_conn_interval = BLE_SIG_CP_MIN_CONN_INTERVAL_NONE;
    setConnParams.max_conn_interval = BLE_SIG_CP_MAX_CONN_INTERVAL_MAX + 1;
    ret = ble_gap_set_ppcp(&setConnParams);
    assertNotEqual(ret, 0);

    setConnParams.min_conn_interval = BLE_SIG_CP_MIN_CONN_INTERVAL_NONE;
    setConnParams.max_conn_interval = BLE_SIG_CP_MAX_CONN_INTERVAL_NONE;
    setConnParams.slave_latency     = BLE_SIG_CP_SLAVE_LATENCY_MAX + 1;
    ret = ble_gap_set_ppcp(&setConnParams);
    assertNotEqual(ret, 0);

    setConnParams.slave_latency     = 1;
    setConnParams.conn_sup_timeout  = BLE_SIG_CP_CONN_SUP_TIMEOUT_MAX + 1;
    ret = ble_gap_set_ppcp(&setConnParams);
    assertNotEqual(ret, 0);

    // Set valid PPCP
    setConnParams.min_conn_interval = 100;
    setConnParams.max_conn_interval = 400;
    setConnParams.slave_latency     = 1;
    setConnParams.conn_sup_timeout  = 400;
    ret = ble_gap_set_ppcp(&setConnParams);
    assertEqual(ret, 0);

    // Get PPCP
    ret = ble_gap_get_ppcp(&getConnParams);
    assertEqual(ret, 0);
    assertEqual(getConnParams.min_conn_interval, 100);
    assertEqual(getConnParams.max_conn_interval, 400);
    assertEqual(getConnParams.slave_latency, 1);
    assertEqual(getConnParams.conn_sup_timeout, 400);
}

test(06_BleSetTxPowerShouldBeRounded) {
    int ret;
    int8_t txPower;

#if HAL_PLATFORM_NRF52840
    // Valid TX power for nRF52840: -20, -16, -12, -8, -4, 0, 4, 8
    ret = ble_gap_set_tx_power(-30);
    assertEqual(ret, 0);
    ret = ble_gap_get_tx_power(&txPower);
    assertEqual(txPower, -20);

    ret = ble_gap_set_tx_power(-18);
    assertEqual(ret, 0);
    ret = ble_gap_get_tx_power(&txPower);
    assertEqual(txPower, -16);

    ret = ble_gap_set_tx_power(-14);
    assertEqual(ret, 0);
    ret = ble_gap_get_tx_power(&txPower);
    assertEqual(txPower, -12);

    ret = ble_gap_set_tx_power(-10);
    assertEqual(ret, 0);
    ret = ble_gap_get_tx_power(&txPower);
    assertEqual(txPower, -8);

    ret = ble_gap_set_tx_power(-6);
    assertEqual(ret, 0);
    ret = ble_gap_get_tx_power(&txPower);
    assertEqual(txPower, -4);

    ret = ble_gap_set_tx_power(-2);
    assertEqual(ret, 0);
    ret = ble_gap_get_tx_power(&txPower);
    assertEqual(txPower, 0);

    ret = ble_gap_set_tx_power(2);
    assertEqual(ret, 0);
    ret = ble_gap_get_tx_power(&txPower);
    assertEqual(txPower, 4);

    ret = ble_gap_set_tx_power(6);
    assertEqual(ret, 0);
    ret = ble_gap_get_tx_power(&txPower);
    assertEqual(txPower, 8);

    ret = ble_gap_set_tx_power(10);
    assertEqual(ret, 0);
    ret = ble_gap_get_tx_power(&txPower);
    assertEqual(txPower, 8);
#endif
}

test(07_BleSetAdvertisingParametersShouldBeValid) {
    int ret;
    hal_ble_advertising_parameters_t advParams;

    advParams.type          = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT;
    advParams.filter_policy = BLE_ADV_FP_ANY;
    advParams.interval      = 100;
    advParams.timeout       = 500; // Advertising for 5 seconds
    ret = ble_gap_set_advertising_parameters(&advParams);
    assertEqual(ret, 0);

    ret = ble_gap_start_advertising();
    assertEqual(ret, 0);
    ret = ble_gap_is_advertising();
    assertEqual(ret, true);

    delay(6000);

    ret = ble_gap_is_advertising();
    assertEqual(ret, false);
}

test(08_BleChangeAdvertisingParametersDuringAdvertising) {
    int ret;
    hal_ble_advertising_parameters_t advParams;

    advParams.type          = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT;
    advParams.filter_policy = BLE_ADV_FP_ANY;
    advParams.interval      = 100;
    advParams.timeout       = 0; // Advertising forever
    ret = ble_gap_set_advertising_parameters(&advParams);
    assertEqual(ret, 0);

    ret = ble_gap_start_advertising();
    assertEqual(ret, 0);
    ret = ble_gap_is_advertising();
    assertEqual(ret, true);

    delay(1000);

    advParams.type          = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT;
    advParams.filter_policy = BLE_ADV_FP_ANY;
    advParams.interval      = 100;
    advParams.timeout       = 500; // Advertising for 5 seconds
    ret = ble_gap_set_advertising_parameters(&advParams);
    assertEqual(ret, 0);

    delay(6000);

    ret = ble_gap_is_advertising();
    assertEqual(ret, false);
}

test(09_BleChangeAdvertisingDataDuringAdvertising) {
    int ret;
    uint8_t advDataSet1[] = {
        0x02,
        BLE_SIG_AD_TYPE_FLAGS,
        BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE,
        0x0A,
        BLE_SIG_AD_TYPE_COMPLETE_LOCAL_NAME,
        'B','L','E',' ','N','A','M','E','1'
    };
    uint8_t advDataSet2[] = {
        0x02,
        BLE_SIG_AD_TYPE_FLAGS,
        BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE,
        0x0A,
        BLE_SIG_AD_TYPE_COMPLETE_LOCAL_NAME,
        'B','L','E',' ','N','A','M','E','2'
    };

    ret = ble_gap_set_advertising_data(advDataSet1, sizeof(advDataSet1));
    assertEqual(ret, 0);

    // Advertising for 5 seconds
    ret = ble_gap_start_advertising();
    assertEqual(ret, 0);
    ret = ble_gap_is_advertising();
    assertEqual(ret, true);

    delay(2500);

    // Update the advertising data. It will restart advertising for 5 seconds
    ret = ble_gap_set_advertising_data(advDataSet2, sizeof(advDataSet1));
    assertEqual(ret, 0);
    ret = ble_gap_is_advertising();
    assertEqual(ret, true);

    delay(1000);

    ret = ble_gap_stop_advertising();
    assertEqual(ret, 0);
    delay(100);
    ret = ble_gap_is_advertising();
    assertEqual(ret, false);
}

test(10_BleAddServicesAndCharacteristics_NeedToConnectAndCheckOnTheCentralSide) {
    int ret;
    hal_ble_advertising_parameters_t advParams;
    uint16_t svcHandle;

    uint8_t svcUUID1[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x00,0x00,0x0e,0x0f};
    uint8_t charUUID1[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x01,0x00,0x0e,0x0f};
    ret = ble_gatt_server_add_service_uuid128(BLE_SERVICE_TYPE_PRIMARY, svcUUID1, &svcHandle);
    assertEqual(ret, 0);
    ret = ble_gatt_server_add_characteristic_uuid128(svcHandle, charUUID1, BLE_SIG_CHAR_PROP_READ|BLE_SIG_CHAR_PROP_WRITE, NULL, &characteristic1);
    assertEqual(ret, 0);

    uint8_t svcUUID2[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x00,0x00,0x0e,0x10};
    uint8_t charUUID2[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x01,0x00,0x0e,0x10};
    ret = ble_gatt_server_add_service_uuid128(BLE_SERVICE_TYPE_PRIMARY, svcUUID2, &svcHandle);
    assertEqual(ret, 0);
    ret = ble_gatt_server_add_characteristic_uuid128(svcHandle, charUUID2, BLE_SIG_CHAR_PROP_NOTIFY, NULL, &characteristic2);
    assertEqual(ret, 0);

    ret = ble_gatt_server_add_service_uuid16(BLE_SERVICE_TYPE_PRIMARY, 0x1234, &svcHandle);
    assertEqual(ret, 0);
    ret = ble_gatt_server_add_characteristic_uuid16(svcHandle, 0x5678, BLE_SIG_CHAR_PROP_WRITE_WO_RESP, "hello", &characteristic3);
    assertEqual(ret, 0);

    uint8_t data[20] = {0x11};
    ret = ble_gatt_server_set_characteristic_value(characteristic1.value_handle, data, 5);
    assertEqual(ret, 0);

    advParams.type          = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT;
    advParams.filter_policy = BLE_ADV_FP_ANY;
    advParams.interval      = 100;
    advParams.timeout       = 0; // Advertising forever
    ret = ble_gap_set_advertising_parameters(&advParams);
    assertEqual(ret, 0);

    ret = ble_gap_start_advertising();
    assertEqual(ret, 0);

    // Use a BLE central device to connect to it and check the BLE services and characteristics
    uint16_t timeout = 0;
    while(!ble_gap_is_connected()) {
        delay(500);
        timeout += 500;
        if(timeout >= 15000) {
            break;
        }
    }

    ret = ble_gap_is_connected();
    assertEqual(ret, true);
}

test(11_BlePeripheralCanDisconnectInitially_NeedToConnectByCentralSide) {
    int ret;

    // Wait until the previous test case finished.
    // This disconnection should be initiated by the central side.
    // Once disconnected, it should restart advertising automatically.
    while(ble_gap_is_connected());

    // Use a BLE central device to connect to it and wait for 2 seconds without doing anything.
    uint16_t timeout = 0;
    while(!ble_gap_is_connected()) {
        delay(500);
        timeout += 500;
        if(timeout >= 15000) {
            break;
        }
    }

    ret = ble_gap_is_connected();
    assertEqual(ret, true);

    delay(5000);
    ret = ble_gap_disconnect(0);
    assertEqual(ret, 0);

    delay(500);

    // It should restart advertising
    ret = ble_gap_is_connected();
    assertEqual(ret, false);
    ret = ble_gap_is_advertising();
    assertEqual(ret, true);
}

