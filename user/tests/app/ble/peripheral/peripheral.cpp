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

hal_ble_char_t bleChar1; // Read and Write
hal_ble_char_t bleChar2; // Notify
hal_ble_char_t bleChar3; // Write without response

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
    else if (event->evt_type == BLE_EVT_TYPE_DATA) {
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

    hal_ble_init(BLE_ROLE_PERIPHERAL, NULL);

    ble_set_device_name(devName, sizeof(devName));

    hal_ble_conn_params_t connParams;
    connParams.min_conn_interval = 100;
    connParams.max_conn_interval = 400;
    connParams.slave_latency     = 0;
    connParams.conn_sup_timeout  = 400;
    ble_set_ppcp(&connParams);

    hal_ble_adv_params_t advParams;
    advParams.type          = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT;
    advParams.filter_policy = BLE_GAP_ADV_FP_ANY;
    advParams.interval      = 100;
    advParams.duration      = 0;
    advParams.inc_tx_power  = false;
    ble_set_advertising_params(&advParams);

    ble_config_adv_data(BLE_SIG_AD_TYPE_COMPLETE_LOCAL_NAME, devName, sizeof(devName));
    uint8_t uuid[2] = {0xab, 0xcd};
    ble_config_adv_data(BLE_SIG_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE, uuid, sizeof(uuid));
    uint8_t mfgData[10] = {0x12, 0x34};
    ble_config_scan_resp_data(BLE_SIG_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, mfgData, sizeof(mfgData));

    uint16_t svcHandle;

    uint8_t svcUUID1[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x00,0x00,0x0e,0x0f};
    uint8_t charUUID1[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x01,0x00,0x0e,0x0f};
    ble_add_service_uuid128(BLE_SERVICE_TYPE_PRIMARY, svcUUID1, &svcHandle);
    ble_add_char_uuid128(svcHandle, charUUID1, BLE_SIG_CHAR_PROP_READ|BLE_SIG_CHAR_PROP_WRITE, NULL, &bleChar1);

    uint8_t svcUUID2[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x00,0x00,0x0e,0x10};
    uint8_t charUUID2[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x01,0x00,0x0e,0x10};
    ble_add_service_uuid128(BLE_SERVICE_TYPE_PRIMARY, svcUUID2, &svcHandle);
    ble_add_char_uuid128(svcHandle, charUUID2, BLE_SIG_CHAR_PROP_NOTIFY, NULL, &bleChar2);

    ble_add_service_uuid16(BLE_SERVICE_TYPE_PRIMARY, 0x1234, &svcHandle);
    ble_add_char_uuid16(svcHandle, 0x5678, BLE_SIG_CHAR_PROP_WRITE_WO_RESP, "hello", &bleChar3);

    uint8_t data[20] = {0x11};
    ble_set_characteristic_value(&bleChar1, data, 5);

    ble_register_callback(handleBleEvent);

    ble_start_advertising();
}

/* This function loops forever --------------------------------------------*/
void loop()
{
    static uint16_t cnt = 0;

    if (ble_is_connected()) {
        ble_publish(&bleChar2, (uint8_t *)&cnt, 2);
        cnt++;

        if (cnt == 10) {
            hal_ble_disconnect(0);
        }
    }
    else {
        cnt = 0;
    }

    delay(3000);
}
