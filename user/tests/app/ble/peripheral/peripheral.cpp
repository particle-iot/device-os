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

hal_ble_char_handles_t char1Handles; // Read and Write
hal_ble_char_handles_t char2Handles; // Notify
hal_ble_char_handles_t char3Handles; // Write without response

const char* addrType[4] = {
    "Public",
    "Random Static",
    "Random Private Resolvable",
    "Random Static Non-resolvable"
};

bool notifyEnabled = false;

uint8_t svc1UUID[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x00,0x00,0x0e,0x0f};
uint8_t char1UUID[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x01,0x00,0x0e,0x0f};
uint8_t svc2UUID[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x00,0x00,0x0e,0x10};
uint8_t char2UUID[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x01,0x00,0x0e,0x10};


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

static int adStructEncode(uint8_t adsType, uint8_t* data, uint16_t len, bool sr, uint8_t* advData, uint16_t* advDataLen) {
    uint16_t  offset;
    uint16_t  adsLen;
    bool      adsExist = false;

    if (sr && adsType == BLE_SIG_AD_TYPE_FLAGS) {
        // The scan response data should not contain the AD Flags AD structure.
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if (locateAdStructure(adsType, advData, *advDataLen, &offset, &adsLen) == SYSTEM_ERROR_NONE) {
        adsExist = true;
    }

    if (data == NULL) {
        // The advertising data must contain the AD Flags AD structure.
        if (adsType == BLE_SIG_AD_TYPE_FLAGS) {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }

        // Remove the AD structure from advertising data.
        if (adsExist == true) {
            uint16_t moveLen = *advDataLen - offset - adsLen;
            memcpy(&advData[offset], &advData[offset + adsLen], moveLen);
            *advDataLen = *advDataLen - adsLen;
        }

        return SYSTEM_ERROR_NONE;
    }
    else if (adsExist) {
        // Update the existing AD structure.
        uint16_t staLen = *advDataLen - adsLen;
        if ((staLen + len + 2) <= BLE_MAX_ADV_DATA_LEN) {
            // Firstly, move the last consistent block.
            uint16_t moveLen = *advDataLen - offset - adsLen;
            memmove(&advData[offset + len], &advData[offset + adsLen], moveLen);

            // Secondly, Update the AD structure.
            // The Length field is the total length of Type field and Data field.
            advData[offset + 1] = len + 1;
            memcpy(&advData[offset + 2], data, len);

            // An AD structure is composed of one byte length field, one byte Type field and Data field.
            *advDataLen = staLen + len + 2;
            return SYSTEM_ERROR_NONE;
        }
        else {
            return SYSTEM_ERROR_LIMIT_EXCEEDED;
        }
    }
    else {
        // Append the AD structure at the and of advertising data.
        if ((*advDataLen + len + 2) <= BLE_MAX_ADV_DATA_LEN) {
            advData[(*advDataLen)++] = len + 1;
            advData[(*advDataLen)++] = adsType;
            memcpy(&advData[*advDataLen], data, len);
            *advDataLen += len;
            return SYSTEM_ERROR_NONE;
        }
        else {
            return SYSTEM_ERROR_LIMIT_EXCEEDED;
        }
    }
}

static int encodeAdvertisingData(uint8_t ads_type, uint8_t* data, uint16_t len, uint8_t* advData, uint16_t* advDataLen) {
    int ret = adStructEncode(ads_type, data, len, false, advData, advDataLen);
    if (ret == SYSTEM_ERROR_NONE) {
        ret = ble_gap_set_advertising_data(advData, *advDataLen, NULL);
    }

    return ret;
}

static int encodeScanResponseData(uint8_t ads_type, uint8_t* data, uint16_t len, uint8_t* advData, uint16_t* advDataLen) {
    int ret = adStructEncode(ads_type, data, len, true, advData, advDataLen);
    if (ret == SYSTEM_ERROR_NONE) {
        ret = ble_gap_set_scan_response_data(advData, *advDataLen, NULL);
    }

    return ret;
}

static void ble_on_adv_stopped(hal_ble_gap_on_adv_stopped_evt_t* event) {
    LOG(TRACE, "BLE advertising stopped");
}

static void ble_on_connected(hal_ble_gap_on_connected_evt_t* event) {
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

static void ble_on_disconnected(hal_ble_gap_on_disconnected_evt_t* event) {
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

static void ble_on_connection_parameters_updated(hal_ble_gap_on_conn_params_evt_t* event) {
    LOG(TRACE, "BLE connection parameters updated, connection handle: 0x%04X.", event->conn_handle);
    LOG(TRACE, "Interval: %.2fms, Latency: %d, Timeout: %dms", event->conn_interval*1.25, event->slave_latency, event->conn_sup_timeout*10);
}

static void ble_on_data_received(hal_ble_gatt_on_data_evt_t* event) {
    LOG(TRACE, "BLE data received, connection handle: 0x%04X.", event->conn_handle);

    if (event->attr_handle == char1Handles.value_handle) {
        LOG(TRACE, "Write BLE characteristic 1 value:");
    }
    else if (event->attr_handle == char2Handles.cccd_handle) {
        LOG(TRACE, "Configure BLE characteristic 2 CCCD:");
        if (event->data[0] == 0x01) {
            notifyEnabled = true;
        }
        else if (event->data[0] == 0x00) {
            notifyEnabled = false;
        }
    }
    else if (event->attr_handle == char3Handles.value_handle) {
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

static void ble_on_events(hal_ble_evts_t* event, void* context) {
    if (event->type == BLE_EVT_ADV_STOPPED) {
        ble_on_adv_stopped(&event->params.adv_stopped);
    }
    else if (event->type == BLE_EVT_CONNECTED) {
        ble_on_connected(&event->params.connected);
    }
    else if (event->type == BLE_EVT_DISCONNECTED) {
        ble_on_disconnected(&event->params.disconnected);
    }
    else if (event->type == BLE_EVT_CONN_PARAMS_UPDATED) {
        ble_on_connection_parameters_updated(&event->params.conn_params_updated);
    }
    else if (event->type == BLE_EVT_DATA_WRITTEN) {
        ble_on_data_received(&event->params.data_rec);
    }
}

/* This function is called once at start up ----------------------------------*/
void setup()
{
    uint8_t devName[] = "Xenon BLE Sample";
    uint8_t advData[31] = {
        0x02,
        BLE_SIG_AD_TYPE_FLAGS,
        BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE
    };
    uint16_t advDataLen = 3;
    uint8_t srData[31];
    uint16_t srDataLen = 0;

    ble_stack_init(NULL);

    ble_gap_set_device_name(devName, sizeof(devName));

    hal_ble_conn_params_t connParams;
    connParams.min_conn_interval = 100;
    connParams.max_conn_interval = 400;
    connParams.slave_latency     = 0;
    connParams.conn_sup_timeout  = 400;
    ble_gap_set_ppcp(&connParams, NULL);

    hal_ble_adv_params_t advParams;
    advParams.type          = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT;
    advParams.filter_policy = BLE_ADV_FP_ANY;
    advParams.interval      = 100;
    advParams.timeout       = 0;
    advParams.inc_tx_power  = false;
    ble_gap_set_advertising_parameters(&advParams, NULL);

    encodeAdvertisingData(BLE_SIG_AD_TYPE_COMPLETE_LOCAL_NAME, devName, sizeof(devName), advData, &advDataLen);
    uint8_t uuid[2] = {0xab, 0xcd};
    encodeAdvertisingData(BLE_SIG_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE, uuid, sizeof(uuid), advData, &advDataLen);
    uint8_t mfgData[] = {0x12, 0x34};
    encodeScanResponseData(BLE_SIG_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, mfgData, sizeof(mfgData), srData, &srDataLen);

    uint16_t svcHandle;
    hal_ble_uuid_t bleUuid;
    hal_ble_char_init_t char_init;

    bleUuid.type = BLE_UUID_TYPE_128BIT;
    memcpy(bleUuid.uuid128, svc1UUID, 16);
    ble_gatt_server_add_service(BLE_SERVICE_TYPE_PRIMARY, &bleUuid, &svcHandle, NULL);

    memset(&char_init, 0x00, sizeof(hal_ble_char_init_t));
    memcpy(bleUuid.uuid128, char1UUID, 16);
    char_init.properties = BLE_SIG_CHAR_PROP_READ|BLE_SIG_CHAR_PROP_WRITE;
    char_init.service_handle = svcHandle;
    char_init.uuid = bleUuid;
    ble_gatt_server_add_characteristic(&char_init, &char1Handles, NULL);

    memcpy(bleUuid.uuid128, svc2UUID, 16);
    ble_gatt_server_add_service(BLE_SERVICE_TYPE_PRIMARY, &bleUuid, &svcHandle, NULL);

    memset(&char_init, 0x00, sizeof(hal_ble_char_init_t));
    memcpy(bleUuid.uuid128, char2UUID, 16);
    char_init.properties = BLE_SIG_CHAR_PROP_NOTIFY;
    char_init.service_handle = svcHandle;
    char_init.uuid = bleUuid;
    ble_gatt_server_add_characteristic(&char_init, &char2Handles, NULL);

    bleUuid.type = BLE_UUID_TYPE_16BIT;
    bleUuid.uuid16 = 0x1234;
    ble_gatt_server_add_service(BLE_SERVICE_TYPE_PRIMARY, &bleUuid, &svcHandle, NULL);

    memset(&char_init, 0x00, sizeof(hal_ble_char_init_t));
    bleUuid.uuid16 = 0x5678;
    char_init.properties = BLE_SIG_CHAR_PROP_WRITE_WO_RESP;
    char_init.service_handle = svcHandle;
    char_init.uuid = bleUuid;
    char_init.description = "hello";
    ble_gatt_server_add_characteristic(&char_init, &char3Handles, NULL);

    uint8_t data[20] = {0x11};
    ble_gatt_server_set_characteristic_value(char1Handles.value_handle, data, 5, NULL);

    ble_set_callback_on_events(ble_on_events, NULL);

    ble_gap_start_advertising(NULL);
}

/* This function loops forever --------------------------------------------*/
void loop()
{
    static uint16_t cnt = 0;

    if (ble_gap_is_connected() && notifyEnabled) {
        ble_gatt_server_notify_characteristic_value(char2Handles.value_handle, (uint8_t *)&cnt, 2, NULL);
        cnt++;
    }
    else {
        cnt = 0;
    }

    delay(3000);
}
