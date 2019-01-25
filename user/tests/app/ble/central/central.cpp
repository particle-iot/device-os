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

uint8_t  svc1Uuid[]  = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x00,0x00,0x0e,0x0f};
uint8_t  char1Uuid[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x01,0x00,0x0e,0x0f};

uint8_t  svc2Uuid[]  = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x00,0x00,0x0e,0x10};
uint8_t  char2Uuid[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x01,0x00,0x0e,0x10};

uint16_t svc3Uuid  = 0x1234;
uint16_t char3Uuid = 0x5678;

hal_ble_service_t service1, service2, service3;
hal_ble_char_t char1, char2, char3;
bool char1Discovered, char2Discovered, char3Discovered;

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
    else if (event->evt_type == BLE_EVT_TYPE_CONNECTION) {
        if (event->conn_event.evt_id == BLE_CONN_EVT_ID_CONNECTED) {
            LOG(TRACE, "BLE connected, handle: %d", event->conn_event.conn_handle);

            LOG(TRACE, "Start discovering service...");
            ble_discover_all_services(event->conn_event.conn_handle);
        }
        else {
            LOG(TRACE, "BLE disconnected, handle: %d", event->conn_event.conn_handle);
            LOG(TRACE, "Start scanning...");
            ble_start_scanning();
        }
    }
    else if (event->evt_type == BLE_EVT_TYPE_DISCOVERY) {
        if (event->disc_event.evt_id == BLE_DISC_EVT_ID_SVC_DISCOVERED) {
            hal_ble_service_t* service;
            for (uint8_t i = 0; i < event->disc_event.count; i++) {
                service = NULL;
                if (event->disc_event.services[i].uuid.type == BLE_UUID_TYPE_16BIT) {
                    if (event->disc_event.services[i].uuid.uuid16 == svc3Uuid) {
                        service = &service3;
                        LOG(TRACE, "BLE Service3 found.");
                    }
                }
                else if (event->disc_event.services[i].uuid.type == BLE_UUID_TYPE_128BIT) {
                    if (!memcmp(svc1Uuid, event->disc_event.services[i].uuid.uuid128, BLE_SIG_UUID_128BIT_LEN)) {
                        service = &service1;
                        LOG(TRACE, "BLE Service1 found.");
                    }
                    else if (!memcmp(svc2Uuid, event->disc_event.services[i].uuid.uuid128, BLE_SIG_UUID_128BIT_LEN)) {
                        service = &service2;
                        LOG(TRACE, "BLE Service2 found.");
                    }
                }
                if (service != NULL) {
                    memcpy(service, &event->disc_event.services[i], sizeof(hal_ble_service_t));
                }
            }

            if (!char1Discovered) {
                ble_discover_characteristics(event->disc_event.conn_handle, service1.start_handle, service1.end_handle);
            }
        }
        else if (event->disc_event.evt_id == BLE_DISC_EVT_ID_CHAR_DISCOVERED) {
            hal_ble_char_t* characteristic = NULL;
            for (uint8_t i = 0; i < event->disc_event.count; i++) {
                if (event->disc_event.characteristics[i].uuid.type == BLE_UUID_TYPE_16BIT) {
                    if (event->disc_event.characteristics[i].uuid.uuid16 == char3Uuid) {
                        characteristic = &char3;
                        char3Discovered = true;
                        LOG(TRACE, "BLE Characteristic3 found.");
                    }
                }
                else if (event->disc_event.characteristics[i].uuid.type == BLE_UUID_TYPE_128BIT) {
                    if (!memcmp(char1Uuid, event->disc_event.characteristics[i].uuid.uuid128, BLE_SIG_UUID_128BIT_LEN)) {
                        characteristic = &char1;
                        char1Discovered = true;
                        LOG(TRACE, "BLE Characteristic1 found.");
                    }
                    else if (!memcmp(char2Uuid, event->disc_event.characteristics[i].uuid.uuid128, BLE_SIG_UUID_128BIT_LEN)) {
                        characteristic = &char2;
                        char2Discovered = true;
                        LOG(TRACE, "BLE Characteristic2 found.");
                    }
                }
                if (characteristic != NULL) {
                    memcpy(characteristic, &event->disc_event.characteristics[i], sizeof(hal_ble_char_t));
                }
            }

            if (!char2Discovered) {
                ble_discover_characteristics(event->disc_event.conn_handle, service2.start_handle, service2.end_handle);
            }
            else if (!char3Discovered) {
                ble_discover_characteristics(event->disc_event.conn_handle, service3.start_handle, service3.end_handle);
            }
            else {
                ble_discover_descriptors(event->disc_event.conn_handle, char2.value_handle, char3.decl_handle);
            }
        }
        else if (event->disc_event.evt_id == BLE_DISC_EVT_ID_DESC_DISCOVERED) {
            for (uint8_t i = 0; i < event->disc_event.count; i++) {
                if (event->disc_event.descriptors[i].uuid.uuid16 == BLE_SIG_UUID_CLIENT_CHAR_CONFIG_DESC) {
                    char2.cccd_handle = event->disc_event.descriptors[i].handle;
                    LOG(TRACE, "BLE Characteristic2 CCCD found.");

                    ble_configure_cccd(event->disc_event.conn_handle, &char2, true);
                }
            }
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
