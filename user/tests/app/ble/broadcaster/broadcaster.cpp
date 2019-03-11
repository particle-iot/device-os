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

hal_ble_char_t bleChar1; // Read and Write
hal_ble_char_t bleChar2; // Notify
hal_ble_char_t bleChar3; // Write without response

const char* addrType[4] = {
    "Public",
    "Random Static",
    "Random Private Resolvable",
    "Random Static Non-resolvable"
};

uint8_t devName[] = "Xenon BLE Sample";
uint8_t advDataSet1[] = {
    0x02,
    BLE_SIG_AD_TYPE_FLAGS,
    BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE,
    0x11,
    BLE_SIG_AD_TYPE_COMPLETE_LOCAL_NAME,
    'X','e','n','o','n',' ','B','L','E',' ','S','a','m','p','l','e'
};

uint8_t advDataSet2[] = {
    0x02,
    BLE_SIG_AD_TYPE_FLAGS,
    BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE,
    0x11,
    BLE_SIG_AD_TYPE_MANUFACTURER_SPECIFIC_DATA,
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f
};


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

    ble_set_callback_on_events(ble_on_events, NULL);

    ble_gap_start_advertising(NULL);
}

/* This function loops forever --------------------------------------------*/
void loop()
{
    LOG(TRACE, "Start advertising using data set 1.");
    ble_gap_set_advertising_data(advDataSet1, sizeof(advDataSet1), NULL);
    delay(5000);

    LOG(TRACE, "Start advertising using data set 2.");
    ble_gap_set_advertising_data(advDataSet2, sizeof(advDataSet2), NULL);
    delay(5000);
}
