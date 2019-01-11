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

const char *addrType[4] = {
    "Public",
    "Random Static",
    "Random Private Resolvable",
    "Random Static Non-resolvable"
};

static void handleBleEvent(hal_ble_event_t *event)
{
    if (event->evt_type == BLE_EVT_TYPE_SCAN_RESULT) {
        if (event->scan_result_event.data_type == BLE_SCAN_RESULT_EVT_DATA_TYPE_ADV) {
            LOG(TRACE, "BLE Advertising data");
        }
        else {
            LOG(TRACE, "BLE Scan Sesponse data");
        }
        Serial1.printf(" RSSI         : %i dBm\r\n", event->scan_result_event.rssi);

        if (event->scan_result_event.peer_addr.addr_type <= 3) {
            Serial1.printf(" Address type : %s\r\n", addrType[event->scan_result_event.peer_addr.addr_type]);
        }
        else {
            Serial1.printf(" Address type : Anonymous\r\n");
        }
        Serial1.printf(" Address      : %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                event->scan_result_event.peer_addr.addr[0],
                event->scan_result_event.peer_addr.addr[1],
                event->scan_result_event.peer_addr.addr[2],
                event->scan_result_event.peer_addr.addr[3],
                event->scan_result_event.peer_addr.addr[4],
                event->scan_result_event.peer_addr.addr[5]);

        Serial1.print(" Payload      : ");
        for (uint8_t i = 0; i < event->scan_result_event.data_len; i++) {
            Serial1.printf("%02X ", event->scan_result_event.data[i]);
        }
        Serial1.print("\r\n");

        uint8_t  mfgData[20];
        uint16_t mfgDataLen = sizeof(mfgData);
        ble_adv_data_decode(BLE_SIG_AD_TYPE_MANUFACTURER_SPECIFIC_DATA,
                event->scan_result_event.data, event->scan_result_event.data_len,
                mfgData, &mfgDataLen);
        if (mfgDataLen != 0) {
            Serial1.print(" Manufacturing data found: ");
            for (uint8_t i = 0; i < mfgDataLen; i++) {
                Serial1.printf("%02X ", mfgData[i]);
            }
            Serial1.print("\r\n");
        }

        uint8_t  shortName[20];
        uint16_t shortNameLen = sizeof(shortName);
        ble_adv_data_decode(BLE_SIG_AD_TYPE_SHORT_LOCAL_NAME,
                event->scan_result_event.data, event->scan_result_event.data_len,
                shortName, &shortNameLen);
        if (shortNameLen != 0) {
            shortName[shortNameLen] = '\0';
            Serial1.printf(" Shorted device name found: %s\r\n", shortName);
        }

        uint8_t  uuid128[16];
        uint16_t uuidLen = sizeof(uuid128);
        ble_adv_data_decode(BLE_SIG_AD_TYPE_128BIT_SERVICE_UUID_MORE_AVAILABLE,
                event->scan_result_event.data, event->scan_result_event.data_len,
                uuid128, &uuidLen);
        if (uuidLen != 0) {
            Serial1.print(" 128-bits UUID found: ");
            for (uint8_t i = 0; i < uuidLen; i++) {
                Serial1.printf("%02X ", uuid128[i]);
            }
            Serial1.print("\r\n");
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

    hal_ble_scan_params_t scanParams;
    scanParams.active = true;
    scanParams.filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL;
    scanParams.interval = 3200; // 2 seconds
    scanParams.window   = 100;
    scanParams.timeout  = 2000; // 0 for forever unless stop initially
    ble_set_scanning_params(&scanParams);

    ble_register_callback(handleBleEvent);

    ble_start_scanning();
}

/* This function loops forever --------------------------------------------*/
void loop()
{

}
