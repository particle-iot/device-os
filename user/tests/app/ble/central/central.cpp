/*
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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

/* Includes ------------------------------------------------------------------*/
#include "application.h"
#include "ble_hal.h"

SYSTEM_MODE(MANUAL);

Serial1LogHandler log(115200, LOG_LEVEL_ALL);

uint8_t  svc1Uuid[]  = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x00,0x00,0x0e,0x0f};
uint8_t  char1Uuid[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x01,0x00,0x0e,0x0f};

uint8_t  svc2Uuid[]  = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x00,0x00,0x0e,0x10};
uint8_t  char2Uuid[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x01,0x00,0x0e,0x10};

uint16_t svc3Uuid  = 0x1234;
uint16_t char3Uuid = 0x5678;

hal_ble_service_t service1, service2, service3;
hal_ble_characteristic_t char1, char2, char3;
bool char1Discovered, char2Discovered, char3Discovered;
uint16_t connHandle = BLE_INVALID_CONN_HANDLE;

const char *addrType[4] = {
    "Public",
    "Random Static",
    "Random Private Resolvable",
    "Random Static Non-resolvable"
};

static int locateAdStructure(uint8_t adsType, const uint8_t* data, uint16_t len, uint16_t* offset, uint16_t* adsLen) {
    // A valid AD structure is composed of Length field, Type field and Data field.
    // Each field should be filled with at least one byte.
    for (uint16_t i = 0; (i + 3) <= len; i = i) {
        *adsLen = data[i];

        uint8_t type = data[i + 1];
        if (type == adsType) {
            // The value of adsLen doesn't include the length field of an AD structure.
            if ((i + *adsLen + 1) <= len) {
                *offset = i;
                *adsLen += 1;
                return SYSTEM_ERROR_NONE;
            }
            else {
                return SYSTEM_ERROR_INTERNAL;
            }
        }
        else {
            // Navigate to the next AD structure.
            i += (*adsLen + 1);
        }
    }

    return SYSTEM_ERROR_NOT_FOUND;
}

static int decodeAdvertisingData(uint8_t ads_type, const uint8_t* adv_data, uint16_t adv_data_len, uint8_t* data, uint16_t* len) {
    // An AD structure must consist of 1 byte length field, 1 byte type field and at least 1 byte data field
    if (adv_data == NULL || adv_data_len < 3) {
        *len = 0;
        return SYSTEM_ERROR_NOT_FOUND;
    }

    uint16_t dataOffset, dataLen;
    if (locateAdStructure(ads_type, adv_data, adv_data_len, &dataOffset, &dataLen) == SYSTEM_ERROR_NONE) {
        if (len != NULL) {
            dataLen = dataLen - 2;
            if (data != NULL && *len > 0) {
                // Only copy the data field of the found AD structure.
                *len = MIN(*len, dataLen);
                memcpy(data, &adv_data[dataOffset+2], *len);
            }
            *len = dataLen;
        }

        return SYSTEM_ERROR_NONE;
    }

    *len = 0;
    return SYSTEM_ERROR_NOT_FOUND;
}

static void ble_on_scan_result(hal_ble_gap_on_scan_result_evt_t *event) {
    uint8_t  devName[20];
    uint16_t devNameLen = sizeof(devName);
    decodeAdvertisingData(BLE_SIG_AD_TYPE_COMPLETE_LOCAL_NAME, event->data, event->data_len, devName, &devNameLen);

    if (devNameLen != 0 && devNameLen < sizeof(devName)) {
        devName[devNameLen] = '\0';
        if (!strcmp((const char*)devName, "Xenon BLE Sample")) {
            LOG(TRACE, "Target device found. Start connecting...");
            ble_gap_connect(&event->peer_addr);
        }
    }
}

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

    ble_gatt_client_discover_all_services(event->conn_handle);

    connHandle = event->conn_handle;
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

    connHandle = BLE_INVALID_CONN_HANDLE;
}

static void ble_on_services_discovered(hal_ble_gatt_client_on_services_discovered_evt_t *event) {
    hal_ble_service_t* service;
    for (uint8_t i = 0; i < event->count; i++) {
        service = NULL;
        if (event->services[i].uuid.type == BLE_UUID_TYPE_16BIT) {
            if (event->services[i].uuid.uuid16 == svc3Uuid) {
                service = &service3;
                LOG(TRACE, "BLE Service3 found.");
            }
        }
        else if (event->services[i].uuid.type == BLE_UUID_TYPE_128BIT) {
            if (!memcmp(svc1Uuid, event->services[i].uuid.uuid128, BLE_SIG_UUID_128BIT_LEN)) {
                service = &service1;
                LOG(TRACE, "BLE Service1 found.");
            }
            else if (!memcmp(svc2Uuid, event->services[i].uuid.uuid128, BLE_SIG_UUID_128BIT_LEN)) {
                service = &service2;
                LOG(TRACE, "BLE Service2 found.");
            }
        }
        if (service != NULL) {
            memcpy(service, &event->services[i], sizeof(hal_ble_service_t));
        }
    }

    if (!char1Discovered) {
        ble_gatt_client_discover_characteristics(event->conn_handle, service1.start_handle, service1.end_handle);
    }
}

static void ble_on_characteristics_discovered(hal_ble_gatt_client_on_characteristics_discovered_evt_t *event) {
    hal_ble_characteristic_t* characteristic = NULL;
    for (uint8_t i = 0; i < event->count; i++) {
        if (event->characteristics[i].uuid.type == BLE_UUID_TYPE_16BIT) {
            if (event->characteristics[i].uuid.uuid16 == char3Uuid) {
                characteristic = &char3;
                char3Discovered = true;
                LOG(TRACE, "BLE Characteristic3 found.");
            }
        }
        else if (event->characteristics[i].uuid.type == BLE_UUID_TYPE_128BIT) {
            if (!memcmp(char1Uuid, event->characteristics[i].uuid.uuid128, BLE_SIG_UUID_128BIT_LEN)) {
                characteristic = &char1;
                char1Discovered = true;
                LOG(TRACE, "BLE Characteristic1 found.");
            }
            else if (!memcmp(char2Uuid, event->characteristics[i].uuid.uuid128, BLE_SIG_UUID_128BIT_LEN)) {
                characteristic = &char2;
                char2Discovered = true;
                LOG(TRACE, "BLE Characteristic2 found.");
            }
        }
        if (characteristic != NULL) {
            memcpy(characteristic, &event->characteristics[i], sizeof(hal_ble_characteristic_t));
        }
    }

    if (!char2Discovered) {
        ble_gatt_client_discover_characteristics(event->conn_handle, service2.start_handle, service2.end_handle);
    }
    else if (!char3Discovered) {
        ble_gatt_client_discover_characteristics(event->conn_handle, service3.start_handle, service3.end_handle);
    }
    else {
        ble_gatt_client_discover_descriptors(event->conn_handle, char2.value_handle, char3.decl_handle);
    }
}

static void ble_on_descriptors_discovered(hal_ble_gatt_client_on_descriptors_discovered_evt_t *event) {
    for (uint8_t i = 0; i < event->count; i++) {
        if (event->descriptors[i].uuid.uuid16 == BLE_SIG_UUID_CLIENT_CHAR_CONFIG_DESC) {
            char2.cccd_handle = event->descriptors[i].handle;
            LOG(TRACE, "BLE Characteristic2 CCCD found.");

            ble_gatt_client_configure_cccd(event->conn_handle, char2.cccd_handle, true);
        }
    }
}

static void ble_on_data_received(hal_ble_gatt_client_on_data_received_evt_t *event) {
    LOG(TRACE, "BLE data received, connection handle: 0x%04X.", event->conn_handle);

    if (event->peer_attr_handle == char1.value_handle) {
        LOG(TRACE, "Read BLE characteristic 1 value:");
    }
    else if (event->peer_attr_handle == char2.value_handle) {
        LOG(TRACE, "Notified BLE characteristic 2 value:");
    }
    else {
        LOG(TRACE, "BLE received data, attribute handle: %d.", event->peer_attr_handle);
    }

    for (uint8_t i = 0; i < event->data_len; i++) {
        Serial1.printf("0x%02X,", event->data[i]);
    }
    Serial1.print("\r\n");
}

/* This function is called once at start up ----------------------------------*/
void setup()
{
    uint8_t devName[] = "Xenon BLE Central";

    ble_stack_init(NULL);

    ble_gap_set_device_name(devName, sizeof(devName));

    hal_ble_scan_parameters_t scanParams;
    scanParams.active = true;
    scanParams.filter_policy = BLE_SCAN_FP_ACCEPT_ALL;
    scanParams.interval = 3200; // 2 seconds
    scanParams.window   = 100;
    scanParams.timeout  = 2000; // 0 for forever unless stop initially
    ble_gap_set_scan_parameters(&scanParams);

    ble_gap_set_callback_on_scan_result(ble_on_scan_result);
    ble_gap_set_callback_on_connected(ble_on_connected);
    ble_gap_set_callback_on_disconnected(ble_on_disconnected);
    ble_gatt_client_set_callback_on_services_discovered(ble_on_services_discovered);
    ble_gatt_client_set_callback_on_characteristics_discovered(ble_on_characteristics_discovered);
    ble_gatt_client_set_callback_on_descriptors_discovered(ble_on_descriptors_discovered);
    ble_gatt_client_set_callback_on_data_received(ble_on_data_received);

    ble_gap_start_scan();
}

/* This function loops forever --------------------------------------------*/
void loop()
{
    if (ble_gap_is_connected() && char3Discovered) {
        delay(2000);
        ble_gatt_client_read(connHandle, char1.value_handle);
    }
}
