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

#include "logging.h"
LOG_SOURCE_CATEGORY("hal.ble_api")

/* Headers included from nRF5_SDK/components/softdevice/s140/headers */
#include "ble.h"
#include "ble_gap.h"
#include "ble_gatt.h"
#include "ble_gatts.h"
#include "ble_gattc.h"
#include "ble_types.h"
/* Headers included from nRF5_SDK/components/softdevice/common */
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"

#include "app_util.h"

#include "concurrent_hal.h"
#include "static_recursive_mutex.h"
#include <mutex>

#include "ble_hal_api.h"
#include "system_error.h"

#include <string.h>


#define BLE_CONN_CFG_TAG                        1
#define BLE_OBSERVER_PRIO                       1

#define BLE_ADV_DATA_ADVERTISING                0
#define BLE_ADV_DATA_SCAN_RESPONSE              1


StaticRecursiveMutex s_bleMutex;

class bleLock {
    static void lock() {
        s_bleMutex.lock();
    }

    static void unlock() {
        s_bleMutex.unlock();
    }
};

static bool         s_bleInitialized = false;

static ble_event_callback_t     s_bleEvtCallbacks[BLE_MAX_EVENT_CALLBACK_COUNT];

static ble_gap_addr_t           s_whitelist[BLE_MAX_WHITELIST_ADDR_COUNT];
static ble_gap_addr_t const*    s_whitelist_ptr[BLE_MAX_WHITELIST_ADDR_COUNT];

static bool     s_isAdvertising = false;
static bool     s_advPending = false;
static uint8_t  s_advHandle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;
static int8_t   s_advTxPower = 0;
static uint8_t  s_advData[BLE_MAX_ADV_DATA_LEN] = {
    0x02,
    BLE_SIG_AD_TYPE_FLAGS,
    BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE,
};
static uint16_t s_advDataLen = 3;
static uint8_t  s_scanRespData[BLE_MAX_ADV_DATA_LEN];
static uint16_t s_scanRespDataLen = 0;

static hal_ble_scan_params_t    s_scanParams = {
    .active        = true,
    .filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL,
    .interval      = BLE_DEFAULT_SCANNING_INTERVAL,
    .window        = BLE_DEFAULT_SCANNING_WINDOW,
    .timeout       = BLE_DEFAULT_SCANNING_TIMEOUT
};
static uint8_t                  s_scanReportBuff[BLE_MAX_SCAN_REPORT_BUF_LEN];
static ble_data_t               s_bleScanData = {
    s_scanReportBuff,
    BLE_MAX_SCAN_REPORT_BUF_LEN
};

static uint8_t s_connParamsUpdateAttempts = 0;

static hal_ble_connection_t s_bleConnInfo;

static bool s_indInProgress = false;

static os_queue_t  s_bleEvtQueue = NULL;
static os_thread_t s_bleEvtThread = NULL;
static os_timer_t  s_connParamsUpdateTimer = NULL;

/*
 * Valid TX Power for nRF52840:
 * -40dBm, -20dBm, -16dBm, -12dBm, -8dBm, -4dBm, 0dBm, +2dBm, +3dBm, +4dBm, +5dBm, +6dBm, +7dBm and +8dBm.
 */
static const int8_t s_validTxPower[] = {
    -20, -16, -12, -8, -4, 0, 4, 8
};


static system_error_t sysError(uint32_t error) {
    switch (error) {
        case NRF_SUCCESS:                       return SYSTEM_ERROR_NONE;
        case NRF_ERROR_SVC_HANDLER_MISSING:
        case NRF_ERROR_NOT_SUPPORTED:           return SYSTEM_ERROR_NOT_SUPPORTED;
        case NRF_ERROR_SOFTDEVICE_NOT_ENABLED:
        case NRF_ERROR_INVALID_STATE:           return SYSTEM_ERROR_INVALID_STATE;
        case NRF_ERROR_INTERNAL:                return SYSTEM_ERROR_INTERNAL;
        case NRF_ERROR_NO_MEM:                  return SYSTEM_ERROR_NO_MEMORY;
        case NRF_ERROR_NOT_FOUND:               return SYSTEM_ERROR_NOT_FOUND;
        case NRF_ERROR_INVALID_PARAM:
        case NRF_ERROR_INVALID_LENGTH:
        case NRF_ERROR_INVALID_FLAGS:
        case NRF_ERROR_INVALID_DATA:
        case NRF_ERROR_DATA_SIZE:
        case NRF_ERROR_NULL:                    return SYSTEM_ERROR_INVALID_ARGUMENT;
        case NRF_ERROR_TIMEOUT:                 return SYSTEM_ERROR_TIMEOUT;
        case NRF_ERROR_FORBIDDEN:
        case NRF_ERROR_INVALID_ADDR:            return SYSTEM_ERROR_NOT_ALLOWED;
        case NRF_ERROR_BUSY:                    return SYSTEM_ERROR_BUSY;
        case NRF_ERROR_CONN_COUNT:              return SYSTEM_ERROR_LIMIT_EXCEEDED;
        case NRF_ERROR_RESOURCES:               return SYSTEM_ERROR_ABORTED;
        default:                                return SYSTEM_ERROR_UNKNOWN;
    }
}

static int8_t roundTxPower(int8_t value) {
    uint8_t i = 0;

    for (i = 0; i < sizeof(s_validTxPower); i++) {
        if (value == s_validTxPower[i]) {
            return value;
        }
        else {
            if (value < s_validTxPower[i]) {
                return s_validTxPower[i];
            }
        }
    }

    return s_validTxPower[i - 1];
}

static int locateAdStructure(uint8_t flag, uint8_t* data, uint16_t len, uint16_t* offset, uint16_t* adsLen) {
    if (flag == BLE_SIG_AD_TYPE_FLAGS) {
        *offset = 0;
        *adsLen = 3;
        return SYSTEM_ERROR_NONE;
    }

    // A valid AD structure is composed of Length field, Type field and Data field.
    // Each field should be filled with at least one byte.
    for (uint16_t i = 3; (i + 3) <= len; i = i) {
        *adsLen = data[i];

        uint8_t adsType = data[i + 1];
        if (adsType == flag) {
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

static int adStructEncode(uint8_t adType, uint8_t* data, uint16_t len, uint8_t flag) {
    uint8_t*  pAdvData = NULL;
    uint16_t* pAdvDataLen = NULL;
    uint16_t  offset;
    uint16_t  adsLen;
    bool      adsExist = false;

    if (flag == BLE_ADV_DATA_ADVERTISING) {
        pAdvData = s_advData;
        pAdvDataLen = &s_advDataLen;
    }
    else if (flag == BLE_ADV_DATA_SCAN_RESPONSE && adType != BLE_SIG_AD_TYPE_FLAGS) {
        // The scan response data should not contain the AD Flags AD structure.
        pAdvData = s_scanRespData;
        pAdvDataLen = &s_scanRespDataLen;
    }
    else {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if (locateAdStructure(adType, pAdvData, *pAdvDataLen, &offset, &adsLen) == SYSTEM_ERROR_NONE) {
        adsExist = true;
    }

    if (data == NULL) {
        // The advertising data must contain the AD Flags AD structure.
        if (adType == BLE_SIG_AD_TYPE_FLAGS) {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }

        // Remove the AD structure from advertising data.
        if (adsExist == true) {
            uint16_t moveLen = *pAdvDataLen - offset - adsLen;
            memcpy(&pAdvData[offset], &pAdvData[offset + adsLen], moveLen);
            *pAdvDataLen = *pAdvDataLen - adsLen;
        }

        return SYSTEM_ERROR_NONE;
    }
    else if (adsExist) {
        // Update the existing AD structure.
        uint16_t staLen = *pAdvDataLen - adsLen;
        if ((staLen + len + 2) <= BLE_MAX_ADV_DATA_LEN) {
            // Firstly, move the last consistent block.
            uint16_t moveLen = *pAdvDataLen - offset - adsLen;
            memmove(&pAdvData[offset + len], &pAdvData[offset + adsLen], moveLen);

            // Secondly, Update the AD structure.
            // The Length field is the total length of Type field and Data field.
            pAdvData[offset + 1] = len + 1;
            memcpy(&pAdvData[offset + 2], data, len);

            // An AD structure is composed of one byte length field, one byte Type field and Data field.
            *pAdvDataLen = staLen + len + 2;
            return SYSTEM_ERROR_NONE;
        }
        else {
            return SYSTEM_ERROR_LIMIT_EXCEEDED;
        }
    }
    else {
        // Append the AD structure at the and of advertising data.
        if ((*pAdvDataLen + len + 2) <= BLE_MAX_ADV_DATA_LEN) {
            pAdvData[(*pAdvDataLen)++] = len + 1;
            pAdvData[(*pAdvDataLen)++] = adType;
            memcpy(&pAdvData[*pAdvDataLen], data, len);
            *pAdvDataLen += len;
            return SYSTEM_ERROR_NONE;
        }
        else {
            return SYSTEM_ERROR_LIMIT_EXCEEDED;
        }
    }
}

int setAdvData(uint8_t* data, uint16_t len, uint8_t flag) {
    if ((data == NULL) || (len < 3 && flag == BLE_ADV_DATA_ADVERTISING)) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    // It is invalid to provide the same data buffers while advertising.
    if (s_isAdvertising) {
        int err = ble_stop_advertising();
        if (err != SYSTEM_ERROR_NONE) {
            return err;
        }
        s_advPending = true;
    }

    // Make sure the scan response data is consistent.
    ble_gap_adv_data_t bleGapAdvData;
    if (flag == BLE_ADV_DATA_ADVERTISING) {
        bleGapAdvData.adv_data.p_data      = data;
        bleGapAdvData.adv_data.len         = len;
        bleGapAdvData.scan_rsp_data.p_data = s_scanRespData;
        bleGapAdvData.scan_rsp_data.len    = s_scanRespDataLen;
    }
    else {
        bleGapAdvData.adv_data.p_data      = s_advData;
        bleGapAdvData.adv_data.len         = s_advDataLen;
        bleGapAdvData.scan_rsp_data.p_data = data;
        bleGapAdvData.scan_rsp_data.len    = len;
    }

    ret_code_t ret = sd_ble_gap_adv_set_configure(&s_advHandle, &bleGapAdvData, NULL);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_adv_set_configure() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    if (flag == BLE_ADV_DATA_ADVERTISING) {
        memcpy(s_advData, data, len);
        s_advDataLen = len;
    }
    else {
        memcpy(s_scanRespData, data, len);
        s_scanRespDataLen = len;
    }

    if (s_advPending) {
        s_advPending = false;
        return ble_start_advertising();
    }

    return SYSTEM_ERROR_NONE;
}

static bool isConnParamsFeeded(hal_ble_connection_t* conn) {
    hal_ble_conn_params_t ppcp;

    if (ble_get_ppcp(&ppcp) == SYSTEM_ERROR_NONE) {
        uint16_t minAcceptedSl = ppcp.slave_latency - MIN(BLE_CONN_PARAMS_SLAVE_LATENCY_ERR, ppcp.slave_latency);
        uint16_t maxAcceptedSl = ppcp.slave_latency + BLE_CONN_PARAMS_SLAVE_LATENCY_ERR;
        uint16_t minAcceptedTo = ppcp.conn_sup_timeout - MIN(BLE_CONN_PARAMS_TIMEOUT_ERR, ppcp.conn_sup_timeout);
        uint16_t maxAcceptedTo = ppcp.conn_sup_timeout + BLE_CONN_PARAMS_TIMEOUT_ERR;

        if (conn->conn_interval < ppcp.min_conn_interval ||
                conn->conn_interval > ppcp.max_conn_interval) {
            return false;
        }
        if (conn->slave_latency < minAcceptedSl ||
                conn->slave_latency > maxAcceptedSl) {
            return false;
        }
        if (conn->conn_sup_timeout < minAcceptedTo ||
                conn->conn_sup_timeout > maxAcceptedTo) {
            return false;
        }
    }

    return true;
}

static void connParamsUpdateTimerCb(os_timer_t timer) {
    if (s_bleConnInfo.conn_handle == BLE_INVALID_CONN_HANDLE) {
        return;
    }

    hal_ble_conn_params_t ppcp;
    if (ble_get_ppcp(&ppcp) == SYSTEM_ERROR_NONE) {
        ble_gap_conn_params_t connParams;
        connParams.min_conn_interval = ppcp.min_conn_interval;
        connParams.max_conn_interval = ppcp.max_conn_interval;
        connParams.slave_latency     = ppcp.slave_latency;
        connParams.conn_sup_timeout  = ppcp.conn_sup_timeout;

        ret_code_t ret = sd_ble_gap_conn_param_update(s_bleConnInfo.conn_handle, &connParams);
        if (ret != NRF_SUCCESS) {
            LOG(WARN, "sd_ble_gap_conn_param_update() failed: %u", (unsigned)ret);
            return;
        }

        s_connParamsUpdateAttempts++;
    }
}

static void updateConnParamsIfNeeded(void) {
    if (s_bleConnInfo.role == BLE_ROLE_PERIPHERAL && !isConnParamsFeeded(&s_bleConnInfo)) {
        if (s_connParamsUpdateAttempts < BLE_CONN_PARAMS_UPDATE_ATTEMPS) {
            if (s_connParamsUpdateTimer != NULL) {
                if (os_timer_change(s_connParamsUpdateTimer, OS_TIMER_CHANGE_START, true, 0, 0, NULL)) {
                    LOG(ERROR, "os_timer_change() failed.");
                    return;
                }
            }
            LOG_DEBUG(TRACE, "Attempts to update BLE connection parameters, try: %d after %d ms",
                    s_connParamsUpdateAttempts, BLE_CONN_PARAMS_UPDATE_DELAY_MS);
        }
        else {
            ret_code_t ret = sd_ble_gap_disconnect(s_bleConnInfo.conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gap_disconnect() failed: %u", (unsigned)ret);
                return;
            }
            LOG_DEBUG(TRACE, "Disconnecting. Update BLE connection parameters failed.");
        }
    }
}

static int addService(uint8_t type, const ble_uuid_t* uuid, uint16_t* handle) {
    ret_code_t ret;
    ret = sd_ble_gatts_service_add(type, uuid, handle);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_service_add() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    LOG_DEBUG(TRACE, "Service handle: %d.", *handle);

    return SYSTEM_ERROR_NONE;
}

static int addCharacteristic(uint16_t service_handle, const ble_uuid_t* uuid, uint8_t properties, hal_ble_char_t* ble_char) {
    ret_code_t               ret;
    ble_gatts_char_md_t      char_md;
    ble_gatts_attr_md_t      value_attr_md;
    ble_gatts_attr_md_t      cccd_attr_md;
    ble_gatts_attr_t         char_value_attr;
    ble_gatts_char_handles_t char_handles;

    /* Characteristic metadata */
    memset(&char_md, 0, sizeof(char_md));
    char_md.char_props.broadcast      = (properties & BLE_SIG_CHAR_PROP_BROADCAST) ? 1 : 0;
    char_md.char_props.read           = (properties & BLE_SIG_CHAR_PROP_READ) ? 1 : 0;
    char_md.char_props.write_wo_resp  = (properties & BLE_SIG_CHAR_PROP_WRITE_WO_RESP) ? 1 : 0;
    char_md.char_props.write          = (properties & BLE_SIG_CHAR_PROP_WRITE) ? 1 : 0;
    char_md.char_props.notify         = (properties & BLE_SIG_CHAR_PROP_NOTIFY) ? 1 : 0;
    char_md.char_props.indicate       = (properties & BLE_SIG_CHAR_PROP_INDICATE) ? 1 : 0;
    char_md.char_props.auth_signed_wr = (properties & BLE_SIG_CHAR_PROP_AUTH_SIGN_WRITES) ? 1 : 0;
    // User Description Descriptor
    char_md.p_char_user_desc = NULL;
    char_md.p_user_desc_md   = NULL;
    // Client Characteristic Configuration Descriptor
    if (char_md.char_props.notify || char_md.char_props.indicate) {
        memset(&cccd_attr_md, 0, sizeof(cccd_attr_md));
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_attr_md.read_perm);
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_attr_md.write_perm);
        cccd_attr_md.vloc = BLE_GATTS_VLOC_STACK;

        char_md.p_cccd_md = &cccd_attr_md;
    }
    // TODO:
    char_md.p_char_pf = NULL;
    char_md.p_sccd_md = NULL;

    /* Characteristic value attribute metadata */
    memset(&value_attr_md, 0, sizeof(value_attr_md));
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&value_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&value_attr_md.write_perm);
    value_attr_md.vloc    = BLE_GATTS_VLOC_STACK;
    value_attr_md.rd_auth = 0;
    value_attr_md.wr_auth = 0;
    value_attr_md.vlen    = 1;

    /* Characteristic value attribute */
    memset(&char_value_attr, 0, sizeof(char_value_attr));
    char_value_attr.p_uuid    = uuid;
    char_value_attr.p_attr_md = &value_attr_md;
    char_value_attr.init_len  = 0;
    char_value_attr.init_offs = 0;
    char_value_attr.max_len   = BLE_MAX_CHAR_VALUE_LEN;
    char_value_attr.p_value   = NULL;

    ret = sd_ble_gatts_characteristic_add(service_handle, &char_md, &char_value_attr, &char_handles);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_characteristic_add() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    ble_char->properties       = properties;
    ble_char->value_handle     = char_handles.value_handle;
    ble_char->user_desc_handle = char_handles.user_desc_handle;
    ble_char->cccd_handle      = char_handles.cccd_handle;
    ble_char->sccd_handle      = char_handles.sccd_handle;

    LOG_DEBUG(TRACE, "Characteristic value handle: %d.", char_handles.value_handle);
    LOG_DEBUG(TRACE, "Characteristic cccd handle: %d.", char_handles.cccd_handle);

    return SYSTEM_ERROR_NONE;
}

static void dispatchBleEvent(hal_ble_event_t* evt) {
    for (uint8_t i = 0; i < BLE_MAX_EVENT_CALLBACK_COUNT; i++) {
        if (s_bleEvtCallbacks[i] != NULL) {
            s_bleEvtCallbacks[i](evt);
        }
    }
}

static void isrProcessBleEvent(const ble_evt_t* event, void* context) {
    ret_code_t ret;

    switch (event->header.evt_id) {
        case BLE_GAP_EVT_ADV_SET_TERMINATED: {
            LOG_DEBUG(TRACE, "BLE GAP event: advertising stopped.");
            s_isAdvertising = false;
        } break;

        case BLE_GAP_EVT_CONNECTED: {
            // FIXME: If multi role is enabled, this flag should not be clear here.
            s_isAdvertising = false;

            LOG_DEBUG(TRACE, "BLE GAP event: connected.");

            s_bleConnInfo.role             = event->evt.gap_evt.params.connected.role;
            s_bleConnInfo.conn_handle      = event->evt.gap_evt.conn_handle;
            s_bleConnInfo.conn_interval    = event->evt.gap_evt.params.connected.conn_params.max_conn_interval;
            s_bleConnInfo.slave_latency    = event->evt.gap_evt.params.connected.conn_params.slave_latency;
            s_bleConnInfo.conn_sup_timeout = event->evt.gap_evt.params.connected.conn_params.conn_sup_timeout;

            LOG_DEBUG(TRACE, "BLE role: %d", s_bleConnInfo.role);
            LOG_DEBUG(TRACE, "BLE connection handle: %d", s_bleConnInfo.conn_handle);
            LOG_DEBUG(TRACE, "BLE connection interval: (%d x 1.25) ms", s_bleConnInfo.conn_interval);
            LOG_DEBUG(TRACE, "BLE slave latency: %d", s_bleConnInfo.slave_latency);
            LOG_DEBUG(TRACE, "BLE connection supervision timeout: (%d x 10) ms", s_bleConnInfo.conn_sup_timeout);

            // Update connection parameters if needed.
            s_connParamsUpdateAttempts = 0;
            updateConnParamsIfNeeded();

            hal_ble_event_t bleEvt;
            bleEvt.evt_type               = BLE_EVT_TYPE_CONNECTION;
            bleEvt.conn_event.conn_handle = s_bleConnInfo.conn_handle;
            bleEvt.conn_event.evt_id      = BLE_CONN_EVT_ID_CONNECTED;
            if (os_queue_put(s_bleEvtQueue, &bleEvt, 0, NULL)) {
                LOG(ERROR, "os_queue_put() failed.");
            }
        } break;

        case BLE_GAP_EVT_DISCONNECTED: {
            LOG_DEBUG(TRACE, "BLE GAP event: disconnected, handle: 0x%04X, reason: %d",
                      event->evt.gap_evt.conn_handle,
                      event->evt.gap_evt.params.disconnected.reason);

            s_bleConnInfo.conn_handle = BLE_INVALID_CONN_HANDLE;

            // Stop the connection parameters update timer.
            if (!os_timer_is_active(s_connParamsUpdateTimer, NULL)) {
                os_timer_change(s_connParamsUpdateTimer, OS_TIMER_CHANGE_STOP, true, 0, 0, NULL);
            }

            hal_ble_event_t bleEvt;
            bleEvt.evt_type               = BLE_EVT_TYPE_CONNECTION;
            bleEvt.conn_event.conn_handle = s_bleConnInfo.conn_handle;
            bleEvt.conn_event.evt_id      = BLE_CONN_EVT_ID_DISCONNECTED;
            bleEvt.conn_event.reason      = event->evt.gap_evt.params.disconnected.reason;
            if (os_queue_put(s_bleEvtQueue, &bleEvt, 0, NULL)) {
                LOG(ERROR, "os_queue_put() failed.");
            }

            // Re-start advertising.
            LOG_DEBUG(TRACE, "Restart BLE advertising.");
            ret = sd_ble_gap_adv_start(s_advHandle, BLE_CONN_CFG_TAG);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gap_adv_start() failed: %u", (unsigned)ret);
            }
            else {
                s_isAdvertising = true;
            }
        } break;

        case BLE_GAP_EVT_CONN_PARAM_UPDATE: {
            LOG_DEBUG(TRACE, "BLE GAP event: connection parameter updated.");

            s_bleConnInfo.conn_interval    = event->evt.gap_evt.params.conn_param_update.conn_params.max_conn_interval;
            s_bleConnInfo.slave_latency    = event->evt.gap_evt.params.conn_param_update.conn_params.slave_latency;
            s_bleConnInfo.conn_sup_timeout = event->evt.gap_evt.params.conn_param_update.conn_params.conn_sup_timeout;

            LOG_DEBUG(TRACE, "BLE connection interval: (%d x 1.25) ms", s_bleConnInfo.conn_interval);
            LOG_DEBUG(TRACE, "BLE slave latency: %d", s_bleConnInfo.slave_latency);
            LOG_DEBUG(TRACE, "BLE connection supervision timeout: (%d x 10) ms", s_bleConnInfo.conn_sup_timeout);

            // Update connection parameters if needed.
            updateConnParamsIfNeeded();
        } break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST: {
            LOG_DEBUG(TRACE, "BLE GAP event: physical update request.");
            ble_gap_phys_t phys = {};
            phys.rx_phys = BLE_GAP_PHY_AUTO;
            phys.tx_phys = BLE_GAP_PHY_AUTO;
            ret = sd_ble_gap_phy_update(event->evt.gap_evt.conn_handle, &phys);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gap_phy_update() failed: %u", (unsigned)ret);
            }
        } break;

        case BLE_GAP_EVT_PHY_UPDATE: {
            LOG_DEBUG(TRACE, "BLE GAP event: physical updated.");
        } break;

        case BLE_GAP_EVT_DATA_LENGTH_UPDATE: {
            LOG_DEBUG(TRACE, "BLE GAP event: gap data length updated.");
            LOG_DEBUG(TRACE, "Max tx octets: %d", event->evt.gap_evt.params.data_length_update.effective_params.max_tx_octets);
            LOG_DEBUG(TRACE, "Max rx octets: %d", event->evt.gap_evt.params.data_length_update.effective_params.max_rx_octets);
            LOG_DEBUG(TRACE, "Max tx time: %d us", event->evt.gap_evt.params.data_length_update.effective_params.max_tx_time_us);
            LOG_DEBUG(TRACE, "Max rx time: %d us", event->evt.gap_evt.params.data_length_update.effective_params.max_rx_time_us);
        } break;

        case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST: {
            LOG_DEBUG(TRACE, "BLE GAP event: gap data length update request.");

            ble_gap_data_length_params_t gapDataLenParams;
            gapDataLenParams.max_tx_octets  = BLE_GAP_DATA_LENGTH_AUTO;
            gapDataLenParams.max_rx_octets  = BLE_GAP_DATA_LENGTH_AUTO;
            gapDataLenParams.max_tx_time_us = BLE_GAP_DATA_LENGTH_AUTO;
            gapDataLenParams.max_rx_time_us = BLE_GAP_DATA_LENGTH_AUTO;
            ret_code_t ret = sd_ble_gap_data_length_update(event->evt.gap_evt.conn_handle, &gapDataLenParams, NULL);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gap_data_length_update() failed: %u", (unsigned)ret);
            }
        } break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST: {
            LOG_DEBUG(TRACE, "BLE GAP event: security parameters request.");

            // Pairing is not supported
            ret_code_t ret = sd_ble_gap_sec_params_reply(event->evt.gap_evt.conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, nullptr, nullptr);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gap_sec_params_reply() failed: %u", (unsigned)ret);
            }
        } break;

        case BLE_GAP_EVT_AUTH_STATUS: {
            LOG_DEBUG(TRACE, "BLE GAP event: authentication status updated.");

            if (event->evt.gap_evt.params.auth_status.auth_status == BLE_GAP_SEC_STATUS_SUCCESS) {
                LOG_DEBUG(TRACE, "Authentication succeeded");
            }
            else {
                LOG_DEBUG(WARN, "Authentication failed, status: %d", (int)event->evt.gap_evt.params.auth_status.auth_status);
            }
        } break;

        case BLE_GAP_EVT_TIMEOUT: {
            LOG_DEBUG(ERROR, "BLE GAP event: timeout, source: %d", event->evt.gap_evt.params.timeout.src);
        } break;

        case BLE_GATTC_EVT_EXCHANGE_MTU_RSP: {
            LOG_DEBUG(TRACE, "BLE GATT Client event: exchange ATT MTU response.");
            LOG_DEBUG(TRACE, "Effective Server RX MTU: %d.", event->evt.gattc_evt.params.exchange_mtu_rsp.server_rx_mtu);
        } break;

        case BLE_GATTC_EVT_TIMEOUT: {
            LOG_DEBUG(TRACE, "BLE GATT Client event: timeout, conn handle: %d, source: %d",
                    event->evt.gattc_evt.conn_handle, event->evt.gattc_evt.params.timeout.src);

            // Disconnect on GATT Client timeout event.
            ret = sd_ble_gap_disconnect(event->evt.gattc_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gap_disconnect() failed: %u", (unsigned)ret);
            }
        } break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING: {
            LOG_DEBUG(TRACE, "BLE GATT Server event: system attribute is missing.");

            // No persistent system attributes
            const uint32_t ret = sd_ble_gatts_sys_attr_set(event->evt.gatts_evt.conn_handle, nullptr, 0, 0);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gatts_sys_attr_set() failed: %u", (unsigned)ret);
            }
        } break;

        case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST: {
            LOG_DEBUG(TRACE, "BLE GATT Server event: exchange ATT MTU request.");
            LOG_DEBUG(TRACE, "Effective Client RX MTU: %d.", event->evt.gatts_evt.params.exchange_mtu_request.client_rx_mtu);

            ret_code_t ret = sd_ble_gatts_exchange_mtu_reply(event->evt.gatts_evt.conn_handle, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gatts_exchange_mtu_reply() failed: %u", (unsigned)ret);
            }

            LOG_DEBUG(TRACE, "Reply with Server RX MTU: %d.", NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
        } break;

        case BLE_GATTS_EVT_WRITE: {
            LOG_DEBUG(TRACE, "BLE GATT Server event: write characteristic.");

            hal_ble_event_t bleEvt;
            bleEvt.evt_type               = BLE_EVT_TYPE_DATA;
            bleEvt.data_event.conn_handle = event->evt.gatts_evt.conn_handle;
            bleEvt.data_event.char_handle = event->evt.gatts_evt.params.write.handle;

            bleEvt.data_event.data_len    = event->evt.gatts_evt.params.write.len;
            memcpy(bleEvt.data_event.data, event->evt.gatts_evt.params.write.data, bleEvt.data_event.data_len);

            if (os_queue_put(s_bleEvtQueue, &bleEvt, 0, NULL)) {
                LOG(ERROR, "os_queue_put() failed.");
            }
        } break;

        case BLE_GATTS_EVT_HVN_TX_COMPLETE: {
            LOG_DEBUG(TRACE, "BLE GATT Server event: notification sent.");
        } break;

        case BLE_GATTS_EVT_HVC: {
            LOG_DEBUG(TRACE, "BLE GATT Server event: indication confirmed.");
            s_indInProgress = false;
        } break;

        case BLE_GATTS_EVT_TIMEOUT: {
            LOG_DEBUG(TRACE, "BLE GATT Server event: timeout, source: %d.", event->evt.gatts_evt.params.timeout.src);

            // Disconnect on GATT Server timeout event.
            ret = sd_ble_gap_disconnect(event->evt.gatts_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gap_disconnect() failed: %u", (unsigned)ret);
            }
        } break;

        default: {
            LOG_DEBUG(TRACE, "Unhandled BLE event: %u", (unsigned)event->header.evt_id);
        } break;
    }
}

static os_thread_return_t handleBleEventThread(void* param) {
    while (1) {
        hal_ble_event_t bleEvent;
        if (!os_queue_take(s_bleEvtQueue, &bleEvent, CONCURRENT_WAIT_FOREVER, NULL)) {
            dispatchBleEvent(&bleEvent);
        }
        else {
            LOG(ERROR, "BLE event thread exited.");
            break;
        }
    }

    os_thread_exit(s_bleEvtThread);
}

int hal_ble_init(uint8_t role, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());

    /* The SoftDevice has been enabled in core_hal.c */

    if (!s_bleInitialized) {
        LOG_DEBUG(TRACE, "Initializing BLE stack.");

        // Configure the BLE stack using the default settings.
        // Fetch the start address of the application RAM.
        uint32_t ramStart = 0;
        ret_code_t ret = nrf_sdh_ble_default_cfg_set(BLE_CONN_CFG_TAG, &ramStart);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "nrf_sdh_ble_default_cfg_set() failed: %u", (unsigned)ret);
            return sysError(ret);
        }
        LOG_DEBUG(TRACE, "RAM start: 0x%08x", (unsigned)ramStart);

        // Enable the stack
        ret = nrf_sdh_ble_enable(&ramStart);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "nrf_sdh_ble_enable() failed: %u", (unsigned)ret);
            return sysError(ret);
        }

        s_bleInitialized = true;

        // Register a handler for BLE events.
        NRF_SDH_BLE_OBSERVER(bleObserver, BLE_OBSERVER_PRIO, isrProcessBleEvent, nullptr);

        for (uint8_t i = 0; i < BLE_MAX_EVENT_CALLBACK_COUNT; i++) {
            s_bleEvtCallbacks[i] = NULL;
        }

        s_bleConnInfo.conn_handle = BLE_INVALID_CONN_HANDLE;

        // Configure an advertising set to obtain an advertising handle.
        if (role & BLE_ROLE_PERIPHERAL) {
            hal_ble_adv_params_t params;
            params.type          = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
            params.filter_policy = BLE_GAP_ADV_FP_ANY;
            params.interval      = BLE_DEFAULT_ADVERTISING_INTERVAL;
            params.duration      = BLE_DEFAULT_ADVERTISING_DURATION;
            params.inc_tx_power  = false;

            int error = ble_set_advertising_params(&params);
            if (error != SYSTEM_ERROR_NONE) {
                return error;
            }

            // Set the default TX Power
            ret = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, s_advHandle, s_advTxPower);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gap_tx_power_set() failed: %u", (unsigned)ret);
                return sysError(ret);
            }
        }

        if (os_queue_create(&s_bleEvtQueue, sizeof(hal_ble_event_t), BLE_EVENT_QUEUE_ITEM_COUNT, NULL)) {
            LOG(ERROR, "os_queue_create() failed");
            return SYSTEM_ERROR_NO_MEMORY;
        }

        if (os_thread_create(&s_bleEvtThread, "BLE Event Thread", OS_THREAD_PRIORITY_NETWORK, handleBleEventThread, NULL, BLE_EVENT_THREAD_STACK_SIZE)) {
            LOG(ERROR, "os_thread_create() failed");
            os_queue_destroy(s_bleEvtQueue, NULL);
            return SYSTEM_ERROR_INTERNAL;
        }

        if (os_timer_create(&s_connParamsUpdateTimer, BLE_CONN_PARAMS_UPDATE_DELAY_MS, connParamsUpdateTimerCb, NULL, true, NULL)) {
            LOG(ERROR, "os_timer_create() failed.");
        }

        return SYSTEM_ERROR_NONE;
    }
    else {
        return SYSTEM_ERROR_INVALID_STATE;
    }
}

int ble_register_callback(ble_event_callback_t callback) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);
    for (uint8_t i = 0; i < BLE_MAX_EVENT_CALLBACK_COUNT; i++) {
        if (s_bleEvtCallbacks[i] == NULL) {
            s_bleEvtCallbacks[i] = callback;
            return SYSTEM_ERROR_NONE;
        }
    }
    return SYSTEM_ERROR_LIMIT_EXCEEDED;
}

int ble_deregister_callback(ble_event_callback_t callback) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);
    for (uint8_t i = 0; i < BLE_MAX_EVENT_CALLBACK_COUNT; i++) {
        if (s_bleEvtCallbacks[i] == callback) {
            s_bleEvtCallbacks[i] = NULL;
            return SYSTEM_ERROR_NONE;
        }
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int ble_set_device_address(hal_ble_address_t const* addr) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_set_device_address().");

    if (addr->addr_type != BLE_SIG_ADDR_TYPE_PUBLIC && addr->addr_type != BLE_SIG_ADDR_TYPE_RANDOM_STATIC) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ble_gap_addr_t localAddr;
    localAddr.addr_id_peer = false;
    localAddr.addr_type    = addr->addr_type;
    memcpy(localAddr.addr, addr->addr, BLE_SIG_ADDR_LEN);

    ret_code_t ret = sd_ble_gap_addr_set(&localAddr);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_addr_set() failed: %u", (unsigned)ret);
    }

    return sysError(ret);
}

int ble_get_device_address(hal_ble_address_t* addr) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_get_device_address().");

    ble_gap_addr_t localAddr;
    ret_code_t ret = sd_ble_gap_addr_get(&localAddr);
    if (ret == NRF_SUCCESS) {
        addr->addr_type = localAddr.addr_type;
        memcpy(addr->addr, localAddr.addr, BLE_SIG_ADDR_LEN);
    }
    else {
        LOG(ERROR, "sd_ble_gap_addr_get() failed: %u", (unsigned)ret);
    }

    return sysError(ret);
}

int ble_set_device_name(uint8_t const* device_name, uint16_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_set_device_name().");

    ble_gap_conn_sec_mode_t secMode;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&secMode);

    ret_code_t ret = sd_ble_gap_device_name_set(&secMode, device_name, len);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_device_name_set() failed: %u", (unsigned)ret);
    }

    return sysError(ret);
}

int ble_get_device_name(uint8_t* device_name, uint16_t* len) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_get_device_name().");

    // non NULL-terminated string returned.
    ret_code_t ret = sd_ble_gap_device_name_get(device_name, len);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_device_name_get() failed: %u", (unsigned)ret);
    }

    return sysError(ret);
}

int ble_set_appearance(uint16_t appearance) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_set_appearance().");

    ret_code_t ret = sd_ble_gap_appearance_set(appearance);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_appearance_set() failed: %u", (unsigned)ret);
    }

    return sysError(ret);
}

int ble_get_appearance(uint16_t* appearance) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_get_appearance().");

    ret_code_t ret = sd_ble_gap_appearance_get(appearance);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_appearance_get() failed: %u", (unsigned)ret);
    }

    return sysError(ret);
}

int ble_add_whitelist(hal_ble_address_t* addr_list, uint8_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_add_whitelist().");

    if (addr_list == NULL || len == 0 || len > BLE_MAX_WHITELIST_ADDR_COUNT) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    for (uint8_t i = 0; i < len; i++) {
        s_whitelist[i].addr_id_peer = true;
        s_whitelist[i].addr_type = addr_list[i].addr_type;
        memcpy(s_whitelist[i].addr, addr_list[i].addr, BLE_SIG_ADDR_LEN);
        s_whitelist_ptr[i] = &s_whitelist[i];
    }

    ret_code_t ret = sd_ble_gap_whitelist_set(s_whitelist_ptr, len);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_whitelist_set() failed: %u", (unsigned)ret);
    }

    return sysError(ret);
}

int ble_delete_whitelist(void) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_delete_whitelist().");

    ret_code_t ret = sd_ble_gap_whitelist_set(NULL, 0);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_whitelist_set() failed: %u", (unsigned)ret);
    }

    return sysError(ret);
}

int ble_set_tx_power(int8_t value) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_set_tx_power().");

    ret_code_t ret = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, s_advHandle, roundTxPower(value));
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_tx_power_set() failed: %u", (unsigned)ret);
    }

    return sysError(ret);
}

int ble_get_tx_power(int8_t* value) {
    std::lock_guard<bleLock> lk(bleLock());

    LOG_DEBUG(TRACE, "ble_get_tx_power().");

    *value = s_advTxPower;

    return SYSTEM_ERROR_NONE;
}

int ble_set_advertising_params(hal_ble_adv_params_t* adv_params) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_set_advertising_params().");

    ret_code_t ret;

    // It is invalid to change advertising set parameters while advertising.
    if (s_isAdvertising) {
        int err = ble_stop_advertising();
        if (err != SYSTEM_ERROR_NONE) {
            return err;
        }
        s_advPending = true;
    }

    if (adv_params == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ble_gap_adv_params_t bleGapAdvParams;
    memset(&bleGapAdvParams, 0x00, sizeof(ble_gap_adv_params_t));

    bleGapAdvParams.properties.type             = adv_params->type;
    bleGapAdvParams.properties.include_tx_power = adv_params->inc_tx_power;
    bleGapAdvParams.p_peer_addr                 = NULL;
    bleGapAdvParams.interval                    = adv_params->interval;
    bleGapAdvParams.duration                    = adv_params->duration;
    bleGapAdvParams.filter_policy               = adv_params->filter_policy;
    bleGapAdvParams.primary_phy                 = BLE_GAP_PHY_1MBPS;

    LOG_DEBUG(TRACE, "BLE advertising interval: (%d x 0.625) ms.", bleGapAdvParams.interval);
    if (bleGapAdvParams.duration == 0) {
        LOG_DEBUG(TRACE, "BLE advertising duration: infinite.");
    }
    else {
        LOG_DEBUG(TRACE, "BLE advertising duration: (%d x 10) ms.", bleGapAdvParams.duration);
    }

    // Make sure the advertising data and scan response data is consistent.
    ble_gap_adv_data_t bleGapAdvData;
    bleGapAdvData.adv_data.p_data      = s_advData;
    bleGapAdvData.adv_data.len         = s_advDataLen;
    bleGapAdvData.scan_rsp_data.p_data = s_scanRespData;
    bleGapAdvData.scan_rsp_data.len    = s_scanRespDataLen;

    ret = sd_ble_gap_adv_set_configure(&s_advHandle, &bleGapAdvData, &bleGapAdvParams);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_adv_set_configure() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    if (s_advPending) {
        s_advPending = false;
        return ble_start_advertising();
    }

    return sysError(ret);
}

int ble_set_adv_data_snippet(uint8_t adType, uint8_t* data, uint16_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_set_adv_data_snippet().");

    return adStructEncode(adType, data, len, BLE_ADV_DATA_ADVERTISING);
}

int ble_refresh_adv_data(void) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_refresh_adv_data().");

    return setAdvData(s_advData, s_advDataLen, BLE_ADV_DATA_ADVERTISING);
}

int ble_set_adv_data(uint8_t* data, uint16_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_set_adv_data().");

    return setAdvData(data, len, BLE_ADV_DATA_ADVERTISING);
}

int ble_set_scan_resp_data_snippet(uint8_t adType, uint8_t* data, uint16_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_set_scan_resp_data_snippet().");

    return adStructEncode(adType, data, len, BLE_ADV_DATA_SCAN_RESPONSE);
}

int ble_refresh_scan_resp_data(void) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_refresh_scan_resp_data().");

    return setAdvData(s_scanRespData, s_scanRespDataLen, BLE_ADV_DATA_SCAN_RESPONSE);
}

int ble_set_scan_resp_data(uint8_t* data, uint16_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_set_scan_resp_data().");

    return setAdvData(data, len, BLE_ADV_DATA_SCAN_RESPONSE);
}

int ble_start_advertising(void) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_start_advertising().");

    if (s_isAdvertising) {
        return SYSTEM_ERROR_NONE;
    }

    ret_code_t ret = sd_ble_gap_adv_start(s_advHandle, BLE_CONN_CFG_TAG);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_adv_start() failed: %u", (unsigned)ret);
        return sysError(ret);
    }
    else {
        s_isAdvertising = true;
        return SYSTEM_ERROR_NONE;
    }
}

int ble_stop_advertising(void) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_stop_advertising().");

    if (!s_isAdvertising) {
        return SYSTEM_ERROR_NONE;
    }

    ret_code_t ret = sd_ble_gap_adv_stop(s_advHandle);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_adv_stop() failed: %u", (unsigned)ret);
        return sysError(ret);
    }
    else {
        s_isAdvertising = false;
        return SYSTEM_ERROR_NONE;
    }
}

bool ble_is_advertising(void) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    return s_isAdvertising;
}

int ble_set_scanning_params(hal_ble_scan_params_t* scan_params) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_set_scanning_params().");

    if (scan_params->interval < BLE_GAP_SCAN_INTERVAL_MIN) {
        scan_params->interval = BLE_GAP_SCAN_INTERVAL_MIN;
    }
    if (scan_params->interval > BLE_GAP_SCAN_INTERVAL_MAX) {
        scan_params->interval = BLE_GAP_SCAN_INTERVAL_MAX;
    }
    if (scan_params->window < BLE_GAP_SCAN_WINDOW_MIN) {
        scan_params->window = BLE_GAP_SCAN_WINDOW_MIN;
    }
    if (scan_params->window > BLE_GAP_SCAN_WINDOW_MAX) {
        scan_params->window = BLE_GAP_SCAN_WINDOW_MAX;
    }

    memcpy(&s_scanParams, scan_params, sizeof(hal_ble_scan_params_t));

    return SYSTEM_ERROR_NONE;
}

int ble_start_scanning(void) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_start_scanning().");

    ble_gap_scan_params_t bleGapScanParams;
    bleGapScanParams.active        = s_scanParams.active;
    bleGapScanParams.interval      = s_scanParams.interval;
    bleGapScanParams.window        = s_scanParams.window;
    bleGapScanParams.timeout       = s_scanParams.timeout;
    bleGapScanParams.scan_phys     = BLE_GAP_PHY_1MBPS;
    bleGapScanParams.filter_policy = s_scanParams.filter_policy;

    LOG_DEBUG(TRACE, "BLE scan interval: (%d x 0.625) ms.", bleGapScanParams.interval);
    LOG_DEBUG(TRACE, "BLE scan window:   (%d x 0.625) ms.", bleGapScanParams.window);
    LOG_DEBUG(TRACE, "BLE scan timeout:  (%d x 10) ms.", bleGapScanParams.timeout);

    ret_code_t ret = sd_ble_gap_scan_start(&bleGapScanParams, &s_bleScanData);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_scan_start() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    return SYSTEM_ERROR_NONE;
}

int ble_stop_scanning(void) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_stop_scanning().");

    ret_code_t ret = sd_ble_gap_scan_stop();
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_scan_start() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    return SYSTEM_ERROR_NONE;
}

int ble_connect(hal_ble_address_t* addr) {
    std::lock_guard<bleLock> lk(bleLock());
    return 0;
}

int ble_connect_cancel(void) {
    std::lock_guard<bleLock> lk(bleLock());

    LOG_DEBUG(TRACE, "ble_connect_cancel().");

    return 0;
}

int ble_set_ppcp(hal_ble_conn_params_t* conn_params) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_set_ppcp().");

    if (conn_params == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if (conn_params->min_conn_interval < BLE_SIG_CP_MIN_CONN_INTERVAL_MIN ||
            conn_params->min_conn_interval > BLE_SIG_CP_MIN_CONN_INTERVAL_MAX) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (conn_params->max_conn_interval < BLE_SIG_CP_MAX_CONN_INTERVAL_MIN ||
            conn_params->max_conn_interval > BLE_SIG_CP_MAX_CONN_INTERVAL_MAX) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (conn_params->slave_latency > BLE_SIG_CP_SLAVE_LATENCY_MAX) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (conn_params->conn_sup_timeout < BLE_SIG_CP_CONN_SUP_TIMEOUT_MIN ||
            conn_params->conn_sup_timeout > BLE_SIG_CP_CONN_SUP_TIMEOUT_MAX) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ble_gap_conn_params_t connParams;
    connParams.min_conn_interval = conn_params->min_conn_interval;
    connParams.max_conn_interval = conn_params->max_conn_interval;
    connParams.slave_latency     = conn_params->slave_latency;
    connParams.conn_sup_timeout  = conn_params->conn_sup_timeout;

    ret_code_t ret = sd_ble_gap_ppcp_set(&connParams);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_ppcp_set() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    return SYSTEM_ERROR_NONE;
}

int ble_get_ppcp(hal_ble_conn_params_t* conn_params) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_get_ppcp().");

    if (conn_params == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ble_gap_conn_params_t connParams;
    ret_code_t ret = sd_ble_gap_ppcp_get(&connParams);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_ppcp_set() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    conn_params->min_conn_interval = connParams.min_conn_interval;
    conn_params->max_conn_interval = connParams.max_conn_interval;
    conn_params->slave_latency     = connParams.slave_latency;
    conn_params->conn_sup_timeout  = connParams.conn_sup_timeout;

    return SYSTEM_ERROR_NONE;
}

int ble_update_connection_params(uint16_t conn_handle, hal_ble_conn_params_t* conn_params) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_update_connection_params().");

    ret_code_t ret;

    if (conn_handle == BLE_INVALID_CONN_HANDLE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if (conn_params == NULL) {
        // For Central role, this will reject the connection parameter update request.
        // For Peripheral role, this will use the PPCP characteristic value as the connection parameters,
        // and send the request to Central.
        ret = sd_ble_gap_conn_param_update(conn_handle, NULL);
    }
    else {
        // For Central role, this will initiate the connection parameter update procedure.
        // For Peripheral role, this will use the passed in parameters and send the request to central.
        ble_gap_conn_params_t connParams;
        connParams.min_conn_interval = conn_params->min_conn_interval;
        connParams.max_conn_interval = conn_params->max_conn_interval;
        connParams.slave_latency     = conn_params->slave_latency;
        connParams.conn_sup_timeout  = conn_params->conn_sup_timeout;
        ret = sd_ble_gap_conn_param_update(conn_handle, &connParams);
    }

    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_conn_param_update() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    return SYSTEM_ERROR_NONE;
}

int hal_ble_disconnect(uint16_t conn_handle, uint8_t reason) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "hal_ble_disconnect().");

    if (conn_handle == BLE_INVALID_CONN_HANDLE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ret_code_t ret = sd_ble_gap_disconnect(conn_handle, reason);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_disconnect() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    return SYSTEM_ERROR_NONE;
}

int ble_add_service_uuid128(uint8_t type, const uint8_t* uuid128, uint16_t* handle) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_add_service_uuid128().");

    ret_code_t ret;
    ble_uuid_t svcUuid;

    if (uuid128 == NULL || handle == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ble_uuid128_t uuid;
    memcpy(uuid.uuid128, uuid128, BLE_SIG_UUID_128BIT_LEN);
    ret = sd_ble_uuid_vs_add(&uuid, &svcUuid.type);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_uuid_vs_add() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    ret = sd_ble_uuid_decode(BLE_SIG_UUID_128BIT_LEN, uuid128, &svcUuid);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_uuid_decode() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    return addService(type, &svcUuid, handle);
}

int ble_add_service_uuid16(uint8_t type, uint16_t uuid16, uint16_t* handle) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_add_service_uuid16().");

    if (handle == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ble_uuid_t svcUuid;
    svcUuid.type = BLE_UUID_TYPE_BLE;
    svcUuid.uuid = uuid16;

    return addService(type, &svcUuid, handle);
}

int ble_add_char_uuid128(uint16_t service_handle, const uint8_t *uuid128, uint8_t properties, hal_ble_char_t* ble_char) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_add_char_uuid128().");

    ret_code_t ret;
    ble_uuid_t charUuid;

    if (uuid128 == NULL || ble_char == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ble_uuid128_t uuid;
    memcpy(uuid.uuid128, uuid128, BLE_SIG_UUID_128BIT_LEN);
    ret = sd_ble_uuid_vs_add(&uuid, &charUuid.type);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_uuid_vs_add() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    ret = sd_ble_uuid_decode(BLE_SIG_UUID_128BIT_LEN, uuid128, &charUuid);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_uuid_decode() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    int error = addCharacteristic(service_handle, &charUuid, properties, ble_char);
    if (SYSTEM_ERROR_NONE == error) {
        ble_char->uuid.type = BLE_UUID_TYPE_128BIT;
        memcpy(ble_char->uuid.uuid128, uuid128, BLE_SIG_UUID_128BIT_LEN);
    }

    return error;
}

int ble_add_char_uuid16(uint16_t service_handle, uint16_t uuid16, uint8_t properties, hal_ble_char_t* ble_char) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_add_char_uuid16().");

    if (ble_char == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ble_uuid_t charUuid;
    charUuid.type = BLE_UUID_TYPE_BLE;
    charUuid.uuid = uuid16;

    int error = addCharacteristic(service_handle, &charUuid, properties, ble_char);
    if (SYSTEM_ERROR_NONE == error) {
        ble_char->uuid.type   = BLE_UUID_TYPE_16BIT;
        ble_char->uuid.uuid16 = uuid16;
    }

    return error;
}

int ble_add_char_desc(uint8_t* desc, uint16_t len, hal_ble_char_t* ble_char) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_add_char_desc().");

    if (ble_char == NULL || desc == NULL || ble_char->value_handle == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ret_code_t ret;
    ble_gatts_attr_t desc_attr;
    ble_gatts_attr_md_t desc_attr_md;
    ble_uuid_t descUuid;

    if (ble_char->uuid.type == BLE_UUID_TYPE_16BIT) {
        descUuid.type = BLE_UUID_TYPE_BLE;
        descUuid.uuid = ble_char->uuid.uuid16;
    }
    else {
        ret = sd_ble_uuid_decode(BLE_SIG_UUID_128BIT_LEN, ble_char->uuid.uuid128, &descUuid);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "sd_ble_uuid_decode() failed: %u", (unsigned)ret);
            return sysError(ret);
        }
    }

    memset(&desc_attr_md, 0, sizeof(desc_attr_md));
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&desc_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&desc_attr_md.write_perm);
    desc_attr_md.vloc = BLE_GATTS_VLOC_STACK;
    desc_attr_md.rd_auth = 0;
    desc_attr_md.wr_auth = 0;
    desc_attr_md.vlen    = 0;

    memset(&desc_attr, 0, sizeof(desc_attr));
    desc_attr.p_uuid    = &descUuid;
    desc_attr.p_attr_md = &desc_attr_md;
    desc_attr.init_len  = len;
    desc_attr.init_offs = 0;
    desc_attr.max_len   = len;
    desc_attr.p_value   = desc;

    ret = sd_ble_gatts_descriptor_add(ble_char->value_handle, &desc_attr, &ble_char->user_desc_handle);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_descriptor_add() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    return SYSTEM_ERROR_NONE;
}

int ble_publish(uint16_t conn_handle, hal_ble_char_t* ble_char, uint8_t* data, uint16_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_publish().");

    if (ble_char == NULL || data == NULL || conn_handle == BLE_INVALID_CONN_HANDLE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if (!(ble_char->properties & BLE_SIG_CHAR_PROP_NOTIFY) && !(ble_char->properties & BLE_SIG_CHAR_PROP_INDICATE)) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }

    // Check if the client has enabled notification or indication
    uint8_t cccd[2];
    ble_gatts_value_t gattValue;
    gattValue.p_value = cccd;
    gattValue.len = sizeof(cccd);
    gattValue.offset = 0;
    ret_code_t ret = sd_ble_gatts_value_get(conn_handle, ble_char->cccd_handle, &gattValue);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_value_get() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    uint16_t cccd_value = uint16_decode(cccd);
    if (cccd_value == BLE_SIG_CCCD_VAL_DISABLED || cccd_value > BLE_SIG_CCCD_VAL_INDICATION || s_indInProgress) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    else {
        ble_gatts_hvx_params_t hvxParams;

        if (cccd_value == BLE_SIG_CCCD_VAL_NOTIFICATION) {
            hvxParams.type = BLE_GATT_HVX_NOTIFICATION;
        }
        else {
            // Only one indication procedure can be ongoing per connection at a time.
            hvxParams.type = BLE_GATT_HVX_INDICATION;
        }

        uint16_t hvxLen = len;
        hvxParams.handle = ble_char->value_handle;
        hvxParams.offset = 0;
        hvxParams.p_data = data;
        hvxParams.p_len  = &hvxLen;

        ret_code_t ret = sd_ble_gatts_hvx(conn_handle, &hvxParams);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "sd_ble_gatts_hvx() failed: %u", (unsigned)ret);
            return sysError(ret);
        }

        if (cccd_value == BLE_SIG_CCCD_VAL_INDICATION) {
            s_indInProgress = true;
        }
    }

    return SYSTEM_ERROR_NONE;
}

int ble_set_characteristic_value(hal_ble_char_t* ble_char, uint8_t* data, uint16_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_set_characteristic_value().");

    if (ble_char == NULL || data == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ble_gatts_value_t gattValue;
    gattValue.len     = len;
    gattValue.offset  = 0;
    gattValue.p_value = data;

    ret_code_t ret = sd_ble_gatts_value_set(0, ble_char->value_handle, &gattValue);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_value_set() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    return SYSTEM_ERROR_NONE;
}

int ble_get_characteristic_value(hal_ble_char_t* ble_char, uint8_t* data, uint16_t* len) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInitialized);

    LOG_DEBUG(TRACE, "ble_get_characteristic_value().");

    if (ble_char == NULL || data == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ble_gatts_value_t gattValue;
    gattValue.len     = *len;
    gattValue.offset  = 0;
    gattValue.p_value = data;

    ret_code_t ret = sd_ble_gatts_value_get(0, ble_char->value_handle, &gattValue);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_value_get() failed: %u", (unsigned)ret);
        *len = 0;
        return sysError(ret);
    }

    *len = gattValue.len;

    return SYSTEM_ERROR_NONE;
}

int ble_subscribe(uint16_t conn_handle, uint8_t char_handle) {
    std::lock_guard<bleLock> lk(bleLock());

    LOG_DEBUG(TRACE, "ble_subscribe().");

    return 0;
}

int ble_unsubscribe(uint16_t conn_handle, uint8_t char_handle) {
    std::lock_guard<bleLock> lk(bleLock());

    LOG_DEBUG(TRACE, "ble_unsubscribe().");

    return 0;
}

int ble_write_with_response(uint16_t conn_handle, uint8_t char_handle, uint8_t* data, uint16_t len) {
    std::lock_guard<bleLock> lk(bleLock());

    LOG_DEBUG(TRACE, "ble_write_with_response().");

    return 0;
}

int ble_write_without_response(uint16_t conn_handle, uint8_t char_handle, uint8_t* data, uint16_t len) {
    std::lock_guard<bleLock> lk(bleLock());

    LOG_DEBUG(TRACE, "ble_write_without_response().");

    return 0;
}

int ble_read(uint16_t conn_handle, uint8_t char_handle, uint8_t* data, uint16_t* len) {
    std::lock_guard<bleLock> lk(bleLock());

    LOG_DEBUG(TRACE, "ble_read().");

    return 0;
}

int ble_get_rssi(uint16_t conn_handle) {
    std::lock_guard<bleLock> lk(bleLock());

    LOG_DEBUG(TRACE, "ble_get_rssi().");

    return 0;
}

int ble_discovery_services(uint16_t conn_handle) {
    std::lock_guard<bleLock> lk(bleLock());

    LOG_DEBUG(TRACE, "ble_discovery_services().");

    return 0;
}

int ble_discovery_characteristics(uint16_t conn_handle, uint16_t service_handle) {
    std::lock_guard<bleLock> lk(bleLock());

    LOG_DEBUG(TRACE, "ble_discovery_characteristics().");

    return 0;
}

int ble_discovery_descriptors(uint16_t conn_handle, uint16_t char_handle) {
    std::lock_guard<bleLock> lk(bleLock());

    LOG_DEBUG(TRACE, "ble_discovery_descriptors().");

    return 0;
}


