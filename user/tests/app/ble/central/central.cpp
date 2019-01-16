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

static void handleBleEvent(hal_ble_event_t *event)
{
    if (event->evt_type == BLE_EVT_TYPE_SCAN_RESULT) {
        uint8_t  devName[20];
        uint16_t devNameLen = sizeof(devName);
        ble_adv_data_decode(BLE_SIG_AD_TYPE_COMPLETE_LOCAL_NAME,
                event->scan_result_event.data, event->scan_result_event.data_len,
                devName, &devNameLen);

        if (devNameLen != 0 && devNameLen < sizeof(devName)) {
            devName[devNameLen] = '\0';
            if (!strcmp((const char*)devName, "Xenon BLE Sample")) {
                LOG(TRACE, "Target device found. Start connecting...");
                ble_connect(&event->scan_result_event.peer_addr);
            }
        }
    }
    if (event->evt_type == BLE_EVT_TYPE_CONNECTION) {
        if (event->conn_event.evt_id == BLE_CONN_EVT_ID_CONNECTED) {
            LOG(TRACE, "BLE connected, handle: %d", event->conn_event.conn_handle);
        }
        else {
            LOG(TRACE, "BLE disconnected, handle: %d", event->conn_event.conn_handle);
            LOG(TRACE, "Start scanning...");
            ble_start_scanning();
        }
    }
}

/* This function is called once at start up ----------------------------------*/
void setup()
{
    uint8_t devName[] = "Xenon BLE Central";

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
