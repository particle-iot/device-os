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

#include "ble_control_request_channel.h"

#if HAS_BLE_CONTROL_REQUEST_CHANNEL

extern "C" {

#include "platform-softdevice.h"

}

#include "logging.h"

#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "nrf_ble_qwr.h"
#include "ble_advertising.h"

#include <cstdlib>

#if !defined(NRF_SDH_BLE_GATT_MAX_MTU_SIZE) || (NRF_SDH_BLE_GATT_MAX_MTU_SIZE == 0)
#error "NRF_SDH_BLE_GATT_MAX_MTU_SIZE is not defined"
#endif

namespace particle {

namespace system {

namespace {

struct __attribute__((packed)) ReqHeader {
    uint16_t id;
    uint16_t type;
    uint32_t size;
};

struct __attribute__((packed)) RepHeader {
    uint16_t id;
    int16_t result;
    uint32_t size;
};

const char* const DEVICE_NAME = "Xenon"; // FIXME

// Maximum number of requests that can be active at the same time
const unsigned MAX_ACTIVE_REQUEST_COUNT = 4;

// A tag identifying the SoftDevice BLE configuration
const unsigned APP_BLE_CONN_CFG_TAG = 1;

// Application's BLE observer priority
const unsigned APP_BLE_OBSERVER_PRIO = 3;

// Slave latency
const unsigned SLAVE_LATENCY = 0;

// Minimum acceptable connection interval in 1.25 ms units
const unsigned MIN_CONN_INTERVAL = MSEC_TO_UNITS(20, UNIT_1_25_MS); // 20 ms

// Maximum acceptable connection interval in 1.25 ms units
const unsigned MAX_CONN_INTERVAL = MSEC_TO_UNITS(75, UNIT_1_25_MS); // 75 ms

// Connection supervisory timeout in 10 ms units
const unsigned CONN_SUP_TIMEOUT = MSEC_TO_UNITS(4000, UNIT_10_MS); // 4 sec

// Advertising interval in 0.625 ms units
const unsigned APP_ADV_INTERVAL = MSEC_TO_UNITS(40, UNIT_0_625_MS); // 40 ms

// Advertising duration in 10 ms units
const unsigned APP_ADV_DURATION = MSEC_TO_UNITS(60000, UNIT_10_MS); // 180 sec

// Advertising interval in 0.625 ms units
const unsigned APP_ADV_INTERVAL_SLOW = MSEC_TO_UNITS(1000, UNIT_0_625_MS);

// Advertising duration in 10 ms units
const unsigned APP_ADV_DURATION_SLOW = MSEC_TO_UNITS(180000, UNIT_10_MS);

// Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update() is called
// const unsigned FIRST_CONN_PARAMS_UPDATE_DELAY = APP_TIMER_TICKS(5000); // 5 sec

// Time between each call to sd_ble_gap_conn_param_update() after the first call
// const unsigned NEXT_CONN_PARAMS_UPDATE_DELAY = APP_TIMER_TICKS(30000); // 30 sec

// Number of attempts before giving up the connection parameter negotiation
const unsigned MAX_CONN_PARAMS_UPDATE_COUNT = 3;

// Size of the attribute opcode in bytes
const unsigned OPCODE_LENGTH = 1;

// Size of the attribute handle in bytes
const unsigned HANDLE_LENGTH = 2;

// Vendor specific base UUID (bytes at indices 12 and 13 are ignored)
const uint8_t BASE_UUID[16] = { 0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0, 0x93, 0xf3, 0xa3, 0xb5, 0x00, 0x00, 0x40, 0x6e };

// Setup service UUID
const unsigned SETUP_SERVICE_UUID = 0x0001;

// RX characteristic UUID
const unsigned RX_CHAR_UUID = 0x0002;

// TX characteristic UUID
const unsigned TX_CHAR_UUID = 0x0003;

// Maximum size of data that can be transmitted to the peer by the service
const unsigned CHAR_MAX_DATA_SIZE = NRF_SDH_BLE_GATT_MAX_MTU_SIZE - OPCODE_LENGTH - HANDLE_LENGTH;

// Size of the RX buffer
const unsigned FIFO_BUF_SIZE = 512;

// Size of the TX buffer
const unsigned SEND_BUF_SIZE = 512;

// GATT module instance
NRF_BLE_GATT_DEF(g_gatt);

// Context for the Queued Write Module (QWR)
NRF_BLE_QWR_DEF(g_qwr);

// Advertising module instance
BLE_ADVERTISING_DEF(g_advert);

// Custom UUID type
uint8_t g_uuidType = 0;

// Service handle
uint16_t g_serviceHandle = 0;

// RX characteristic handles
ble_gatts_char_handles_t g_rxCharHandles = {};

// TX characteristic handles
ble_gatts_char_handles_t g_txCharHandles = {};

BleControlRequestChannel* g_instance = nullptr;

// Connection handle
volatile uint16_t g_connHandle = BLE_CONN_HANDLE_INVALID;

// Maximum size of data that can be transmitted to the peer
volatile uint16_t g_maxDataSize = BLE_GATT_ATT_MTU_DEFAULT - OPCODE_LENGTH - HANDLE_LENGTH;

volatile bool g_advertActive = false;

volatile bool g_logEnabled = false;

// FIXME: Move to the connection context
volatile bool g_writable = false;
bool g_notifEnabled = false;

inline void dumpFifo(const app_fifo_t& fifo) {
    LOG(INFO, "fifo: read_pos: %u, write_pos: %u, mask: %u", (unsigned)fifo.read_pos, (unsigned)fifo.write_pos,
            (unsigned)fifo.buf_size_mask);
}

void socEventHandler(uint32_t sys_evt, void *p_context) {
    PlatformSoftdeviceSocEvtHandler(sys_evt);
}

int initSocObserver() {
    NRF_SDH_SOC_OBSERVER(socObserver, NRF_SDH_SOC_STACK_OBSERVER_PRIO, socEventHandler, nullptr);
    return 0;
}

} // particle::system::

BleControlRequestChannel::BleControlRequestChannel(ControlRequestHandler* handler) :
        ControlRequestChannel(handler),
        pendingReqs_(nullptr),
        readyReqs_(nullptr),
        recvReq_(nullptr),
        recvFifo_(),
        recvBuf_(nullptr),
        recvDataPos_(0),
        recvHeader_(false),
        sendReq_(nullptr),
        sendBuf_(nullptr),
        sendDataPos_(0),
        sendHeader_(false) {
    g_instance = this;
}

BleControlRequestChannel::~BleControlRequestChannel() {
    destroy();
}

int BleControlRequestChannel::init() {
    int ret = initBle();
    if (ret != 0) {
        LOG(ERROR, "Unable to initialize BLE stack: %d", ret);
        return ret;
    }
    ret = initGap();
    if (ret != 0) {
        LOG(ERROR, "Unable to configure GAP: %d", ret);
        return ret;
    }
    ret = initGatt();
    if (ret != 0) {
        LOG(ERROR, "Unable to initialize GATT: %d", ret);
        return ret;
    }
    ret = initServices();
    if (ret != 0) {
        LOG(ERROR, "Unable to initialize services: %d", ret);
        return ret;
    }
    ret = initAdvert();
    if (ret != 0) {
        LOG(ERROR, "Unable to initialize the advertising module: %d", ret);
        return ret;
    }
    ret = initConnParam();
    if (ret != 0) {
        LOG(ERROR, "Unable to initialize the connection parameters module: %d", ret);
        return ret;
    }
    recvBuf_ = (uint8_t*)malloc(FIFO_BUF_SIZE);
    if (!recvBuf_) {
        LOG(ERROR, "Unable to allocate RX buffer");
        return SYSTEM_ERROR_NO_MEMORY;
    }
    const uint32_t nrfRet = app_fifo_init(&recvFifo_, recvBuf_, FIFO_BUF_SIZE);
    if (nrfRet != 0) {
        LOG(ERROR, "Unable to initialize FIFO");
        return SYSTEM_ERROR_UNKNOWN;
    }
    sendBuf_ = (uint8_t*)malloc(SEND_BUF_SIZE); // FIXME
    if (!sendBuf_) {
        LOG(ERROR, "Unable to allocate TX buffer");
        return SYSTEM_ERROR_NO_MEMORY;
    }
/*
    ret = startAdvert();
    if (ret != 0) {
        LOG(ERROR, "Unable to start advertising");
        return ret;
    }
*/
    return 0;
}

void BleControlRequestChannel::destroy() {
    // TODO
}

int BleControlRequestChannel::startAdvert() {
    const uint32_t ret = ble_advertising_start(&g_advert, BLE_ADV_MODE_FAST);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "ble_advertising_start() failed: %u", (unsigned)ret);
        return -1;
    }
    return 0;
}

void BleControlRequestChannel::stopAdvert() {
    // TODO
}

int BleControlRequestChannel::run() {
    if (g_connHandle == BLE_CONN_HANDLE_INVALID) {
        // Free completed requests
        while (readyReqs_) {
            readyReqs_ = freeReq(readyReqs_);
        }
        freeReq(recvReq_);
        recvReq_ = nullptr;
        freeReq(sendReq_);
        sendReq_ = nullptr;
        // Clear RX buffer
        app_fifo_flush(&recvFifo_);
        recvDataPos_ = 0;
        recvHeader_ = false;
        // Clear TX buffer
        sendDataPos_ = 0;
        sendHeader_ = false;
    } else {
        // Proceed sending and receiving data
        int ret = recvNext();
        if (ret != 0) {
            LOG(ERROR, "Unable to receive request data");
            return ret;
        }
        ret = sendNext();
        if (ret != 0) {
            LOG(ERROR, "Unable to send reply data");
            return ret;
        }
    }
    return 0;
}

int BleControlRequestChannel::allocReplyData(ctrl_request* ctrlReq, size_t size) {
    const auto req = static_cast<Req*>(ctrlReq);
    if (size > 0) {
        const auto data = (char*)realloc(req->reply_data, size);
        if (!data) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        req->reply_data = data;
        req->reply_size = size;
    } else {
        free(req->reply_data);
        req->reply_data = nullptr;
        req->reply_size = 0;
    }
    return 0;
}

void BleControlRequestChannel::freeRequestData(ctrl_request* ctrlReq) {
    const auto req = static_cast<Req*>(ctrlReq);
    free(req->request_data);
    req->request_data = nullptr;
    req->request_size = 0;
}

void BleControlRequestChannel::setResult(ctrl_request* ctrlReq, int result, ctrl_completion_handler_fn handler, void* data) {
    // FIXME: Synchronization
    const auto req = static_cast<Req*>(ctrlReq);
    req->result = result;
    req->next = readyReqs_;
    readyReqs_ = req;
}

int BleControlRequestChannel::sendNext() {
    if (!g_writable) {
        return 0; // Can't send now
    }
    if (!sendReq_) {
        if (!readyReqs_) {
            return 0; // Nothing to send
        }
        sendReq_ = readyReqs_;
        readyReqs_ = readyReqs_->next;
    }
    // Serialize reply header
    uint16_t size = 0;
    if (!sendHeader_) {
        RepHeader h = {};
        h.id = sendReq_->id;
        h.result = sendReq_->result;
        h.size = sendReq_->reply_size;
        memcpy(sendBuf_, &h, sizeof(RepHeader));
        size = sizeof(RepHeader);
    }
    // Copy reply data
    size_t dataSize = 0;
    if (sendReq_->reply_size > 0) {
        dataSize = sendReq_->reply_size - sendDataPos_;
        if (size + dataSize > g_maxDataSize) {
            dataSize = g_maxDataSize - size;
        }
        memcpy(sendBuf_ + size, sendReq_->reply_data + sendDataPos_, dataSize);
        size += dataSize;
    }
    // Send packet
    ble_gatts_hvx_params_t hvx = {};
    hvx.handle = g_txCharHandles.value_handle;
    hvx.p_data = sendBuf_;
    hvx.p_len = &size;
    hvx.type = BLE_GATT_HVX_NOTIFICATION;
    uint32_t ret = 0;
    do {
        ret = sd_ble_gatts_hvx(g_connHandle, &hvx);
    } while (ret == NRF_ERROR_BUSY);
    if (ret == NRF_ERROR_RESOURCES) {
        g_writable = false; // FIXME: Use an atomic counter
        return 0; // Retry later
    } else if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_hvx() failed: %u", (unsigned)ret);
        return -1;
    }
    LOG(TRACE, "Sent %u bytes", (unsigned)size);
    sendDataPos_ += dataSize;
    if (sendDataPos_ == sendReq_->reply_size) {
        freeReq(sendReq_);
        sendReq_ = nullptr;
        sendDataPos_ = 0;
        sendHeader_ = false;
    } else {
        sendHeader_ = true;
    }
    return 0;
}

int BleControlRequestChannel::recvNext() {
    // Read request header
    if (!recvHeader_) {
        uint32_t n = 0;
        const uint32_t ret = app_fifo_read(&recvFifo_, nullptr, &n);
        if (ret != NRF_SUCCESS && ret != NRF_ERROR_NOT_FOUND) {
            LOG(ERROR, "app_fifo_read() failed: %u", (unsigned)ret);
            return -1;
        }
        if (n < sizeof(ReqHeader)) {
            return 0; // Keep reading
        }
        ReqHeader h = {};
        n = sizeof(ReqHeader);
        app_fifo_read(&recvFifo_, (uint8_t*)&h, &n);
        recvReq_ = allocReq(h.size);
        if (!recvReq_) {
            LOG(ERROR, "Unable to allocate request object");
            return -1;
        }
        recvReq_->id = h.id;
        recvReq_->type = h.type;
        recvReq_->channel = this;
        recvHeader_ = true;
    }
    // Read request data
    if (recvReq_->request_size > 0) {
        uint32_t n = recvReq_->request_size - recvDataPos_;
        const uint32_t ret = app_fifo_read(&recvFifo_, (uint8_t*)recvReq_->request_data + recvDataPos_, &n);
        if (ret != NRF_SUCCESS && ret != NRF_ERROR_NOT_FOUND) {
            LOG(ERROR, "app_fifo_read() failed: %u", (unsigned)ret);
            return -1;
        }
        recvDataPos_ += n;
        if (recvDataPos_ < recvReq_->request_size) {
            return 0; // Keep reading
        }
    }
    // Process request
    recvReq_->next = pendingReqs_;
    pendingReqs_ = recvReq_;
    recvReq_ = nullptr;
    recvDataPos_ = 0;
    recvHeader_ = false;
    handler()->processRequest(pendingReqs_, this);
    return 0;
}

void BleControlRequestChannel::onConnect(const ble_evt_t* event) {
    // Check if the client has enabled notifications
    uint8_t cccdVal[2] = {};
    ble_gatts_value_t val = {};
    val.p_value = cccdVal;
    val.len = sizeof(cccdVal);
    val.offset = 0;
    const uint32_t ret = sd_ble_gatts_value_get(event->evt.gap_evt.conn_handle, g_txCharHandles.cccd_handle, &val);
    g_notifEnabled = (ret == NRF_SUCCESS && ble_srv_is_notification_enabled(val.p_value));
    g_writable = g_notifEnabled;
}

void BleControlRequestChannel::onDisconnect(const ble_evt_t* event) {
    g_notifEnabled = false;
    g_writable = false;
}

void BleControlRequestChannel::onWrite(const ble_evt_t* event) {
    const ble_gatts_evt_write_t& writeEvent = event->evt.gatts_evt.params.write;
    if (writeEvent.handle == g_txCharHandles.cccd_handle && writeEvent.len == 2) {
        g_notifEnabled = ble_srv_is_notification_enabled(writeEvent.data);
        g_writable = g_notifEnabled;
        LOG(TRACE, "Notifications enabled: %u", (unsigned)g_notifEnabled);
    } else if (writeEvent.handle == g_rxCharHandles.value_handle) {
        LOG(TRACE, "Received %u bytes", (unsigned)writeEvent.len);
        uint32_t n = writeEvent.len;
        uint32_t ret = app_fifo_write(&recvFifo_, writeEvent.data, &n);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "app_fifo_write() failed: %u", (unsigned)ret);
        }
    }
}

void BleControlRequestChannel::onTxComplete(const ble_evt_t* event) {
    if (g_notifEnabled) {
        g_writable = true;
    }
}

void BleControlRequestChannel::bleEventHandler(const ble_evt_t* event) {
    switch (event->header.evt_id) {
    // GAP
    case BLE_GAP_EVT_CONNECTED: {
        g_connHandle = event->evt.gap_evt.conn_handle;
        LOG(TRACE, "Connected: 0x%02x", (unsigned)g_connHandle);
        // Configure QWR module
        const ret_code_t ret = nrf_ble_qwr_conn_handle_assign(&g_qwr, g_connHandle);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "nrf_ble_qwr_conn_handle_assign() failed: %u", (unsigned)ret);
        }
        onConnect(event);
        break;
    }
    case BLE_GAP_EVT_DISCONNECTED: {
        LOG(TRACE, "Disconnected: 0x%02x", (unsigned)g_connHandle);
        g_connHandle = BLE_CONN_HANDLE_INVALID;
        break;
    }
    case BLE_GATTS_EVT_WRITE: {
        onWrite(event);
        break;
    }
    case BLE_GATTS_EVT_HVN_TX_COMPLETE: {
        onTxComplete(event);
        break;
    }
    case BLE_GAP_EVT_PHY_UPDATE_REQUEST: {
        LOG(TRACE, "PHY update request");
        ble_gap_phys_t phys = {};
        phys.rx_phys = BLE_GAP_PHY_AUTO;
        phys.tx_phys = BLE_GAP_PHY_AUTO;
        const uint32_t ret = sd_ble_gap_phy_update(event->evt.gap_evt.conn_handle, &phys);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "sd_ble_gap_phy_update() failed: %u", (unsigned)ret);
        }
        break;
    }
    case BLE_GAP_EVT_SEC_PARAMS_REQUEST: {
        // Pairing not supported
        const uint32_t ret = sd_ble_gap_sec_params_reply(g_connHandle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, nullptr, nullptr);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "sd_ble_gap_sec_params_reply() failed: %u", (unsigned)ret);
        }
        break;
    }
    case BLE_GATTS_EVT_SYS_ATTR_MISSING: {
        // No system attributes have been stored
        const uint32_t ret = sd_ble_gatts_sys_attr_set(g_connHandle, nullptr, 0, 0);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "sd_ble_gatts_sys_attr_set() failed: %u", (unsigned)ret);
        }
        break;
    }
    case BLE_GATTC_EVT_TIMEOUT: {
        // Disconnect on GATT client timeout
        const uint32_t ret = sd_ble_gap_disconnect(event->evt.gattc_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "sd_ble_gap_disconnect() failed: %u", (unsigned)ret);
        }
        break;
    }
    case BLE_GATTS_EVT_TIMEOUT: {
        // Disconnect on GATT server timeout
        const uint32_t ret = sd_ble_gap_disconnect(event->evt.gatts_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "sd_ble_gap_disconnect() failed: %u", (unsigned)ret);
        }
        break;
    }
    default:
        LOG(TRACE, "Unhandled BLE event: %u", (unsigned)event->header.evt_id);
        break;
    }
}

int BleControlRequestChannel::initConnParam() {
    // TODO: ble_conn_params depends on app_timer module
/*
    // Initialize the connection parameters module
    ble_conn_params_init_t init = {};
    init.p_conn_params = nullptr;
    init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    init.next_conn_params_update_delay = NEXT_CONN_PARAMS_UPDATE_DELAY;
    init.max_conn_params_update_count = MAX_CONN_PARAMS_UPDATE_COUNT;
    init.start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID;
    init.disconnect_on_fail = false;
    init.evt_handler = connParamEventHandler;
    init.error_handler = connParamErrorHandler;
    const uint32_t ret = ble_conn_params_init(&init);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "ble_conn_params_init() failed: %u", (unsigned)ret);
        return -1;
    }
*/
    return 0;
}

int BleControlRequestChannel::initAdvert() {
    // Service UUID
    ble_uuid_t svcUuid = {};
    svcUuid.uuid = SETUP_SERVICE_UUID;
    svcUuid.type = g_uuidType;
    // Initialize the advertising module
    ble_advertising_init_t init = {};
    init.advdata.name_type = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance = false;
    init.advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;
    init.srdata.uuids_complete.uuid_cnt = 1;
    init.srdata.uuids_complete.p_uuids = &svcUuid;
    init.config.ble_adv_fast_enabled = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout = APP_ADV_DURATION;
    init.config.ble_adv_slow_enabled = true;
    init.config.ble_adv_slow_interval = APP_ADV_INTERVAL_SLOW;
    init.config.ble_adv_slow_timeout = APP_ADV_DURATION_SLOW;
    init.evt_handler = advertEventHandler;
    const uint32_t ret = ble_advertising_init(&g_advert, &init);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "ble_advertising_init() failed: %u", (unsigned)ret);
        return -1;
    }
    ble_advertising_conn_cfg_tag_set(&g_advert, APP_BLE_CONN_CFG_TAG);
    return 0;
}

int BleControlRequestChannel::initTxChar() {
    // Characteristic UUID
    ble_uuid_t charUuid = {};
    charUuid.uuid = TX_CHAR_UUID;
    charUuid.type = g_uuidType;
    // Metadata for the Client Characteristic Configuration Descriptor (CCCD)
    ble_gatts_attr_md_t cccdMd = {};
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccdMd.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccdMd.write_perm);
    cccdMd.vloc = BLE_GATTS_VLOC_STACK;
    // Characteristic metadata
    ble_gatts_char_md_t charMd = {};
    charMd.char_props.notify = 1;
    charMd.p_cccd_md = &cccdMd;
    // GATT attribute metadata
    ble_gatts_attr_md_t attrMd = {};
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attrMd.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attrMd.write_perm);
    attrMd.vloc = BLE_GATTS_VLOC_STACK;
    attrMd.rd_auth = 0;
    attrMd.wr_auth = 0;
    attrMd.vlen = 1;
    // GATT attribute
    ble_gatts_attr_t attr = {};
    attr.p_uuid = &charUuid;
    attr.p_attr_md = &attrMd;
    attr.init_len = 1;
    attr.init_offs = 0;
    attr.max_len = CHAR_MAX_DATA_SIZE;
    const uint32_t ret = sd_ble_gatts_characteristic_add(g_serviceHandle, &charMd, &attr, &g_txCharHandles);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_characteristic_add() failed: %u", (unsigned)ret);
        return -1;
    }
    return 0;
}

int BleControlRequestChannel::initRxChar() {
    // Characteristic UUID
    ble_uuid_t charUuid = {};
    charUuid.uuid = RX_CHAR_UUID;
    charUuid.type = g_uuidType;
    // Characteristic metadata
    ble_gatts_char_md_t charMd = {};
    charMd.char_props.write = 1;
    charMd.char_props.write_wo_resp = 1;
    // GATT attribute metadata
    ble_gatts_attr_md_t attrMd = {};
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attrMd.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attrMd.write_perm);
    attrMd.vloc = BLE_GATTS_VLOC_STACK;
    attrMd.rd_auth = 0;
    attrMd.wr_auth = 0;
    attrMd.vlen = 1;
    // GATT attribute
    ble_gatts_attr_t attr = {};
    attr.p_uuid = &charUuid;
    attr.p_attr_md = &attrMd;
    attr.init_len = 1;
    attr.init_offs = 0;
    attr.max_len = CHAR_MAX_DATA_SIZE;
    const uint32_t ret = sd_ble_gatts_characteristic_add(g_serviceHandle, &charMd, &attr, &g_rxCharHandles);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_characteristic_add() failed: %u", (unsigned)ret);
        return -1;
    }
    return 0;
}

int BleControlRequestChannel::initServices() {
    // Initialize the queued write module
    nrf_ble_qwr_init_t init = {};
    init.error_handler = qwrErrorHandler;
    uint32_t ret = nrf_ble_qwr_init(&g_qwr, &init);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "nrf_ble_qwr_init() failed: %u", (unsigned)ret);
        return -1;
    }
    // Register a custom base UUID
    ble_uuid128_t baseUuid = {};
    SPARK_ASSERT(sizeof(baseUuid.uuid128) == sizeof(BASE_UUID));
    memcpy(baseUuid.uuid128, BASE_UUID, sizeof(baseUuid.uuid128));
    ret = sd_ble_uuid_vs_add(&baseUuid, &g_uuidType);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_uuid_vs_add() failed: %u", (unsigned)ret);
        return -1;
    }
    // Register the device setup service
    ble_uuid_t svcUuid = {};
    svcUuid.uuid = SETUP_SERVICE_UUID;
    svcUuid.type = g_uuidType;
    ret = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &svcUuid, &g_serviceHandle);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gatts_service_add() failed: %u", (unsigned)ret);
        return -1;
    }
    if (initRxChar() != 0 || initTxChar() != 0) {
        LOG(ERROR, "Unable to initialize service characteristics");
        return -1;
    }
    return 0;
}

int BleControlRequestChannel::initGatt() {
    // Initialize the GATT module
    ret_code_t ret = nrf_ble_gatt_init(&g_gatt, gattEventHandler);
    if (ret != NRF_SUCCESS) {
        return -1;
    }
    // Set MTU size
    ret = nrf_ble_gatt_att_mtu_periph_set(&g_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "nrf_ble_gatt_att_mtu_periph_set() failed: %u", (unsigned)ret);
        return -1;
    }
    return 0;
}

int BleControlRequestChannel::initGap() {
    ble_gap_conn_sec_mode_t secMode = {};
    // Require no protection
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&secMode);
    // Set device name
    uint32_t ret = sd_ble_gap_device_name_set(&secMode, (const uint8_t*)DEVICE_NAME, strlen(DEVICE_NAME));
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_device_name_set() failed: %u", (unsigned)ret);
        return -1;
    }
    // Set Peripheral Preferred Connection Parameters (PPCP)
    ble_gap_conn_params_t connParams = {};
    connParams.min_conn_interval = MIN_CONN_INTERVAL;
    connParams.max_conn_interval = MAX_CONN_INTERVAL;
    connParams.slave_latency = SLAVE_LATENCY;
    connParams.conn_sup_timeout = CONN_SUP_TIMEOUT;
    ret = sd_ble_gap_ppcp_set(&connParams);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gap_ppcp_set() failed: %u", (unsigned)ret);
        return -1;
    }
    return 0;
}

int BleControlRequestChannel::initBle() {
    ret_code_t ret = 0;
    // Request to enable SoftDevice
    //ret_code_t ret = nrf_sdh_enable_request();
    //if (ret != NRF_SUCCESS) {
    //    LOG(ERROR, "nrf_sdh_enable_request() failed: %u", (unsigned)ret);
    //    return -1;
    //}
    // Configure the BLE stack using default settings
    uint32_t ramStart = 0; // Start address of the application RAM
    ret = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ramStart);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "nrf_sdh_ble_default_cfg_set() failed: %u", (unsigned)ret);
        return -1;
    }
    LOG(INFO, "RAM start: 0x%08x", (unsigned)ramStart);
    // Configure and enable the BLE stack
    ret = nrf_sdh_ble_enable(&ramStart);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "nrf_sdh_ble_enable() failed: %u", (unsigned)ret);
        return -1;
    }
    // Register a handler for BLE events
    NRF_SDH_BLE_OBSERVER(bleObserver, APP_BLE_OBSERVER_PRIO, bleEventHandler, nullptr);
    // Register a handler for SOC events
    initSocObserver();
    return 0;
}

void BleControlRequestChannel::bleEventHandler(const ble_evt_t* event, void* data) {
    g_instance->bleEventHandler(event);
}

void BleControlRequestChannel::connParamEventHandler(ble_conn_params_evt_t* event) {
    if (event->evt_type == BLE_CONN_PARAMS_EVT_FAILED) {
        const uint32_t ret = sd_ble_gap_disconnect(g_connHandle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "Unable to terminate GAP link: %u", (unsigned)ret);
        }
    }
}

void BleControlRequestChannel::connParamErrorHandler(uint32_t error) {
    LOG(ERROR, "Connection parameters module error: %u", (unsigned)error);
}

void BleControlRequestChannel::advertEventHandler(ble_adv_evt_t event) {
    switch (event) {
    case BLE_ADV_EVT_FAST:
        g_advertActive = true;
        LOG(TRACE, "Fast advertising mode started");
        break;
    case BLE_ADV_EVT_SLOW:
        g_advertActive = true;
        LOG(TRACE, "Slow advertising mode started");
        break;
    case BLE_ADV_EVT_IDLE:
        g_advertActive = false;
        LOG(TRACE, "No connectable advertising is ongoing");
        break;
    default:
        LOG(TRACE, "Unhandled advertisement event: %u", (unsigned)event);
        break;
    }
}

void BleControlRequestChannel::gattEventHandler(nrf_ble_gatt_t* gatt, const nrf_ble_gatt_evt_t* event) {
    if (g_connHandle == event->conn_handle && event->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED) {
        g_maxDataSize = event->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
        LOG(TRACE, "Data length is set to %u", (unsigned)g_maxDataSize);
    }
    LOG(TRACE, "ATT MTU exchange completed, central: %u, peripheral: %u", (unsigned)gatt->att_mtu_desired_central,
            (unsigned)gatt->att_mtu_desired_periph);
}

void BleControlRequestChannel::qwrErrorHandler(uint32_t error) {
    LOG(ERROR, "Queued write module error: %u", (unsigned)error);
}

BleControlRequestChannel::Req* BleControlRequestChannel::allocReq(size_t size) {
    const auto req = (Req*)malloc(sizeof(Req));
    if (!req) {
        return nullptr;
    }
    memset(req, 0, sizeof(Req));
    if (size > 0) {
        req->request_data = (char*)malloc(size);
        if (!req->request_data) {
            free(req);
            return nullptr;
        }
        req->request_size = size;
    }
    return req;
}

BleControlRequestChannel::Req* BleControlRequestChannel::freeReq(Req* req) {
    if (!req) {
        return nullptr;
    }
    const auto next = req->next;
    free(req->request_data);
    free(req->reply_data);
    free(req);
    return next;
}

bool BleControlRequestChannel::advertActive() const { // FIXME
    return g_advertActive;
}

bool BleControlRequestChannel::connected() const { // FIXME
    const auto connHandle = g_connHandle;
    return (connHandle != BLE_CONN_HANDLE_INVALID);
}

} // particle::system

} // particle

void enable_log() {
    particle::system::g_logEnabled = true;
}

#endif // HAS_BLE_CONTROL_REQUEST_CHANNEL
