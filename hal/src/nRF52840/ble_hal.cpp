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
LOG_SOURCE_CATEGORY("hal.ble")

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

#include "gpio_hal.h"
#include "ble_hal.h"
#include "system_error.h"
#include "sdk_config_system.h"

#include <string.h>


#define BLE_CONN_CFG_TAG                            1
#define BLE_OBSERVER_PRIO                           1

#define BLE_ADV_DATA_ADVERTISING                    0
#define BLE_ADV_DATA_SCAN_RESPONSE                  1

#define BLE_DISCOVERY_PROCEDURE_IDLE                0x00
#define BLE_DISCOVERY_PROCEDURE_SERVICES            0x01
#define BLE_DISCOVERY_PROCEDURE_CHARACTERISTICS     0x02
#define BLE_DISCOVERY_PROCEDURE_DESCRIPTORS         0x03

#define BLE_GENERAL_PROCEDURE_TIMEOUT            	5000 // In milliseconds
#define BLE_DICOVERY_PROCEDURE_TIMEOUT				15000 // In milliseconds


StaticRecursiveMutex s_bleMutex;

class bleLock {
    static void lock() {
        s_bleMutex.lock();
    }

    static void unlock() {
        s_bleMutex.unlock();
    }
};


typedef void (*evt_handler_t)(void*, void*);

typedef struct {
    evt_handler_t    handler;
    hal_ble_events_t arg;
    void*            context;
} HalBleEvtMsg_t;

typedef struct {
    uint16_t                 discStartHandle;
    uint16_t                 discEndHandle;
    hal_ble_service_t        services[BLE_MAX_SVC_COUNT];
    hal_ble_characteristic_t characteristics[BLE_MAX_CHAR_COUNT];
    hal_ble_descriptor_t     descriptors[BLE_MAX_DESC_COUNT];
    uint8_t                  discAll : 1;
    uint8_t                  count;
    uint8_t                  currIndex;
    uint8_t                  currDiscProcedure;
    os_semaphore_t           semaphore;
} HalBleDiscovery_t;

// TODO: used when muti-role is implemented
typedef struct {
    uint16_t connHandles[BLE_MAX_LINK_COUNT];
    hal_ble_address_t peerAddrs[BLE_MAX_LINK_COUNT];
} HalBleConnections_t;

/* BLE Instance */
typedef struct {
    uint8_t  initialized  : 1;
    uint8_t  advertising  : 1;
    uint8_t  scanning     : 1;
    uint8_t  connecting   : 1;
    uint8_t  discovering  : 1;
    uint8_t  connected    : 1;
    int8_t   txPower;
    uint8_t  role;
    uint16_t connHandle;
    uint8_t  advHandle;
    uint8_t  connParamsUpdateAttempts;

    hal_ble_scan_parameters_t        scanParams;
    hal_ble_advertising_parameters_t advParams;
    hal_ble_connection_parameters_t  ppcp;

    uint8_t  advData[BLE_MAX_ADV_DATA_LEN];
    uint16_t advDataLen;
    uint8_t  scanRespData[BLE_MAX_ADV_DATA_LEN];
    uint16_t scanRespDataLen;

    os_semaphore_t connectSemaphore;
    os_semaphore_t disconnectSemaphore;
    os_semaphore_t connUpdateSemaphore;
    os_semaphore_t discoverySemaphore;
    os_semaphore_t readWriteSemaphore;
    os_queue_t     evtQueue;
    os_thread_t    evtThread;
    os_timer_t     connParamsUpdateTimer;

    ble_on_events_callback_t onEvtCb;
    void*                    context;

    hal_ble_address_t     peer_addr;
    hal_ble_connection_parameters_t effectiveConnParams;
    ble_gap_addr_t        whitelist[BLE_MAX_WHITELIST_ADDR_COUNT];
    ble_gap_addr_t const* whitelistPointer[BLE_MAX_WHITELIST_ADDR_COUNT];

    HalBleDiscovery_t     discovery;
} HalBleInstance_t;


static HalBleInstance_t s_bleInstance = {
    .initialized  = 0,
    .advertising  = 0,
    .scanning     = 0,
    .connecting   = 0,
    .discovering  = 0,
    .connected    = 0,
    .txPower      = 0,
    .role         = BLE_ROLE_INVALID,
    .connHandle   = BLE_INVALID_CONN_HANDLE,
    .advHandle    = BLE_GAP_ADV_SET_HANDLE_NOT_SET,
    .connParamsUpdateAttempts = 0,

    .scanParams = {
        .version       = 0x01,
        .active        = true,
        .filter_policy = BLE_SCAN_FP_ACCEPT_ALL,
        .interval      = BLE_DEFAULT_SCANNING_INTERVAL,
        .window        = BLE_DEFAULT_SCANNING_WINDOW,
        .timeout       = BLE_DEFAULT_SCANNING_TIMEOUT
    },

    .advParams = {
        .version       = 0x01,
        .type          = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT,
        .filter_policy = BLE_ADV_FP_ANY,
        .interval      = BLE_DEFAULT_ADVERTISING_INTERVAL,
        .timeout       = BLE_DEFAULT_ADVERTISING_TIMEOUT,
        .inc_tx_power  = false
    },

    /*
     * For BLE Central, this is the initial connection parameters.
     * For BLE Peripheral, this is the Peripheral Preferred Connection Parameters.
     */
    .ppcp = {
        .version           = 0x01,
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

    .connectSemaphore = NULL,
    .disconnectSemaphore = NULL,
    .connUpdateSemaphore = NULL,
	.discoverySemaphore = NULL,
    .readWriteSemaphore = NULL,
    .evtQueue  = NULL,
    .evtThread = NULL,
    .connParamsUpdateTimer = NULL,

    .onEvtCb = NULL,
    .context = NULL,
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

int setAdvData(uint8_t* data, uint16_t len, uint8_t flag) {
    bool advPending = false;

    // It is invalid to provide the same data buffers while advertising.
    if (s_bleInstance.advertising) {
        int err = ble_gap_stop_advertising();
        if (err != SYSTEM_ERROR_NONE) {
            return err;
        }
        advPending = true;
    }

    // Make sure the advertising data or scan response data is consistent.
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
        if (data != NULL) {
            memcpy(s_bleInstance.scanRespData, data, len);
        }
        s_bleInstance.scanRespDataLen = len;
    }

    if (advPending) {
        advPending = false;
        return ble_gap_start_advertising(NULL);
    }

    return SYSTEM_ERROR_NONE;
}

static int fromHalScanParams(const hal_ble_scan_parameters_t* halScanParams, ble_gap_scan_params_t* scanParams) {
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

static int fromHalConnParams(const hal_ble_connection_parameters_t* halConnParams, ble_gap_conn_params_t* connParams) {
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

static int toHalConnParams(const ble_gap_conn_params_t* connParams, hal_ble_connection_parameters_t* halConnParams) {
    if (halConnParams == NULL || connParams == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    memset(halConnParams, 0x00, sizeof(hal_ble_connection_parameters_t));

    halConnParams->min_conn_interval = connParams->min_conn_interval;
    halConnParams->max_conn_interval = connParams->max_conn_interval;
    halConnParams->slave_latency = connParams->slave_latency;
    halConnParams->conn_sup_timeout = connParams->conn_sup_timeout;

    return SYSTEM_ERROR_NONE;
}

static int fromHalUuid(const hal_ble_uuid_t* halUuid, ble_uuid_t* uuid) {
    if (uuid == NULL || halUuid == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if (halUuid->type == BLE_UUID_TYPE_128BIT) {
        ble_uuid128_t uuid128;
        memcpy(uuid128.uuid128, halUuid->uuid128, BLE_SIG_UUID_128BIT_LEN);

        ret_code_t ret = sd_ble_uuid_vs_add(&uuid128, &uuid->type);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "sd_ble_uuid_vs_add() failed: %u", (unsigned)ret);
            return sysError(ret);
        }

        ret = sd_ble_uuid_decode(BLE_SIG_UUID_128BIT_LEN, halUuid->uuid128, uuid);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "sd_ble_uuid_decode() failed: %u", (unsigned)ret);
            return sysError(ret);
        }
    }
    else if (halUuid->type == BLE_UUID_TYPE_16BIT) {
        uuid->type = BLE_UUID_TYPE_BLE;
        uuid->uuid = halUuid->uuid16;
    }
    else {
        uuid->type = BLE_UUID_TYPE_UNKNOWN;
        uuid->uuid = halUuid->uuid16;
    }

    return SYSTEM_ERROR_NONE;
}

static int toHalUuid(const ble_uuid_t* uuid, hal_ble_uuid_t* halUuid) {
    if (uuid == NULL || halUuid == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if (uuid->type == BLE_UUID_TYPE_BLE) {
        halUuid->type = BLE_UUID_TYPE_16BIT;
        halUuid->uuid16 = uuid->uuid;
        return SYSTEM_ERROR_NONE;
    }
    else if (uuid->type != BLE_UUID_TYPE_UNKNOWN) {
        uint8_t uuidLen;
        halUuid->type = BLE_UUID_TYPE_128BIT;
        ret_code_t ret = sd_ble_uuid_encode(uuid, &uuidLen, halUuid->uuid128);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "sd_ble_uuid_encode() failed: %u", (unsigned)ret);
            halUuid->type = BLE_UUID_TYPE_128BIT_SHORTED;
            halUuid->uuid16 = uuid->uuid;
            return sysError(ret);
        }
        return SYSTEM_ERROR_NONE;
    }

    halUuid->type = BLE_UUID_TYPE_128BIT_SHORTED;
    halUuid->uuid16 = uuid->uuid;

    return SYSTEM_ERROR_UNKNOWN;
}

static uint8_t toHalCharacteristicProperties(ble_gatt_char_props_t properties) {
    uint8_t halProperties = 0;

    if (properties.broadcast)      halProperties |= BLE_SIG_CHAR_PROP_BROADCAST;
    if (properties.read)           halProperties |= BLE_SIG_CHAR_PROP_READ;
    if (properties.write_wo_resp)  halProperties |= BLE_SIG_CHAR_PROP_WRITE_WO_RESP;
    if (properties.write)          halProperties |= BLE_SIG_CHAR_PROP_WRITE;
    if (properties.notify)         halProperties |= BLE_SIG_CHAR_PROP_NOTIFY;
    if (properties.indicate)       halProperties |= BLE_SIG_CHAR_PROP_INDICATE;
    if (properties.auth_signed_wr) halProperties |= BLE_SIG_CHAR_PROP_AUTH_SIGN_WRITES;

    return halProperties;
}

static void fromHalCharacteristicProperties(uint8_t halProperties, ble_gatt_char_props_t* properties) {
    properties->broadcast      = (halProperties & BLE_SIG_CHAR_PROP_BROADCAST) ? 1 : 0;
    properties->read           = (halProperties & BLE_SIG_CHAR_PROP_READ) ? 1 : 0;
    properties->write_wo_resp  = (halProperties & BLE_SIG_CHAR_PROP_WRITE_WO_RESP) ? 1 : 0;
    properties->write          = (halProperties & BLE_SIG_CHAR_PROP_WRITE) ? 1 : 0;
    properties->notify         = (halProperties & BLE_SIG_CHAR_PROP_NOTIFY) ? 1 : 0;
    properties->indicate       = (halProperties & BLE_SIG_CHAR_PROP_INDICATE) ? 1 : 0;
    properties->auth_signed_wr = (halProperties & BLE_SIG_CHAR_PROP_AUTH_SIGN_WRITES) ? 1 : 0;
}

static bool isConnParamsFeeded(hal_ble_connection_parameters_t* connParams) {
    hal_ble_connection_parameters_t ppcp;

    if (ble_gap_get_ppcp(&ppcp, NULL) == SYSTEM_ERROR_NONE) {
        uint16_t minAcceptedSl = ppcp.slave_latency - MIN(BLE_CONN_PARAMS_SLAVE_LATENCY_ERR, ppcp.slave_latency);
        uint16_t maxAcceptedSl = ppcp.slave_latency + BLE_CONN_PARAMS_SLAVE_LATENCY_ERR;
        uint16_t minAcceptedTo = ppcp.conn_sup_timeout - MIN(BLE_CONN_PARAMS_TIMEOUT_ERR, ppcp.conn_sup_timeout);
        uint16_t maxAcceptedTo = ppcp.conn_sup_timeout + BLE_CONN_PARAMS_TIMEOUT_ERR;

        if (connParams->max_conn_interval < ppcp.min_conn_interval ||
                connParams->max_conn_interval > ppcp.max_conn_interval) {
            return false;
        }
        if (connParams->slave_latency < minAcceptedSl ||
                connParams->slave_latency > maxAcceptedSl) {
            return false;
        }
        if (connParams->conn_sup_timeout < minAcceptedTo ||
                connParams->conn_sup_timeout > maxAcceptedTo) {
            return false;
        }
    }

    return true;
}

static void connParamsUpdateTimerCb(os_timer_t timer) {
    if (s_bleInstance.connHandle == BLE_INVALID_CONN_HANDLE) {
        return;
    }

    hal_ble_connection_parameters_t ppcp;
    if (ble_gap_get_ppcp(&ppcp, NULL) == SYSTEM_ERROR_NONE) {
        int ret = ble_gap_update_connection_params(s_bleInstance.connHandle, &ppcp, NULL);
        if (ret == SYSTEM_ERROR_NONE) {
            s_bleInstance.connParamsUpdateAttempts++;
            return;
        }
    }

    ret_code_t ret = sd_ble_gap_disconnect(s_bleInstance.connHandle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_disconnect() failed: %u", (unsigned)ret);
        return;
    }
    LOG_DEBUG(TRACE, "Disconnecting. Update BLE connection parameters failed.");
}

// Only used for BLE Peripheral
static void updateConnParamsIfNeeded(void) {
    if (s_bleInstance.role == BLE_ROLE_PERIPHERAL && !isConnParamsFeeded(&s_bleInstance.effectiveConnParams)) {
        if (s_bleInstance.connParamsUpdateAttempts < BLE_CONN_PARAMS_UPDATE_ATTEMPS) {
            if (s_bleInstance.connParamsUpdateTimer != NULL) {
                if (!os_timer_change(s_bleInstance.connParamsUpdateTimer, OS_TIMER_CHANGE_START, true, 0, 0, NULL)) {
                    LOG_DEBUG(TRACE, "Attempts to update BLE connection parameters, try: %d after %d ms",
                            s_bleInstance.connParamsUpdateAttempts, BLE_CONN_PARAMS_UPDATE_DELAY_MS);
                    return;
                }
            }
        }

        ret_code_t ret = sd_ble_gap_disconnect(s_bleInstance.connHandle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "sd_ble_gap_disconnect() failed: %u", (unsigned)ret);
            return;
        }
        LOG_DEBUG(TRACE, "Disconnecting. Update BLE connection parameters failed.");
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

static int addCharacteristic(uint16_t svcHandle, const ble_uuid_t* uuid, uint8_t properties,
                             const char* description, hal_ble_characteristic_t* characteristic) {
    ret_code_t               ret;
    ble_gatts_char_md_t      charMd;
    ble_gatts_attr_md_t      valueAttrMd;
    ble_gatts_attr_md_t      userDescAttrMd;
    ble_gatts_attr_md_t      cccdAttrMd;
    ble_gatts_attr_t         charValueAttr;
    ble_gatts_char_handles_t charHandles;

    /* Characteristic metadata */
    memset(&charMd, 0, sizeof(charMd));
    fromHalCharacteristicProperties(properties, &charMd.char_props);
    // User Description Descriptor
    if (description != NULL) {
        memset(&userDescAttrMd, 0, sizeof(userDescAttrMd));
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&userDescAttrMd.read_perm);
        BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&userDescAttrMd.write_perm);
        userDescAttrMd.vloc    = BLE_GATTS_VLOC_STACK;
        userDescAttrMd.rd_auth = 0;
        userDescAttrMd.wr_auth = 0;
        userDescAttrMd.vlen    = 0;

        charMd.p_char_user_desc        = (const uint8_t *)description;
        charMd.char_user_desc_max_size = strlen(description);
        charMd.char_user_desc_size     = strlen(description);
        charMd.p_user_desc_md          = &userDescAttrMd;
    }
    else {
        charMd.p_char_user_desc = NULL;
        charMd.p_user_desc_md   = NULL;
    }
    // Client Characteristic Configuration Descriptor
    if (charMd.char_props.notify || charMd.char_props.indicate) {
        memset(&cccdAttrMd, 0, sizeof(cccdAttrMd));
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccdAttrMd.read_perm);
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccdAttrMd.write_perm);
        cccdAttrMd.vloc = BLE_GATTS_VLOC_STACK;

        charMd.p_cccd_md = &cccdAttrMd;
    }
    // TODO:
    charMd.p_char_pf = NULL;
    charMd.p_sccd_md = NULL;

    /* Characteristic value attribute metadata */
    memset(&valueAttrMd, 0, sizeof(valueAttrMd));
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&valueAttrMd.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&valueAttrMd.write_perm);
    valueAttrMd.vloc    = BLE_GATTS_VLOC_STACK;
    valueAttrMd.rd_auth = 0;
    valueAttrMd.wr_auth = 0;
    valueAttrMd.vlen    = 1;

    /* Characteristic value attribute */
    memset(&charValueAttr, 0, sizeof(charValueAttr));
    charValueAttr.p_uuid    = uuid;
    charValueAttr.p_attr_md = &valueAttrMd;
    charValueAttr.init_len  = 0;
    charValueAttr.init_offs = 0;
    charValueAttr.max_len   = BLE_MAX_CHAR_VALUE_LEN;
    charValueAttr.p_value   = NULL;

    ret = sd_ble_gatts_characteristic_add(svcHandle, &charMd, &charValueAttr, &charHandles);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_characteristic_add() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    characteristic->properties       = properties;
    characteristic->value_handle     = charHandles.value_handle;
    characteristic->user_desc_handle = charHandles.user_desc_handle;
    characteristic->cccd_handle      = charHandles.cccd_handle;
    characteristic->sccd_handle      = charHandles.sccd_handle;

    LOG_DEBUG(TRACE, "Characteristic value handle: %d.", charHandles.value_handle);
    LOG_DEBUG(TRACE, "Characteristic cccd handle: %d.", charHandles.cccd_handle);

    return SYSTEM_ERROR_NONE;
}

static int writeAttribute(uint16_t connHandle, uint16_t attrHandle, uint8_t* data, uint16_t len, bool response) {
    ble_gattc_write_params_t writeParams;

    if (os_semaphore_create(&s_bleInstance.readWriteSemaphore, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }

    if (response) {
        writeParams.write_op = BLE_GATT_OP_WRITE_REQ;
    }
    else {
        writeParams.write_op = BLE_GATT_OP_WRITE_CMD;
    }
    writeParams.flags    = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE;
    writeParams.handle   = attrHandle;
    writeParams.offset   = 0;
    writeParams.len      = len;
    writeParams.p_value  = data;

    ret_code_t ret = sd_ble_gattc_write(connHandle, &writeParams);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gattc_write() failed: %u", (unsigned)ret);
        os_semaphore_destroy(s_bleInstance.readWriteSemaphore);
        return sysError(ret);
    }

    os_semaphore_take(s_bleInstance.readWriteSemaphore, BLE_GENERAL_PROCEDURE_TIMEOUT, false);
    os_semaphore_destroy(s_bleInstance.readWriteSemaphore);

    return SYSTEM_ERROR_NONE;
}

static void isrProcessBleEvent(const ble_evt_t* event, void* context) {
    ret_code_t      ret;
    HalBleEvtMsg_t  msg;
    memset(&msg, 0x00, sizeof(HalBleEvtMsg_t));

    switch (event->header.evt_id) {
        case BLE_GAP_EVT_ADV_SET_TERMINATED: {
            LOG_DEBUG(TRACE, "BLE GAP event: advertising stopped.");
            s_bleInstance.advertising = false;

            if (s_bleInstance.onEvtCb != NULL) {
                msg.handler = (evt_handler_t)s_bleInstance.onEvtCb;
                msg.arg.type = BLE_EVT_ADV_STOPPED;
                msg.arg.params.adv_stopped.version = 0x01;
                msg.arg.params.adv_stopped.reserved = NULL;
                if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
                    LOG(ERROR, "os_queue_put() failed.");
                }
            }
        } break;

        case BLE_GAP_EVT_ADV_REPORT: {
            LOG_DEBUG(TRACE, "BLE GAP event: advertising report.");

            if (s_bleInstance.onEvtCb != NULL) {
                msg.handler = (evt_handler_t)s_bleInstance.onEvtCb;
                msg.arg.type = BLE_EVT_SCAN_RESULT;
                msg.arg.params.scan_result.version = 0x01;
                msg.arg.params.scan_result.type.connectable = event->evt.gap_evt.params.adv_report.type.connectable;
                msg.arg.params.scan_result.type.scannable = event->evt.gap_evt.params.adv_report.type.scannable;
                msg.arg.params.scan_result.type.directed = event->evt.gap_evt.params.adv_report.type.directed;
                msg.arg.params.scan_result.type.scan_response = event->evt.gap_evt.params.adv_report.type.scan_response;
                msg.arg.params.scan_result.type.extended_pdu = event->evt.gap_evt.params.adv_report.type.extended_pdu;
                msg.arg.params.scan_result.rssi = event->evt.gap_evt.params.adv_report.rssi;
                msg.arg.params.scan_result.data_len = event->evt.gap_evt.params.adv_report.data.len;
                memcpy(msg.arg.params.scan_result.data, event->evt.gap_evt.params.adv_report.data.p_data, msg.arg.params.scan_result.data_len);
                msg.arg.params.scan_result.peer_addr.addr_type = event->evt.gap_evt.params.adv_report.peer_addr.addr_type;
                memcpy(msg.arg.params.scan_result.peer_addr.addr, event->evt.gap_evt.params.adv_report.peer_addr.addr, BLE_SIG_ADDR_LEN);
                if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
                    LOG(ERROR, "os_queue_put() failed.");
                }
            }

            // Continue scanning, scanning parameters pointer must be set to NULL.
            ret = sd_ble_gap_scan_start(NULL, &s_bleScanData);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gap_scan_start() failed: %u", (unsigned)ret);
            }
        } break;

        case BLE_GAP_EVT_CONNECTED: {
            // FIXME: If multi role is enabled, this flag should not be clear here.
            s_bleInstance.advertising = false;

            LOG_DEBUG(TRACE, "BLE GAP event: connected.");

            if (s_bleInstance.onEvtCb != NULL) {
                msg.handler = (evt_handler_t)s_bleInstance.onEvtCb;
                msg.arg.type = BLE_EVT_CONNECTED;
                msg.arg.params.connected.version = 0x01;
                msg.arg.params.connected.role = event->evt.gap_evt.params.connected.role;
                msg.arg.params.connected.conn_handle = event->evt.gap_evt.conn_handle;
                msg.arg.params.connected.conn_interval = event->evt.gap_evt.params.connected.conn_params.max_conn_interval;
                msg.arg.params.connected.slave_latency = event->evt.gap_evt.params.connected.conn_params.slave_latency;
                msg.arg.params.connected.conn_sup_timeout = event->evt.gap_evt.params.connected.conn_params.conn_sup_timeout;
                msg.arg.params.connected.peer_addr.addr_type = event->evt.gap_evt.params.connected.peer_addr.addr_type;
                memcpy(msg.arg.params.connected.peer_addr.addr, event->evt.gap_evt.params.connected.peer_addr.addr, BLE_SIG_ADDR_LEN);
                if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
                    LOG(ERROR, "os_queue_put() failed.");
                }
            }

            s_bleInstance.role       = event->evt.gap_evt.params.connected.role;
            s_bleInstance.connHandle = event->evt.gap_evt.conn_handle;
            s_bleInstance.connecting = false;
            s_bleInstance.connected  = true;
            toHalConnParams(&event->evt.gap_evt.params.connected.conn_params, &s_bleInstance.effectiveConnParams);
            s_bleInstance.peer_addr.addr_type = event->evt.gap_evt.params.connected.peer_addr.addr_type;
            memcpy(s_bleInstance.peer_addr.addr, event->evt.gap_evt.params.connected.peer_addr.addr, BLE_SIG_ADDR_LEN);

            LOG_DEBUG(TRACE, "BLE role: %d, connection handle: %d", s_bleInstance.role, s_bleInstance.connHandle);
            LOG_DEBUG(TRACE, "| interval(ms)  latency  timeout(ms) |");
            LOG_DEBUG(TRACE, "  %.2f          %d       %d",
                    s_bleInstance.effectiveConnParams.max_conn_interval*1.25,
                    s_bleInstance.effectiveConnParams.slave_latency,
                    s_bleInstance.effectiveConnParams.conn_sup_timeout*10);

            // Update connection parameters if needed.
            s_bleInstance.connParamsUpdateAttempts = 0;
            updateConnParamsIfNeeded();

            os_semaphore_give(s_bleInstance.connectSemaphore, false);
        } break;

        case BLE_GAP_EVT_DISCONNECTED: {
            LOG_DEBUG(TRACE, "BLE GAP event: disconnected, handle: 0x%04X, reason: %d",
                      event->evt.gap_evt.conn_handle,
                      event->evt.gap_evt.params.disconnected.reason);

            // Stop the connection parameters update timer.
            if (!os_timer_is_active(s_bleInstance.connParamsUpdateTimer, NULL)) {
                os_timer_change(s_bleInstance.connParamsUpdateTimer, OS_TIMER_CHANGE_STOP, true, 0, 0, NULL);
            }

            if (s_bleInstance.onEvtCb != NULL) {
                msg.handler = (evt_handler_t)s_bleInstance.onEvtCb;
                msg.arg.type = BLE_EVT_DISCONNECTED;
                msg.arg.params.disconnected.version = 0x01;
                msg.arg.params.disconnected.reason = event->evt.gap_evt.params.disconnected.reason;
                msg.arg.params.disconnected.conn_handle = event->evt.gap_evt.conn_handle;
                msg.arg.params.disconnected.peer_addr.addr_type = s_bleInstance.peer_addr.addr_type;
                memcpy(msg.arg.params.disconnected.peer_addr.addr, s_bleInstance.peer_addr.addr, BLE_SIG_ADDR_LEN);
                if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
                    LOG(ERROR, "os_queue_put() failed.");
                }
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

            os_semaphore_give(s_bleInstance.disconnectSemaphore, false);
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

            toHalConnParams(&event->evt.gap_evt.params.conn_param_update.conn_params, &s_bleInstance.effectiveConnParams);

            LOG_DEBUG(TRACE, "| interval(ms)  latency  timeout(ms) |");
            LOG_DEBUG(TRACE, "  %.2f          %d       %d",
                    s_bleInstance.effectiveConnParams.max_conn_interval*1.25,
                    s_bleInstance.effectiveConnParams.slave_latency,
                    s_bleInstance.effectiveConnParams.conn_sup_timeout*10);

            // Update connection parameters if needed.
            updateConnParamsIfNeeded();

            if (s_bleInstance.onEvtCb != NULL) {
                msg.handler = (evt_handler_t)s_bleInstance.onEvtCb;
                msg.arg.type = BLE_EVT_CONN_PARAMS_UPDATED;
                msg.arg.params.conn_params_updated.version = 0x01;
                msg.arg.params.conn_params_updated.conn_handle = event->evt.gap_evt.conn_handle;
                msg.arg.params.conn_params_updated.conn_interval = event->evt.gap_evt.params.conn_param_update.conn_params.max_conn_interval;
                msg.arg.params.conn_params_updated.slave_latency = event->evt.gap_evt.params.conn_param_update.conn_params.slave_latency;
                msg.arg.params.conn_params_updated.conn_sup_timeout = event->evt.gap_evt.params.conn_param_update.conn_params.conn_sup_timeout;
                if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
                    LOG(ERROR, "os_queue_put() failed.");
                }
            }
            os_semaphore_give(s_bleInstance.connUpdateSemaphore, false);
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
            ret = sd_ble_gap_data_length_update(event->evt.gap_evt.conn_handle, &gapDataLenParams, NULL);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gap_data_length_update() failed: %u", (unsigned)ret);
            }
        } break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST: {
            LOG_DEBUG(TRACE, "BLE GAP event: security parameters request.");

            // Pairing is not supported
            ret = sd_ble_gap_sec_params_reply(event->evt.gap_evt.conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, nullptr, nullptr);
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

                if (s_bleInstance.onEvtCb != NULL) {
                    msg.handler = (evt_handler_t)s_bleInstance.onEvtCb;
                    msg.arg.type = BLE_EVT_SCAN_STOPPED;
                    msg.arg.params.scan_stopped.version = 0x01;
                    msg.arg.params.scan_stopped.reserved = NULL;
                    if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
                        LOG(ERROR, "os_queue_put() failed.");
                    }
                }
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

        case BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP: {
            uint8_t svcCount = event->evt.gattc_evt.params.prim_srvc_disc_rsp.count;
            LOG_DEBUG(TRACE, "BLE GATT Client event: %d primary service discovered.", svcCount);

            bool terminate = true;
            if ( (event->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_SUCCESS)
              && (s_bleInstance.discovery.currDiscProcedure == BLE_DISCOVERY_PROCEDURE_SERVICES) ) {
                for (uint8_t i = 0; i < svcCount; i++) {
                    s_bleInstance.discovery.services[s_bleInstance.discovery.count].start_handle = event->evt.gattc_evt.params.prim_srvc_disc_rsp.services[i].handle_range.start_handle;
                    s_bleInstance.discovery.services[s_bleInstance.discovery.count].end_handle   = event->evt.gattc_evt.params.prim_srvc_disc_rsp.services[i].handle_range.end_handle;
                    toHalUuid(&event->evt.gattc_evt.params.prim_srvc_disc_rsp.services[i].uuid, &s_bleInstance.discovery.services[s_bleInstance.discovery.count].uuid);
                    s_bleInstance.discovery.count++;
                    if (s_bleInstance.discovery.count >= BLE_MAX_SVC_COUNT) {
                        break;
                    }
                }
                uint16_t currEndHandle = event->evt.gattc_evt.params.prim_srvc_disc_rsp.services[svcCount-1].handle_range.end_handle;
                if ( (s_bleInstance.discovery.discAll)
                  && (s_bleInstance.discovery.count < BLE_MAX_SVC_COUNT)
                  && (currEndHandle < 0xFFFF) ) {
                    ret = sd_ble_gattc_primary_services_discover(event->evt.gattc_evt.conn_handle, currEndHandle + 1, NULL);
                    if (ret != NRF_SUCCESS) {
                        LOG(ERROR, "sd_ble_gattc_primary_services_discover() failed: %u", (unsigned)ret);
                    }
                    else {
                        terminate = false;
                    }
                }
            }
            else {
                LOG(ERROR, "BLE service discovery failed: %d, handle: %d.", event->evt.gattc_evt.gatt_status, event->evt.gattc_evt.error_handle);
            }

            if (terminate) {
                if (s_bleInstance.onEvtCb != NULL) {
                    msg.handler = (evt_handler_t)s_bleInstance.onEvtCb;
                    msg.arg.type = BLE_EVT_SVC_DISCOVERED;
                    msg.arg.params.svc_disc.version = 0x01;
                    msg.arg.params.svc_disc.conn_handle = event->evt.gattc_evt.conn_handle;
                    msg.arg.params.svc_disc.count = s_bleInstance.discovery.count;
                    msg.arg.params.svc_disc.services = s_bleInstance.discovery.services;
                    if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
                        LOG(ERROR, "os_queue_put() failed.");
                        // This flag should be cleared after this event being popped, in case that
                        // further operation is needed to retrieve 128-bits UUID. If this event is
                        // not enqueued successfully, this flag should be clear here.
                        s_bleInstance.discovery.currDiscProcedure = BLE_DISCOVERY_PROCEDURE_IDLE;
                        s_bleInstance.discovering = false;
                        os_semaphore_give(s_bleInstance.discoverySemaphore, false);
                    }
                }
            }
        } break;

        case BLE_GATTC_EVT_CHAR_DISC_RSP: {
            uint8_t charCount = event->evt.gattc_evt.params.char_disc_rsp.count;
            LOG_DEBUG(TRACE, "BLE GATT Client event: %d characteristic discovered.", charCount);

            bool terminate = true;
            if ( (event->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_SUCCESS)
              && (s_bleInstance.discovery.currDiscProcedure == BLE_DISCOVERY_PROCEDURE_CHARACTERISTICS) ) {
                for (uint8_t i = 0; i < charCount; i++) {
                    s_bleInstance.discovery.characteristics[s_bleInstance.discovery.count].char_ext_props = event->evt.gattc_evt.params.char_disc_rsp.chars[i].char_ext_props;
                    s_bleInstance.discovery.characteristics[s_bleInstance.discovery.count].properties     = toHalCharacteristicProperties(event->evt.gattc_evt.params.char_disc_rsp.chars[i].char_props);
                    s_bleInstance.discovery.characteristics[s_bleInstance.discovery.count].decl_handle    = event->evt.gattc_evt.params.char_disc_rsp.chars[i].handle_decl;
                    s_bleInstance.discovery.characteristics[s_bleInstance.discovery.count].value_handle   = event->evt.gattc_evt.params.char_disc_rsp.chars[i].handle_value;
                    toHalUuid(&event->evt.gattc_evt.params.char_disc_rsp.chars[i].uuid, &s_bleInstance.discovery.characteristics[s_bleInstance.discovery.count].uuid);

                    s_bleInstance.discovery.count++;
                    if (s_bleInstance.discovery.count >= BLE_MAX_CHAR_COUNT) {
                        break;
                    }
                }

                uint16_t currEndHandle = event->evt.gattc_evt.params.char_disc_rsp.chars[charCount-1].handle_value;
                if ( (currEndHandle < s_bleInstance.discovery.discEndHandle)
                  && (s_bleInstance.discovery.count < BLE_MAX_CHAR_COUNT) ) {
                    ble_gattc_handle_range_t handleRange;
                    handleRange.start_handle = currEndHandle + 1;
                    handleRange.end_handle   = s_bleInstance.discovery.discEndHandle;
                    ret = sd_ble_gattc_characteristics_discover(event->evt.gattc_evt.conn_handle, &handleRange);
                    if (ret != NRF_SUCCESS) {
                        LOG(ERROR, "sd_ble_gattc_characteristics_discover() failed: %u", (unsigned)ret);
                    }
                    else {
                        terminate = false;
                    }
                }
            }
            else {
                LOG(ERROR, "BLE characteristic discovery failed: %d, handle: %d.", event->evt.gattc_evt.gatt_status, event->evt.gattc_evt.error_handle);
            }

            if (terminate) {
                if (s_bleInstance.onEvtCb != NULL) {
                    msg.handler = (evt_handler_t)s_bleInstance.onEvtCb;
                    msg.arg.type = BLE_EVT_CHAR_DISCOVERED;
                    msg.arg.params.char_disc.version = 0x01;
                    msg.arg.params.char_disc.conn_handle = event->evt.gattc_evt.conn_handle;
                    msg.arg.params.char_disc.count = s_bleInstance.discovery.count;
                    msg.arg.params.char_disc.characteristics = s_bleInstance.discovery.characteristics;
                    if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
                        LOG(ERROR, "os_queue_put() failed.");
                        // This flag should be cleared after this event being popped, in case that
                        // further operation is needed to retrieve 128-bits UUID. If this event is
                        // not enqueued successfully, this flag should be clear here.
                        s_bleInstance.discovery.currDiscProcedure = BLE_DISCOVERY_PROCEDURE_IDLE;
                        s_bleInstance.discovering = false;
                        os_semaphore_give(s_bleInstance.discoverySemaphore, false);
                    }
                }
            }
        } break;

        case BLE_GATTC_EVT_DESC_DISC_RSP: {
            uint8_t descCount = event->evt.gattc_evt.params.desc_disc_rsp.count;
            LOG_DEBUG(TRACE, "BLE GATT Client event: %d descriptors discovered.", descCount);

            bool terminate = true;
            if ( (event->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_SUCCESS)
              && (s_bleInstance.discovery.currDiscProcedure == BLE_DISCOVERY_PROCEDURE_DESCRIPTORS) ) {
                for (uint8_t i = 0; i < descCount; i++) {
                    // It will report all attributes with 16-bits UUID, filter descriptors.
                    switch (event->evt.gattc_evt.params.desc_disc_rsp.descs[i].uuid.uuid) {
                        case BLE_SIG_UUID_CHAR_EXTENDED_PROPERTIES_DESC:
                        case BLE_SIG_UUID_CHAR_USER_DESCRIPTION_DESC:
                        case BLE_SIG_UUID_CLIENT_CHAR_CONFIG_DESC:
                        case BLE_SIG_UUID_SERVER_CHAR_CONFIG_DESC:
                        case BLE_SIG_UUID_CHAR_PRESENT_FORMAT_DESC:
                        case BLE_SIG_UUID_CHAR_AGGREGATE_FORMAT: {
                            s_bleInstance.discovery.descriptors[s_bleInstance.discovery.count].handle = event->evt.gattc_evt.params.desc_disc_rsp.descs[i].handle;
                            toHalUuid(&event->evt.gattc_evt.params.desc_disc_rsp.descs[i].uuid, &s_bleInstance.discovery.descriptors[s_bleInstance.discovery.count].uuid);
                            s_bleInstance.discovery.count++;
                        }
                        default: break;
                    }
                    if (s_bleInstance.discovery.count >= BLE_MAX_DESC_COUNT) {
                        break;
                    }
                }

                uint16_t currEndHandle = event->evt.gattc_evt.params.desc_disc_rsp.descs[descCount-1].handle;
                if ( (currEndHandle < s_bleInstance.discovery.discEndHandle)
                  && (s_bleInstance.discovery.count < BLE_MAX_DESC_COUNT) ) {
                    ble_gattc_handle_range_t handleRange;
                    handleRange.start_handle = currEndHandle + 1;
                    handleRange.end_handle   = s_bleInstance.discovery.discEndHandle;
                    ret = sd_ble_gattc_descriptors_discover(event->evt.gattc_evt.conn_handle, &handleRange);
                    if (ret != NRF_SUCCESS) {
                        LOG(ERROR, "sd_ble_gattc_descriptors_discover() failed: %u", (unsigned)ret);
                    }
                    else {
                        terminate = false;
                    }
                }
            }
            else {
                LOG(ERROR, "BLE characteristic discovery failed: %d, handle: %d.", event->evt.gattc_evt.gatt_status, event->evt.gattc_evt.error_handle);
            }

            if (terminate) {
                if (s_bleInstance.onEvtCb != NULL) {
                    msg.handler = (evt_handler_t)s_bleInstance.onEvtCb;
                    msg.arg.type = BLE_EVT_DESC_DISCOVERED;
                    msg.arg.params.desc_disc.version = 0x01;
                    msg.arg.params.desc_disc.conn_handle = event->evt.gattc_evt.conn_handle;
                    msg.arg.params.desc_disc.count = s_bleInstance.discovery.count;
                    msg.arg.params.desc_disc.descriptors = s_bleInstance.discovery.descriptors;
                    if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
                        LOG(ERROR, "os_queue_put() failed.");
                        // This flag should be cleared after this event being popped, in case that
                        // further operation is needed to retrieve 128-bits UUID. If this event is
                        // not enqueued successfully, this flag should be clear here.
                        s_bleInstance.discovery.currDiscProcedure = BLE_DISCOVERY_PROCEDURE_IDLE;
                        s_bleInstance.discovering = false;
                        os_semaphore_give(s_bleInstance.discoverySemaphore, false);
                    }
                }
            }
        } break;

        case BLE_GATTC_EVT_READ_RSP: {
            LOG_DEBUG(TRACE, "BLE GATT Client event: read response.");

            if (event->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_SUCCESS) {
                if (s_bleInstance.discovering) {
                    if ( (s_bleInstance.discovery.currDiscProcedure == BLE_DISCOVERY_PROCEDURE_SERVICES)
                      && (event->evt.gattc_evt.params.read_rsp.handle == s_bleInstance.discovery.services[s_bleInstance.discovery.currIndex].start_handle) ) {
                        s_bleInstance.discovery.services[s_bleInstance.discovery.currIndex].uuid.type = BLE_UUID_TYPE_128BIT;
                        memcpy(s_bleInstance.discovery.services[s_bleInstance.discovery.currIndex].uuid.uuid128, event->evt.gattc_evt.params.read_rsp.data, BLE_SIG_UUID_128BIT_LEN);
                        os_semaphore_give(s_bleInstance.discovery.semaphore, false);
                    }
                    else if ( (s_bleInstance.discovery.currDiscProcedure == BLE_DISCOVERY_PROCEDURE_CHARACTERISTICS)
                           && (event->evt.gattc_evt.params.read_rsp.handle == s_bleInstance.discovery.characteristics[s_bleInstance.discovery.currIndex].decl_handle) ) {
                        s_bleInstance.discovery.characteristics[s_bleInstance.discovery.currIndex].uuid.type = BLE_UUID_TYPE_128BIT;
                        // The offset of Characteristic UUID in the Characteristic declaration attribute is 3.
                        memcpy(s_bleInstance.discovery.characteristics[s_bleInstance.discovery.currIndex].uuid.uuid128, &event->evt.gattc_evt.params.read_rsp.data[3], BLE_SIG_UUID_128BIT_LEN);
                        os_semaphore_give(s_bleInstance.discovery.semaphore, false);
                    }
                }
                else {
                    if (s_bleInstance.onEvtCb != NULL) {
                        msg.handler = (evt_handler_t)s_bleInstance.onEvtCb;
                        msg.arg.type = BLE_EVT_DATA_READ;
                        msg.arg.params.data_rec.version = 0x01;
                        msg.arg.params.data_rec.conn_handle = event->evt.gattc_evt.conn_handle;
                        msg.arg.params.data_rec.attr_handle = event->evt.gattc_evt.params.read_rsp.handle;
                        msg.arg.params.data_rec.offset = event->evt.gattc_evt.params.read_rsp.offset;
                        msg.arg.params.data_rec.data_len = event->evt.gattc_evt.params.read_rsp.len;
                        memcpy(msg.arg.params.data_rec.data, event->evt.gattc_evt.params.read_rsp.data, msg.arg.params.data_rec.data_len);
                        if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
                            LOG(ERROR, "os_queue_put() failed.");
                        }
                    }
                    os_semaphore_give(s_bleInstance.readWriteSemaphore, false);
                }
            }
            else {
                LOG(ERROR, "BLE read characteristic failed: %d, handle: %d.", event->evt.gattc_evt.gatt_status, event->evt.gattc_evt.error_handle);
            }
        } break;

        case BLE_GATTC_EVT_WRITE_RSP: {
            LOG_DEBUG(TRACE, "BLE GATT Client event: write with response completed.");
            os_semaphore_give(s_bleInstance.readWriteSemaphore, false);
        } break;

        case BLE_GATTC_EVT_WRITE_CMD_TX_COMPLETE: {
            LOG_DEBUG(TRACE, "BLE GATT Client event: write without response completed.");
            os_semaphore_give(s_bleInstance.readWriteSemaphore, false);
        } break;

        case BLE_GATTC_EVT_HVX: {
            LOG_DEBUG(TRACE, "BLE GATT Client event: data received. conn_handle: %d, attr_handle: %d, len: %d, type: %d",
                    event->evt.gattc_evt.conn_handle, event->evt.gattc_evt.params.hvx.handle,
                    event->evt.gattc_evt.params.hvx.len, event->evt.gattc_evt.params.hvx.type);

            if (event->evt.gattc_evt.params.hvx.type == BLE_GATT_HVX_INDICATION) {
                ret = sd_ble_gattc_hv_confirm(event->evt.gattc_evt.conn_handle, event->evt.gattc_evt.params.hvx.handle);
                if (ret != NRF_SUCCESS) {
                    LOG(ERROR, "sd_ble_gattc_hv_confirm() failed: %u", (unsigned)ret);
                }
            }

            if (s_bleInstance.onEvtCb != NULL) {
                msg.handler = (evt_handler_t)s_bleInstance.onEvtCb;
                msg.arg.type = BLE_EVT_DATA_NOTIFIED;
                msg.arg.params.data_rec.version = 0x01;
                msg.arg.params.data_rec.conn_handle = event->evt.gattc_evt.conn_handle;
                msg.arg.params.data_rec.attr_handle = event->evt.gattc_evt.params.hvx.handle;
                msg.arg.params.data_rec.offset = 0;
                msg.arg.params.data_rec.data_len = event->evt.gattc_evt.params.hvx.len;
                memcpy(msg.arg.params.data_rec.data, event->evt.gattc_evt.params.hvx.data, msg.arg.params.data_rec.data_len);
                if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
                    LOG(ERROR, "os_queue_put() failed.");
                }
            }
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

            ret = sd_ble_gatts_exchange_mtu_reply(event->evt.gatts_evt.conn_handle, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gatts_exchange_mtu_reply() failed: %u", (unsigned)ret);
            }

            LOG_DEBUG(TRACE, "Reply with Server RX MTU: %d.", NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
        } break;

        case BLE_GATTS_EVT_WRITE: {
            LOG_DEBUG(TRACE, "BLE GATT Server event: write characteristic.");

            // TODO: deal with different write commands.

            if (s_bleInstance.onEvtCb != NULL) {
                msg.handler = (evt_handler_t)s_bleInstance.onEvtCb;
                msg.arg.type = BLE_EVT_DATA_WRITTEN;
                msg.arg.params.data_rec.version = 0x01;
                msg.arg.params.data_rec.conn_handle = event->evt.gatts_evt.conn_handle;
                msg.arg.params.data_rec.attr_handle = event->evt.gatts_evt.params.write.handle;
                msg.arg.params.data_rec.offset = event->evt.gatts_evt.params.write.offset;
                msg.arg.params.data_rec.data_len = event->evt.gatts_evt.params.write.len;
                memcpy(msg.arg.params.data_rec.data, event->evt.gatts_evt.params.write.data, msg.arg.params.data_rec.data_len);
                if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
                    LOG(ERROR, "os_queue_put() failed.");
                }
            }
        } break;

        case BLE_GATTS_EVT_HVN_TX_COMPLETE: {
            LOG_DEBUG(TRACE, "BLE GATT Server event: notification sent.");
            os_semaphore_give(s_bleInstance.readWriteSemaphore, false);
        } break;

        case BLE_GATTS_EVT_HVC: {
            LOG_DEBUG(TRACE, "BLE GATT Server event: indication confirmed.");
            os_semaphore_give(s_bleInstance.readWriteSemaphore, false);
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
        HalBleEvtMsg_t msg;
        if (!os_queue_take(s_bleInstance.evtQueue, &msg, CONCURRENT_WAIT_FOREVER, NULL)) {
            // Try retrieving 128-bits service or characteristic UUID before dispatching this event.
            if (   msg.arg.type == BLE_EVT_SVC_DISCOVERED
                || msg.arg.type == BLE_EVT_CHAR_DISCOVERED
                || msg.arg.type == BLE_EVT_DESC_DISCOVERED) {
                if (msg.arg.type == BLE_EVT_SVC_DISCOVERED) {
                    hal_ble_gatt_client_on_services_discovered_evt_t *svcDiscEvt = (hal_ble_gatt_client_on_services_discovered_evt_t*)&msg.arg.params;
                    for (uint8_t i = 0; i < svcDiscEvt->count; i++) {
                        if (svcDiscEvt->services[i].uuid.type == BLE_UUID_TYPE_128BIT_SHORTED) {
                            if (!os_semaphore_create(&s_bleInstance.discovery.semaphore, 1, 0)) {
                                ret_code_t ret = sd_ble_gattc_read(svcDiscEvt->conn_handle, svcDiscEvt->services[i].start_handle, 0);
                                if (ret == NRF_SUCCESS) {
                                    s_bleInstance.discovery.currIndex = i;
                                    if (!os_semaphore_take(s_bleInstance.discovery.semaphore, BLE_GENERAL_PROCEDURE_TIMEOUT, false)) {
                                        os_semaphore_destroy(s_bleInstance.discovery.semaphore);
                                        continue;
                                    }
                                }
                                else {
                                    LOG(ERROR, "sd_ble_gattc_read() failed: %u", (unsigned)ret);
                                }
                                os_semaphore_destroy(s_bleInstance.discovery.semaphore);
                                break;
                            }
                            else {
                                LOG(ERROR, "os_semaphore_create() failed");
                                break;
                            }
                        }
                    }
                }
                else if (msg.arg.type == BLE_EVT_CHAR_DISCOVERED) {
                    hal_ble_gatt_client_on_characteristics_discovered_evt_t* charDiscEvt = (hal_ble_gatt_client_on_characteristics_discovered_evt_t*)&msg.arg.params;
                    for (uint8_t i = 0; i < charDiscEvt->count; i++) {
                        if (charDiscEvt->characteristics[i].uuid.type == BLE_UUID_TYPE_128BIT_SHORTED) {
                            if (!os_semaphore_create(&s_bleInstance.discovery.semaphore, 1, 0)) {
                                ret_code_t ret = sd_ble_gattc_read(charDiscEvt->conn_handle, charDiscEvt->characteristics[i].decl_handle, 0);
                                if (ret == NRF_SUCCESS) {
                                    s_bleInstance.discovery.currIndex = i;
                                    if (!os_semaphore_take(s_bleInstance.discovery.semaphore, 2000, false)) {
                                        os_semaphore_destroy(s_bleInstance.discovery.semaphore);
                                        continue;
                                    }
                                }
                                else {
                                    LOG(ERROR, "sd_ble_gattc_read() failed: %u", (unsigned)ret);
                                }
                                os_semaphore_destroy(s_bleInstance.discovery.semaphore);
                                break;
                            }
                            else {
                                LOG(ERROR, "os_semaphore_create() failed");
                                break;
                            }
                        }
                    }
                }
            }

            if (msg.handler != NULL) {
                msg.handler(&msg.arg, msg.context);
            }

            if (   msg.arg.type == BLE_EVT_SVC_DISCOVERED
                || msg.arg.type == BLE_EVT_CHAR_DISCOVERED
                || msg.arg.type == BLE_EVT_DESC_DISCOVERED) {
                s_bleInstance.discovery.currDiscProcedure = BLE_DISCOVERY_PROCEDURE_IDLE;
                s_bleInstance.discovering = false;
                os_semaphore_give(s_bleInstance.discoverySemaphore, false);
            }
        }
        else {
            LOG(ERROR, "BLE event thread exited.");
            break;
        }
    }

    os_thread_exit(s_bleInstance.evtThread);
}


/**********************************************
 * Particle BLE APIs
 */
bool ble_stack_is_initialized(void) {
    return s_bleInstance.initialized;
}

int ble_stack_init(void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());

    /* The SoftDevice has been enabled in core_hal.c */
    LOG_DEBUG(TRACE, "ble_stack_init().");

    if (!s_bleInstance.initialized) {
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

        s_bleInstance.connHandle = BLE_INVALID_CONN_HANDLE;

        // Configure an advertising set to obtain an advertising handle.
        int error = ble_gap_set_advertising_parameters(&s_bleInstance.advParams, NULL);
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

        // Select internal antenna by default
        ble_select_antenna(BLE_ANT_INTERNAL);

        if (os_queue_create(&s_bleInstance.evtQueue, sizeof(HalBleEvtMsg_t), BLE_EVENT_QUEUE_ITEM_COUNT, NULL)) {
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

int ble_select_antenna(hal_ble_antenna_type_t antenna) {
    // FIXME: mesh SoMs specific configurations
    HAL_Pin_Mode(ANTSW1, OUTPUT);
#if (PLATFORM_ID == PLATFORM_XENON) || (PLATFORM_ID == PLATFORM_ARGON)
    HAL_Pin_Mode(ANTSW2, OUTPUT);
#endif

    if (antenna == BLE_ANT_EXTERNAL) {
#if (PLATFORM_ID == PLATFORM_ARGON)
        HAL_GPIO_Write(ANTSW1, 1);
        HAL_GPIO_Write(ANTSW2, 0);
#elif (PLATFORM_ID == PLATFORM_BORON)
        HAL_GPIO_Write(ANTSW1, 0);
#else
        HAL_GPIO_Write(ANTSW1, 0);
        HAL_GPIO_Write(ANTSW2, 1);
#endif
    }
    else {
#if (PLATFORM_ID == PLATFORM_ARGON)
        HAL_GPIO_Write(ANTSW1, 0);
        HAL_GPIO_Write(ANTSW2, 1);
#elif (PLATFORM_ID == PLATFORM_BORON)
        HAL_GPIO_Write(ANTSW1, 1);
#else
        HAL_GPIO_Write(ANTSW1, 1);
        HAL_GPIO_Write(ANTSW2, 0);
#endif
    }

    return SYSTEM_ERROR_NONE;
}

int ble_set_callback_on_events(ble_on_events_callback_t callback, void* context) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

    s_bleInstance.onEvtCb = callback;
    s_bleInstance.context = context;
    return SYSTEM_ERROR_NONE;
}


/**********************************************
 * BLE GAP APIs
 */
int ble_gap_set_device_address(hal_ble_address_t const* address) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_set_device_address().");

    if (address == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (address->addr_type != BLE_SIG_ADDR_TYPE_PUBLIC && address->addr_type != BLE_SIG_ADDR_TYPE_RANDOM_STATIC) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (s_bleInstance.advertising || s_bleInstance.scanning || s_bleInstance.connecting) {
        // The identity address cannot be changed while advertising, scanning or creating a connection.
        return SYSTEM_ERROR_INVALID_STATE;
    }

    ble_gap_addr_t localAddr;
    localAddr.addr_id_peer = false;
    localAddr.addr_type    = address->addr_type;
    memcpy(localAddr.addr, address->addr, BLE_SIG_ADDR_LEN);

    ret_code_t ret = sd_ble_gap_addr_set(&localAddr);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_addr_set() failed: %u", (unsigned)ret);
    }

    return sysError(ret);
}

int ble_gap_get_device_address(hal_ble_address_t* address) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_get_device_address().");

    if (address == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ble_gap_addr_t localAddr;
    ret_code_t ret = sd_ble_gap_addr_get(&localAddr);
    if (ret == NRF_SUCCESS) {
        address->addr_type = localAddr.addr_type;
        memcpy(address->addr, localAddr.addr, BLE_SIG_ADDR_LEN);
    }
    else {
        LOG(ERROR, "sd_ble_gap_addr_get() failed: %u", (unsigned)ret);
    }

    return sysError(ret);
}

int ble_gap_set_device_name(uint8_t const* device_name, uint16_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_set_device_name().");

    if (device_name == NULL || len == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ble_gap_conn_sec_mode_t secMode;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&secMode);

    ret_code_t ret = sd_ble_gap_device_name_set(&secMode, device_name, len);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_device_name_set() failed: %u", (unsigned)ret);
    }

    return sysError(ret);
}

int ble_gap_get_device_name(uint8_t* device_name, uint16_t* len) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_get_device_name().");

    if (device_name == NULL || len == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    // non NULL-terminated string returned.
    ret_code_t ret = sd_ble_gap_device_name_get(device_name, len);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_device_name_get() failed: %u", (unsigned)ret);
    }

    return sysError(ret);
}

int ble_gap_set_appearance(uint16_t appearance) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_set_appearance().");

    ret_code_t ret = sd_ble_gap_appearance_set(appearance);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_appearance_set() failed: %u", (unsigned)ret);
    }

    return sysError(ret);
}

int ble_gap_get_appearance(uint16_t* appearance) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_get_appearance().");

    if (appearance == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ret_code_t ret = sd_ble_gap_appearance_get(appearance);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_appearance_get() failed: %u", (unsigned)ret);
    }

    return sysError(ret);
}

int ble_gap_set_ppcp(hal_ble_connection_parameters_t* ppcp, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_set_ppcp().");

    if (ppcp == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if (    ppcp->min_conn_interval != BLE_SIG_CP_MIN_CONN_INTERVAL_NONE
        && (ppcp->min_conn_interval < BLE_SIG_CP_MIN_CONN_INTERVAL_MIN
        ||  ppcp->min_conn_interval > BLE_SIG_CP_MIN_CONN_INTERVAL_MAX)) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (    ppcp->max_conn_interval != BLE_SIG_CP_MAX_CONN_INTERVAL_NONE
        && (ppcp->max_conn_interval < BLE_SIG_CP_MAX_CONN_INTERVAL_MIN
        ||  ppcp->max_conn_interval > BLE_SIG_CP_MAX_CONN_INTERVAL_MAX)) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (ppcp->slave_latency > BLE_SIG_CP_SLAVE_LATENCY_MAX) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (    ppcp->conn_sup_timeout != BLE_SIG_CP_CONN_SUP_TIMEOUT_NONE
        && (ppcp->conn_sup_timeout < BLE_SIG_CP_CONN_SUP_TIMEOUT_MIN
        ||  ppcp->conn_sup_timeout > BLE_SIG_CP_CONN_SUP_TIMEOUT_MAX)) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ble_gap_conn_params_t connParams;
    fromHalConnParams(ppcp, &connParams);

    ret_code_t ret = sd_ble_gap_ppcp_set(&connParams);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_ppcp_set() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    memcpy(&s_bleInstance.ppcp, ppcp, sizeof(hal_ble_connection_parameters_t));

    return SYSTEM_ERROR_NONE;
}

int ble_gap_get_ppcp(hal_ble_connection_parameters_t* ppcp, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

    LOG_DEBUG(TRACE, "ble_get_ppcp().");

    if (ppcp == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ble_gap_conn_params_t connParams;
    ret_code_t ret = sd_ble_gap_ppcp_get(&connParams);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_ppcp_set() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    return toHalConnParams(&connParams, ppcp);
}

int ble_gap_add_whitelist(hal_ble_address_t* addr_list, uint8_t len, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_add_whitelist().");

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

int ble_gap_delete_whitelist(void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_delete_whitelist().");

    ret_code_t ret = sd_ble_gap_whitelist_set(NULL, 0);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_whitelist_set() failed: %u", (unsigned)ret);
    }

    return sysError(ret);
}

int ble_gap_set_tx_power(int8_t value) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_set_tx_power().");

    ret_code_t ret = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, s_bleInstance.advHandle, roundTxPower(value));
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_tx_power_set() failed: %u", (unsigned)ret);
    }

    s_bleInstance.txPower = roundTxPower(value);

    return sysError(ret);
}

int ble_gap_get_tx_power(int8_t* value) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_get_tx_power().");

    if (value == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    *value = s_bleInstance.txPower;

    return SYSTEM_ERROR_NONE;
}

int ble_gap_set_advertising_parameters(hal_ble_advertising_parameters_t* adv_params, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_set_advertising_parameters().");

    if (adv_params == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ret_code_t ret;
    bool advPending = false;

    // It is invalid to change advertising set parameters while advertising.
    if (s_bleInstance.advertising) {
        int err = ble_gap_stop_advertising();
        if (err != SYSTEM_ERROR_NONE) {
            return err;
        }
        advPending = true;
    }

    ble_gap_adv_params_t bleGapAdvParams;
    memset(&bleGapAdvParams, 0x00, sizeof(ble_gap_adv_params_t));

    bleGapAdvParams.properties.type             = adv_params->type;
    bleGapAdvParams.properties.include_tx_power = false; // FIXME: for extended advertising packet
    bleGapAdvParams.p_peer_addr                 = NULL;
    bleGapAdvParams.interval                    = adv_params->interval;
    bleGapAdvParams.duration                    = adv_params->timeout;
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
        return ble_gap_start_advertising(NULL);
    }

    memcpy(&s_bleInstance.advParams, adv_params, sizeof(hal_ble_advertising_parameters_t));

    return sysError(ret);
}

int ble_gap_set_advertising_data(uint8_t* data, uint16_t len, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_set_advertising_data().");

    if ((data == NULL) || (len < 3)) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    return setAdvData(data, len, BLE_ADV_DATA_ADVERTISING);
}

int ble_gap_set_scan_response_data(uint8_t* data, uint16_t len, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_set_scan_response_data().");

    if (data == NULL) {
        len = 0;
    }

    return setAdvData(data, len, BLE_ADV_DATA_SCAN_RESPONSE);
}

int ble_gap_start_advertising(void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_start_advertising().");

    if (s_bleInstance.advertising) {
        // Restart advertising
        int error = ble_gap_stop_advertising();
        if (error != SYSTEM_ERROR_NONE) {
            return error;
        }
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

int ble_gap_stop_advertising(void) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_stop_advertising().");

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

bool ble_gap_is_advertising(void) {
    return s_bleInstance.advertising;
}

int ble_gap_set_scan_parameters(hal_ble_scan_parameters_t* scan_params, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_set_scan_parameters().");

    if (scan_params == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if (    (scan_params->interval < BLE_GAP_SCAN_INTERVAL_MIN)
         || (scan_params->interval > BLE_GAP_SCAN_INTERVAL_MAX)
         || (scan_params->window < BLE_GAP_SCAN_WINDOW_MIN)
         || (scan_params->window > BLE_GAP_SCAN_WINDOW_MAX)
         || (scan_params->window > scan_params->interval) ) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    memcpy(&s_bleInstance.scanParams, scan_params, sizeof(hal_ble_scan_parameters_t));

    return SYSTEM_ERROR_NONE;
}

int ble_gap_start_scan(void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_start_scan().");

    if (s_bleInstance.scanning) {
        int error = ble_gap_stop_scan();
        if (error != SYSTEM_ERROR_NONE) {
            return error;
        }
    }

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

bool ble_gap_is_scanning(void) {
    return s_bleInstance.scanning;
}

int ble_gap_stop_scan(void) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_stop_scan().");

    if (!s_bleInstance.scanning) {
        return SYSTEM_ERROR_NONE;
    }

    ret_code_t ret = sd_ble_gap_scan_stop();
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_scan_stop() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    s_bleInstance.scanning = false;
    return SYSTEM_ERROR_NONE;
}

int ble_gap_connect(hal_ble_address_t* address, hal_ble_connection_parameters_t* params, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_connect().");

    int error = ble_gap_stop_scan();
    if (error != SYSTEM_ERROR_NONE) {
        return error;
    }

    if (os_semaphore_create(&s_bleInstance.connectSemaphore, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }

    ble_gap_addr_t devAddr;
    memset(&devAddr, 0x00, sizeof(ble_gap_addr_t));
    devAddr.addr_type = address->addr_type;
    memcpy(devAddr.addr, address->addr, BLE_SIG_ADDR_LEN);

    ble_gap_scan_params_t bleGapScanParams;
    fromHalScanParams(&s_bleInstance.scanParams, &bleGapScanParams);

    ble_gap_conn_params_t connParams;
    fromHalConnParams(params, &connParams);

    ret_code_t ret = sd_ble_gap_connect(&devAddr, &bleGapScanParams, &connParams, BLE_CONN_CFG_TAG);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_connect() failed: %u", (unsigned)ret);
        os_semaphore_destroy(s_bleInstance.connectSemaphore);
        return sysError(ret);
    }

    s_bleInstance.connecting = true;

    os_semaphore_take(s_bleInstance.connectSemaphore, BLE_GENERAL_PROCEDURE_TIMEOUT, false);
    os_semaphore_destroy(s_bleInstance.connectSemaphore);

    return 0;
}

bool ble_gap_is_connecting(void) {
    return s_bleInstance.connecting;
}

bool ble_gap_is_connected(void) {
    return s_bleInstance.connected;
}

int ble_gap_connect_cancel(void) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_connect_cancel().");

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

int ble_gap_disconnect(uint16_t conn_handle, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_disconnect().");

    if (!s_bleInstance.connected) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    else if (s_bleInstance.role == BLE_ROLE_PERIPHERAL) {
        conn_handle = s_bleInstance.connHandle;
    }
    else if (conn_handle == BLE_INVALID_CONN_HANDLE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if (os_semaphore_create(&s_bleInstance.disconnectSemaphore, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }

    ret_code_t ret = sd_ble_gap_disconnect(conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_disconnect() failed: %u", (unsigned)ret);
        os_semaphore_destroy(s_bleInstance.disconnectSemaphore);
        return sysError(ret);
    }

    os_semaphore_take(s_bleInstance.disconnectSemaphore, BLE_GENERAL_PROCEDURE_TIMEOUT, false);
    os_semaphore_destroy(s_bleInstance.disconnectSemaphore);

    return SYSTEM_ERROR_NONE;
}

int ble_gap_update_connection_params(uint16_t conn_handle, hal_ble_connection_parameters_t* conn_params, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_update_connection_params().");

    ret_code_t ret;

    if (conn_handle == BLE_INVALID_CONN_HANDLE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (!s_bleInstance.connected) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    if (os_semaphore_create(&s_bleInstance.connUpdateSemaphore, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
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
        fromHalConnParams(conn_params, &connParams);
        ret = sd_ble_gap_conn_param_update(conn_handle, &connParams);
    }

    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_conn_param_update() failed: %u", (unsigned)ret);
        os_semaphore_destroy(s_bleInstance.connUpdateSemaphore);
        return sysError(ret);
    }

    os_semaphore_take(s_bleInstance.connUpdateSemaphore, BLE_GENERAL_PROCEDURE_TIMEOUT, false);
    os_semaphore_destroy(s_bleInstance.connUpdateSemaphore);

    return SYSTEM_ERROR_NONE;
}

int ble_gap_get_rssi(uint16_t conn_handle, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_get_rssi().");

    return 0;
}


/**********************************************
 * BLE GATT Server APIs
 */
int ble_gatt_server_add_service_uuid128(uint8_t type, const uint8_t* uuid128, uint16_t* handle, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_server_add_service_uuid128().");

    ble_uuid_t     svcUuid;
    hal_ble_uuid_t halUuid;

    if (uuid128 == NULL || handle == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    halUuid.type = BLE_UUID_TYPE_128BIT;
    memcpy(halUuid.uuid128, uuid128, BLE_SIG_UUID_128BIT_LEN);

    int ret = fromHalUuid(&halUuid, &svcUuid);
    if (ret == SYSTEM_ERROR_NONE) {
        ret = addService(type, &svcUuid, handle);
    }

    return ret;
}

int ble_gatt_server_add_service_uuid16(uint8_t type, uint16_t uuid16, uint16_t* handle, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_server_add_service_uuid16().");

    if (handle == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ble_uuid_t svcUuid;
    svcUuid.type = BLE_UUID_TYPE_BLE;
    svcUuid.uuid = uuid16;

    return addService(type, &svcUuid, handle);
}

int ble_gatt_server_add_characteristic_uuid128(uint16_t service_handle, const uint8_t *uuid128, uint8_t properties,
                                               const char* description, hal_ble_characteristic_t* characteristic, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_server_add_characteristic_uuid128().");

    ble_uuid_t     charUuid;
    hal_ble_uuid_t halUuid;

    if (uuid128 == NULL || characteristic == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    halUuid.type = BLE_UUID_TYPE_128BIT;
    memcpy(halUuid.uuid128, uuid128, BLE_SIG_UUID_128BIT_LEN);

    int ret = fromHalUuid(&halUuid, &charUuid);
    if (ret == SYSTEM_ERROR_NONE) {
        ret = addCharacteristic(service_handle, &charUuid, properties, description, characteristic);
        if (SYSTEM_ERROR_NONE == ret) {
            characteristic->uuid.type = BLE_UUID_TYPE_128BIT;
            memcpy(characteristic->uuid.uuid128, uuid128, BLE_SIG_UUID_128BIT_LEN);
        }
    }

    return ret;
}

int ble_gatt_server_add_characteristic_uuid16(uint16_t service_handle, uint16_t uuid16, uint8_t properties,
                                              const char* description, hal_ble_characteristic_t* characteristic, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_server_add_characteristic_uuid16().");

    if (characteristic == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ble_uuid_t charUuid;
    charUuid.type = BLE_UUID_TYPE_BLE;
    charUuid.uuid = uuid16;

    int error = addCharacteristic(service_handle, &charUuid, properties, description, characteristic);
    if (SYSTEM_ERROR_NONE == error) {
        characteristic->uuid.type   = BLE_UUID_TYPE_16BIT;
        characteristic->uuid.uuid16 = uuid16;
    }

    return error;
}

int ble_gatt_server_add_characteristic_descriptor(uint8_t* descriptor, uint16_t len, hal_ble_characteristic_t* characteristic, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_server_add_characteristic_descriptor().");

    if (characteristic == NULL || descriptor == NULL || characteristic->value_handle == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ret_code_t ret;
    ble_gatts_attr_t descAttr;
    ble_gatts_attr_md_t descAttrMd;
    ble_uuid_t descUuid;

    if (characteristic->uuid.type == BLE_UUID_TYPE_16BIT) {
        descUuid.type = BLE_UUID_TYPE_BLE;
        descUuid.uuid = characteristic->uuid.uuid16;
    }
    else {
        ret = sd_ble_uuid_decode(BLE_SIG_UUID_128BIT_LEN, characteristic->uuid.uuid128, &descUuid);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "sd_ble_uuid_decode() failed: %u", (unsigned)ret);
            return sysError(ret);
        }
    }

    memset(&descAttrMd, 0, sizeof(descAttrMd));
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&descAttrMd.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&descAttrMd.write_perm);
    descAttrMd.vloc = BLE_GATTS_VLOC_STACK;
    descAttrMd.rd_auth = 0;
    descAttrMd.wr_auth = 0;
    descAttrMd.vlen    = 0;

    memset(&descAttr, 0, sizeof(descAttr));
    descAttr.p_uuid    = &descUuid;
    descAttr.p_attr_md = &descAttrMd;
    descAttr.init_len  = len;
    descAttr.init_offs = 0;
    descAttr.max_len   = len;
    descAttr.p_value   = descriptor;

    // FIXME: check the descriptor to be added
    ret = sd_ble_gatts_descriptor_add(characteristic->value_handle, &descAttr, &characteristic->user_desc_handle);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_descriptor_add() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    return SYSTEM_ERROR_NONE;
}

int ble_gatt_server_set_characteristic_value(uint16_t value_handle, uint8_t* data, uint16_t len, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_server_set_characteristic_value().");

    if (value_handle == BLE_INVALID_ATTR_HANDLE || data == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ble_gatts_value_t gattValue;
    gattValue.len     = len;
    gattValue.offset  = 0;
    gattValue.p_value = data;

    ret_code_t ret = sd_ble_gatts_value_set(BLE_CONN_HANDLE_INVALID, value_handle, &gattValue);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_value_set() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    return SYSTEM_ERROR_NONE;
}

int ble_gatt_server_get_characteristic_value(uint16_t value_handle, uint8_t* data, uint16_t* len, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_server_get_characteristic_value().");

    if (value_handle == BLE_INVALID_ATTR_HANDLE || data == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ble_gatts_value_t gattValue;
    gattValue.len     = *len;
    gattValue.offset  = 0;
    gattValue.p_value = data;

    ret_code_t ret = sd_ble_gatts_value_get(BLE_CONN_HANDLE_INVALID, value_handle, &gattValue);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_value_get() failed: %u", (unsigned)ret);
        *len = 0;
        return sysError(ret);
    }

    *len = gattValue.len;

    return SYSTEM_ERROR_NONE;
}

int ble_gatt_server_notify_characteristic_value(uint16_t value_handle, uint8_t* data, uint16_t len, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_server_notify_characteristic_value().");

    if (value_handle == BLE_INVALID_ATTR_HANDLE || data == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (s_bleInstance.connHandle == BLE_INVALID_CONN_HANDLE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    if (os_semaphore_create(&s_bleInstance.readWriteSemaphore, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }

    ble_gatts_hvx_params_t hvxParams;

    uint16_t hvxLen = len;
    hvxParams.type   = BLE_GATT_HVX_NOTIFICATION;
    hvxParams.handle = value_handle;
    hvxParams.offset = 0;
    hvxParams.p_data = data;
    hvxParams.p_len  = &hvxLen;

    ret_code_t ret = sd_ble_gatts_hvx(s_bleInstance.connHandle, &hvxParams);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_hvx() failed: %u", (unsigned)ret);
        os_semaphore_destroy(s_bleInstance.readWriteSemaphore);
        return sysError(ret);
    }

    os_semaphore_take(s_bleInstance.readWriteSemaphore, BLE_GENERAL_PROCEDURE_TIMEOUT, false);
    os_semaphore_destroy(s_bleInstance.readWriteSemaphore);

    return ble_gatt_server_set_characteristic_value(value_handle, data, len, NULL);
}

int ble_gatt_server_indicate_characteristic_value(uint16_t value_handle, uint8_t* data, uint16_t len, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_server_indicate_characteristic_value().");

    if (value_handle == BLE_INVALID_ATTR_HANDLE || data == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (s_bleInstance.connHandle == BLE_INVALID_CONN_HANDLE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    if (os_semaphore_create(&s_bleInstance.readWriteSemaphore, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }

    ble_gatts_hvx_params_t hvxParams;

    uint16_t hvxLen = len;
    hvxParams.type   = BLE_GATT_HVX_INDICATION;
    hvxParams.handle = value_handle;
    hvxParams.offset = 0;
    hvxParams.p_data = data;
    hvxParams.p_len  = &hvxLen;

    ret_code_t ret = sd_ble_gatts_hvx(s_bleInstance.connHandle, &hvxParams);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_hvx() failed: %u", (unsigned)ret);
        os_semaphore_destroy(s_bleInstance.readWriteSemaphore);
        return sysError(ret);
    }

    os_semaphore_take(s_bleInstance.readWriteSemaphore, BLE_GENERAL_PROCEDURE_TIMEOUT, false);
    os_semaphore_destroy(s_bleInstance.readWriteSemaphore);

    return ble_gatt_server_set_characteristic_value(value_handle, data, len, NULL);
}


/**********************************************
 * BLE GATT Client APIs
 */
int ble_gatt_client_discover_all_services(uint16_t conn_handle, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_client_discover_all_services().");

    if (conn_handle == BLE_INVALID_CONN_HANDLE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (s_bleInstance.discovery.currDiscProcedure != BLE_DISCOVERY_PROCEDURE_IDLE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    if (os_semaphore_create(&s_bleInstance.discoverySemaphore, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }

    ret_code_t ret = sd_ble_gattc_primary_services_discover(conn_handle, 0x01, NULL);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gattc_primary_services_discover() failed: %u", (unsigned)ret);
        os_semaphore_destroy(s_bleInstance.discoverySemaphore);
        return sysError(ret);
    }

    memset(&s_bleInstance.discovery, 0x00, sizeof(s_bleInstance.discovery));
    s_bleInstance.discovery.discAll = true;
    s_bleInstance.discovery.currDiscProcedure = BLE_DISCOVERY_PROCEDURE_SERVICES;
    s_bleInstance.discovering = true;

    os_semaphore_take(s_bleInstance.discoverySemaphore, BLE_DICOVERY_PROCEDURE_TIMEOUT, false);
    os_semaphore_destroy(s_bleInstance.discoverySemaphore);

    return SYSTEM_ERROR_NONE;
}

int ble_gatt_client_discover_service_by_uuid128(uint16_t conn_handle, const uint8_t *uuid128, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_client_discover_service_by_uuid128().");

    if (conn_handle == BLE_INVALID_CONN_HANDLE || uuid128 == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (s_bleInstance.discovery.currDiscProcedure != BLE_DISCOVERY_PROCEDURE_IDLE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    if (os_semaphore_create(&s_bleInstance.discoverySemaphore, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }

    ble_uuid_t     svcUuid;
    hal_ble_uuid_t halUuid;
    halUuid.type = BLE_UUID_TYPE_128BIT;
    memcpy(halUuid.uuid128, uuid128, BLE_SIG_UUID_128BIT_LEN);

    int ret = fromHalUuid(&halUuid, &svcUuid);
    if (ret == SYSTEM_ERROR_NONE) {
        ret = sd_ble_gattc_primary_services_discover(conn_handle, 0x01, &svcUuid);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "sd_ble_gattc_primary_services_discover() failed: %u", (unsigned)ret);
            os_semaphore_destroy(s_bleInstance.discoverySemaphore);
            return sysError(ret);
        }

        memset(&s_bleInstance.discovery, 0x00, sizeof(s_bleInstance.discovery));
        s_bleInstance.discovery.discAll = false;
        s_bleInstance.discovery.currDiscProcedure = BLE_DISCOVERY_PROCEDURE_SERVICES;
        s_bleInstance.discovering = true;

        os_semaphore_take(s_bleInstance.discoverySemaphore, BLE_DICOVERY_PROCEDURE_TIMEOUT, false);
    }

    os_semaphore_destroy(s_bleInstance.discoverySemaphore);

    return ret;
}

int ble_gatt_client_discover_service_by_uuid16(uint16_t conn_handle, uint16_t uuid, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_client_discover_service_by_uuid16().");

    if (conn_handle == BLE_INVALID_CONN_HANDLE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (s_bleInstance.discovery.currDiscProcedure != BLE_DISCOVERY_PROCEDURE_IDLE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    if (os_semaphore_create(&s_bleInstance.discoverySemaphore, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }

    ble_uuid_t svcUuid;
    svcUuid.type = BLE_UUID_TYPE_BLE;
    svcUuid.uuid = uuid;

    ret_code_t ret = sd_ble_gattc_primary_services_discover(conn_handle, 0x01, &svcUuid);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gattc_primary_services_discover() failed: %u", (unsigned)ret);
        os_semaphore_destroy(s_bleInstance.discoverySemaphore);
        return sysError(ret);
    }

    memset(&s_bleInstance.discovery, 0x00, sizeof(s_bleInstance.discovery));
    s_bleInstance.discovery.discAll = false;
    s_bleInstance.discovery.currDiscProcedure = BLE_DISCOVERY_PROCEDURE_SERVICES;
    s_bleInstance.discovering = true;

    os_semaphore_take(s_bleInstance.discoverySemaphore, BLE_DICOVERY_PROCEDURE_TIMEOUT, false);
    os_semaphore_destroy(s_bleInstance.discoverySemaphore);

    return SYSTEM_ERROR_NONE;
}

int ble_gatt_client_discover_characteristics(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_client_discover_characteristics().");

    if (conn_handle == BLE_INVALID_CONN_HANDLE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if (s_bleInstance.discovery.currDiscProcedure != BLE_DISCOVERY_PROCEDURE_IDLE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    if (os_semaphore_create(&s_bleInstance.discoverySemaphore, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }

    ble_gattc_handle_range_t handleRange;
    handleRange.start_handle = start_handle;
    handleRange.end_handle   = end_handle;

    ret_code_t ret = sd_ble_gattc_characteristics_discover(conn_handle, &handleRange);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gattc_characteristics_discover() failed: %u", (unsigned)ret);
        os_semaphore_destroy(s_bleInstance.discoverySemaphore);
        return sysError(ret);
    }

    memset(&s_bleInstance.discovery, 0x00, sizeof(s_bleInstance.discovery));
    s_bleInstance.discovery.currDiscProcedure = BLE_DISCOVERY_PROCEDURE_CHARACTERISTICS;
    s_bleInstance.discovery.discStartHandle = start_handle;
    s_bleInstance.discovery.discEndHandle = end_handle;
    s_bleInstance.discovering = true;

    os_semaphore_take(s_bleInstance.discoverySemaphore, BLE_DICOVERY_PROCEDURE_TIMEOUT, false);
    os_semaphore_destroy(s_bleInstance.discoverySemaphore);

    return SYSTEM_ERROR_NONE;
}

int ble_gatt_client_discover_descriptors(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_client_discover_descriptors().");

    if (conn_handle == BLE_INVALID_CONN_HANDLE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if (s_bleInstance.discovery.currDiscProcedure != BLE_DISCOVERY_PROCEDURE_IDLE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    if (os_semaphore_create(&s_bleInstance.discoverySemaphore, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }

    ble_gattc_handle_range_t handleRange;
    handleRange.start_handle = start_handle;
    handleRange.end_handle   = end_handle;

    ret_code_t ret = sd_ble_gattc_descriptors_discover(conn_handle, &handleRange);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gattc_descriptors_discover() failed: %u", (unsigned)ret);
        os_semaphore_destroy(s_bleInstance.discoverySemaphore);
        return sysError(ret);
    }

    memset(&s_bleInstance.discovery, 0x00, sizeof(s_bleInstance.discovery));
    s_bleInstance.discovery.currDiscProcedure = BLE_DISCOVERY_PROCEDURE_DESCRIPTORS;
    s_bleInstance.discovery.discStartHandle = start_handle;
    s_bleInstance.discovery.discEndHandle = end_handle;
    s_bleInstance.discovering = true;

    os_semaphore_take(s_bleInstance.discoverySemaphore, BLE_DICOVERY_PROCEDURE_TIMEOUT, false);
    os_semaphore_destroy(s_bleInstance.discoverySemaphore);

    return SYSTEM_ERROR_NONE;
}

bool ble_gatt_client_is_discovering(void) {
    return s_bleInstance.discovering;
}

int ble_gatt_client_configure_cccd(uint16_t conn_handle, uint16_t cccd_handle, uint8_t cccd_value, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_client_configure_cccd().");

    if (conn_handle == BLE_CONN_HANDLE_INVALID || cccd_handle == BLE_INVALID_ATTR_HANDLE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (cccd_value > BLE_SIG_CCCD_VAL_INDICATION) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }

    uint8_t buf[2] = {0x00, 0x00};
    buf[0] = cccd_value;

    return writeAttribute(conn_handle, cccd_handle, buf, 2, true);
}

int ble_gatt_client_write_with_response(uint16_t conn_handle, uint16_t value_handle, uint8_t* data, uint16_t len, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_client_write_with_response().");

    if (conn_handle == BLE_CONN_HANDLE_INVALID || value_handle == BLE_INVALID_ATTR_HANDLE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    return writeAttribute(conn_handle, value_handle, data, len, true);
}

int ble_gatt_client_write_without_response(uint16_t conn_handle, uint16_t value_handle, uint8_t* data, uint16_t len, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_client_write_without_response().");

    if (conn_handle == BLE_CONN_HANDLE_INVALID || value_handle == BLE_INVALID_ATTR_HANDLE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    return writeAttribute(conn_handle, value_handle, data, len, false);
}

int ble_gatt_client_read(uint16_t conn_handle, uint16_t value_handle, void *reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_client_read().");

    if (conn_handle == BLE_CONN_HANDLE_INVALID || value_handle == BLE_INVALID_ATTR_HANDLE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if (os_semaphore_create(&s_bleInstance.readWriteSemaphore, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }

    ret_code_t ret = sd_ble_gattc_read(conn_handle, value_handle, 0);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gattc_read() failed: %u", (unsigned)ret);
        os_semaphore_destroy(s_bleInstance.readWriteSemaphore);
        return sysError(ret);
    }

    os_semaphore_take(s_bleInstance.readWriteSemaphore, BLE_GENERAL_PROCEDURE_TIMEOUT, false);
    os_semaphore_destroy(s_bleInstance.readWriteSemaphore);

    return SYSTEM_ERROR_NONE;
}


