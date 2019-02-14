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

static int decodeAdvertisementData(uint8_t ads_type, const uint8_t* adv_data, uint16_t adv_data_len, uint8_t* data, uint16_t* len) {
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
        decodeAdvertisementData(BLE_SIG_AD_TYPE_MANUFACTURER_SPECIFIC_DATA,
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
        decodeAdvertisementData(BLE_SIG_AD_TYPE_SHORT_LOCAL_NAME,
                event->scan_result_event.data, event->scan_result_event.data_len,
                shortName, &shortNameLen);
        if (shortNameLen != 0) {
            shortName[shortNameLen] = '\0';
            Serial1.printf(" Shorted device name found: %s\r\n", shortName);
        }

        uint8_t  uuid128[16];
        uint16_t uuidLen = sizeof(uuid128);
        decodeAdvertisementData(BLE_SIG_AD_TYPE_128BIT_SERVICE_UUID_MORE_AVAILABLE,
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

    ble_stack_init(NULL);

    ble_gap_set_device_name(devName, sizeof(devName));

    hal_ble_scan_parameters_t scanParams;
    scanParams.active = true;
    scanParams.filter_policy = BLE_SCAN_FP_ACCEPT_ALL;
    scanParams.interval = 3200; // 2 seconds
    scanParams.window   = 100;
    scanParams.timeout  = 2000; // 0 for forever unless stop initially
    ble_gap_set_scan_parameters(&scanParams);

    ble_register_callback(handleBleEvent);

    ble_gap_start_scan();
}

/* This function loops forever --------------------------------------------*/
void loop()
{

}
