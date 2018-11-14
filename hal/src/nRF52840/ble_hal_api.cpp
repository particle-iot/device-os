/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/* Headers included from nRF5_SDK/components/softdevice/s140/headers */
#include "ble.h"
#include "ble_types.h"
#include "ble_gap.h"
#include "ble_gatt.h"
#include "ble_gattc.h"
#include "ble_gatts.h"
/* Headers included from nRF5_SDK/components/softdevice/common */
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"

#include "concurrent_hal.h"
#include "static_recursive_mutex.h"
#include <mutex>

#include "ble_hal_api.h"
#include "system_error.h"
#include "logging.h"

LOG_SOURCE_CATEGORY("hal.ble_api")

using namespace particle::ble;

#define BLE_CONN_CFG_TAG                1
#define BLE_OBSERVER_PRIO               1


StaticRecursiveMutex s_bleMutex;

class bleLock {
    static void lock() {
        s_bleMutex.lock();
    }

    static void unlock() {
        s_bleMutex.unlock();
    }
};

static system_error_t sysError(uint32_t error) {
    switch (error) {
        case NRF_SUCCESS:             return SYSTEM_ERROR_NONE;
        case NRF_ERROR_INVALID_STATE: return SYSTEM_ERROR_INVALID_STATE;
        case NRF_ERROR_BUSY:          return SYSTEM_ERROR_BUSY;
        case NRF_ERROR_NO_MEM:        return SYSTEM_ERROR_NO_MEMORY;
        default:                      return SYSTEM_ERROR_UNKNOWN;
    }
}

static void processBleEvent(const ble_evt_t* event, void* p_context) {
    ret_code_t ret;

    switch (event->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED: {
            LOG_DEBUG(TRACE, "BLE connected, handle: 0x%04X", event->evt.gap_evt.conn_handle);
            break;
        }
        case BLE_GAP_EVT_DISCONNECTED: {
            LOG_DEBUG(TRACE, "BLE Disconnected, handle: 0x%04X, reason: %d",
                      event->evt.gap_evt.conn_handle,
                      event->evt.gap_evt.params.disconnected.reason);
            break;
        }
        case BLE_GAP_EVT_PHY_UPDATE_REQUEST: {
            LOG_DEBUG(TRACE, "PHY update request.");
            ble_gap_phys_t phys = {};
            phys.rx_phys = BLE_GAP_PHY_AUTO;
            phys.tx_phys = BLE_GAP_PHY_AUTO;
            ret = sd_ble_gap_phy_update(event->evt.gap_evt.conn_handle, &phys);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gap_phy_update() failed: %u", (unsigned)ret);
            }
            break;
        }
        default: {
            LOG_DEBUG(TRACE, "Unhandled BLE event: %u", (unsigned)event->header.evt_id);
            break;
        }
    }
}

/* This function should be called previous to any other BLE APIs. */
int hal_ble_init(void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());

    /* The SoftDevice has been enabled in core_hal.c */

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    ret_code_t ret = nrf_sdh_ble_default_cfg_set(BLE_CONN_CFG_TAG, &ram_start);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "nrf_sdh_ble_default_cfg_set() failed: %u", (unsigned)ret);
        return sysError(ret);
    }
    LOG_DEBUG(TRACE, "RAM start: 0x%08x", (unsigned)ram_start);

    // Enable the stack
    ret = nrf_sdh_ble_enable(&ram_start);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "nrf_sdh_ble_enable() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(bleObserver, BLE_OBSERVER_PRIO, processBleEvent, nullptr);

    return SYSTEM_ERROR_NONE;
}

int hal_ble_on_gap_event_callbacks(ble_gap_event_callbacks_t* callbacks) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_on_gattc_event_callbacks(ble_gattc_event_callbacks_t* callback) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_on_gatts_event_callbacks(ble_gatts_event_callbacks_t* callback) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_set_device_address(ble_address_t const* addr) {
    std::lock_guard<bleLock> lk(bleLock());
    ble_gap_addr_t local_addr;

    local_addr.addr_id_peer = 0;
    local_addr.addr_type    = addr->type;
    memcpy(local_addr.addr, addr->data, BLE_GAP_ADDR_LEN);

    ret_code_t ret = sd_ble_gap_addr_set(&local_addr);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_addr_set() failed: %u", (unsigned)ret);
    }
    return sysError(ret);
}

int hal_ble_get_device_address(ble_address_t* addr) {
    std::lock_guard<bleLock> lk(bleLock());
    ble_gap_addr_t local_addr;

    ret_code_t ret = sd_ble_gap_addr_get(&local_addr);
    if (ret == NRF_SUCCESS) {
        addr->type = (ble_address_type_t)local_addr.addr_type;
        memcpy(addr->data, local_addr.addr, BLE_GAP_ADDR_LEN);
    }
    else {
        LOG(ERROR, "sd_ble_gap_addr_get() failed: %u", (unsigned)ret);
    }
    return sysError(ret);
}

int hal_ble_set_device_name(const char* device_name) {
    std::lock_guard<bleLock> lk(bleLock());
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    ret_code_t ret = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t*)device_name, strlen(device_name));
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_device_name_set() failed: %u", (unsigned)ret);
    }
    return sysError(ret);
}

int hal_ble_get_device_name(uint8_t* device_name, uint16_t* len) {
    std::lock_guard<bleLock> lk(bleLock());

    // non NULL-terminated string returned.
    ret_code_t ret = sd_ble_gap_device_name_get(device_name, len);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_device_name_get() failed: %u", (unsigned)ret);
    }
    return sysError(ret);
}

int hal_ble_set_appearance(uint16_t appearance) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_get_appearance(uint16_t* appearance) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_add_to_whitelist(ble_address_t* addr) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_remove_from_whitelist(ble_address_t* addr) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_set_tx_power(int8_t value) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_get_tx_power(int8_t* value) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_set_advertising_params(ble_adv_params_t* adv_params) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_set_advertising_interval(uint16_t interval) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_set_advertising_duration(uint16_t duration) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_set_advertising_type(uint8_t type) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_enable_advertising_filter(bool enable) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_set_advertising_data(uint8_t* data, uint8_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_set_scan_response_data(uint8_t* data, uint8_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_start_advertising(void) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_stop_advertising(void) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

bool hal_ble_is_advertising(void) {
    return false;
}

int hal_ble_set_scanning_params(ble_scan_params_t* scan_params) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_set_scanning_interval(uint16_t interval) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_set_scanning_window(uint16_t window) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_set_scanning_timeout(uint16_t timeout) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_enable_scanning_filter(bool enable) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_set_scanning_policy(uint8_t value) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_start_scanning(void) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_stop_scanning(void) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_connect(ble_address_t* addr) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_connect_cancel(void) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_disconnect(uint16_t conn_handle) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_update_connection_params(uint16_t conn_handle, ble_conn_params_t* conn_params) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_set_ppcp(ble_conn_params_t* conn_params) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_get_ppcp(ble_conn_params_t* conn_params) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_get_rssi(uint16_t conn_handle) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_discovery_services(uint16_t conn_handle) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_discovery_characteristics(uint16_t conn_handle, uint16_t service_handle) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_discovery_descriptors(uint16_t conn_handle, uint16_t char_handle) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_add_service(particle::ble::ble_uuid_t const* uuid, uint16_t* service_handle) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_add_characteristic(uint16_t service_handle, ble_characteristic_t* characteristic, uint16_t *char_handle) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_subscribe(uint16_t conn_handle, uint8_t char_handle) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_unsubscribe(uint16_t conn_handle, uint8_t char_handle) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_write_with_response(uint16_t conn_handle, uint8_t char_handle, uint8_t* data, uint16_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_write_without_response(uint16_t conn_handle, uint8_t char_handle, uint8_t* data, uint16_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_read(uint16_t conn_handle, uint8_t char_handle, uint8_t* data, uint16_t* len) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_publish(uint16_t conn_handle, uint8_t char_handle, uint8_t* data, uint16_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_set_characteristic_value(uint8_t char_handle, uint8_t* data, uint16_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int hal_ble_get_characteristic_value(uint8_t char_handle, uint8_t* data, uint16_t* len) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}


