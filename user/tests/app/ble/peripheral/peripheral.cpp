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
#include "ble_hal_api.h"

SYSTEM_MODE(MANUAL);

Serial1LogHandler log(115200, LOG_LEVEL_ALL);

hal_ble_characteristic_t bleChar1; // Read and Write
hal_ble_characteristic_t bleChar2; // Notify
hal_ble_characteristic_t bleChar3; // Write without response

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

static int encodeAdvertisingData(uint8_t ads_type, uint8_t* data, uint16_t len, uint8_t* advData, uint16_t*advDataLen) {
    int ret = adStructEncode(ads_type, data, len, false, advData, advDataLen);
    if (ret == SYSTEM_ERROR_NONE) {
        ret = ble_gap_set_advertising_data(advData, *advDataLen);
    }

    return ret;
}

static int encodeScanResponseData(uint8_t ads_type, uint8_t* data, uint16_t len, uint8_t* advData, uint16_t*advDataLen) {
    int ret = adStructEncode(ads_type, data, len, true, advData, advDataLen);
    if (ret == SYSTEM_ERROR_NONE) {
        ret = ble_gap_set_scan_response_data(advData, *advDataLen);
    }

    return ret;
}

static void handleBleEvent(hal_ble_event_t *event)
{
    if (event->evt_type == BLE_EVT_TYPE_CONNECTION) {
        if (event->conn_event.evt_id == BLE_CONN_EVT_ID_CONNECTED) {
            LOG(TRACE, "BLE connected, handle: %d", event->conn_event.conn_handle);
        }
        else {
            LOG(TRACE, "BLE disconnected, handle: %d", event->conn_event.conn_handle);
        }
    }
    else if (event->evt_type == BLE_EVT_TYPE_DATA && event->data_event.evt_id == BLE_DATA_EVT_ID_WRITE) {
        if (event->data_event.attr_handle == bleChar1.value_handle) {
            LOG(TRACE, "BLE characteristic 1 received data.");
        }
        else if (event->data_event.attr_handle == bleChar3.value_handle) {
            LOG(TRACE, "BLE characteristic 3 received data.");
        }
        else {
            LOG(TRACE, "BLE received data, attribute handle: %d.", event->data_event.attr_handle);
        }

        for (uint8_t i = 0; i < event->data_event.data_len; i++) {
            Serial1.printf("0x%02X,", event->data_event.data[i]);
        }
        Serial1.print("\r\n");
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

    ble_stack_init(NULL);

    ble_gap_set_device_name(devName, sizeof(devName));

    hal_ble_connection_parameters_t connParams;
    connParams.min_conn_interval = 100;
    connParams.max_conn_interval = 400;
    connParams.slave_latency     = 0;
    connParams.conn_sup_timeout  = 400;
    ble_gap_set_ppcp(&connParams);

    hal_ble_advertising_parameters_t advParams;
    advParams.type          = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT;
    advParams.filter_policy = BLE_ADV_FP_ANY;
    advParams.interval      = 100;
    advParams.timeout       = 0;
    advParams.inc_tx_power  = false;
    ble_gap_set_advertising_parameters(&advParams);

    encodeAdvertisingData(BLE_SIG_AD_TYPE_COMPLETE_LOCAL_NAME, devName, sizeof(devName), advData, &advDataLen);
    uint8_t uuid[2] = {0xab, 0xcd};
    encodeAdvertisingData(BLE_SIG_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE, uuid, sizeof(uuid), advData, &advDataLen);
    uint8_t mfgData[10] = {0x12, 0x34};
    encodeScanResponseData(BLE_SIG_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, mfgData, sizeof(mfgData), advData, &advDataLen);

    uint16_t svcHandle;

    uint8_t svcUUID1[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x00,0x00,0x0e,0x0f};
    uint8_t charUUID1[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x01,0x00,0x0e,0x0f};
    ble_gatt_server_add_service_uuid128(BLE_SERVICE_TYPE_PRIMARY, svcUUID1, &svcHandle);
    ble_gatt_server_add_characteristic_uuid128(svcHandle, charUUID1, BLE_SIG_CHAR_PROP_READ|BLE_SIG_CHAR_PROP_WRITE, NULL, &bleChar1);

    uint8_t svcUUID2[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x00,0x00,0x0e,0x10};
    uint8_t charUUID2[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x01,0x00,0x0e,0x10};
    ble_gatt_server_add_service_uuid128(BLE_SERVICE_TYPE_PRIMARY, svcUUID2, &svcHandle);
    ble_gatt_server_add_characteristic_uuid128(svcHandle, charUUID2, BLE_SIG_CHAR_PROP_NOTIFY, NULL, &bleChar2);

    ble_gatt_server_add_service_uuid16(BLE_SERVICE_TYPE_PRIMARY, 0x1234, &svcHandle);
    ble_gatt_server_add_characteristic_uuid16(svcHandle, 0x5678, BLE_SIG_CHAR_PROP_WRITE_WO_RESP, "hello", &bleChar3);

    uint8_t data[20] = {0x11};
    ble_gatt_server_set_characteristic_value(bleChar1.value_handle, data, 5);

    ble_register_callback(handleBleEvent);

    ble_gap_start_advertising();
}

/* This function loops forever --------------------------------------------*/
void loop()
{
    static uint16_t cnt = 0;

    if (ble_gap_is_connected()) {
        ble_gatt_server_notify_characteristic_value(bleChar2.value_handle, (uint8_t *)&cnt, 2);
        cnt++;
    }
    else {
        cnt = 0;
    }

    delay(3000);
}
