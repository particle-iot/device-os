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


const char* addrType[4] = {
    "Public",
    "Random Static",
    "Random Private Resolvable",
    "Random Static Non-resolvable"
};

uint8_t devName[] = "Xenon BLE Sample";
uint8_t advData[] = {
    0x02,
    BLE_SIG_AD_TYPE_FLAGS,
    BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE,
    0x11,
    BLE_SIG_AD_TYPE_COMPLETE_LOCAL_NAME,
    'X','e','n','o','n',' ','B','L','E',' ','S','a','m','p','l','e'
};


static void ble_on_adv_stopped(hal_ble_gap_on_adv_stopped_evt_t* event) {
    LOG(TRACE, "BLE advertising stopped");
}

static void ble_on_scan_result(hal_ble_gap_on_scan_result_evt_t* event) {
    if (event->type.scan_response) {
        LOG(TRACE, "BLE Scan Response event");
    }
    else if (event->type.connectable) {
        LOG(TRACE, "BLE Advertising Connectable event");
    }
    else if (event->type.scannable) {
        LOG(TRACE, "BLE Advertising Scannable event");
    }
    else if (event->type.directed) {
        LOG(TRACE, "BLE Advertising Directed event");
    }
    else if (event->type.extended_pdu) {
        LOG(TRACE, "BLE Advertising Extended PDU event");
    }
    LOG(TRACE, "RSSI: %i dBm", event->rssi);
    if (event->peer_addr.addr_type <= 3) {
        LOG(TRACE, "Peer address type: %s", addrType[event->peer_addr.addr_type]);
    }
    else {
        LOG(TRACE, "Peer address type: Anonymous");
    }
    LOG(TRACE, "Peer address: %02X:%02X:%02X:%02X:%02X:%02X.", event->peer_addr.addr[0], event->peer_addr.addr[1],
                event->peer_addr.addr[2], event->peer_addr.addr[3], event->peer_addr.addr[4], event->peer_addr.addr[5]);

    if (event->data_len != 0) {
        LOG(TRACE, "Data payload: ");
        for (uint8_t i = 0; i < event->data_len; i++) {
            Serial1.printf("%02X ", event->data[i]);
        }
        Serial1.print("\r\n");
    }
}

static void ble_on_scan_stopped(hal_ble_gap_on_scan_stopped_evt_t* event) {
    LOG(TRACE, "BLE scan stopped.");
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
    else if (event->type == BLE_EVT_SCAN_RESULT) {
        ble_on_scan_result(&event->params.scan_result);
    }
    else if (event->type == BLE_EVT_SCAN_STOPPED) {
        ble_on_scan_stopped(&event->params.scan_stopped);
    }
}

/* This function is called once at start up ----------------------------------*/
void setup()
{
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

    ble_gap_set_advertising_data(advData, sizeof(advData), NULL);

    hal_ble_scan_params_t scanParams;
    scanParams.active = true;
    scanParams.filter_policy = BLE_SCAN_FP_ACCEPT_ALL;
    scanParams.interval = 3200; // 1 seconds
    scanParams.window   = 20;
    scanParams.timeout  = 0; // 0 for forever unless stop initially
    ble_gap_set_scan_parameters(&scanParams, NULL);

    ble_set_callback_on_events(ble_on_events, NULL);

    ble_gap_start_advertising(NULL);
    ble_gap_start_scan(NULL);
}

/* This function loops forever --------------------------------------------*/
void loop()
{

}
