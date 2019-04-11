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

#include "ble_hal.h"

#if HAL_PLATFORM_BLE

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
#include "device_code.h"
#include "system_error.h"
#include "sdk_config_system.h"
#include "spark_wiring_vector.h"
#include <string.h>


using spark::Vector;


#define BLE_CONN_CFG_TAG                            1
#define BLE_OBSERVER_PRIO                           1

#define BLE_DISCOVERY_PROCEDURE_IDLE                0x00
#define BLE_DISCOVERY_PROCEDURE_SERVICES            0x01
#define BLE_DISCOVERY_PROCEDURE_CHARACTERISTICS     0x02
#define BLE_DISCOVERY_PROCEDURE_DESCRIPTORS         0x03

#define BLE_GENERAL_PROCEDURE_TIMEOUT               5000 // In milliseconds
#define BLE_DICOVERY_PROCEDURE_TIMEOUT              15000 // In milliseconds

#define BLE_MAX_EVT_CB_COUNT                        2

#define NRF_CHECK_RETURN(ret, str) \
        do { \
            const int _ret = ret; \
            const char* _str = str; \
            if (_ret != NRF_SUCCESS) { \
                if (_str != nullptr) { \
                    LOG(ERROR, "%s failed: %u", _str, _ret); \
                } \
                return sysError(_ret); \
            } \
        } while (false)

#define NRF_CHECK_RETURN_VOID(ret, str) \
        do { \
            const int _ret = ret; \
            const char* _str = str; \
            if (_ret != NRF_SUCCESS) { \
                if (_str != nullptr) { \
                    LOG(ERROR, "%s failed: %u", _str, _ret); \
                } \
            } \
        } while (false)

#define CHECK_RETURN(ret, str) \
        do { \
            const int _ret = ret; \
            const char* _str = str; \
            if (_ret != SYSTEM_ERROR_NONE) { \
                if (_str != nullptr) { \
                    LOG(ERROR, "%s failed: %u", _str, _ret); \
                } \
                return _ret; \
            } \
        } while (false)


#define CHECK_RETURN_VOID(ret, str) \
        do { \
            const int _ret = ret; \
            const char* _str = str; \
            if (_ret != SYSTEM_ERROR_NONE) { \
                if (_str != nullptr) { \
                    LOG(ERROR, "%s failed: %u", _str, _ret); \
                } \
                return; \
            } \
        } while (false)


StaticRecursiveMutex s_bleMutex;

class bleLock {
    static void lock() {
        s_bleMutex.lock();
    }

    static void unlock() {
        s_bleMutex.unlock();
    }
};

typedef struct {
    hal_ble_evts_t arg;
} HalBleEvtMsg_t;

typedef struct {
    on_ble_evt_cb_t handler;
    void* context;
} HalBleEvtCb_t;

typedef struct {
    uint16_t       discStartHandle;
    uint16_t       discEndHandle;
    hal_ble_svc_t  services[BLE_MAX_SVC_COUNT];
    hal_ble_char_t characteristics[BLE_MAX_CHAR_COUNT];
    hal_ble_desc_t descriptors[BLE_MAX_DESC_COUNT];
    uint8_t        discAll : 1;
    uint8_t        count;
    uint8_t        currIndex;
    uint8_t        currDiscProcedure;
    os_semaphore_t semaphore;
} HalBleDiscovery_t;

/* BLE Instance */
typedef struct {
    uint8_t  initialized  : 1;
    uint8_t  discovering  : 1;
    os_semaphore_t discoverySemaphore;
    os_semaphore_t readWriteSemaphore;
    os_queue_t     evtQueue;
    os_thread_t    evtThread;

    ble_gap_addr_t        whitelist[BLE_MAX_WHITELIST_ADDR_COUNT];
    ble_gap_addr_t const* whitelistPointer[BLE_MAX_WHITELIST_ADDR_COUNT];

    HalBleDiscovery_t     discovery;

    uint16_t readAttrHandle;
    uint8_t  readData[20];
    uint16_t readDataLen;

    HalBleEvtCb_t evtCbs[BLE_MAX_EVT_CB_COUNT];
} HalBleInstance_t;


static HalBleInstance_t s_bleInstance = {
    .initialized  = 0,
    .discovering  = 0,

    .discoverySemaphore = NULL,
    .readWriteSemaphore = NULL,
    .evtQueue  = NULL,
    .evtThread = NULL,
};

static uint8_t charValue[BLE_MAX_CHAR_COUNT][BLE_MAX_ATTR_VALUE_PACKET_SIZE];
static uint8_t charValueCount = 0;


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
                             const char* description, hal_ble_char_handles_t* handles) {
    ret_code_t               ret;
    ble_gatts_char_md_t      charMd;
    ble_gatts_attr_md_t      valueAttrMd;
    ble_gatts_attr_md_t      userDescAttrMd;
    ble_gatts_attr_md_t      cccdAttrMd;
    ble_gatts_attr_t         charValueAttr;
    ble_gatts_char_handles_t charHandles;

    if (charValueCount >= BLE_MAX_CHAR_COUNT) {
        return SYSTEM_ERROR_LIMIT_EXCEEDED;
    }

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
    valueAttrMd.vloc    = BLE_GATTS_VLOC_USER;//BLE_GATTS_VLOC_STACK;
    valueAttrMd.rd_auth = 0;
    valueAttrMd.wr_auth = 0;
    valueAttrMd.vlen    = 1;

    /* Characteristic value attribute */
    memset(&charValueAttr, 0, sizeof(charValueAttr));
    charValueAttr.p_uuid    = uuid;
    charValueAttr.p_attr_md = &valueAttrMd;
    charValueAttr.init_len  = 0;
    charValueAttr.init_offs = 0;
    charValueAttr.max_len   = BLE_MAX_ATTR_VALUE_PACKET_SIZE;
    charValueAttr.p_value   = charValue[charValueCount++];

    ret = sd_ble_gatts_characteristic_add(svcHandle, &charMd, &charValueAttr, &charHandles);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_characteristic_add() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    handles->value_handle     = charHandles.value_handle;
    handles->user_desc_handle = charHandles.user_desc_handle;
    handles->cccd_handle      = charHandles.cccd_handle;
    handles->sccd_handle      = charHandles.sccd_handle;

    LOG_DEBUG(TRACE, "Characteristic value handle: %d.", charHandles.value_handle);
    LOG_DEBUG(TRACE, "Characteristic cccd handle: %d.", charHandles.cccd_handle);

    return SYSTEM_ERROR_NONE;
}

static int writeAttribute(uint16_t connHandle, uint16_t attrHandle, const uint8_t* data, uint16_t len, bool response) {
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
            if (event->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_AUTH_PAYLOAD) {
                LOG_DEBUG(ERROR, "BLE GAP event: Authenticated payload timeout");
            }
        } break;

        case BLE_GATTC_EVT_EXCHANGE_MTU_RSP: {
            LOG_DEBUG(TRACE, "BLE GATT Client event: exchange ATT MTU response.");

            uint16_t serverMtu = event->evt.gattc_evt.params.exchange_mtu_rsp.server_rx_mtu;
            LOG_DEBUG(TRACE, "Effective Server RX MTU: %d.", serverMtu);

            uint16_t effectMtu = serverMtu > NRF_SDH_BLE_GATT_MAX_MTU_SIZE ? NRF_SDH_BLE_GATT_MAX_MTU_SIZE : serverMtu;
            msg.arg.type = BLE_EVT_GATT_PARAMS_UPDATED;
            msg.arg.params.gatt_params_updated.version = 0x01;
            msg.arg.params.gatt_params_updated.conn_handle = event->evt.gattc_evt.conn_handle;
            msg.arg.params.gatt_params_updated.att_mtu_size = effectMtu;
            if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
                LOG(ERROR, "os_queue_put() failed.");
            }
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
                    if (s_bleInstance.readAttrHandle == event->evt.gattc_evt.params.read_rsp.handle) {
                        s_bleInstance.readDataLen = event->evt.gattc_evt.params.read_rsp.len;
                        memcpy(s_bleInstance.readData, event->evt.gattc_evt.params.read_rsp.data, s_bleInstance.readDataLen);
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

            uint16_t clientMtu = event->evt.gatts_evt.params.exchange_mtu_request.client_rx_mtu;
            LOG_DEBUG(TRACE, "Effective Client RX MTU: %d.", clientMtu);

            uint16_t effectMtu = clientMtu > NRF_SDH_BLE_GATT_MAX_MTU_SIZE ? NRF_SDH_BLE_GATT_MAX_MTU_SIZE : clientMtu;
            ret = sd_ble_gatts_exchange_mtu_reply(event->evt.gatts_evt.conn_handle, effectMtu);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gatts_exchange_mtu_reply() failed: %u", (unsigned)ret);
            }
            LOG_DEBUG(TRACE, "Reply with Server RX MTU: %d.", effectMtu);

            msg.arg.type = BLE_EVT_GATT_PARAMS_UPDATED;
            msg.arg.params.gatt_params_updated.version = 0x01;
            msg.arg.params.gatt_params_updated.conn_handle = event->evt.gatts_evt.conn_handle;
            msg.arg.params.gatt_params_updated.att_mtu_size = effectMtu;
            if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
                LOG(ERROR, "os_queue_put() failed.");
            }
        } break;

        case BLE_GATTS_EVT_WRITE: {
            LOG_DEBUG(TRACE, "BLE GATT Server event: write characteristic, handle: %d, len: %d.",
                    event->evt.gatts_evt.params.write.handle, event->evt.gatts_evt.params.write.len);

            // TODO: deal with different write commands.
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
            //LOG_DEBUG(TRACE, "Unhandled BLE event: %u", (unsigned)event->header.evt_id);
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
                    hal_ble_gattc_on_svc_disc_evt_t *svcDiscEvt = (hal_ble_gattc_on_svc_disc_evt_t*)&msg.arg.params;
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
                    hal_ble_gattc_on_char_disc_evt_t* charDiscEvt = (hal_ble_gattc_on_char_disc_evt_t*)&msg.arg.params;
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

            for (uint8_t i = 0; i < BLE_MAX_EVT_CB_COUNT; i++) {
                if (s_bleInstance.evtCbs[i].handler != NULL) {
                    s_bleInstance.evtCbs[i].handler(&msg.arg, s_bleInstance.evtCbs[i].context);
                }
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


/* FIXME: It causes a section type conflict if using NRF_SDH_BLE_OBSERVER() in c++ class.*/
class Broadcaster;
typedef struct {
    Broadcaster* instance;
} BroadcasterImpl;
static BroadcasterImpl broadcasterImpl;

class Broadcaster {
public:
    // Singleton
    static Broadcaster& getInstance(void) {
        static Broadcaster instance;
        return instance;
    }

    bool advertising(void) const {
        return isAdvertising_;
    }

    int setAdvertisingParams(const hal_ble_adv_params_t* params) {
        if (params == nullptr) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        int ret = suspend();
        CHECK_RETURN(ret, nullptr);
        ret = configure(params);
        NRF_CHECK_RETURN(ret, nullptr);
        memcpy(&advParams_, params, sizeof(hal_ble_adv_params_t));
        return resume();
    }

    int getAdvertisingParams(hal_ble_adv_params_t* params) const {
        if (params == nullptr) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        memcpy(params, &advParams_, sizeof(hal_ble_adv_params_t));
        return SYSTEM_ERROR_NONE;
    }

    int setAdvertisingData(const uint8_t* buf, size_t len) {
        // It is invalid to provide the same data buffers while advertising.
        int ret = suspend();
        CHECK_RETURN(ret, nullptr);
        if (buf != nullptr) {
            len = len > BLE_MAX_ADV_DATA_LEN ? BLE_MAX_ADV_DATA_LEN : len;
            memcpy(advData_, buf, len);
        }
        else {
            // It should at least contain the Flags in advertising data.
            len = 3;
        }
        advDataLen_ = len;
        ret = configure(nullptr);
        CHECK_RETURN(ret, nullptr);
        return resume();
    }

    size_t getAdvertisingData(uint8_t* buf, size_t len) const {
        if (buf == nullptr) {
            return 0;
        }
        len = len > advDataLen_ ? advDataLen_ : len;
        memcpy(buf, advData_, len);
        return len;
    }

    int setScanResponseData(const uint8_t* buf, size_t len) {
        // It is invalid to provide the same data buffers while advertising.
        int ret = suspend();
        CHECK_RETURN(ret, nullptr);
        if (buf != nullptr) {
            len = len > BLE_MAX_ADV_DATA_LEN ? BLE_MAX_ADV_DATA_LEN : len;
            memcpy(scanRespData_, buf, len);
        }
        else {
            len = 0;
        }
        scanRespDataLen_ = len;
        ret = configure(nullptr);
        CHECK_RETURN(ret, nullptr);
        return resume();
    }

    size_t getScanResponseData(uint8_t* buf, size_t len) const {
        if (buf == nullptr) {
            return 0;
        }
        len = len > scanRespDataLen_ ? scanRespDataLen_ : len;
        memcpy(buf, scanRespData_, len);
        return len;
    }

    int setTxPower(int8_t val) {
        int ret = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, advHandle_, roundTxPower(val));
        NRF_CHECK_RETURN(ret, "sd_ble_gap_tx_power_set");
        return SYSTEM_ERROR_NONE;
    }

    int8_t getTxPower(void) const {
        return txPower_;
    }

    int startAdvertising(void) {
        int ret;
        if (isAdvertising_) {
            // Stop advertising first.
            ret = sd_ble_gap_adv_stop(advHandle_);
            NRF_CHECK_RETURN(ret, "sd_ble_gap_adv_stop");
            isAdvertising_ = false;
        }
        ret = sd_ble_gap_adv_start(advHandle_, BLE_CONN_CFG_TAG);
        NRF_CHECK_RETURN(ret, "sd_ble_gap_adv_start");
        isAdvertising_ = true;
        return SYSTEM_ERROR_NONE;
    }

    int stopAdvertising(void) {
        if (isAdvertising_) {
            int ret = sd_ble_gap_adv_stop(advHandle_);
            NRF_CHECK_RETURN(ret, "sd_ble_gap_adv_stop");
            isAdvertising_ = false;
        }
        return SYSTEM_ERROR_NONE;
    }

private:
    uint8_t isAdvertising_ : 1;                     /**< If it is advertising or not. */
    uint8_t advHandle_;                             /**< Advertising handle. */
    hal_ble_adv_params_t advParams_;                /**< Current advertising parameters. */
    uint8_t advData_[BLE_MAX_ADV_DATA_LEN];         /**< Current advertising data. */
    size_t  advDataLen_;                            /**< Current advertising data length. */
    uint8_t scanRespData_[BLE_MAX_ADV_DATA_LEN];    /**< Current scan response data. */
    size_t  scanRespDataLen_;                       /**< Current scan response data length. */
    int8_t  txPower_;                               /**< TX Power. */
    bool advPending_;                               /**< Advertising is pending. */
    uint16_t connHandle_;                           /**< Connection handle. It is assigned once device is connected
                                                         as Peripheral. It is used for re-start advertising. */
    static const int8_t validTxPower_[8];

    Broadcaster() {
        isAdvertising_ = false;
        advPending_ = false;
        advHandle_ = BLE_GAP_ADV_SET_HANDLE_NOT_SET;
        /* Default advertising parameters. */
        advParams_.version = 0x01;
        advParams_.type = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT;
        advParams_.filter_policy = BLE_ADV_FP_ANY;
        advParams_.interval = BLE_DEFAULT_ADVERTISING_INTERVAL;
        advParams_.timeout = BLE_DEFAULT_ADVERTISING_TIMEOUT;
        advParams_.inc_tx_power = false;
        memset(advData_, 0x00, sizeof(advData_));
        memset(scanRespData_, 0x00, sizeof(scanRespData_));
        advDataLen_ = scanRespDataLen_ = 0;
        /* Default advertising data. */
        // FLags
        advData_[advDataLen_++] = 0x02;
        advData_[advDataLen_++] = BLE_SIG_AD_TYPE_FLAGS;
        advData_[advDataLen_++] = BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
        int ret = configure(&advParams_);
        CHECK_RETURN_VOID(ret, nullptr);
        /* Default TX power. */
        txPower_ = 0;
        setTxPower(txPower_);
        broadcasterImpl.instance = this;
        NRF_SDH_BLE_OBSERVER(bleBroadcaster, 1, processBroadcasterEvents, &broadcasterImpl);
    }

    ~Broadcaster() = default;

    int suspend(void) {
        if (isAdvertising_) {
            int ret = ble_gap_stop_advertising();
            CHECK_RETURN(ret, "ble_gap_stop_advertising");
            advPending_ = true;
        }
        return SYSTEM_ERROR_NONE;
    }

    int resume(void) {
        if (advPending_) {
            int ret = startAdvertising();
            CHECK_RETURN(ret, nullptr);
            advPending_ = false;
        }
        return SYSTEM_ERROR_NONE;
    }

    void toPlatformAdvParams(ble_gap_adv_params_t* params, const hal_ble_adv_params_t* halParams) const {
        memset(params, 0x00, sizeof(ble_gap_adv_params_t));
        params->properties.type = halParams->type;
        params->properties.include_tx_power = false; // FIXME: for extended advertising packet
        params->p_peer_addr = NULL;
        params->interval = halParams->interval;
        params->duration = halParams->timeout;
        params->filter_policy = halParams->filter_policy;
        params->primary_phy = BLE_GAP_PHY_1MBPS;
    }

    void toPlatformAdvData(ble_gap_adv_data_t* data) {
        data->adv_data.p_data = advData_;
        data->adv_data.len = advDataLen_;
        data->scan_rsp_data.p_data = scanRespData_;
        data->scan_rsp_data.len = scanRespDataLen_;
    }

    int configure(const hal_ble_adv_params_t* params) {
        int ret;
        ble_gap_adv_data_t bleGapAdvData;
        toPlatformAdvData(&bleGapAdvData);
        if (params == nullptr) {
            ret = sd_ble_gap_adv_set_configure(&advHandle_, &bleGapAdvData, NULL);
        }
        else {
            ble_gap_adv_params_t bleGapAdvParams;
            toPlatformAdvParams(&bleGapAdvParams, params);
            LOG_DEBUG(TRACE, "BLE advertising interval: %.3fms, timeout: %dms.",
                      bleGapAdvParams.interval*0.625, bleGapAdvParams.duration*10);
            ret = sd_ble_gap_adv_set_configure(&advHandle_, &bleGapAdvData, &bleGapAdvParams);
        }
        NRF_CHECK_RETURN(ret, "sd_ble_gap_adv_set_configure");
        return SYSTEM_ERROR_NONE;
    }

    int8_t roundTxPower(int8_t value) {
        uint8_t i = 0;
        for (i = 0; i < sizeof(validTxPower_); i++) {
            if (value == validTxPower_[i]) {
                return value;
            }
            else {
                if (value < validTxPower_[i]) {
                    return validTxPower_[i];
                }
            }
        }
        return validTxPower_[i - 1];
    }

    static void processBroadcasterEvents(const ble_evt_t* event, void* context) {
        Broadcaster* broadcaster = static_cast<BroadcasterImpl*>(context)->instance;
        switch (event->header.evt_id) {
            case BLE_GAP_EVT_ADV_SET_TERMINATED: {
                LOG_DEBUG(TRACE, "BLE GAP event: advertising stopped.");
                broadcaster->isAdvertising_ = false;

                HalBleEvtMsg_t msg;
                memset(&msg, 0x00, sizeof(HalBleEvtMsg_t));
                msg.arg.type = BLE_EVT_ADV_STOPPED;
                msg.arg.params.adv_stopped.version = 0x01;
                msg.arg.params.adv_stopped.reserved = NULL;
                if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
                    LOG(ERROR, "os_queue_put() failed.");
                }
            } break;

            case BLE_GAP_EVT_CONNECTED: {
                // FIXME: If multi role is enabled, this flag should not be clear here.
                broadcaster->isAdvertising_ = false;
                if (event->evt.gap_evt.params.connected.role == BLE_ROLE_PERIPHERAL) {
                    broadcaster->connHandle_ = event->evt.gap_evt.conn_handle;
                }
            } break;

            case BLE_GAP_EVT_DISCONNECTED: {
                // Re-start advertising.
                // FIXME: when multi-role implemented.
                if (broadcaster->connHandle_ == event->evt.gap_evt.conn_handle) {
                    LOG_DEBUG(TRACE, "Restart BLE advertising.");
                    int ret = broadcaster->startAdvertising();
                    CHECK_RETURN_VOID(ret, nullptr);
                    broadcaster->isAdvertising_ = true;
                    broadcaster->connHandle_ = BLE_INVALID_CONN_HANDLE;
                }
            } break;

            default: break;
        }
    }
};

const int8_t Broadcaster::validTxPower_[8] = { -20, -16, -12, -8, -4, 0, 4, 8 };


class Observer;
typedef struct {
    Observer* instance;
} ObserverImpl;
static ObserverImpl observerImpl;

class Observer {
public:
    // Singleton
    static Observer& getInstance(void) {
        static Observer instance;
        return instance;
    }

    bool scanning(void) {
        return isScanning_;
    }

    int setScanParams(const hal_ble_scan_params_t* params) {
        if (params == nullptr) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        if ((params->interval < BLE_GAP_SCAN_INTERVAL_MIN) ||
                (params->interval > BLE_GAP_SCAN_INTERVAL_MAX) ||
                (params->window < BLE_GAP_SCAN_WINDOW_MIN) ||
                (params->window > BLE_GAP_SCAN_WINDOW_MAX) ||
                (params->window > params->interval) ) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        memcpy(&scanParams_, params, sizeof(hal_ble_scan_params_t));
        return SYSTEM_ERROR_NONE;
    }

    int getScanParams(hal_ble_scan_params_t* params) const {
        if (params == nullptr) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        memcpy(params, &scanParams_, sizeof(hal_ble_scan_params_t));
        return SYSTEM_ERROR_NONE;
    }

    int startScanning(bool restart) {
        if(restart) {
            int ret = sd_ble_gap_scan_start(NULL, &bleScanData_);
            NRF_CHECK_RETURN(ret, "sd_ble_gap_scan_start");
            return SYSTEM_ERROR_NONE;
        }
        if (isScanning_) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        if (os_semaphore_create(&scanSemaphore_, 1, 0)) {
            LOG(ERROR, "os_semaphore_create() failed");
            return SYSTEM_ERROR_INTERNAL;
        }
        ble_gap_scan_params_t bleGapScanParams;
        toPlatformScanParams(&bleGapScanParams);
        LOG_DEBUG(TRACE, "| interval(ms)   window(ms)   timeout(ms) |");
        LOG_DEBUG(TRACE, "  %.3f        %.3f      %d",
                bleGapScanParams.interval*0.625, bleGapScanParams.window*0.625, bleGapScanParams.timeout*10);

        cache_.clear();
        int ret = sd_ble_gap_scan_start(&bleGapScanParams, &bleScanData_);
        NRF_CHECK_RETURN(ret, "sd_ble_gap_scan_start");
        isScanning_ = true;
        os_semaphore_take(scanSemaphore_, CONCURRENT_WAIT_FOREVER, false);
        os_semaphore_destroy(scanSemaphore_);
        return SYSTEM_ERROR_NONE;
    }

    int stopScanning(void) {
        if (!isScanning_) {
            return SYSTEM_ERROR_NONE;
        }
        int ret = sd_ble_gap_scan_stop();
        NRF_CHECK_RETURN(ret, "sd_ble_gap_scan_stop");
        os_semaphore_give(scanSemaphore_, false);
        isScanning_ = false;
        return SYSTEM_ERROR_NONE;
    }

    void toPlatformScanParams(ble_gap_scan_params_t* params) const {
        memset(params, 0x00, sizeof(ble_gap_scan_params_t));
        params->extended = 0;
        params->active = scanParams_.active;
        params->interval = scanParams_.interval;
        params->window = scanParams_.window;
        params->timeout = scanParams_.timeout;
        params->scan_phys = BLE_GAP_PHY_1MBPS;
        params->filter_policy = scanParams_.filter_policy;
    }

private:
    uint8_t isScanning_: 1;                                 /**< If it is scanning or not. */
    hal_ble_scan_params_t scanParams_;                      /**< BLE scanning parameters. */
    os_semaphore_t scanSemaphore_;                          /**< Semaphore to wait until the scan procedure completed. */
    uint8_t scanReportBuff_[BLE_MAX_SCAN_REPORT_BUF_LEN];   /**< Buffer to hold the scanned report data. */
    ble_data_t bleScanData_;                                /**< BLE scanned data. */
    Vector<hal_ble_addr_t> cache_;                          /**< Cached address of scanned devices to filter-out duplicated result. */

    Observer() {
        isScanning_ = false;
        scanParams_.version = 0x01;
        scanParams_.active = true;
        scanParams_.filter_policy = BLE_SCAN_FP_ACCEPT_ALL;
        scanParams_.interval = BLE_DEFAULT_SCANNING_INTERVAL;
        scanParams_.window = BLE_DEFAULT_SCANNING_WINDOW;
        scanParams_.timeout = BLE_DEFAULT_SCANNING_TIMEOUT;
        scanSemaphore_ = nullptr;
        bleScanData_.p_data = scanReportBuff_;
        bleScanData_.len = sizeof(scanReportBuff_);
        observerImpl.instance = this;
        NRF_SDH_BLE_OBSERVER(bleObserver, 1, processObserverEvents, &observerImpl);
    }

    ~Observer() = default;

    bool chacheDevice(const hal_ble_addr_t& newAddr) {
        for (int i = 0; i < cache_.size(); i++) {
            const hal_ble_addr_t& existAddr = cache_[i];
            if (existAddr.addr_type == newAddr.addr_type &&
                    !memcmp(existAddr.addr, newAddr.addr, BLE_SIG_ADDR_LEN)) {
                LOG_DEBUG(TRACE, "Duplicated scan result. Filter it out.");
                return false;
            }
        }
        cache_.append(newAddr);
        return true;
    }

    static void processObserverEvents(const ble_evt_t* event, void* context) {
        Observer* observer = static_cast<ObserverImpl*>(context)->instance;
        HalBleEvtMsg_t msg;
        memset(&msg, 0x00, sizeof(HalBleEvtMsg_t));
        switch (event->header.evt_id) {
            case BLE_GAP_EVT_ADV_REPORT: {
                LOG_DEBUG(TRACE, "BLE GAP event: advertising report.");

                hal_ble_addr_t newAddr;
                newAddr.addr_type = event->evt.gap_evt.params.adv_report.peer_addr.addr_type;
                memcpy(newAddr.addr, event->evt.gap_evt.params.adv_report.peer_addr.addr, BLE_SIG_ADDR_LEN);
                if (observer->chacheDevice(newAddr)) {
                    // The new scanned device is not in the cached device list.
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
                    msg.arg.params.scan_result.peer_addr.addr_type = newAddr.addr_type;
                    memcpy(msg.arg.params.scan_result.peer_addr.addr, newAddr.addr, BLE_SIG_ADDR_LEN);
                    if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
                        LOG(ERROR, "os_queue_put() failed.");
                    }
                }
                // Continue scanning, scanning parameters pointer must be set to NULL.
                observer->startScanning(true);
            } break;

            case BLE_GAP_EVT_TIMEOUT: {
                if (event->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_SCAN) {
                    LOG_DEBUG(TRACE, "BLE GAP event: Scanning timeout");
                    observer->isScanning_ = false;

                    msg.arg.type = BLE_EVT_SCAN_STOPPED;
                    msg.arg.params.scan_stopped.version = 0x01;
                    msg.arg.params.scan_stopped.reserved = NULL;
                    if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
                        LOG(ERROR, "os_queue_put() failed.");
                    }
                    os_semaphore_give(observer->scanSemaphore_, false);
                }
            } break;

            default: break;
        }
    }
};


class ConnectionsManager;
typedef struct {
    ConnectionsManager* instance;
} ConnectionsManagerImpl;
static ConnectionsManagerImpl connMgrImpl;

class ConnectionsManager {
public:
    // Singleton
    static ConnectionsManager& getInstance(void) {
        static ConnectionsManager instance;
        return instance;
    }

    int setPPCP(const hal_ble_conn_params_t* ppcp) {
        if (ppcp == nullptr) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        if ((ppcp->min_conn_interval != BLE_SIG_CP_MIN_CONN_INTERVAL_NONE) &&
                (ppcp->min_conn_interval < BLE_SIG_CP_MIN_CONN_INTERVAL_MIN ||
                 ppcp->min_conn_interval > BLE_SIG_CP_MIN_CONN_INTERVAL_MAX)) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        if ((ppcp->max_conn_interval != BLE_SIG_CP_MAX_CONN_INTERVAL_NONE) &&
                (ppcp->max_conn_interval < BLE_SIG_CP_MAX_CONN_INTERVAL_MIN ||
                 ppcp->max_conn_interval > BLE_SIG_CP_MAX_CONN_INTERVAL_MAX)) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        if (ppcp->slave_latency > BLE_SIG_CP_SLAVE_LATENCY_MAX) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        if ((ppcp->conn_sup_timeout != BLE_SIG_CP_CONN_SUP_TIMEOUT_NONE) &&
                (ppcp->conn_sup_timeout < BLE_SIG_CP_CONN_SUP_TIMEOUT_MIN ||
                 ppcp->conn_sup_timeout > BLE_SIG_CP_CONN_SUP_TIMEOUT_MAX)) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        ble_gap_conn_params_t bleGapConnParams;
        toPlatformConnParams(&bleGapConnParams, ppcp);
        int ret = sd_ble_gap_ppcp_set(&bleGapConnParams);
        NRF_CHECK_RETURN(ret, "sd_ble_gap_ppcp_set");
        return SYSTEM_ERROR_NONE;
    }

    int getPPCP(hal_ble_conn_params_t* ppcp) const {
        if (ppcp == nullptr) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        ble_gap_conn_params_t bleGapConnParams;
        int ret = sd_ble_gap_ppcp_get(&bleGapConnParams);
        NRF_CHECK_RETURN(ret, "sd_ble_gap_ppcp_get");
        toHalConnParams(ppcp, &bleGapConnParams);
        return SYSTEM_ERROR_NONE;
    }

    bool connecting(void) const {
        return isConnecting_;
    }

    bool connected(const hal_ble_addr_t* address=nullptr) {
        if (address == nullptr) {
            return connections_.size() > 0;
        }
        else if (fetchConnection(address)) {
            return true;
        }
        return false;
    }

    int connect(const hal_ble_addr_t* address) {
        // Stop scanning first to give the scanning semaphore if possible.
        int ret = Observer::getInstance().stopScanning();
        CHECK_RETURN(ret, nullptr);
        if (os_semaphore_create(&connectSemaphore_, 1, 0)) {
            LOG(ERROR, "os_semaphore_create() failed");
            return SYSTEM_ERROR_INTERNAL;
        }
        ble_gap_addr_t bleDevAddr;
        ble_gap_scan_params_t bleGapScanParams;
        ble_gap_conn_params_t bleGapConnParams;
        memset(&bleDevAddr, 0x00, sizeof(ble_gap_addr_t));
        bleDevAddr.addr_type = address->addr_type;
        memcpy(bleDevAddr.addr, address->addr, BLE_SIG_ADDR_LEN);
        Observer::getInstance().toPlatformScanParams(&bleGapScanParams);
        ret = sd_ble_gap_ppcp_get(&bleGapConnParams);
        NRF_CHECK_RETURN(ret, "sd_ble_gap_ppcp_get");
        ret = sd_ble_gap_connect(&bleDevAddr, &bleGapScanParams, &bleGapConnParams, BLE_CONN_CFG_TAG);
        if (ret != NRF_SUCCESS) {
            os_semaphore_destroy(connectSemaphore_);
            NRF_CHECK_RETURN(ret, "sd_ble_gap_connect");
        }
        isConnecting_ = true;
        memcpy(&connectingAddr_, address, sizeof(hal_ble_addr_t));
        ret = SYSTEM_ERROR_NONE;
        if (os_semaphore_take(connectSemaphore_, CONNECTION_OPERATION_TIMEOUT_MS, false)) {
            ret = SYSTEM_ERROR_TIMEOUT;
        }
        os_semaphore_destroy(connectSemaphore_);
        isConnecting_ = false;
        memset(&connectingAddr_, 0x00, sizeof(hal_ble_addr_t));
        return ret;
    }

    int connectCancel(void) {
        if (isConnecting_) {
            int ret = sd_ble_gap_connect_cancel();
            NRF_CHECK_RETURN(ret, "sd_ble_gap_connect_cancel");
            isConnecting_ = false;
        }
        return SYSTEM_ERROR_NONE;
    }

    int disconnect(uint16_t connHandle) {
        if (fetchConnection(connHandle) == nullptr) {
            return SYSTEM_ERROR_NOT_FOUND;
        }
        if (os_semaphore_create(&disconnectSemaphore_, 1, 0)) {
            LOG(ERROR, "os_semaphore_create() failed");
            return SYSTEM_ERROR_INTERNAL;
        }
        int ret = sd_ble_gap_disconnect(connHandle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        if (ret != NRF_SUCCESS) {
            os_semaphore_destroy(disconnectSemaphore_);
            NRF_CHECK_RETURN(ret, "sd_ble_gap_disconnect");
        }
        disconnectingHandle_ = connHandle;
        ret = SYSTEM_ERROR_NONE;
        if (os_semaphore_take(disconnectSemaphore_, CONNECTION_OPERATION_TIMEOUT_MS, false)) {
            ret = SYSTEM_ERROR_TIMEOUT;
        }
        os_semaphore_destroy(disconnectSemaphore_);
        disconnectingHandle_ = BLE_INVALID_CONN_HANDLE;
        return ret;
    }

    int updateConnectionParams(uint16_t connHandle, const hal_ble_conn_params_t* params) {
        int ret;
        if (fetchConnection(connHandle) == nullptr) {
            return SYSTEM_ERROR_NOT_FOUND;
        }
        if (os_semaphore_create(&connParamsUpdateSemaphore_, 1, 0)) {
            LOG(ERROR, "os_semaphore_create() failed");
            return SYSTEM_ERROR_INTERNAL;
        }
        ble_gap_conn_params_t bleGapConnParams;
        if (params == NULL) {
            // Use the PPCP characteristic value as the connection parameters.
            ret = sd_ble_gap_ppcp_get(&bleGapConnParams);
            NRF_CHECK_RETURN(ret, "sd_ble_gap_ppcp_get");
        }
        // For Central role, this will initiate the connection parameter update procedure.
        // For Peripheral role, this will use the passed in parameters and send the request to central.
        ret = sd_ble_gap_conn_param_update(connHandle, &bleGapConnParams);
        if (ret != NRF_SUCCESS) {
            os_semaphore_destroy(connParamsUpdateSemaphore_);
            NRF_CHECK_RETURN(ret, "sd_ble_gap_conn_param_update");
        }
        connParamUpdateHandle_ = connHandle;
        ret = SYSTEM_ERROR_NONE;
        if (os_semaphore_take(connParamsUpdateSemaphore_, CONNECTION_OPERATION_TIMEOUT_MS, false)) {
            ret = SYSTEM_ERROR_TIMEOUT;
        }
        os_semaphore_destroy(connParamsUpdateSemaphore_);
        connParamUpdateHandle_ = BLE_INVALID_CONN_HANDLE;
        return ret;
    }

    int getConnectionParams(uint16_t connHandle, hal_ble_conn_params_t* params) {
        BleConnection_t* connection = fetchConnection(connHandle);
        if (connection == nullptr) {
            return SYSTEM_ERROR_NOT_FOUND;
        }
        memcpy(params, &connection->effectiveConnParams, sizeof(hal_ble_conn_params_t));
        return SYSTEM_ERROR_NONE;
    }

private:
    typedef struct {
        uint8_t role;
        uint16_t connHandle;
        hal_ble_conn_params_t effectiveConnParams;
        hal_ble_addr_t peer;
    } BleConnection_t;

    uint8_t isConnecting_ : 1;                      /**< If it is connecting or not. */
    hal_ble_addr_t connectingAddr_;                 /**< Address of peer the Central is connecting to. */
    uint16_t disconnectingHandle_;                  /**< Handle of connection to be disconnected. */
    uint16_t connParamUpdateHandle_;                /**< Handle of the connection that is to send peripheral connection update request. */
    uint8_t connParamsUpdateAttempts_;              /**< Attempts for peripheral to update connection parameters. */
    os_timer_t connParamsUpdateTimer_;              /**< Timer used for sending peripheral connection update request after connection established. */
    os_semaphore_t connParamsUpdateSemaphore_;      /**< Semaphore to wait until connection parameters updated. */
    os_semaphore_t connectSemaphore_;               /**< Semaphore to wait until connection established. */
    os_semaphore_t disconnectSemaphore_;            /**< Semaphore to wait until connection disconnected. */
    Vector<BleConnection_t> connections_;           /**< Current on-going connections. */

    static const uint32_t CONNECTION_OPERATION_TIMEOUT_MS = 5000;

    ConnectionsManager() {
        isConnecting_ = 0;
        memset(&connectingAddr_, 0x00, sizeof(hal_ble_addr_t));
        connParamsUpdateAttempts_ = 0;
        disconnectingHandle_ = BLE_INVALID_CONN_HANDLE;
        connParamUpdateHandle_ = BLE_INVALID_CONN_HANDLE;
        /*
         * Default Peripheral Preferred Connection Parameters.
         * For BLE Central, this is the initial connection parameters.
         * For BLE Peripheral, this is the Peripheral Preferred Connection Parameters.
         */
        ble_gap_conn_params_t bleGapConnParams;
        bleGapConnParams.min_conn_interval = BLE_DEFAULT_MIN_CONN_INTERVAL;
        bleGapConnParams.max_conn_interval = BLE_DEFAULT_MAX_CONN_INTERVAL;
        bleGapConnParams.slave_latency = BLE_DEFAULT_SLAVE_LATENCY;
        bleGapConnParams.conn_sup_timeout = BLE_DEFAULT_CONN_SUP_TIMEOUT;
        int ret = sd_ble_gap_ppcp_set(&bleGapConnParams);
        NRF_CHECK_RETURN_VOID(ret, "sd_ble_gap_ppcp_set");
        connectSemaphore_ = nullptr;
        disconnectSemaphore_ = nullptr;
        connParamsUpdateSemaphore_ = nullptr;
        connParamsUpdateTimer_ = nullptr;
        if (os_timer_create(&connParamsUpdateTimer_, BLE_CONN_PARAMS_UPDATE_DELAY_MS, onConnParamsUpdateTimerExpired, this, true, nullptr)) {
            LOG(ERROR, "os_timer_create() failed.");
        }
        connMgrImpl.instance = this;
        NRF_SDH_BLE_OBSERVER(bleConnectionManager, 1, processConnectionEvents, &connMgrImpl);
    }

    ~ConnectionsManager() = default;

    void toPlatformConnParams(ble_gap_conn_params_t* params, const hal_ble_conn_params_t* halConnParams) const {
        memset(params, 0x00, sizeof(ble_gap_conn_params_t));
        params->min_conn_interval = halConnParams->min_conn_interval;
        params->max_conn_interval = halConnParams->max_conn_interval;
        params->slave_latency = halConnParams->slave_latency;
        params->conn_sup_timeout = halConnParams->conn_sup_timeout;
    }

    void toHalConnParams(hal_ble_conn_params_t* halConnParams, const ble_gap_conn_params_t* params) const {
        memset(halConnParams, 0x00, sizeof(hal_ble_conn_params_t));
        halConnParams->min_conn_interval = params->min_conn_interval;
        halConnParams->max_conn_interval = params->max_conn_interval;
        halConnParams->slave_latency = params->slave_latency;
        halConnParams->conn_sup_timeout = params->conn_sup_timeout;
    }

    BleConnection_t* fetchConnection(uint16_t connHandle) {
        if (connHandle == BLE_INVALID_CONN_HANDLE) {
            return nullptr;
        }
        for (int i = 0; i < connections_.size(); i++) {
            BleConnection_t& connection = connections_.at(i);
            if (connection.connHandle == connHandle) {
                return &connection;
            }
        }
        return nullptr;
    }

    BleConnection_t* fetchConnection(const hal_ble_addr_t* address) {
        if (address == nullptr) {
            return nullptr;
        }
        for (int i = 0; i < connections_.size(); i++) {
            BleConnection_t& connection = connections_.at(i);
            hal_ble_addr_t& peer = connection.peer;
            if (peer.addr_type == address->addr_type &&
                    !memcmp(peer.addr, address->addr, BLE_SIG_ADDR_LEN)) {
                return &connection;
            }
        }
        return nullptr;
    }

    void addConnection(const BleConnection_t& connection) {
        removeConnection(connection.connHandle);
        connections_.append(connection);
    }

    void removeConnection(uint16_t connHandle) {
        for (int i = 0; i < connections_.size(); i++) {
            const BleConnection_t& connection = connections_[i];
            if (connection.connHandle == connHandle) {
                connections_.removeAt(i);
            }
        }
    }

    void initiateConnParamsUpdateIfNeeded(const BleConnection_t* connection) {
        if (connParamsUpdateTimer_ == nullptr) {
            return;
        }
        if (!isConnParamsFeeded(&connection->effectiveConnParams)) {
            if (connParamsUpdateAttempts_ < BLE_CONN_PARAMS_UPDATE_ATTEMPS) {
                if (!os_timer_change(connParamsUpdateTimer_, OS_TIMER_CHANGE_START, true, 0, 0, NULL)) {
                    LOG_DEBUG(TRACE, "Attempts to update BLE connection parameters, try: %d after %d ms",
                            connParamsUpdateAttempts_, BLE_CONN_PARAMS_UPDATE_DELAY_MS);
                    connParamUpdateHandle_ = connection->connHandle;
                    return;
                }
            }
            else {
                int ret = sd_ble_gap_disconnect(connection->connHandle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
                NRF_CHECK_RETURN_VOID(ret, "sd_ble_gap_disconnect");
                LOG_DEBUG(TRACE, "Disconnecting. Update BLE connection parameters failed.");
            }
        }
    }

    bool isConnParamsFeeded(const hal_ble_conn_params_t* params) const {
        hal_ble_conn_params_t ppcp;
        if (getPPCP(&ppcp) == SYSTEM_ERROR_NONE) {
            uint16_t minAcceptedSl = ppcp.slave_latency - MIN(BLE_CONN_PARAMS_SLAVE_LATENCY_ERR, ppcp.slave_latency);
            uint16_t maxAcceptedSl = ppcp.slave_latency + BLE_CONN_PARAMS_SLAVE_LATENCY_ERR;
            uint16_t minAcceptedTo = ppcp.conn_sup_timeout - MIN(BLE_CONN_PARAMS_TIMEOUT_ERR, ppcp.conn_sup_timeout);
            uint16_t maxAcceptedTo = ppcp.conn_sup_timeout + BLE_CONN_PARAMS_TIMEOUT_ERR;
            if (params->max_conn_interval < ppcp.min_conn_interval ||
                    params->max_conn_interval > ppcp.max_conn_interval) {
                return false;
            }
            if (params->slave_latency < minAcceptedSl ||
                    params->slave_latency > maxAcceptedSl) {
                return false;
            }
            if (params->conn_sup_timeout < minAcceptedTo ||
                    params->conn_sup_timeout > maxAcceptedTo) {
                return false;
            }
        }
        return true;
    }

    static void onConnParamsUpdateTimerExpired(os_timer_t timer) {
        ConnectionsManager* connMgr;
        os_timer_get_id(timer, (void**)&connMgr);
        if (connMgr->connParamUpdateHandle_ == BLE_INVALID_CONN_HANDLE) {
            return;
        }
        // For Peripheral, it will use the PPCP characteristic value.
        int ret = sd_ble_gap_conn_param_update(connMgr->connParamUpdateHandle_, nullptr);
        if (ret != NRF_SUCCESS) {
            sd_ble_gap_disconnect(connMgr->connParamUpdateHandle_, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
            LOG_DEBUG(TRACE, "Disconnecting. Update BLE connection parameters failed.");
            NRF_CHECK_RETURN_VOID(ret, "sd_ble_gap_conn_param_update");
        }
        connMgr->connParamsUpdateAttempts_++;
    }

    static void processConnectionEvents(const ble_evt_t* event, void* context) {
        ConnectionsManager* connMgr = static_cast<ConnectionsManagerImpl*>(context)->instance;
        HalBleEvtMsg_t msg;
        memset(&msg, 0x00, sizeof(HalBleEvtMsg_t));
        switch (event->header.evt_id) {
            case BLE_GAP_EVT_CONNECTED: {
                LOG_DEBUG(TRACE, "BLE GAP event: connected.");

                BleConnection_t newConnection;
                newConnection.role = event->evt.gap_evt.params.connected.role;
                newConnection.connHandle = event->evt.gap_evt.conn_handle;
                connMgr->toHalConnParams(&newConnection.effectiveConnParams, &event->evt.gap_evt.params.connected.conn_params);
                newConnection.peer.addr_type = event->evt.gap_evt.params.connected.peer_addr.addr_type;
                memcpy(newConnection.peer.addr, event->evt.gap_evt.params.connected.peer_addr.addr, BLE_SIG_ADDR_LEN);
                connMgr->addConnection(newConnection);

                LOG_DEBUG(TRACE, "BLE role: %d, connection handle: %d", newConnection.role, newConnection.connHandle);
                LOG_DEBUG(TRACE, "| interval(ms)  latency  timeout(ms) |");
                LOG_DEBUG(TRACE, "  %.2f          %d       %d",
                        newConnection.effectiveConnParams.max_conn_interval*1.25,
                        newConnection.effectiveConnParams.slave_latency,
                        newConnection.effectiveConnParams.conn_sup_timeout*10);

                // If the connection is initiated by Central.
                if (connMgr->isConnecting_ && newConnection.peer.addr_type == connMgr->connectingAddr_.addr_type &&
                        !memcmp(newConnection.peer.addr, connMgr->connectingAddr_.addr, BLE_SIG_ADDR_LEN)) {
                    os_semaphore_give(connMgr->connectSemaphore_, false);
                }

                // Update connection parameters if needed.
                if (newConnection.role == BLE_ROLE_PERIPHERAL) {
                    connMgr->connParamsUpdateAttempts_ = 0;
                    connMgr->initiateConnParamsUpdateIfNeeded(&newConnection);
                }

                msg.arg.type = BLE_EVT_CONNECTED;
                msg.arg.params.connected.version = 0x01;
                msg.arg.params.connected.role = newConnection.role;
                msg.arg.params.connected.conn_handle = newConnection.connHandle;
                msg.arg.params.connected.conn_interval = newConnection.effectiveConnParams.max_conn_interval;
                msg.arg.params.connected.slave_latency = newConnection.effectiveConnParams.slave_latency;
                msg.arg.params.connected.conn_sup_timeout = newConnection.effectiveConnParams.conn_sup_timeout;
                msg.arg.params.connected.peer_addr.addr_type = newConnection.peer.addr_type;
                memcpy(msg.arg.params.connected.peer_addr.addr, newConnection.peer.addr, BLE_SIG_ADDR_LEN);
                if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
                    LOG(ERROR, "os_queue_put() failed.");
                }
            } break;

            case BLE_GAP_EVT_DISCONNECTED: {
                LOG_DEBUG(TRACE, "BLE GAP event: disconnected, handle: 0x%04X, reason: %d",
                          event->evt.gap_evt.conn_handle, event->evt.gap_evt.params.disconnected.reason);

                BleConnection_t* connection = connMgr->fetchConnection(event->evt.gap_evt.conn_handle);
                if (connection != nullptr) {
                    // Cancel the on-going connection parameters update procedure.
                    if (connMgr->connParamUpdateHandle_ == event->evt.gap_evt.conn_handle) {
                        if (connection->role == BLE_ROLE_PERIPHERAL) {
                            connMgr->connParamUpdateHandle_ = BLE_INVALID_CONN_HANDLE;
                            if (!os_timer_is_active(connMgr->connParamsUpdateTimer_, NULL)) {
                                os_timer_change(connMgr->connParamsUpdateTimer_, OS_TIMER_CHANGE_STOP, true, 0, 0, NULL);
                            }
                        }
                        else {
                            os_semaphore_give(connMgr->connParamsUpdateSemaphore_, false);
                        }
                    }
                    if (connMgr->disconnectingHandle_ == event->evt.gap_evt.conn_handle) {
                        os_semaphore_give(connMgr->disconnectSemaphore_, false);
                    }
                    connMgr->removeConnection(event->evt.gap_evt.conn_handle);
                }
                else {
                    LOG(ERROR, "Connection not found.");
                    return;
                }

                msg.arg.type = BLE_EVT_DISCONNECTED;
                msg.arg.params.disconnected.version = 0x01;
                msg.arg.params.disconnected.reason = event->evt.gap_evt.params.disconnected.reason;
                msg.arg.params.disconnected.conn_handle = event->evt.gap_evt.conn_handle;
                if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
                    LOG(ERROR, "os_queue_put() failed.");
                }
            } break;

            case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST: {
                LOG_DEBUG(TRACE, "BLE GAP event: connection parameter update request.");

                int ret = sd_ble_gap_conn_param_update(event->evt.gap_evt.conn_handle,
                        &event->evt.gap_evt.params.conn_param_update_request.conn_params);
                if (ret != NRF_SUCCESS) {
                    LOG(ERROR, "sd_ble_gap_conn_param_update() failed: %u", (unsigned)ret);
                }
            } break;

            case BLE_GAP_EVT_CONN_PARAM_UPDATE: {
                LOG_DEBUG(TRACE, "BLE GAP event: connection parameter updated.");

                BleConnection_t* connection = connMgr->fetchConnection(event->evt.gap_evt.conn_handle);
                if (connection != nullptr) {
                    connMgr->toHalConnParams(&connection->effectiveConnParams, &event->evt.gap_evt.params.conn_param_update.conn_params);
                    if (connection->role == BLE_ROLE_PERIPHERAL) {
                        connMgr->initiateConnParamsUpdateIfNeeded(connection);
                    }
                    else {
                        os_semaphore_give(connMgr->connParamsUpdateSemaphore_, false);
                    }
                }
                else {
                    LOG(ERROR, "Connection not found.");
                    return;
                }

                LOG_DEBUG(TRACE, "| interval(ms)  latency  timeout(ms) |");
                LOG_DEBUG(TRACE, "  %.2f          %d       %d",
                        connection->effectiveConnParams.max_conn_interval*1.25,
                        connection->effectiveConnParams.slave_latency,
                        connection->effectiveConnParams.conn_sup_timeout*10);

                msg.arg.type = BLE_EVT_CONN_PARAMS_UPDATED;
                msg.arg.params.conn_params_updated.version = 0x01;
                msg.arg.params.conn_params_updated.conn_handle = event->evt.gap_evt.conn_handle;
                msg.arg.params.conn_params_updated.conn_interval = event->evt.gap_evt.params.conn_param_update.conn_params.max_conn_interval;
                msg.arg.params.conn_params_updated.slave_latency = event->evt.gap_evt.params.conn_param_update.conn_params.slave_latency;
                msg.arg.params.conn_params_updated.conn_sup_timeout = event->evt.gap_evt.params.conn_param_update.conn_params.conn_sup_timeout;
                if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
                    LOG(ERROR, "os_queue_put() failed.");
                }
            } break;

            case BLE_GAP_EVT_TIMEOUT: {
                if (event->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN) {
                    LOG_DEBUG(ERROR, "BLE GAP event: Connection timeout");
                    if (connMgr->isConnecting_) {
                        os_semaphore_give(connMgr->connectSemaphore_, false);
                    }
                }
            } break;

            default: break;
        }
    }
};


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

        for (uint8_t i = 0; i < BLE_MAX_EVT_CB_COUNT; i++) {
            s_bleInstance.evtCbs[i].handler = NULL;
            s_bleInstance.evtCbs[i].context = NULL;
        }

        // Register a handler for BLE events.
        NRF_SDH_BLE_OBSERVER(bleObserver, BLE_OBSERVER_PRIO, isrProcessBleEvent, nullptr);

        // Set the default device name
        char devName[32] = {};
        get_device_name(devName, sizeof(devName));
        ble_gap_set_device_name((const uint8_t*)devName, strlen(devName));

        // Initialize BLE Broadcaster.
        Broadcaster::getInstance();
        Observer::getInstance();
        ConnectionsManager::getInstance();

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

        return SYSTEM_ERROR_NONE;
    }
    else {
        return SYSTEM_ERROR_INVALID_STATE;
    }
}

int ble_select_antenna(hal_ble_ant_type_t antenna) {
#if (PLATFORM_ID == PLATFORM_XENON_SOM) || (PLATFORM_ID == PLATFORM_ARGON_SOM) || (PLATFORM_ID == PLATFORM_BORON_SOM)
    // Mesh SoM don't have on-board antenna switch.
    return SYSTEM_ERROR_NOT_SUPPORTED;
#else
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
#endif // (PLATFORM_ID == PLATFORM_XENON_SOM) || (PLATFORM_ID == PLATFORM_ARGON_SOM) || (PLATFORM_ID == PLATFORM_ARGON_SOM)
}

int ble_set_callback_on_events(on_ble_evt_cb_t callback, void* context) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);

    for (uint8_t i = 0; i < BLE_MAX_EVT_CB_COUNT; i++) {
        if (s_bleInstance.evtCbs[i].handler == NULL) {
            s_bleInstance.evtCbs[i].handler = callback;
            s_bleInstance.evtCbs[i].context = context;
            break;
        }
    }

    return SYSTEM_ERROR_NONE;
}


/**********************************************
 * BLE GAP APIs
 */
int ble_gap_set_device_address(const hal_ble_addr_t* address) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_set_device_address().");

    if (address == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (address->addr_type != BLE_SIG_ADDR_TYPE_PUBLIC && address->addr_type != BLE_SIG_ADDR_TYPE_RANDOM_STATIC) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (Broadcaster::getInstance().advertising() ||
            Observer::getInstance().scanning() ||
            ConnectionsManager::getInstance().connecting()) {
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

int ble_gap_get_device_address(hal_ble_addr_t* address) {
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

int ble_gap_set_device_name(const uint8_t* device_name, uint16_t len) {
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

int ble_gap_set_ppcp(const hal_ble_conn_params_t* ppcp, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_set_ppcp().");

    return ConnectionsManager::getInstance().setPPCP(ppcp);
}

int ble_gap_get_ppcp(hal_ble_conn_params_t* ppcp, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_get_ppcp().");

    return ConnectionsManager::getInstance().getPPCP(ppcp);
}

int ble_gap_add_whitelist(const hal_ble_addr_t* addr_list, uint8_t len, void* reserved) {
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

int ble_gap_delete_whitelist(void* reserved) {
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

    return Broadcaster::getInstance().setTxPower(value);
}

int8_t ble_gap_get_tx_power(void) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_get_tx_power().");

    return Broadcaster::getInstance().getTxPower();
}

int ble_gap_set_advertising_parameters(const hal_ble_adv_params_t* adv_params, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_set_advertising_parameters().");

    return Broadcaster::getInstance().setAdvertisingParams(adv_params);
}

int ble_gap_get_advertising_parameters(hal_ble_adv_params_t* adv_params, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_get_advertising_parameters().");

    return Broadcaster::getInstance().getAdvertisingParams(adv_params);
}

int ble_gap_set_advertising_data(const uint8_t* buf, uint16_t len, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_set_advertising_data().");

    return Broadcaster::getInstance().setAdvertisingData(buf, len);
}

size_t ble_gap_get_advertising_data(uint8_t* buf, uint16_t len, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_get_advertising_data().");

    return Broadcaster::getInstance().getAdvertisingData(buf, len);
}

int ble_gap_set_scan_response_data(const uint8_t* buf, uint16_t len, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_set_scan_response_data().");

    return Broadcaster::getInstance().setScanResponseData(buf, len);
}

size_t ble_gap_get_scan_response_data(uint8_t* buf, uint16_t len, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_get_scan_response_data().");

    return Broadcaster::getInstance().getScanResponseData(buf, len);
}

int ble_gap_start_advertising(void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_start_advertising().");

    return Broadcaster::getInstance().startAdvertising();
}

int ble_gap_stop_advertising(void) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_stop_advertising().");

    return Broadcaster::getInstance().stopAdvertising();
}

bool ble_gap_is_advertising(void) {
    return Broadcaster::getInstance().advertising();
}

int ble_gap_set_scan_parameters(const hal_ble_scan_params_t* scan_params, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_set_scan_parameters().");

    return Observer::getInstance().setScanParams(scan_params);
}

int ble_gap_get_scan_parameters(hal_ble_scan_params_t* scan_params, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_set_scan_parameters().");

    return Observer::getInstance().getScanParams(scan_params);
}

int ble_gap_start_scan(void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_start_scan().");

    return Observer::getInstance().startScanning(false);
}

bool ble_gap_is_scanning(void) {
    return Observer::getInstance().scanning();
}

int ble_gap_stop_scan(void) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_stop_scan().");

    return Observer::getInstance().stopScanning();
}

int ble_gap_connect(const hal_ble_addr_t* address, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_connect().");

    return ConnectionsManager::getInstance().connect(address);
}

bool ble_gap_is_connecting(void) {
    return ConnectionsManager::getInstance().connecting();
}

bool ble_gap_is_connected(const hal_ble_addr_t* address) {
    return ConnectionsManager::getInstance().connected(address);
}

int ble_gap_connect_cancel(void) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_connect_cancel().");

    return ConnectionsManager::getInstance().connectCancel();
}

int ble_gap_disconnect(uint16_t conn_handle, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_disconnect().");

    return ConnectionsManager::getInstance().disconnect(conn_handle);
}

int ble_gap_update_connection_params(uint16_t conn_handle, const hal_ble_conn_params_t* conn_params, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_update_connection_params().");

    return ConnectionsManager::getInstance().updateConnectionParams(conn_handle, conn_params);
}

int ble_gap_get_connection_params(uint16_t conn_handle, hal_ble_conn_params_t* conn_params, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_update_connection_params().");

    return ConnectionsManager::getInstance().getConnectionParams(conn_handle, conn_params);
}

int ble_gap_get_rssi(uint16_t conn_handle, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gap_get_rssi().");

    return 0;
}


/**********************************************
 * BLE GATT Server APIs
 */
int ble_gatt_server_add_service(uint8_t type, const hal_ble_uuid_t* uuid, uint16_t* handle, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_server_add_service().");

    ble_uuid_t svcUuid;

    if (uuid == NULL || handle == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    int ret = fromHalUuid(uuid, &svcUuid);
    if (ret == SYSTEM_ERROR_NONE) {
        ret = addService(type, &svcUuid, handle);
    }

    return ret;
}

int ble_gatt_server_add_characteristic(const hal_ble_char_init_t* char_init, hal_ble_char_handles_t* handles, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_server_add_characteristic().");

    ble_uuid_t charUuid;

    if (handles == NULL || char_init == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    int ret = fromHalUuid(&char_init->uuid, &charUuid);
    if (ret == SYSTEM_ERROR_NONE) {
        ret = addCharacteristic(char_init->service_handle, &charUuid, char_init->properties, char_init->description, handles);
    }

    return ret;
}

int ble_gatt_server_add_descriptor(const hal_ble_desc_init_t* desc_init, uint16_t* handle, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_server_add_descriptor().");

    if (desc_init == NULL || handle == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    ret_code_t ret;
    ble_gatts_attr_t descAttr;
    ble_gatts_attr_md_t descAttrMd;
    ble_uuid_t descUuid;

    fromHalUuid(&desc_init->uuid, &descUuid);

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
    descAttr.init_len  = desc_init->len;
    descAttr.init_offs = 0;
    descAttr.max_len   = desc_init->len;
    descAttr.p_value   = desc_init->descriptor;

    // FIXME: check the descriptor to be added
    ret = sd_ble_gatts_descriptor_add(desc_init->char_handle, &descAttr, handle);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_descriptor_add() failed: %u", (unsigned)ret);
        return sysError(ret);
    }

    return SYSTEM_ERROR_NONE;
}

int ble_gatt_server_set_characteristic_value(uint16_t value_handle, const uint8_t* data, uint16_t len, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_server_set_characteristic_value().");

    if (value_handle == BLE_INVALID_ATTR_HANDLE || data == NULL) {
        return 0;
    }

    ble_gatts_value_t gattValue;
    gattValue.len     = len;
    gattValue.offset  = 0;
    gattValue.p_value = (uint8_t*)data;

    ret_code_t ret = sd_ble_gatts_value_set(BLE_CONN_HANDLE_INVALID, value_handle, &gattValue);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_value_set() failed: %u", (unsigned)ret);
        return 0;
    }

    return len;
}

int ble_gatt_server_get_characteristic_value(uint16_t value_handle, uint8_t* data, uint16_t* len, void* reserved) {
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

int ble_gatt_server_notify_characteristic_value(uint16_t conn_handle, uint16_t value_handle, const uint8_t* data, uint16_t len, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_server_notify_characteristic_value().");

    if (conn_handle == BLE_INVALID_CONN_HANDLE || value_handle == BLE_INVALID_ATTR_HANDLE || data == NULL) {
        return 0;
    }

    if (os_semaphore_create(&s_bleInstance.readWriteSemaphore, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return 0;
    }

    ble_gatts_hvx_params_t hvxParams;

    uint16_t hvxLen = len;
    hvxParams.type   = BLE_GATT_HVX_NOTIFICATION;
    hvxParams.handle = value_handle;
    hvxParams.offset = 0;
    hvxParams.p_data = data;
    hvxParams.p_len  = &hvxLen;

    ret_code_t ret = sd_ble_gatts_hvx(conn_handle, &hvxParams);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_hvx() failed: %u", (unsigned)ret);
        os_semaphore_destroy(s_bleInstance.readWriteSemaphore);
        return 0;
    }

    os_semaphore_take(s_bleInstance.readWriteSemaphore, BLE_GENERAL_PROCEDURE_TIMEOUT, false);
    os_semaphore_destroy(s_bleInstance.readWriteSemaphore);

    return ble_gatt_server_set_characteristic_value(value_handle, data, len, NULL);
}

int ble_gatt_server_indicate_characteristic_value(uint16_t conn_handle, uint16_t value_handle, const uint8_t* data, uint16_t len, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_server_indicate_characteristic_value().");

    if (conn_handle == BLE_INVALID_CONN_HANDLE || value_handle == BLE_INVALID_ATTR_HANDLE || data == NULL) {
        return 0;
    }

    if (os_semaphore_create(&s_bleInstance.readWriteSemaphore, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return 0;
    }

    ble_gatts_hvx_params_t hvxParams;

    uint16_t hvxLen = len;
    hvxParams.type   = BLE_GATT_HVX_INDICATION;
    hvxParams.handle = value_handle;
    hvxParams.offset = 0;
    hvxParams.p_data = data;
    hvxParams.p_len  = &hvxLen;

    ret_code_t ret = sd_ble_gatts_hvx(conn_handle, &hvxParams);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_hvx() failed: %u", (unsigned)ret);
        os_semaphore_destroy(s_bleInstance.readWriteSemaphore);
        return 0;
    }

    os_semaphore_take(s_bleInstance.readWriteSemaphore, BLE_GENERAL_PROCEDURE_TIMEOUT, false);
    os_semaphore_destroy(s_bleInstance.readWriteSemaphore);

    return ble_gatt_server_set_characteristic_value(value_handle, data, len, NULL);
}


/**********************************************
 * BLE GATT Client APIs
 */
int ble_gatt_client_discover_all_services(uint16_t conn_handle, void* reserved) {
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

int ble_gatt_client_discover_service_by_uuid(uint16_t conn_handle, const hal_ble_uuid_t* uuid, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_client_discover_service_by_uuid().");

    if (conn_handle == BLE_INVALID_CONN_HANDLE || uuid == NULL) {
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

    int ret = fromHalUuid(uuid, &svcUuid);
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

int ble_gatt_client_discover_characteristics(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle, void* reserved) {
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

int ble_gatt_client_discover_characteristics_by_uuid(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle, const hal_ble_uuid_t* uuid, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int ble_gatt_client_discover_descriptors(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle, void* reserved) {
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

int ble_gatt_client_configure_cccd(uint16_t conn_handle, uint16_t cccd_handle, uint8_t cccd_value, void* reserved) {
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

int ble_gatt_client_write_with_response(uint16_t conn_handle, uint16_t value_handle, const uint8_t* data, uint16_t len, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_client_write_with_response().");

    if (conn_handle == BLE_CONN_HANDLE_INVALID || value_handle == BLE_INVALID_ATTR_HANDLE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    return writeAttribute(conn_handle, value_handle, data, len, true);
}

int ble_gatt_client_write_without_response(uint16_t conn_handle, uint16_t value_handle, const uint8_t* data, uint16_t len, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_client_write_without_response().");

    if (conn_handle == BLE_CONN_HANDLE_INVALID || value_handle == BLE_INVALID_ATTR_HANDLE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    return writeAttribute(conn_handle, value_handle, data, len, false);
}

int ble_gatt_client_read(uint16_t conn_handle, uint16_t attr_handle, uint8_t* data, uint16_t* len, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    SPARK_ASSERT(s_bleInstance.initialized);
    LOG_DEBUG(TRACE, "ble_gatt_client_read().");

    if (conn_handle == BLE_CONN_HANDLE_INVALID || attr_handle == BLE_INVALID_ATTR_HANDLE || data == NULL || len == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if (os_semaphore_create(&s_bleInstance.readWriteSemaphore, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }

    s_bleInstance.readAttrHandle = attr_handle;
    s_bleInstance.readDataLen = 0;

    ret_code_t ret = sd_ble_gattc_read(conn_handle, attr_handle, 0);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gattc_read() failed: %u", (unsigned)ret);
        os_semaphore_destroy(s_bleInstance.readWriteSemaphore);
        return sysError(ret);
    }

    os_semaphore_take(s_bleInstance.readWriteSemaphore, BLE_GENERAL_PROCEDURE_TIMEOUT, false);
    os_semaphore_destroy(s_bleInstance.readWriteSemaphore);

    if (s_bleInstance.readDataLen > 0) {
        *len = MIN(s_bleInstance.readDataLen, *len);
        memcpy(data, s_bleInstance.readData, *len);
    }
    else {
        *len = 0;
    }

    return SYSTEM_ERROR_NONE;
}

#endif // HAL_PLATFORM_BLE
