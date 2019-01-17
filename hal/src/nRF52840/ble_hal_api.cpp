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

/* BLE connection status */
typedef struct {
    uint8_t  initialized   : 1;
    uint8_t  advertising   : 1;
    uint8_t  scanning      : 1;
    uint8_t  connecting    : 1;
    uint8_t  connected     : 1;
    uint8_t  indConfirmed  : 1;
    int8_t   txPower;
    uint8_t  role;
    uint16_t connHandle;
    uint8_t  advHandle;
    uint8_t  connParamsUpdateAttempts;

    hal_ble_scan_params_t scanParams;
    hal_ble_adv_params_t  advParams;
    hal_ble_conn_params_t ppcp;

    uint8_t               advData[BLE_MAX_ADV_DATA_LEN];
    uint16_t              advDataLen;
    uint8_t               scanRespData[BLE_MAX_ADV_DATA_LEN];
    uint16_t              scanRespDataLen;

    os_queue_t            evtQueue;
    os_thread_t           evtThread;
    os_timer_t            connParamsUpdateTimer;

    hal_ble_conn_params_t effectiveConnParams;
    ble_gap_addr_t        whitelist[BLE_MAX_WHITELIST_ADDR_COUNT];
    ble_gap_addr_t const* whitelistPointer[BLE_MAX_WHITELIST_ADDR_COUNT];
    ble_event_callback_t  evtCallbacks[BLE_MAX_EVENT_CALLBACK_COUNT];
} HalBleInstance_t;


static HalBleInstance_t s_bleInstance = {
    .initialized  = 0,
    .advertising  = 0,
    .scanning     = 0,
    .connecting   = 0,
    .connected    = 0,
    .indConfirmed = 1,
    .txPower      = 0,
    .role         = BLE_ROLE_INVALID,
    .connHandle   = BLE_INVALID_CONN_HANDLE,
    .advHandle    = BLE_GAP_ADV_SET_HANDLE_NOT_SET,
    .connParamsUpdateAttempts = 0,

    .scanParams = {
        .active        = true,
        .filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL,
        .interval      = BLE_DEFAULT_SCANNING_INTERVAL,
        .window        = BLE_DEFAULT_SCANNING_WINDOW,
        .timeout       = BLE_DEFAULT_SCANNING_TIMEOUT
    },

    .advParams = {
        .type          = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED,
        .filter_policy = BLE_GAP_ADV_FP_ANY,
        .interval      = BLE_DEFAULT_ADVERTISING_INTERVAL,
        .duration      = BLE_DEFAULT_ADVERTISING_DURATION,
        .inc_tx_power  = false
    },

    /*
     * For BLE Central, this is the initial connection parameters.
     * For BLE Peripheral, this is the Peripheral Preferred Connection Parameters.
     */
    .ppcp = {
        .min_conn_interval = BLE_DEFAULT_MIN_CONN_INTERVAL,
        .max_conn_interval = BLE_DEFAULT_MAX_CONN_INTERVAL,
        .slave_latency     = BLE_DEFAULT_SLAVE_LATENCY,
        .conn_sup_timeout  = BLE_DEFAULT_CONN_SUP_TIMEOUT
    },

    .advData = {
        0x02,
        BLE_SIG_AD_TYPE_FLAGS,
        BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE
    },
    .advDataLen = 3,

    .scanRespData = { 0 },
    .scanRespDataLen = 0,

    .evtQueue  = NULL,
    .evtThread = NULL,
    .connParamsUpdateTimer = NULL,
};

static uint8_t    s_scanReportBuff[BLE_MAX_SCAN_REPORT_BUF_LEN];
static ble_data_t s_bleScanData = {
    .p_data = s_scanReportBuff,
    .len = BLE_MAX_SCAN_REPORT_BUF_LEN
};


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

static int locateAdStructure(uint8_t flag, const uint8_t* data, uint16_t len, uint16_t* offset, uint16_t* adsLen) {
    if (flag == BLE_SIG_AD_TYPE_FLAGS) {
        *offset = 0;
        *adsLen = 3;
        return SYSTEM_ERROR_NONE;
    }

    // A valid AD structure is composed of Length field, Type field and Data field.
    // Each field should be filled with at least one byte.
    for (uint16_t i = 0; (i + 3) <= len; i = i) {
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
        pAdvData    = s_bleInstance.advData;
        pAdvDataLen = &s_bleInstance.advDataLen;
    }
    else if (flag == BLE_ADV_DATA_SCAN_RESPONSE && adType != BLE_SIG_AD_TYPE_FLAGS) {
        // The scan response data should not contain the AD Flags AD structure.
        pAdvData    = s_bleInstance.scanRespData;
        pAdvDataLen = &s_bleInstance.scanRespDataLen;
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

    bool advPending = false;

    // It is invalid to provide the same data buffers while advertising.
    if (s_bleInstance.advertising) {
        int err = ble_stop_advertising();
        if (err != SYSTEM_ERROR_NONE) {
            return err;
        }
        advPending = true;
    }

    // Make sure the scan response data is consistent.
    ble_gap_adv_data_t bleGapAdvData;
    if (flag == BLE_ADV_DATA_ADVERTISING) {
        bleGapAdvData.adv_data.p_data      = data;
        bleGapAdvData.adv_data.len         = len;
        bleGapAdvData.scan_rsp_data.p_data = s_bleInstance.scanRespData;
        bleGapAdvData.scan_rsp_data.len    = s_bleInstance.scanRespDataLen;
    }
    else {
        bleGapAdvData.adv_data.p_data      = s_bleInstance.advData;
        bleGapAdvData.adv_data.len         = s_bleInstance.advDataLen;
        bleGapAdvData.scan_rsp_data.p_data = data;
        bleGapAdvData.scan_rsp_data.len    = len;
    }

    ret_code_t ret = sd_ble_gap_adv_set_configure(&s_bleInstance.advHandle, &bleGapAdvData, NULL);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_adv_set_configure() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    if (flag == BLE_ADV_DATA_ADVERTISING) {
        memcpy(s_bleInstance.advData, data, len);
        s_bleInstance.advDataLen = len;
    }
    else {
        memcpy(s_bleInstance.scanRespData, data, len);
        s_bleInstance.scanRespDataLen = len;
    }

    if (advPending) {
        advPending = false;
        return ble_start_advertising();
    }

    return SYSTEM_ERROR_NONE;
}

static int fromHalScanParams(hal_ble_scan_params_t* halScanParams, ble_gap_scan_params_t* scanParams) {
    if (halScanParams == NULL || scanParams == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    memset(scanParams, 0x00, sizeof(ble_gap_scan_params_t));

    scanParams->extended      = 0;
    scanParams->active        = halScanParams->active;
    scanParams->interval      = halScanParams->interval;
    scanParams->window        = halScanParams->window;
    scanParams->timeout       = halScanParams->timeout;
    scanParams->scan_phys     = BLE_GAP_PHY_1MBPS;
    scanParams->filter_policy = halScanParams->filter_policy;

    return SYSTEM_ERROR_NONE;
}

static int fromHalConnParams(hal_ble_conn_params_t* halConnParams, ble_gap_conn_params_t* connParams) {
    if (halConnParams == NULL || connParams == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    memset(connParams, 0x00, sizeof(ble_gap_conn_params_t));

    connParams->min_conn_interval = halConnParams->min_conn_interval;
    connParams->max_conn_interval = halConnParams->max_conn_interval;
    connParams->slave_latency     = halConnParams->slave_latency;
    connParams->conn_sup_timeout  = halConnParams->conn_sup_timeout;

    return SYSTEM_ERROR_NONE;
}

static bool isConnParamsFeeded(hal_ble_conn_params_t* conn_params) {
    hal_ble_conn_params_t ppcp;

    if (ble_get_ppcp(&ppcp) == SYSTEM_ERROR_NONE) {
        uint16_t minAcceptedSl = ppcp.slave_latency - MIN(BLE_CONN_PARAMS_SLAVE_LATENCY_ERR, ppcp.slave_latency);
        uint16_t maxAcceptedSl = ppcp.slave_latency + BLE_CONN_PARAMS_SLAVE_LATENCY_ERR;
        uint16_t minAcceptedTo = ppcp.conn_sup_timeout - MIN(BLE_CONN_PARAMS_TIMEOUT_ERR, ppcp.conn_sup_timeout);
        uint16_t maxAcceptedTo = ppcp.conn_sup_timeout + BLE_CONN_PARAMS_TIMEOUT_ERR;

        if (conn_params->max_conn_interval < ppcp.min_conn_interval ||
                conn_params->max_conn_interval > ppcp.max_conn_interval) {
            return false;
        }
        if (conn_params->slave_latency < minAcceptedSl ||
                conn_params->slave_latency > maxAcceptedSl) {
            return false;
        }
        if (conn_params->conn_sup_timeout < minAcceptedTo ||
                conn_params->conn_sup_timeout > maxAcceptedTo) {
            return false;
        }
    }

    return true;
}

static void connParamsUpdateTimerCb(os_timer_t timer) {
    if (s_bleInstance.connHandle == BLE_INVALID_CONN_HANDLE) {
        return;
    }

    hal_ble_conn_params_t ppcp;
    if (ble_get_ppcp(&ppcp) == SYSTEM_ERROR_NONE) {
        ble_gap_conn_params_t connParams;
        connParams.min_conn_interval = ppcp.min_conn_interval;
        connParams.max_conn_interval = ppcp.max_conn_interval;
        connParams.slave_latency     = ppcp.slave_latency;
        connParams.conn_sup_timeout  = ppcp.conn_sup_timeout;

        ret_code_t ret = sd_ble_gap_conn_param_update(s_bleInstance.connHandle, &connParams);
        if (ret != NRF_SUCCESS) {
            LOG(WARN, "sd_ble_gap_conn_param_update() failed: %u", (unsigned)ret);
            return;
        }

        s_bleInstance.connParamsUpdateAttempts++;
    }
}

static void updateConnParamsIfNeeded(void) {
    if (s_bleInstance.role == BLE_ROLE_PERIPHERAL && !isConnParamsFeeded(&s_bleInstance.effectiveConnParams)) {
        if (s_bleInstance.connParamsUpdateAttempts < BLE_CONN_PARAMS_UPDATE_ATTEMPS) {
            if (s_bleInstance.connParamsUpdateTimer != NULL) {
                if (os_timer_change(s_bleInstance.connParamsUpdateTimer, OS_TIMER_CHANGE_START, true, 0, 0, NULL)) {
                    LOG(ERROR, "os_timer_change() failed.");
                    return;
                }
            }
            LOG_DEBUG(TRACE, "Attempts to update BLE connection parameters, try: %d after %d ms",
                    s_bleInstance.connParamsUpdateAttempts, BLE_CONN_PARAMS_UPDATE_DELAY_MS);
        }
        else {
            ret_code_t ret = sd_ble_gap_disconnect(s_bleInstance.connHandle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
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

static int addCharacteristic(uint16_t service_handle, const ble_uuid_t* uuid, uint8_t properties, const char* desc, hal_ble_char_t* ble_char) {
    ret_code_t               ret;
    ble_gatts_char_md_t      char_md;
    ble_gatts_attr_md_t      value_attr_md;
    ble_gatts_attr_md_t      user_desc_attr_md;
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
    if (desc != NULL) {
        memset(&user_desc_attr_md, 0, sizeof(user_desc_attr_md));
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&user_desc_attr_md.read_perm);
        BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&user_desc_attr_md.write_perm);
        user_desc_attr_md.vloc    = BLE_GATTS_VLOC_STACK;
        user_desc_attr_md.rd_auth = 0;
        user_desc_attr_md.wr_auth = 0;
        user_desc_attr_md.vlen    = 0;

        char_md.p_char_user_desc        = (const uint8_t *)desc;
        char_md.char_user_desc_max_size = strlen(desc);
        char_md.char_user_desc_size     = strlen(desc);
        char_md.p_user_desc_md          = &user_desc_attr_md;
    }
    else {
        char_md.p_char_user_desc = NULL;
        char_md.p_user_desc_md   = NULL;
    }
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
        if (s_bleInstance.evtCallbacks[i] != NULL) {
            s_bleInstance.evtCallbacks[i](evt);
        }
    }
}

static void isrProcessBleEvent(const ble_evt_t* event, void* context) {
    ret_code_t      ret;
    hal_ble_event_t bleEvt;

    switch (event->header.evt_id) {
        case BLE_GAP_EVT_ADV_SET_TERMINATED: {
            LOG_DEBUG(TRACE, "BLE GAP event: advertising stopped.");
            s_bleInstance.advertising = false;
        } break;

        case BLE_GAP_EVT_ADV_REPORT: {
            LOG_DEBUG(TRACE, "BLE GAP event: advertising report.");

            bleEvt.evt_type = BLE_EVT_TYPE_SCAN_RESULT;
            if (event->evt.gap_evt.params.adv_report.type.scan_response) {
                bleEvt.scan_result_event.data_type = BLE_SCAN_RESULT_EVT_DATA_TYPE_SCAN_RESP;
            }
            else {
                bleEvt.scan_result_event.data_type = BLE_SCAN_RESULT_EVT_DATA_TYPE_ADV;
            }
            bleEvt.scan_result_event.peer_addr.addr_type = event->evt.gap_evt.params.adv_report.peer_addr.addr_type;
            memcpy(bleEvt.scan_result_event.peer_addr.addr, event->evt.gap_evt.params.adv_report.peer_addr.addr, BLE_SIG_ADDR_LEN);
            bleEvt.scan_result_event.rssi = event->evt.gap_evt.params.adv_report.rssi;
            bleEvt.scan_result_event.data_len = event->evt.gap_evt.params.adv_report.data.len;
            memcpy(bleEvt.scan_result_event.data, event->evt.gap_evt.params.adv_report.data.p_data, bleEvt.scan_result_event.data_len);
            if (os_queue_put(s_bleInstance.evtQueue, &bleEvt, 0, NULL)) {
                LOG(ERROR, "os_queue_put() failed.");
            }

            // Continue scanning, scanning parameters pointer must be set to NULL.
            ret_code_t ret = sd_ble_gap_scan_start(NULL, &s_bleScanData);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gap_scan_start() failed: %u", (unsigned)ret);
            }
        } break;

        case BLE_GAP_EVT_CONNECTED: {
            // FIXME: If multi role is enabled, this flag should not be clear here.
            s_bleInstance.advertising = false;

            LOG_DEBUG(TRACE, "BLE GAP event: connected.");

            s_bleInstance.role       = event->evt.gap_evt.params.connected.role;
            s_bleInstance.connHandle = event->evt.gap_evt.conn_handle;
            s_bleInstance.connecting = false;
            s_bleInstance.connected  = true;
            memcpy(&s_bleInstance.effectiveConnParams, &event->evt.gap_evt.params.connected.conn_params, sizeof(ble_gap_conn_params_t));

            LOG_DEBUG(TRACE, "BLE role: %d, connection handle: %d", s_bleInstance.role, s_bleInstance.connHandle);
            LOG_DEBUG(TRACE, "| interval(ms)  latency  timeout(ms) |");
            LOG_DEBUG(TRACE, "  %.2f          %d       %d",
                    s_bleInstance.effectiveConnParams.max_conn_interval*1.25,
                    s_bleInstance.effectiveConnParams.slave_latency,
                    s_bleInstance.effectiveConnParams.conn_sup_timeout*10);

            // Update connection parameters if needed.
            s_bleInstance.connParamsUpdateAttempts = 0;
            updateConnParamsIfNeeded();

            bleEvt.evt_type               = BLE_EVT_TYPE_CONNECTION;
            bleEvt.conn_event.conn_handle = s_bleInstance.connHandle;
            bleEvt.conn_event.evt_id      = BLE_CONN_EVT_ID_CONNECTED;
            if (os_queue_put(s_bleInstance.evtQueue, &bleEvt, 0, NULL)) {
                LOG(ERROR, "os_queue_put() failed.");
            }
        } break;

        case BLE_GAP_EVT_DISCONNECTED: {
            LOG_DEBUG(TRACE, "BLE GAP event: disconnected, handle: 0x%04X, reason: %d",
                      event->evt.gap_evt.conn_handle,
                      event->evt.gap_evt.params.disconnected.reason);

            // Stop the connection parameters update timer.
            if (!os_timer_is_active(s_bleInstance.connParamsUpdateTimer, NULL)) {
                os_timer_change(s_bleInstance.connParamsUpdateTimer, OS_TIMER_CHANGE_STOP, true, 0, 0, NULL);
            }

            bleEvt.evt_type               = BLE_EVT_TYPE_CONNECTION;
            bleEvt.conn_event.conn_handle = s_bleInstance.connHandle;
            bleEvt.conn_event.evt_id      = BLE_CONN_EVT_ID_DISCONNECTED;
            bleEvt.conn_event.reason      = event->evt.gap_evt.params.disconnected.reason;
            if (os_queue_put(s_bleInstance.evtQueue, &bleEvt, 0, NULL)) {
                LOG(ERROR, "os_queue_put() failed.");
            }

            // Re-start advertising.
            // FIXME: when multi-role implemented.
            if (s_bleInstance.role == BLE_ROLE_PERIPHERAL) {
                LOG_DEBUG(TRACE, "Restart BLE advertising.");
                ret = sd_ble_gap_adv_start(s_bleInstance.advHandle, BLE_CONN_CFG_TAG);
                if (ret != NRF_SUCCESS) {
                    LOG(ERROR, "sd_ble_gap_adv_start() failed: %u", (unsigned)ret);
                }
                else {
                    s_bleInstance.advertising = true;
                }
            }

            s_bleInstance.connHandle   = BLE_INVALID_CONN_HANDLE;
            s_bleInstance.role         = BLE_ROLE_INVALID;
            s_bleInstance.connected    = false;
            s_bleInstance.indConfirmed = true;
        } break;

        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST: {
            LOG_DEBUG(TRACE, "BLE GAP event: connection parameter update request.");

            ret = sd_ble_gap_conn_param_update(event->evt.gap_evt.conn_handle,
                    &event->evt.gap_evt.params.conn_param_update_request.conn_params);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gap_conn_param_update() failed: %u", (unsigned)ret);
            }
        } break;

        case BLE_GAP_EVT_CONN_PARAM_UPDATE: {
            LOG_DEBUG(TRACE, "BLE GAP event: connection parameter updated.");

            memcpy(&s_bleInstance.effectiveConnParams, &event->evt.gap_evt.params.conn_param_update.conn_params, sizeof(ble_gap_conn_params_t));

            LOG_DEBUG(TRACE, "| interval(ms)  latency  timeout(ms) |");
            LOG_DEBUG(TRACE, "  %.2f          %d       %d",
                    s_bleInstance.effectiveConnParams.max_conn_interval*1.25,
                    s_bleInstance.effectiveConnParams.slave_latency,
                    s_bleInstance.effectiveConnParams.conn_sup_timeout*10);

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
            LOG_DEBUG(TRACE, "| txo    rxo     txt(us)     rxt(us) |.");
            LOG_DEBUG(TRACE, "  %d    %d     %d        %d",
                    event->evt.gap_evt.params.data_length_update.effective_params.max_tx_octets,
                    event->evt.gap_evt.params.data_length_update.effective_params.max_rx_octets,
                    event->evt.gap_evt.params.data_length_update.effective_params.max_tx_time_us,
                    event->evt.gap_evt.params.data_length_update.effective_params.max_rx_time_us);
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
            if (event->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_SCAN) {
                LOG_DEBUG(TRACE, "BLE GAP event: Scanning timeout");
                s_bleInstance.scanning = false;
            }
            else if (event->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN) {
                LOG_DEBUG(ERROR, "BLE GAP event: Connection timeout");
                s_bleInstance.connecting = false;
            }
            else if (event->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_AUTH_PAYLOAD) {
                LOG_DEBUG(ERROR, "BLE GAP event: Authenticated payload timeout");
            }
            else {
                LOG_DEBUG(ERROR, "BLE GAP event: Unknown timeout, source: %d", event->evt.gap_evt.params.timeout.src);
            }
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

            bleEvt.evt_type               = BLE_EVT_TYPE_DATA;
            bleEvt.data_event.conn_handle = event->evt.gatts_evt.conn_handle;
            bleEvt.data_event.attr_handle = event->evt.gatts_evt.params.write.handle;
            bleEvt.data_event.data_len    = event->evt.gatts_evt.params.write.len;
            memcpy(bleEvt.data_event.data, event->evt.gatts_evt.params.write.data, bleEvt.data_event.data_len);
            if (os_queue_put(s_bleInstance.evtQueue, &bleEvt, 0, NULL)) {
                LOG(ERROR, "os_queue_put() failed.");
            }
        } break;

        case BLE_GATTS_EVT_HVN_TX_COMPLETE: {
            LOG_DEBUG(TRACE, "BLE GATT Server event: notification sent.");
        } break;

        case BLE_GATTS_EVT_HVC: {
            LOG_DEBUG(TRACE, "BLE GATT Server event: indication confirmed.");
            s_bleInstance.indConfirmed = true;
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
        if (!os_queue_take(s_bleInstance.evtQueue, &bleEvent, CONCURRENT_WAIT_FOREVER, NULL)) {
            dispatchBleEvent(&bleEvent);
        }
        else {
            LOG(ERROR, "BLE event thread exited.");
            break;
        }
    }

    os_thread_exit(s_bleInstance.evtThread);
}

bool ble_is_initialized(void) {
    return s_bleInstance.initialized;
}

int hal_ble_init(uint8_t role, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());

    /* The SoftDevice has been enabled in core_hal.c */

    if (!s_bleInstance.initialized) {
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

        s_bleInstance.initialized = true;

        // Register a handler for BLE events.
        NRF_SDH_BLE_OBSERVER(bleObserver, BLE_OBSERVER_PRIO, isrProcessBleEvent, nullptr);

        for (uint8_t i = 0; i < BLE_MAX_EVENT_CALLBACK_COUNT; i++) {
            s_bleInstance.evtCallbacks[i] = NULL;
        }

        s_bleInstance.connHandle = BLE_INVALID_CONN_HANDLE;

        // Configure an advertising set to obtain an advertising handle.
        if (role & BLE_ROLE_PERIPHERAL) {
            int error = ble_set_advertising_params(&s_bleInstance.advParams);
            if (error != SYSTEM_ERROR_NONE) {
                return error;
            }

            // Set the default TX Power
            ret = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, s_bleInstance.advHandle, s_bleInstance.txPower);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gap_tx_power_set() failed: %u", (unsigned)ret);
                return sysError(ret);
            }
            s_bleInstance.txPower = 0;
        }

        if (os_queue_create(&s_bleInstance.evtQueue, sizeof(hal_ble_event_t), BLE_EVENT_QUEUE_ITEM_COUNT, NULL)) {
            LOG(ERROR, "os_queue_create() failed");
            return SYSTEM_ERROR_NO_MEMORY;
        }

        if (os_thread_create(&s_bleInstance.evtThread, "BLE Event Thread", OS_THREAD_PRIORITY_NETWORK, handleBleEventThread, NULL, BLE_EVENT_THREAD_STACK_SIZE)) {
            LOG(ERROR, "os_thread_create() failed");
            os_queue_destroy(s_bleInstance.evtQueue, NULL);
            return SYSTEM_ERROR_INTERNAL;
        }

        if (os_timer_create(&s_bleInstance.connParamsUpdateTimer, BLE_CONN_PARAMS_UPDATE_DELAY_MS, connParamsUpdateTimerCb, NULL, true, NULL)) {
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
    SPARK_ASSERT(s_bleInstance.initialized);
    for (uint8_t i = 0; i < BLE_MAX_EVENT_CALLBACK_COUNT; i++) {
        if (s_bleInstance.evtCallbacks[i] == NULL) {
            s_bleInstance.evtCallbacks[i] = callback;
            return SYSTEM_ERROR_NONE;
        }
    }
    return SYSTEM_ERROR_LIMIT_EXCEEDED;
}

int ble_deregister_callback(ble_event_callback_t callback) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    for (uint8_t i = 0; i < BLE_MAX_EVENT_CALLBACK_COUNT; i++) {
        if (s_bleInstance.evtCallbacks[i] == callback) {
            s_bleInstance.evtCallbacks[i] = NULL;
            return SYSTEM_ERROR_NONE;
        }
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int ble_set_device_address(hal_ble_address_t const* addr) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

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
    SPARK_ASSERT(s_bleInstance.initialized);

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
    SPARK_ASSERT(s_bleInstance.initialized);

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
    SPARK_ASSERT(s_bleInstance.initialized);

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
    SPARK_ASSERT(s_bleInstance.initialized);

    LOG_DEBUG(TRACE, "ble_set_appearance().");

    ret_code_t ret = sd_ble_gap_appearance_set(appearance);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_appearance_set() failed: %u", (unsigned)ret);
    }

    return sysError(ret);
}

int ble_get_appearance(uint16_t* appearance) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

    LOG_DEBUG(TRACE, "ble_get_appearance().");

    ret_code_t ret = sd_ble_gap_appearance_get(appearance);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_appearance_get() failed: %u", (unsigned)ret);
    }

    return sysError(ret);
}

int ble_add_whitelist(hal_ble_address_t* addr_list, uint8_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

    LOG_DEBUG(TRACE, "ble_add_whitelist().");

    if (addr_list == NULL || len == 0 || len > BLE_MAX_WHITELIST_ADDR_COUNT) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    for (uint8_t i = 0; i < len; i++) {
        s_bleInstance.whitelist[i].addr_id_peer = true;
        s_bleInstance.whitelist[i].addr_type = addr_list[i].addr_type;
        memcpy(s_bleInstance.whitelist[i].addr, addr_list[i].addr, BLE_SIG_ADDR_LEN);
        s_bleInstance.whitelistPointer[i] = &s_bleInstance.whitelist[i];
    }

    ret_code_t ret = sd_ble_gap_whitelist_set(s_bleInstance.whitelistPointer, len);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_whitelist_set() failed: %u", (unsigned)ret);
    }

    return sysError(ret);
}

int ble_delete_whitelist(void) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

    LOG_DEBUG(TRACE, "ble_delete_whitelist().");

    ret_code_t ret = sd_ble_gap_whitelist_set(NULL, 0);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_whitelist_set() failed: %u", (unsigned)ret);
    }

    return sysError(ret);
}

int ble_set_tx_power(int8_t value) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

    LOG_DEBUG(TRACE, "ble_set_tx_power().");

    ret_code_t ret = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, s_bleInstance.advHandle, roundTxPower(value));
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_tx_power_set() failed: %u", (unsigned)ret);
    }

    s_bleInstance.txPower = roundTxPower(value);

    return sysError(ret);
}

int ble_get_tx_power(int8_t* value) {
    std::lock_guard<bleLock> lk(bleLock());

    LOG_DEBUG(TRACE, "ble_get_tx_power().");

    *value = s_bleInstance.txPower;

    return SYSTEM_ERROR_NONE;
}

int ble_set_advertising_params(hal_ble_adv_params_t* adv_params) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

    LOG_DEBUG(TRACE, "ble_set_advertising_params().");

    ret_code_t ret;
    bool advPending = false;

    // It is invalid to change advertising set parameters while advertising.
    if (s_bleInstance.advertising) {
        int err = ble_stop_advertising();
        if (err != SYSTEM_ERROR_NONE) {
            return err;
        }
        advPending = true;
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

    LOG_DEBUG(TRACE, "BLE advertising interval: %.3fms, timeout: %dms.",
            bleGapAdvParams.interval*0.625, bleGapAdvParams.duration*10);

    // Make sure the advertising data and scan response data is consistent.
    ble_gap_adv_data_t bleGapAdvData;
    bleGapAdvData.adv_data.p_data      = s_bleInstance.advData;
    bleGapAdvData.adv_data.len         = s_bleInstance.advDataLen;
    bleGapAdvData.scan_rsp_data.p_data = s_bleInstance.scanRespData;
    bleGapAdvData.scan_rsp_data.len    = s_bleInstance.scanRespDataLen;

    ret = sd_ble_gap_adv_set_configure(&s_bleInstance.advHandle, &bleGapAdvData, &bleGapAdvParams);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_adv_set_configure() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    if (advPending) {
        advPending = false;
        return ble_start_advertising();
    }

    memcpy(&s_bleInstance.advParams, adv_params, sizeof(hal_ble_adv_params_t));

    return sysError(ret);
}

int ble_config_adv_data(uint8_t ad_type, uint8_t* data, uint16_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

    LOG_DEBUG(TRACE, "ble_config_adv_data().");

    int ret = adStructEncode(ad_type, data, len, BLE_ADV_DATA_ADVERTISING);
    if (ret == SYSTEM_ERROR_NONE) {
        ret = setAdvData(s_bleInstance.advData, s_bleInstance.advDataLen, BLE_ADV_DATA_ADVERTISING);
    }

    return ret;
}

int ble_set_adv_data(uint8_t* data, uint16_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

    LOG_DEBUG(TRACE, "ble_set_adv_data().");

    return setAdvData(data, len, BLE_ADV_DATA_ADVERTISING);
}

int ble_config_scan_resp_data(uint8_t ad_type, uint8_t* data, uint16_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

    LOG_DEBUG(TRACE, "ble_config_scan_resp_data().");

    int ret = adStructEncode(ad_type, data, len, BLE_ADV_DATA_SCAN_RESPONSE);
    if (ret == SYSTEM_ERROR_NONE) {
        ret = setAdvData(s_bleInstance.scanRespData, s_bleInstance.scanRespDataLen, BLE_ADV_DATA_SCAN_RESPONSE);
    }

    return ret;
}

int ble_set_scan_resp_data(uint8_t* data, uint16_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

    LOG_DEBUG(TRACE, "ble_set_scan_resp_data().");

    return setAdvData(data, len, BLE_ADV_DATA_SCAN_RESPONSE);
}

int ble_start_advertising(void) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

    LOG_DEBUG(TRACE, "ble_start_advertising().");

    if (s_bleInstance.advertising) {
        return SYSTEM_ERROR_NONE;
    }

    ret_code_t ret = sd_ble_gap_adv_start(s_bleInstance.advHandle, BLE_CONN_CFG_TAG);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_adv_start() failed: %u", (unsigned)ret);
        return sysError(ret);
    }
    else {
        s_bleInstance.advertising = true;
        return SYSTEM_ERROR_NONE;
    }
}

int ble_stop_advertising(void) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

    LOG_DEBUG(TRACE, "ble_stop_advertising().");

    if (!s_bleInstance.advertising) {
        return SYSTEM_ERROR_NONE;
    }

    ret_code_t ret = sd_ble_gap_adv_stop(s_bleInstance.advHandle);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_adv_stop() failed: %u", (unsigned)ret);
        return sysError(ret);
    }
    else {
        s_bleInstance.advertising = false;
        return SYSTEM_ERROR_NONE;
    }
}

bool ble_is_advertising(void) {
    return s_bleInstance.advertising;
}

int ble_set_scanning_params(hal_ble_scan_params_t* scan_params) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

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

    memcpy(&s_bleInstance.scanParams, scan_params, sizeof(hal_ble_scan_params_t));

    return SYSTEM_ERROR_NONE;
}

int ble_start_scanning(void) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

    LOG_DEBUG(TRACE, "ble_start_scanning().");

    sd_ble_gap_scan_stop();
    s_bleInstance.scanning = false;

    ble_gap_scan_params_t bleGapScanParams;
    fromHalScanParams(&s_bleInstance.scanParams, &bleGapScanParams);

    LOG_DEBUG(TRACE, "| interval(ms)   window(ms)   timeout(ms) |");
    LOG_DEBUG(TRACE, "  %.3f        %.3f      %d",
            bleGapScanParams.interval*0.625,
            bleGapScanParams.window*0.625,
            bleGapScanParams.timeout*10);

    ret_code_t ret = sd_ble_gap_scan_start(&bleGapScanParams, &s_bleScanData);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_scan_start() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    s_bleInstance.scanning = true;

    return SYSTEM_ERROR_NONE;
}

bool ble_is_scanning(void) {
    return s_bleInstance.scanning;
}

int ble_stop_scanning(void) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

    LOG_DEBUG(TRACE, "ble_stop_scanning().");

    ret_code_t ret = sd_ble_gap_scan_stop();
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_scan_start() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    s_bleInstance.scanning = false;

    return SYSTEM_ERROR_NONE;
}

int ble_adv_data_decode(uint8_t flag, const uint8_t* adv_data, uint16_t adv_data_len, uint8_t* data, uint16_t* len) {
    std::lock_guard<bleLock> lk(bleLock());

    // An AD structure must consist of 1 byte length field, 1 byte type field and at least 1 byte data field
    if (adv_data == NULL || adv_data_len < 3) {
        *len = 0;
        return SYSTEM_ERROR_NOT_FOUND;
    }

    uint16_t dataOffset, dataLen;
    if (locateAdStructure(flag, adv_data, adv_data_len, &dataOffset, &dataLen) == SYSTEM_ERROR_NONE) {
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

int ble_connect(hal_ble_address_t* addr) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

    s_bleInstance.scanning = false;

    ble_gap_addr_t devAddr;
    memset(&devAddr, 0x00, sizeof(ble_gap_addr_t));
    devAddr.addr_type = addr->addr_type;
    memcpy(devAddr.addr, addr->addr, BLE_SIG_ADDR_LEN);

    ble_gap_scan_params_t bleGapScanParams;
    fromHalScanParams(&s_bleInstance.scanParams, &bleGapScanParams);

    ble_gap_conn_params_t connParams;
    fromHalConnParams(&s_bleInstance.ppcp, &connParams);

    ret_code_t ret = sd_ble_gap_connect(&devAddr, &bleGapScanParams, &connParams, BLE_CONN_CFG_TAG);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_connect() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    s_bleInstance.connecting = true;

    return 0;
}

bool ble_is_connecting(void) {
    return s_bleInstance.connecting;
}

bool ble_is_connected(void) {
    return s_bleInstance.connected;
}

int ble_connect_cancel(void) {
    std::lock_guard<bleLock> lk(bleLock());

    LOG_DEBUG(TRACE, "ble_connect_cancel().");

    if (s_bleInstance.connecting) {
        ret_code_t ret = sd_ble_gap_connect_cancel();
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "sd_ble_gap_connect_cancel() failed: %u", (unsigned)ret);
            return sysError(ret);
        }

        s_bleInstance.connecting = false;
    }

    return 0;
}

int ble_set_ppcp(hal_ble_conn_params_t* conn_params) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

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
    fromHalConnParams(conn_params, &connParams);

    ret_code_t ret = sd_ble_gap_ppcp_set(&connParams);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_ppcp_set() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    memcpy(&s_bleInstance.ppcp, conn_params, sizeof(hal_ble_conn_params_t));

    return SYSTEM_ERROR_NONE;
}

int ble_get_ppcp(hal_ble_conn_params_t* conn_params) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

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
    SPARK_ASSERT(s_bleInstance.initialized);

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

int hal_ble_disconnect(uint16_t conn_handle) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

    LOG_DEBUG(TRACE, "hal_ble_disconnect().");

    if (!s_bleInstance.connected) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    else if (s_bleInstance.role == BLE_ROLE_PERIPHERAL) {
        conn_handle = s_bleInstance.connHandle;
    }
    else if (conn_handle == BLE_INVALID_CONN_HANDLE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ret_code_t ret = sd_ble_gap_disconnect(conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_disconnect() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    return SYSTEM_ERROR_NONE;
}

int ble_add_service_uuid128(uint8_t type, const uint8_t* uuid128, uint16_t* handle) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

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
    SPARK_ASSERT(s_bleInstance.initialized);

    LOG_DEBUG(TRACE, "ble_add_service_uuid16().");

    if (handle == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ble_uuid_t svcUuid;
    svcUuid.type = BLE_UUID_TYPE_BLE;
    svcUuid.uuid = uuid16;

    return addService(type, &svcUuid, handle);
}

int ble_add_char_uuid128(uint16_t service_handle, const uint8_t *uuid128, uint8_t properties, const char* desc, hal_ble_char_t* ble_char) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

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

    int error = addCharacteristic(service_handle, &charUuid, properties, desc, ble_char);
    if (SYSTEM_ERROR_NONE == error) {
        ble_char->uuid.type = BLE_UUID_TYPE_128BIT;
        memcpy(ble_char->uuid.uuid128, uuid128, BLE_SIG_UUID_128BIT_LEN);
    }

    return error;
}

int ble_add_char_uuid16(uint16_t service_handle, uint16_t uuid16, uint8_t properties, const char* desc, hal_ble_char_t* ble_char) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

    LOG_DEBUG(TRACE, "ble_add_char_uuid16().");

    if (ble_char == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ble_uuid_t charUuid;
    charUuid.type = BLE_UUID_TYPE_BLE;
    charUuid.uuid = uuid16;

    int error = addCharacteristic(service_handle, &charUuid, properties, desc, ble_char);
    if (SYSTEM_ERROR_NONE == error) {
        ble_char->uuid.type   = BLE_UUID_TYPE_16BIT;
        ble_char->uuid.uuid16 = uuid16;
    }

    return error;
}

int ble_add_char_desc(uint8_t* desc, uint16_t len, hal_ble_char_t* ble_char) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

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

int ble_publish(hal_ble_char_t* ble_char, uint8_t* data, uint16_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

    LOG_DEBUG(TRACE, "ble_publish().");

    if (ble_char == NULL || data == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if (!(ble_char->properties & BLE_SIG_CHAR_PROP_NOTIFY) && !(ble_char->properties & BLE_SIG_CHAR_PROP_INDICATE)) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }

    if (s_bleInstance.connHandle == BLE_INVALID_CONN_HANDLE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    // Check if the client has enabled notification or indication
    uint8_t cccd[2];
    ble_gatts_value_t gattValue;
    gattValue.p_value = cccd;
    gattValue.len = sizeof(cccd);
    gattValue.offset = 0;
    ret_code_t ret = sd_ble_gatts_value_get(s_bleInstance.connHandle, ble_char->cccd_handle, &gattValue);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_value_get() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    uint16_t cccd_value = uint16_decode(cccd);
    if (cccd_value == BLE_SIG_CCCD_VAL_DISABLED || cccd_value > BLE_SIG_CCCD_VAL_INDICATION || !s_bleInstance.indConfirmed) {
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

        ret_code_t ret = sd_ble_gatts_hvx(s_bleInstance.connHandle, &hvxParams);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "sd_ble_gatts_hvx() failed: %u", (unsigned)ret);
            return sysError(ret);
        }

        if (cccd_value == BLE_SIG_CCCD_VAL_INDICATION) {
            s_bleInstance.indConfirmed = false;
        }
    }

    return SYSTEM_ERROR_NONE;
}

int ble_set_characteristic_value(hal_ble_char_t* ble_char, uint8_t* data, uint16_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

    LOG_DEBUG(TRACE, "ble_set_characteristic_value().");

    if (ble_char == NULL || data == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ble_gatts_value_t gattValue;
    gattValue.len     = len;
    gattValue.offset  = 0;
    gattValue.p_value = data;

    ret_code_t ret = sd_ble_gatts_value_set(BLE_CONN_HANDLE_INVALID, ble_char->value_handle, &gattValue);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_value_set() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    return SYSTEM_ERROR_NONE;
}

int ble_get_characteristic_value(hal_ble_char_t* ble_char, uint8_t* data, uint16_t* len) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

    LOG_DEBUG(TRACE, "ble_get_characteristic_value().");

    if (ble_char == NULL || data == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ble_gatts_value_t gattValue;
    gattValue.len     = *len;
    gattValue.offset  = 0;
    gattValue.p_value = data;

    ret_code_t ret = sd_ble_gatts_value_get(BLE_CONN_HANDLE_INVALID, ble_char->value_handle, &gattValue);
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


