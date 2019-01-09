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

hal_ble_char_t bleChar1;
hal_ble_char_t bleChar2;
hal_ble_char_t bleChar3;

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
    advParams.duration      = 1800;
    advParams.inc_tx_power  = false;
    ble_set_advertising_params(&advParams);

    ble_set_adv_data_snippet(BLE_SIG_AD_TYPE_COMPLETE_LOCAL_NAME, devName, sizeof(devName));
    ble_refresh_adv_data();

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

    ble_start_advertising();
}

/* This function loops forever --------------------------------------------*/
void loop()
{
    static uint16_t cnt = 0;
    uint8_t data[20] = {0x11};
    uint16_t len = 20;

    if (ble_connected()) {
        ble_get_characteristic_value(&bleChar1, data, &len);
        if (len > 0) {
            for (uint8_t i = 0; i < len; i++) {
                Serial1.printf("0x%02x, ", data[i]);
            }
            Serial1.print("\r\n");
        }

//        ble_publish(&bleChar2, (uint8_t *)&cnt, 2);
//        cnt++;
    }

    delay(3000);
}
