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

#include "application.h"
#include "unit-test/unit-test.h"
#include "ble_hal.h"
#include "system_error.h"

Serial1LogHandler log(115200, LOG_LEVEL_ALL);

hal_ble_char_handles_t localCharacteristic1; // Read and Write
hal_ble_char_handles_t localCharacteristic2; // Notify
hal_ble_char_handles_t localCharacteristic3; // Write without response

const char* addrType[4] = {
    "Public",
    "Random Static",
    "Random Private Resolvable",
    "Random Static Non-resolvable"
};

uint8_t svc1UUID[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x00,0x00,0x0e,0x0f};
uint8_t char1UUID[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x01,0x00,0x0e,0x0f};
uint8_t svc2UUID[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x00,0x00,0x0e,0x10};
uint8_t char2UUID[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x01,0x00,0x0e,0x10};
uint16_t svc3UUID  = 0x1234;
uint16_t char3UUID = 0x5678;

uint16_t connHandle = BLE_INVALID_CONN_HANDLE;
hal_ble_addr_t auxPeripheralAddr;
bool auxPeripheralFound = false;

hal_ble_svc_t service1, service2, service3;
hal_ble_char_t char1, char2, char3;
bool svc1Discovered = false, svc2Discovered = false, svc3Discovered= false;
bool char1Discovered = false, char2Discovered = false, char3Discovered = false;
bool notifiedData = false;

static void ble_on_scan_result(const hal_ble_gap_on_scan_result_evt_t* event, void* context) {
    uint8_t  devName[20];
    uint16_t devNameLen = sizeof(devName);
    if (event->data[4] == BLE_SIG_AD_TYPE_COMPLETE_LOCAL_NAME) {
        devNameLen = event->data[3] - 1;
        memcpy(devName, &event->data[5], devNameLen);
        devName[devNameLen] = '\0';
        LOG(TRACE, "Found device: %s", devName);
        if (!strcmp((const char*)devName, "Argon BLE Sample")) {
            ble_gap_stop_scan();
            auxPeripheralAddr = event->peer_addr;
            auxPeripheralFound = true;
        }
    }
}

static void ble_on_connected(const hal_ble_gap_on_connected_evt_t* event) {
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
    connHandle = event->conn_handle;
}

static void ble_on_disconnected(const hal_ble_gap_on_disconnected_evt_t* event) {
    LOG(TRACE, "BLE disconnected, connection handle: 0x%04X.", event->conn_handle);
}

static void ble_on_services_discovered(const hal_ble_gattc_on_svc_disc_evt_t* event, void* context) {
    hal_ble_svc_t* service;
    for (uint8_t i = 0; i < event->count; i++) {
        service = NULL;
        if (event->services[i].uuid.type == BLE_UUID_TYPE_16BIT) {
            if (event->services[i].uuid.uuid16 == svc3UUID) {
                service = &service3;
                svc3Discovered = true;
                LOG(TRACE, "BLE Service3 found.");
            }
        }
        else if (event->services[i].uuid.type == BLE_UUID_TYPE_128BIT) {
            if (!memcmp(svc1UUID, event->services[i].uuid.uuid128, BLE_SIG_UUID_128BIT_LEN)) {
                service = &service1;
                svc1Discovered = true;
                LOG(TRACE, "BLE Service1 found.");
            }
            else if (!memcmp(svc2UUID, event->services[i].uuid.uuid128, BLE_SIG_UUID_128BIT_LEN)) {
                service = &service2;
                svc2Discovered = true;
                LOG(TRACE, "BLE Service2 found.");
            }
        }
        if (service != NULL) {
            memcpy(service, &event->services[i], sizeof(hal_ble_svc_t));
        }
    }
}

static void ble_on_characteristics_discovered(const hal_ble_gattc_on_char_disc_evt_t* event, void* context) {
    hal_ble_char_t* characteristic = NULL;
    for (uint8_t i = 0; i < event->count; i++) {
        if (event->characteristics[i].uuid.type == BLE_UUID_TYPE_16BIT) {
            if (event->characteristics[i].uuid.uuid16 == char3UUID) {
                characteristic = &char3;
                char3Discovered = true;
                LOG(TRACE, "BLE Characteristic3 found.");
            }
        }
        else if (event->characteristics[i].uuid.type == BLE_UUID_TYPE_128BIT) {
            if (!memcmp(char1UUID, event->characteristics[i].uuid.uuid128, BLE_SIG_UUID_128BIT_LEN)) {
                characteristic = &char1;
                char1Discovered = true;
                LOG(TRACE, "BLE Characteristic1 found.");
            }
            else if (!memcmp(char2UUID, event->characteristics[i].uuid.uuid128, BLE_SIG_UUID_128BIT_LEN)) {
                characteristic = &char2;
                char2Discovered = true;
                LOG(TRACE, "BLE Characteristic2 found.");
            }
        }
        if (characteristic != NULL) {
            memcpy(characteristic, &event->characteristics[i], sizeof(hal_ble_char_t));
        }
    }
}

static void ble_on_data_received(const hal_ble_gatt_on_data_evt_t* event) {
    LOG(TRACE, "BLE data received, connection handle: 0x%04X.", event->conn_handle);

    if (event->attr_handle == localCharacteristic1.value_handle) {
        LOG(TRACE, "Write BLE characteristic 1 value:");
    }
    else if (event->attr_handle == localCharacteristic2.cccd_handle) {
        LOG(TRACE, "Configure BLE characteristic 2 CCCD:");
    }
    else if (event->attr_handle == localCharacteristic3.value_handle) {
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

static void ble_on_data_notified(const hal_ble_gatt_on_data_evt_t* event) {
    if (event->attr_handle == char2.value_handle) {
        LOG(TRACE, "Notified BLE characteristic 2 value:");
        notifiedData = true;
    }
}

static void ble_on_events(const hal_ble_evts_t *event, void* context) {
    if (event->type == BLE_EVT_CONNECTED) {
        ble_on_connected(&event->params.connected);
    }
    else if (event->type == BLE_EVT_DISCONNECTED) {
        ble_on_disconnected(&event->params.disconnected);
    }
    else if (event->type == BLE_EVT_DATA_WRITTEN) {
        ble_on_data_received(&event->params.data_rec);
    }
    else if (event->type == BLE_EVT_DATA_NOTIFIED) {
        ble_on_data_notified(&event->params.data_rec);
    }
}

test(01_BleStackReinitializationShouldFail) {
    int ret;

    // First time to initialize BLE stack
    ret = ble_stack_init(nullptr);
    assertEqual(ret, 0);

    ble_set_callback_on_events(ble_on_events, nullptr);
}

test(02_BleSetDeviceAddressShouldBePublicOrRandomStatic) {
    int ret;
    hal_ble_addr_t setAddr;
    hal_ble_addr_t getAddr;
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
#if HAL_PLATFORM_NRF52840
    // nRF52840 must set the random static address to be as the same
    // as the populated random address during IC manufacturing. Otherwise, it will fail.
    // Upon BLE stack being initialized, the device address of type is random static already.
    assertNotEqual(ret, 0);
#else
    #error "Unsupported platform"
#endif

    // Get device address
    ret = ble_gap_get_device_address(&getAddr);
    assertEqual(ret, 0);
#if HAL_PLATFORM_NRF52840
    assertEqual(getAddr.addr_type, BLE_SIG_ADDR_TYPE_PUBLIC);
#else
    #error "Unsupported platform"
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
    hal_ble_conn_params_t setConnParams;
    hal_ble_conn_params_t getConnParams;

    // Set invalid PPCP
    setConnParams.min_conn_interval = BLE_SIG_CP_MIN_CONN_INTERVAL_MAX + 1;
    setConnParams.max_conn_interval = BLE_SIG_CP_MAX_CONN_INTERVAL_NONE;
    setConnParams.slave_latency     = 1;
    setConnParams.conn_sup_timeout  = 400;
    ret = ble_gap_set_ppcp(&setConnParams, nullptr);
    assertNotEqual(ret, 0);

    setConnParams.min_conn_interval = BLE_SIG_CP_MIN_CONN_INTERVAL_NONE;
    setConnParams.max_conn_interval = BLE_SIG_CP_MAX_CONN_INTERVAL_MAX + 1;
    ret = ble_gap_set_ppcp(&setConnParams, nullptr);
    assertNotEqual(ret, 0);

    setConnParams.min_conn_interval = BLE_SIG_CP_MIN_CONN_INTERVAL_NONE;
    setConnParams.max_conn_interval = BLE_SIG_CP_MAX_CONN_INTERVAL_NONE;
    setConnParams.slave_latency     = BLE_SIG_CP_SLAVE_LATENCY_MAX + 1;
    ret = ble_gap_set_ppcp(&setConnParams, nullptr);
    assertNotEqual(ret, 0);

    setConnParams.slave_latency     = 1;
    setConnParams.conn_sup_timeout  = BLE_SIG_CP_CONN_SUP_TIMEOUT_MAX + 1;
    ret = ble_gap_set_ppcp(&setConnParams, nullptr);
    assertNotEqual(ret, 0);

    // Set valid PPCP
    setConnParams.min_conn_interval = 100;
    setConnParams.max_conn_interval = 400;
    setConnParams.slave_latency     = 1;
    setConnParams.conn_sup_timeout  = 400;
    ret = ble_gap_set_ppcp(&setConnParams, nullptr);
    assertEqual(ret, 0);

    // Get PPCP
    ret = ble_gap_get_ppcp(&getConnParams, nullptr);
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
    txPower = ble_gap_get_tx_power();
    assertEqual(txPower, -20);

    ret = ble_gap_set_tx_power(-18);
    assertEqual(ret, 0);
    txPower = ble_gap_get_tx_power();
    assertEqual(txPower, -16);

    ret = ble_gap_set_tx_power(-14);
    assertEqual(ret, 0);
    txPower = ble_gap_get_tx_power();
    assertEqual(txPower, -12);

    ret = ble_gap_set_tx_power(-10);
    assertEqual(ret, 0);
    txPower = ble_gap_get_tx_power();
    assertEqual(txPower, -8);

    ret = ble_gap_set_tx_power(-6);
    assertEqual(ret, 0);
    txPower = ble_gap_get_tx_power();
    assertEqual(txPower, -4);

    ret = ble_gap_set_tx_power(-2);
    assertEqual(ret, 0);
    txPower = ble_gap_get_tx_power();
    assertEqual(txPower, 0);

    ret = ble_gap_set_tx_power(2);
    assertEqual(ret, 0);
    txPower = ble_gap_get_tx_power();
    assertEqual(txPower, 4);

    ret = ble_gap_set_tx_power(6);
    assertEqual(ret, 0);
    txPower = ble_gap_get_tx_power();
    assertEqual(txPower, 8);

    ret = ble_gap_set_tx_power(10);
    assertEqual(ret, 0);
    txPower = ble_gap_get_tx_power();
    assertEqual(txPower, 8);
#endif
}

test(07_BleSetAdvertisingParametersShouldBeValid) {
    int ret;
    hal_ble_adv_params_t advParams;

    advParams.type          = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT;
    advParams.filter_policy = BLE_ADV_FP_ANY;
    advParams.interval      = 100;
    advParams.timeout       = 500; // Advertising for 5 seconds
    ret = ble_gap_set_advertising_parameters(&advParams, nullptr);
    assertEqual(ret, 0);

    ret = ble_gap_start_advertising(nullptr);
    assertEqual(ret, 0);
    ret = ble_gap_is_advertising();
    assertEqual(ret, true);

    delay(6000);

    ret = ble_gap_is_advertising();
    assertEqual(ret, false);
}

test(08_BleChangeAdvertisingParametersDuringAdvertising) {
    int ret;
    hal_ble_adv_params_t advParams;

    advParams.type          = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT;
    advParams.filter_policy = BLE_ADV_FP_ANY;
    advParams.interval      = 100;
    advParams.timeout       = 0; // Advertising forever
    ret = ble_gap_set_advertising_parameters(&advParams, nullptr);
    assertEqual(ret, 0);

    ret = ble_gap_start_advertising(nullptr);
    assertEqual(ret, 0);
    ret = ble_gap_is_advertising();
    assertEqual(ret, true);

    delay(1000);

    advParams.type          = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT;
    advParams.filter_policy = BLE_ADV_FP_ANY;
    advParams.interval      = 100;
    advParams.timeout       = 500; // Advertising for 5 seconds
    ret = ble_gap_set_advertising_parameters(&advParams, nullptr);
    assertEqual(ret, 0);

    delay(6000);

    ret = ble_gap_is_advertising();
    assertEqual(ret, false);
}

test(09_BleChangeAdvertisingDataDuringAdvertising_Next_ConnectUsingApp) {
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

    ret = ble_gap_set_advertising_data(advDataSet1, sizeof(advDataSet1), nullptr);
    assertEqual(ret, 0);

    // Advertising for 5 seconds
    ret = ble_gap_start_advertising(nullptr);
    assertEqual(ret, 0);
    ret = ble_gap_is_advertising();
    assertEqual(ret, true);

    delay(2500);

    // Update the advertising data. It will restart advertising for 5 seconds
    ret = ble_gap_set_advertising_data(advDataSet2, sizeof(advDataSet1), nullptr);
    assertEqual(ret, 0);
    ret = ble_gap_is_advertising();
    assertEqual(ret, true);

    delay(1000);

    ret = ble_gap_stop_advertising();
    assertEqual(ret, 0);
    ret = ble_gap_is_advertising();
    assertEqual(ret, false);
}

test(10_BleAddServicesAndCharacteristics_NeedToConnectAndCheckOnTheCentralSide_Next_DisconnectAndReconnectUsingApp) {
    int ret;
    hal_ble_adv_params_t advParams;

    uint16_t svcHandle;
    hal_ble_uuid_t bleUuid;
    hal_ble_char_init_t char_init;

    bleUuid.type = BLE_UUID_TYPE_128BIT;
    memcpy(bleUuid.uuid128, svc1UUID, 16);
    ble_gatt_server_add_service(BLE_SERVICE_TYPE_PRIMARY, &bleUuid, &svcHandle, nullptr);

    memset(&char_init, 0x00, sizeof(hal_ble_char_init_t));
    memcpy(bleUuid.uuid128, char1UUID, 16);
    char_init.properties = BLE_SIG_CHAR_PROP_READ|BLE_SIG_CHAR_PROP_WRITE;
    char_init.service_handle = svcHandle;
    char_init.uuid = bleUuid;
    ble_gatt_server_add_characteristic(&char_init, &localCharacteristic1, nullptr);

    memcpy(bleUuid.uuid128, svc2UUID, 16);
    ble_gatt_server_add_service(BLE_SERVICE_TYPE_PRIMARY, &bleUuid, &svcHandle, nullptr);

    memset(&char_init, 0x00, sizeof(hal_ble_char_init_t));
    memcpy(bleUuid.uuid128, char2UUID, 16);
    char_init.properties = BLE_SIG_CHAR_PROP_NOTIFY;
    char_init.service_handle = svcHandle;
    char_init.uuid = bleUuid;
    ble_gatt_server_add_characteristic(&char_init, &localCharacteristic2, nullptr);

    bleUuid.type = BLE_UUID_TYPE_16BIT;
    bleUuid.uuid16 = 0x1234;
    ble_gatt_server_add_service(BLE_SERVICE_TYPE_PRIMARY, &bleUuid, &svcHandle, nullptr);

    memset(&char_init, 0x00, sizeof(hal_ble_char_init_t));
    bleUuid.uuid16 = 0x5678;
    char_init.properties = BLE_SIG_CHAR_PROP_WRITE_WO_RESP;
    char_init.service_handle = svcHandle;
    char_init.uuid = bleUuid;
    char_init.description = "hello";
    ble_gatt_server_add_characteristic(&char_init, &localCharacteristic3, nullptr);

    uint8_t data[20] = {0x11};
    ret = ble_gatt_server_set_characteristic_value(localCharacteristic1.value_handle, data, 5, nullptr);
    assertEqual(ret, 5);

    advParams.type          = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT;
    advParams.filter_policy = BLE_ADV_FP_ANY;
    advParams.interval      = 100;
    advParams.timeout       = 0; // Advertising forever
    ret = ble_gap_set_advertising_parameters(&advParams, nullptr);
    assertEqual(ret, 0);

    ret = ble_gap_start_advertising(nullptr);
    assertEqual(ret, 0);

    // Use a BLE central device to connect to it and check the BLE services and characteristics
    uint16_t timeout = 0;
    while(!ble_gap_is_connected(nullptr)) {
        delay(500);
        timeout += 500;
        if(timeout >= 15000) {
            break;
        }
    }

    ret = ble_gap_is_connected(nullptr);
    assertEqual(ret, true);
}

test(11_BlePeripheralCanDisconnectInitially_NeedToConnectByCentralSide) {
    int ret;

    // Wait until the previous test case finished.
    // This disconnection should be initiated by the central side.
    // Once disconnected, it should restart advertising automatically.
    while(ble_gap_is_connected(nullptr));

    // Use a BLE central device to connect to it and wait for 2 seconds without doing anything.
    uint16_t timeout = 0;
    while(!ble_gap_is_connected(nullptr)) {
        delay(500);
        timeout += 500;
        if(timeout >= 15000) {
            break;
        }
    }

    ret = ble_gap_is_connected(nullptr);
    assertEqual(ret, true);

    delay(5000);
    ret = ble_gap_disconnect(connHandle, nullptr);
    assertEqual(ret, 0);

    // It should restart advertising
    ret = ble_gap_is_connected(nullptr);
    assertEqual(ret, false);
    ret = ble_gap_is_advertising();
    assertEqual(ret, true);
}

test(12_BleSetScanParametersShouldBeValid) {
    int ret;
    hal_ble_scan_params_t scanParams;

    scanParams.active = true;
    scanParams.filter_policy = BLE_SCAN_FP_ACCEPT_ALL;
    scanParams.interval = 1;
    scanParams.window   = 1;
    scanParams.timeout  = 0; // 0 for forever unless stop initially
    ret = ble_gap_set_scan_parameters(&scanParams, NULL);
    assertNotEqual(ret, 0);

    scanParams.interval = 1600; // 1 seconds
    ret = ble_gap_set_scan_parameters(&scanParams, NULL);
    assertNotEqual(ret, 0);

    scanParams.window = 1601;
    ret = ble_gap_set_scan_parameters(&scanParams, NULL);
    assertNotEqual(ret, 0);

    scanParams.window = 100;
    ret = ble_gap_set_scan_parameters(&scanParams, NULL);
    assertEqual(ret, 0);
}

test(13_BleScanTimeoutAsExpected) {
    int ret;
    hal_ble_scan_params_t scanParams;

    scanParams.active = true;
    scanParams.filter_policy = BLE_SCAN_FP_ACCEPT_ALL;
    scanParams.interval = 1600;
    scanParams.window   = 100;
    scanParams.timeout  = 200; // Scan for 2 seconds
    ret = ble_gap_set_scan_parameters(&scanParams, NULL);
    assertEqual(ret, 0);

    // It blocks until the scanning timeout.
    ret = ble_gap_start_scan(nullptr, nullptr, nullptr);
    assertEqual(ret, 0);

    ret = ble_gap_is_scanning();
    assertEqual(ret, false);
}

test(14_BleConnectToPeerSuccessfully) {
    int ret;
    hal_ble_scan_params_t scanParams;

    scanParams.active = true;
    scanParams.filter_policy = BLE_SCAN_FP_ACCEPT_ALL;
    scanParams.interval = 1600;
    scanParams.window   = 100;
    scanParams.timeout  = 2000; // Scan for 10 seconds
    ret = ble_gap_set_scan_parameters(&scanParams, NULL);
    assertEqual(ret, 0);

    ret = ble_gap_start_scan(ble_on_scan_result, nullptr, nullptr);
    assertEqual(ret, 0);
    assertEqual(auxPeripheralFound, true);

    ret = ble_gap_connect(&auxPeripheralAddr, nullptr);
    assertEqual(ret, 0);

    delay(2000);
    ret = ble_gap_is_connected(nullptr);
    assertEqual(ret, true);
}

test(15_BleDiscoverServicesSuccessfully) {
    int ret;

    ret = ble_gatt_client_discover_all_services(connHandle, ble_on_services_discovered, nullptr, NULL);
    assertEqual(ret, 0);

    assertEqual(svc1Discovered, true);
    assertEqual(svc2Discovered, true);
    assertEqual(svc3Discovered, true);
}

test(16_BleDiscoverCharacteristicsSuccessfully) {
    int ret;

    ret = ble_gatt_client_discover_characteristics(connHandle, &service1, ble_on_characteristics_discovered, nullptr, NULL);
    assertEqual(ret, 0);
    assertEqual(char1Discovered, true);

    ret = ble_gatt_client_discover_characteristics(connHandle, &service2, ble_on_characteristics_discovered, nullptr, NULL);
    assertEqual(ret, 0);
    assertEqual(char2Discovered, true);

    ret = ble_gatt_client_discover_characteristics(connHandle, &service3, ble_on_characteristics_discovered, nullptr, NULL);
    assertEqual(ret, 0);
    assertEqual(char3Discovered, true);
}

test(17_BleReadWriteDataSuccessfully) {
    int ret;
    uint8_t readData[20];

    ret = ble_gatt_client_read(connHandle, char1.value_handle, readData, sizeof(readData), NULL);
    assertEqual(ret, 5); // Initial value length of this characteristic of Auxiliary peripheral.

    uint8_t writeData[] = {0xaa, 0xbb, 0x5a, 0xa5};
    ret = ble_gatt_client_write_with_response(connHandle, char1.value_handle, writeData, sizeof(writeData), NULL);
    assertEqual(ret, 4);

    ret = ble_gatt_client_read(connHandle, char1.value_handle, readData, sizeof(readData), NULL);
    assertEqual(ret, 4);
    ret = memcmp(readData, writeData, sizeof(writeData));
    assertEqual(ret, 0);

    ret = ble_gatt_client_write_without_response(connHandle, char3.value_handle, writeData, sizeof(writeData), NULL);
    assertEqual(ret, 4);
}

test(18_BleConfigCccdSuccessfully) {
    int ret;

    ret = ble_gatt_client_configure_cccd(connHandle, char2.cccd_handle, BLE_SIG_CCCD_VAL_NOTIFICATION, NULL);
    assertEqual(ret, 0);

    delay(5000);
    assertEqual(notifiedData, true);

    ret = ble_gatt_client_configure_cccd(connHandle, char2.cccd_handle, BLE_SIG_CCCD_VAL_DISABLED, NULL);
    assertEqual(ret, 0);
}

test(19_BleDisconnectSuccessfully) {
    int ret;

    ret = ble_gap_disconnect(connHandle, NULL);
    assertEqual(ret, 0);
    ret = ble_gap_is_connected(nullptr);
    assertEqual(ret, false);
}

