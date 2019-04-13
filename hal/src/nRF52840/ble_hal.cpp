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
#include "simple_pool_allocator.h"
#include <string.h>
#include <memory>


using spark::Vector;

#define BLE_CONN_CFG_TAG 1

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

//anonymous namespace
namespace {

system_error_t sysError(uint32_t error) {
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

int toPlatformUUID(const hal_ble_uuid_t* halUuid, ble_uuid_t* uuid) {
    if (uuid == NULL || halUuid == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (halUuid->type == BLE_UUID_TYPE_128BIT) {
        ble_uuid128_t uuid128;
        memcpy(uuid128.uuid128, halUuid->uuid128, BLE_SIG_UUID_128BIT_LEN);
        ret_code_t ret = sd_ble_uuid_vs_add(&uuid128, &uuid->type);
        NRF_CHECK_RETURN(ret, "sd_ble_uuid_vs_add");
        ret = sd_ble_uuid_decode(BLE_SIG_UUID_128BIT_LEN, halUuid->uuid128, uuid);
        NRF_CHECK_RETURN(ret, "sd_ble_uuid_decode");
    } else if (halUuid->type == BLE_UUID_TYPE_16BIT) {
        uuid->type = BLE_UUID_TYPE_BLE;
        uuid->uuid = halUuid->uuid16;
    } else {
        uuid->type = BLE_UUID_TYPE_UNKNOWN;
        uuid->uuid = halUuid->uuid16;
    }
    return SYSTEM_ERROR_NONE;
}

int toHalUUID(const ble_uuid_t* uuid, hal_ble_uuid_t* halUuid) {
    if (uuid == NULL || halUuid == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (uuid->type == BLE_UUID_TYPE_BLE) {
        halUuid->type = BLE_UUID_TYPE_16BIT;
        halUuid->uuid16 = uuid->uuid;
        return SYSTEM_ERROR_NONE;
    } else if (uuid->type != BLE_UUID_TYPE_UNKNOWN) {
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

uint8_t toHalCharProps(ble_gatt_char_props_t properties) {
    uint8_t halProperties = 0;
    if (properties.broadcast) {
        halProperties |= BLE_SIG_CHAR_PROP_BROADCAST;
    }
    if (properties.read) {
        halProperties |= BLE_SIG_CHAR_PROP_READ;
    }
    if (properties.write_wo_resp) {
        halProperties |= BLE_SIG_CHAR_PROP_WRITE_WO_RESP;
    }
    if (properties.write) {
        halProperties |= BLE_SIG_CHAR_PROP_WRITE;
    }
    if (properties.notify){
        halProperties |= BLE_SIG_CHAR_PROP_NOTIFY;
    }
    if (properties.indicate) {
        halProperties |= BLE_SIG_CHAR_PROP_INDICATE;
    }
    if (properties.auth_signed_wr) {
        halProperties |= BLE_SIG_CHAR_PROP_AUTH_SIGN_WRITES;
    }
    return halProperties;
}

uint8_t toPlatformCharProps(uint8_t halProperties, ble_gatt_char_props_t* properties) {
    memset(properties, 0x00, sizeof(ble_gatt_char_props_t));
    properties->broadcast = (halProperties & BLE_SIG_CHAR_PROP_BROADCAST) ? 1 : 0;
    properties->read = (halProperties & BLE_SIG_CHAR_PROP_READ) ? 1 : 0;
    properties->write_wo_resp = (halProperties & BLE_SIG_CHAR_PROP_WRITE_WO_RESP) ? 1 : 0;
    properties->write = (halProperties & BLE_SIG_CHAR_PROP_WRITE) ? 1 : 0;
    properties->notify = (halProperties & BLE_SIG_CHAR_PROP_NOTIFY) ? 1 : 0;
    properties->indicate = (halProperties & BLE_SIG_CHAR_PROP_INDICATE) ? 1 : 0;
    properties->auth_signed_wr = (halProperties & BLE_SIG_CHAR_PROP_AUTH_SIGN_WRITES) ? 1 : 0;
    return 0;
}

} //anonymous namespace

class BleObject {
public:
    class BleEventDispatcher;
    class BleGap;
    class Broadcaster;
    class Observer;
    class ConnectionsManager;
    class GattServer;
    class GattClient;

    std::unique_ptr<BleEventDispatcher> dispatcher;     /**< BLE event dispatcher. */
    std::unique_ptr<BleGap> gap;                        /**< BLE GAP instance. */
    std::unique_ptr<Broadcaster> broadcaster;           /**< BLE Broadcaster instance. */
    std::unique_ptr<Observer> observer;                 /**< BLE Observer instance. */
    std::unique_ptr<ConnectionsManager> connectionsMgr; /**< BLE connections manager instance. */
    std::unique_ptr<GattServer> gatts;                  /**< BLE GATT server instance. */
    std::unique_ptr<GattClient> gattc;                  /**< BLE GATT client instance. */
    on_ble_evt_cb_t genericEvthandler_;                 /**< BLE generic event handler. */
    void* genericEvtContext_;                           /**< BLE generic event context. */

    static BleObject& getInstance(void);
    int init(void);
    void onGenericEventCallback(on_ble_evt_cb_t cb, void* context);

private:
    static bool initialized_;

    BleObject();
    ~BleObject() = default;
};

class BleObject::BleEventDispatcher {
public:
    typedef void (*BleEvtHandler)(void* data, void* context);

    class EventMessage {
    public:
        BleEvtHandler handler;          /**< BLE event handler. */
        hal_ble_evts_type_t evtType;    /**< BLE event type. */
        void* evtData;                  /**< BLE event data. */
        void* context;                  /**< BLE event context. */
        EventMessage() {
            evtType = BLE_EVT_UNKNOWN;
            handler = nullptr;
            evtData = nullptr;
            context = nullptr;
        }
        ~EventMessage() {}
    };

    BleEventDispatcher(BleObject* obj);
    ~BleEventDispatcher() = default;
    int init(void);
    void* alloc(size_t size);
    int enqueue(EventMessage& msg);

private:
    os_queue_t evtQueue_;                               /**< BLE event queue. */
    os_thread_t evtThread_;                             /**< BLE event thread. */
    AtomicAllocedPool evtPool_;                         /**< Pool to allocate memory for BLE event. */
    BleObject* bleObj_;                                 /**< Pointer to the BLE Object. */
    static const uint16_t BLE_EVT_POOL_SIZE = 2048;     /**< Maximum BLE event pool size. */

    int isGenericEvent(hal_ble_evts_type_t type);
    static os_thread_return_t bleEventDispatch(void* param);
};

class BleObject::BleGap {
public:
    BleGap(BleObject* obj);
    ~BleGap() = default;
    int init(void);
    int setDeviceName(const uint8_t* deviceName, uint16_t len);
    int getDeviceName(uint8_t* deviceName, uint16_t* len);
    int setDeviceAddress(const hal_ble_addr_t* address);
    int getDeviceAddress(hal_ble_addr_t* address);
    int setAppearance(uint16_t appearance);
    int getAppearance(uint16_t* appearance);
    int addWhitelist(const hal_ble_addr_t* addrList, uint8_t len);
    int deleteWhitelist(void);

private:
    ble_gap_addr_t whitelist_[BLE_MAX_WHITELIST_ADDR_COUNT];                /**< BLE device whitelist. */
    ble_gap_addr_t const* whitelistPointer_[BLE_MAX_WHITELIST_ADDR_COUNT];  /**< BLE device whitelists' pointer. */
    BleObject* bleObj_;                                                     /**< Pointer to the BLE Object. */

    static void processBleGapEvents(const ble_evt_t* event, void* context);
};

class BleObject::Broadcaster {
public:
    Broadcaster(BleObject* obj);
    ~Broadcaster() = default;
    int init(void);
    bool advertising(void) const;
    int setAdvertisingParams(const hal_ble_adv_params_t* params);
    int getAdvertisingParams(hal_ble_adv_params_t* params) const;
    int setAdvertisingData(const uint8_t* buf, size_t len);
    size_t getAdvertisingData(uint8_t* buf, size_t len) const;
    int setScanResponseData(const uint8_t* buf, size_t len);
    size_t getScanResponseData(uint8_t* buf, size_t len) const;
    int setTxPower(int8_t val);
    int8_t getTxPower(void) const;
    int startAdvertising(void);
    int stopAdvertising(void);

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
    BleObject* bleObj_;                             /**< Pointer to the BLE Object. */
    static const int8_t validTxPower_[8];           /**< Valid TX power values. */

    int suspend(void);
    int resume(void);
    void toPlatformAdvParams(ble_gap_adv_params_t* params, const hal_ble_adv_params_t* halParams) const;
    void toPlatformAdvData(ble_gap_adv_data_t* data);
    int configure(const hal_ble_adv_params_t* params);
    int8_t roundTxPower(int8_t value);
    static void processBroadcasterEvents(const ble_evt_t* event, void* context);
};

class BleObject::Observer {
public:
    Observer(BleObject* obj);
    ~Observer() = default;
    int init(void);
    bool scanning(void);
    int setScanParams(const hal_ble_scan_params_t* params);
    int getScanParams(hal_ble_scan_params_t* params) const;
    int startScanning(bool restart);
    int stopScanning(void);
    void toPlatformScanParams(ble_gap_scan_params_t* params) const;

private:
    uint8_t isScanning_: 1;                                 /**< If it is scanning or not. */
    hal_ble_scan_params_t scanParams_;                      /**< BLE scanning parameters. */
    os_semaphore_t scanSemaphore_;                          /**< Semaphore to wait until the scan procedure completed. */
    uint8_t scanReportBuff_[BLE_MAX_SCAN_REPORT_BUF_LEN];   /**< Buffer to hold the scanned report data. */
    ble_data_t bleScanData_;                                /**< BLE scanned data. */
    Vector<hal_ble_addr_t> cache_;                          /**< Cached address of scanned devices to filter-out duplicated result. */
    BleObject* bleObj_;                                     /**< Pointer to the BLE Object. */

    bool chacheDevice(const hal_ble_addr_t& newAddr);
    static void processObserverEvents(const ble_evt_t* event, void* context);
};

class BleObject::ConnectionsManager {
public:
    typedef struct {
        uint8_t role;
        uint16_t connHandle;
        hal_ble_conn_params_t effectiveConnParams;
        hal_ble_addr_t peer;
    } BleConnection_t;

    ConnectionsManager(BleObject* obj);
    ~ConnectionsManager() = default;
    int init(void);
    int setPPCP(const hal_ble_conn_params_t* ppcp);
    int getPPCP(hal_ble_conn_params_t* ppcp) const;
    bool connecting(void) const;
    bool connected(const hal_ble_addr_t* address=nullptr);
    int connect(const hal_ble_addr_t* address);
    int connectCancel(void);
    int disconnect(uint16_t connHandle);
    int updateConnectionParams(uint16_t connHandle, const hal_ble_conn_params_t* params);
    int getConnectionParams(uint16_t connHandle, hal_ble_conn_params_t* params);
    bool valid(uint16_t connHandle) const;

private:
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
    BleObject* bleObj_;                             /**< Pointer to the BLE Object. */

    static const uint32_t CONNECTION_OPERATION_TIMEOUT_MS = 5000;

    void toPlatformConnParams(ble_gap_conn_params_t* params, const hal_ble_conn_params_t* halConnParams) const;
    void toHalConnParams(hal_ble_conn_params_t* halConnParams, const ble_gap_conn_params_t* params) const;
    BleConnection_t* fetchConnection(uint16_t connHandle);
    BleConnection_t* fetchConnection(const hal_ble_addr_t* address);
    void addConnection(const BleConnection_t& connection);
    void removeConnection(uint16_t connHandle);
    void initiateConnParamsUpdateIfNeeded(const BleConnection_t* connection);
    bool isConnParamsFeeded(const hal_ble_conn_params_t* params) const;
    static void onConnParamsUpdateTimerExpired(os_timer_t timer);
    static void processConnectionEvents(const ble_evt_t* event, void* context);
};

class BleObject::GattServer {
public:
    GattServer(BleObject* obj);
    ~GattServer() = default;
    int init(void);
    int addService(uint8_t type, const hal_ble_uuid_t* uuid, uint16_t* handle);
    int addCharacteristic(uint16_t svcHandle, uint8_t properties, const hal_ble_uuid_t* uuid, const char* desc, hal_ble_char_handles_t* handles);
    int addDescriptor(uint16_t charHandle, const hal_ble_uuid_t* uuid, uint8_t* descriptor, uint16_t len, uint16_t* handle);
    size_t setValue(uint16_t handle, const uint8_t* buf, size_t len);
    size_t getValue(uint16_t handle, uint8_t* buf, size_t len);

private:
    class BleCharacteristic {
        public:
            uint8_t properties;
            uint16_t svcHandle;
            hal_ble_char_handles_t handles;
            Vector<uint16_t> cccdConnections;
    };

    static const uint32_t BLE_HVX_PROCEDURE_TIMEOUT_MS = 5000;

    os_semaphore_t hvxSemaphore_;                   /**< Semaphore to wait until the HVX operation completed. */
    AtomicAllocedPool pool_;                        /**< Pool to allocate memory for characteristic value. */
    Vector<uint16_t> services_;                     /**< Added services. */
    Vector<BleCharacteristic> characteristics_;     /**< Added characteristic. */
    BleObject* bleObj_;                             /**< Pointer to the BLE Object. */

    bool findService(uint16_t handle) const;
    BleCharacteristic* findCharacteristic(uint16_t handle);
    void addToCCCDList(BleCharacteristic* characteristic, uint16_t connHandle);
    void removeFromCCCDList(BleCharacteristic* characteristic, uint16_t connHandle);
    void removeFromAllCCCDList(uint16_t connHandle);
    static void processGattServerEvents(const ble_evt_t* event, void* context);
};

class BleObject::GattClient {
public:
    GattClient(BleObject* obj);
    ~GattClient() = default;
    int init(void);
    bool discovering(void) const;
    int discoverServices(uint16_t connHandle, const hal_ble_uuid_t* uuid);
    int discoverCharacteristics(uint16_t connHandle, const hal_ble_svc_t* service);
    size_t writeAttribute(uint16_t connHandle, uint16_t attrHandle, const uint8_t* buf, size_t len, bool response);
    size_t readAttribute(uint16_t connHandle, uint16_t attrHandle, uint8_t* buf, size_t len);

private:
    enum {
        BLE_DISCOVERY_PROCEDURE_IDLE,
        BLE_DISCOVERY_PROCEDURE_SERVICES,
        BLE_DISCOVERY_PROCEDURE_CHARACTERISTICS,
        BLE_DISCOVERY_PROCEDURE_DESCRIPTORS,
    };

    static const uint16_t SERVICES_BASE_START_HANDLE = 0x0001;
    static const uint16_t SERVICES_TOP_END_HANDLE = 0xFFFF;
    static const uint32_t BLE_DICOVERY_PROCEDURE_TIMEOUT_MS = 10000;
    static const uint32_t BLE_READ_WRITE_PROCEDURE_TIMEOUT_MS = 5000;
    static const uint16_t GATT_CLIENT_POOL_SIZE = 1024;

    uint8_t discoverAll_ : 1;                           /**< If it is going to discover all service during the service discovery procedure. */
    uint8_t isDiscovering_ : 1;                         /**< If there is on-going discovery procedure. */
    uint16_t currDiscConnHandle_;                       /**< Current connection handle under which the service and characteristics to be discovered. */
    uint8_t currDiscProcedure_;                         /**< Current discovery procedure. */
    hal_ble_svc_t currDiscSvc_;                         /**< Current service to be discovered for the characteristics. */
    os_semaphore_t discoverySemaphore_;                 /**< Semaphore to wait until the discovery procedure completed. */
    os_semaphore_t readWriteSemaphore_;                 /**< Semaphore to wait until the read or write operation completed. */
    Vector<hal_ble_svc_t> discoveredServices_;          /**< Discover services. */
    Vector<hal_ble_char_t> discoveredCharacteristics_;  /**< Discovered characteristics. */
    AtomicAllocedPool gattcPool_;                       /**< Pool to allocate memory for discovered services or characteristics. */
    uint16_t readAttrHandle_;                           /**< Current handle of which attribute to be read. */
    uint8_t* readBuf_;                                  /**< Current buffer to be filled for the read data. */
    uint16_t readLen_;                                  /**< Length of read data. */
    BleObject* bleObj_;                                 /**< Pointer to the BLE Object. */

    void resetDiscoveryState(void);
    bool readServiceUUID128IfNeeded(void);
    bool readCharacteristicUUID128IfNeeded(void);
    hal_ble_svc_t* findDiscoveredService(uint16_t handle);
    hal_ble_char_t* findDiscoveredCharacteristic(uint16_t handle);
    static void processGattClientEvents(const ble_evt_t* event, void* context);
};

BleObject::BleEventDispatcher::BleEventDispatcher(BleObject* obj) : bleObj_(obj ){
    evtQueue_ = nullptr;
    evtThread_ = nullptr;
}

int BleObject::BleEventDispatcher::init(void) {
    evtPool_.init(BLE_EVT_POOL_SIZE);
    if (os_queue_create(&evtQueue_, sizeof(EventMessage), BLE_EVENT_QUEUE_ITEM_COUNT, NULL)) {
        LOG(ERROR, "os_queue_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }
    if (os_thread_create(&evtThread_, "BLE Event Thread", OS_THREAD_PRIORITY_NETWORK, bleEventDispatch, this, BLE_EVENT_THREAD_STACK_SIZE)) {
        LOG(ERROR, "os_thread_create() failed");
        os_queue_destroy(evtQueue_, nullptr);
        return SYSTEM_ERROR_INTERNAL;
    }
    return SYSTEM_ERROR_NONE;
}

void* BleObject::BleEventDispatcher::alloc(size_t size) {
    return evtPool_.alloc(size);
}

int BleObject::BleEventDispatcher::enqueue(EventMessage& msg) {
    if (os_queue_put(evtQueue_, &msg, 0, NULL)) {
        LOG(ERROR, "os_queue_put() failed.");
        if (msg.evtData != nullptr) {
            evtPool_.free(msg.evtData);
        }
        return SYSTEM_ERROR_NO_MEMORY;
    }
    return SYSTEM_ERROR_NONE;
}

int BleObject::BleEventDispatcher::isGenericEvent(hal_ble_evts_type_t type) {
    if (type == BLE_EVT_ADV_STOPPED ||
            type == BLE_EVT_CONNECTED ||
            type == BLE_EVT_DISCONNECTED ||
            type == BLE_EVT_CONN_PARAMS_UPDATED) {
        return true;
    }
    return false;
}

os_thread_return_t BleObject::BleEventDispatcher::bleEventDispatch(void* param) {
    BleEventDispatcher* dispatcher = static_cast<BleEventDispatcher*>(param);
    while (1) {
        EventMessage msg;
        if (!os_queue_take(dispatcher->evtQueue_, &msg, CONCURRENT_WAIT_FOREVER, NULL)) {
            if (msg.handler && msg.evtData && msg.context) {
                if (dispatcher->isGenericEvent(msg.evtType)) {
                    hal_ble_evts_t evt;
                    evt.type = msg.evtType;
                    if (msg.evtType == BLE_EVT_ADV_STOPPED) {
                        memcpy(&evt.params, msg.evtData, sizeof(hal_ble_gap_on_adv_stopped_evt_t));
                    } else if (msg.evtType == BLE_EVT_CONNECTED) {
                        memcpy(&evt.params, msg.evtData, sizeof(hal_ble_gap_on_connected_evt_t));
                    } else if (msg.evtType == BLE_EVT_DISCONNECTED) {
                        memcpy(&evt.params, msg.evtData, sizeof(hal_ble_gap_on_disconnected_evt_t));
                    } else if (msg.evtType == BLE_EVT_CONN_PARAMS_UPDATED) {
                        memcpy(&evt.params, msg.evtData, sizeof(hal_ble_gap_on_conn_params_evt_t));
                    }
                    msg.handler(&evt, msg.context);
                } else {
                    msg.handler(msg.evtData, msg.context);
                }
                LOG_DEBUG(TRACE, "Free memory: %u.", (unsigned)msg.evtData);
                dispatcher->evtPool_.free(msg.evtData);
            }
        } else {
            LOG(ERROR, "BLE event thread exited.");
            break;
        }
    }
    os_thread_exit(dispatcher->evtThread_);
}

typedef struct {
    BleObject::BleGap* instance;
} BleGapImpl;
static BleGapImpl bleGapImpl;

BleObject::BleGap::BleGap(BleObject* obj) : bleObj_(obj) {
}

int BleObject::BleGap::init(void) {
    // Set the default device name
    char devName[32] = {};
    get_device_name(devName, sizeof(devName));
    setDeviceName((const uint8_t*)devName, strlen(devName));
    bleGapImpl.instance = this;
    NRF_SDH_BLE_OBSERVER(bleGap, 1, processBleGapEvents, &bleGapImpl);
    return SYSTEM_ERROR_NONE;
}

int BleObject::BleGap::setDeviceName(const uint8_t* deviceName, uint16_t len) {
    if (deviceName == nullptr || len == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    ble_gap_conn_sec_mode_t secMode;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&secMode);
    int ret = sd_ble_gap_device_name_set(&secMode, deviceName, len);
    NRF_CHECK_RETURN(ret, "sd_ble_gap_device_name_set");
    return SYSTEM_ERROR_NONE;
}

int BleObject::BleGap::getDeviceName(uint8_t* deviceName, uint16_t* len) {
    if (deviceName == NULL || len == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    // non NULL-terminated string returned.
    int ret = sd_ble_gap_device_name_get(deviceName, len);
    NRF_CHECK_RETURN(ret, "sd_ble_gap_device_name_get");
    return SYSTEM_ERROR_NONE;
}

int BleObject::BleGap::setDeviceAddress(const hal_ble_addr_t* address) {
    if (bleObj_->broadcaster->advertising() ||
            bleObj_->observer->scanning() ||
            bleObj_->connectionsMgr->connecting()) {
        // The identity address cannot be changed while advertising, scanning or creating a connection.
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (address == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (address->addr_type != BLE_SIG_ADDR_TYPE_PUBLIC && address->addr_type != BLE_SIG_ADDR_TYPE_RANDOM_STATIC) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    ble_gap_addr_t localAddr;
    localAddr.addr_id_peer = false;
    localAddr.addr_type = address->addr_type;
    memcpy(localAddr.addr, address->addr, BLE_SIG_ADDR_LEN);
    int ret = sd_ble_gap_addr_set(&localAddr);
    NRF_CHECK_RETURN(ret, "sd_ble_gap_addr_set");
    return SYSTEM_ERROR_NONE;
}

int BleObject::BleGap::getDeviceAddress(hal_ble_addr_t* address) {
    if (address == NULL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    ble_gap_addr_t localAddr;
    int ret = sd_ble_gap_addr_get(&localAddr);
    NRF_CHECK_RETURN(ret, "sd_ble_gap_addr_get");
    address->addr_type = localAddr.addr_type;
    memcpy(address->addr, localAddr.addr, BLE_SIG_ADDR_LEN);
    return SYSTEM_ERROR_NONE;
}

int BleObject::BleGap::setAppearance(uint16_t appearance) {
    int ret = sd_ble_gap_appearance_set(appearance);
    NRF_CHECK_RETURN(ret, "sd_ble_gap_appearance_set");
    return SYSTEM_ERROR_NONE;
}

int BleObject::BleGap::getAppearance(uint16_t* appearance) {
    if (appearance == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    int ret = sd_ble_gap_appearance_get(appearance);
    NRF_CHECK_RETURN(ret, "sd_ble_gap_appearance_get");
    return SYSTEM_ERROR_NONE;
}

int BleObject::BleGap::addWhitelist(const hal_ble_addr_t* addrList, uint8_t len) {
    if (addrList == nullptr || len == 0 || len > BLE_MAX_WHITELIST_ADDR_COUNT) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    for (uint8_t i = 0; i < len; i++) {
        whitelist_[i].addr_id_peer = true;
        whitelist_[i].addr_type = addrList[i].addr_type;
        memcpy(whitelist_[i].addr, addrList[i].addr, BLE_SIG_ADDR_LEN);
        whitelistPointer_[i] = &whitelist_[i];
    }
    int ret = sd_ble_gap_whitelist_set(whitelistPointer_, len);
    NRF_CHECK_RETURN(ret, "sd_ble_gap_whitelist_set");
    return SYSTEM_ERROR_NONE;
}

int BleObject::BleGap::deleteWhitelist(void) {
    int ret = sd_ble_gap_whitelist_set(nullptr, 0);
    NRF_CHECK_RETURN(ret, "sd_ble_gap_whitelist_set");
    return SYSTEM_ERROR_NONE;
}

void BleObject::BleGap::processBleGapEvents(const ble_evt_t* event, void* context) {
    int ret;
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
            const ble_gap_data_length_params_t& gapDataLenParam = event->evt.gap_evt.params.data_length_update.effective_params;
            LOG_DEBUG(TRACE, "BLE GAP event: gap data length updated.");
            LOG_DEBUG(TRACE, "| txo    rxo     txt(us)     rxt(us) |.");
            LOG_DEBUG(TRACE, "  %d    %d     %d        %d",
                    gapDataLenParam.max_tx_octets, gapDataLenParam.max_rx_octets,
                    gapDataLenParam.max_tx_time_us, gapDataLenParam.max_rx_time_us);
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
            } else {
                LOG_DEBUG(WARN, "Authentication failed, status: %d", (int)event->evt.gap_evt.params.auth_status.auth_status);
            }
        } break;
        case BLE_GAP_EVT_TIMEOUT: {
            if (event->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_AUTH_PAYLOAD) {
                LOG_DEBUG(ERROR, "BLE GAP event: Authenticated payload timeout");
            }
        } break;
        default: {
            //LOG_DEBUG(TRACE, "Unhandled BLE event: %u", (unsigned)event->header.evt_id);
        } break;
    }
}

/* FIXME: It causes a section type conflict if using NRF_SDH_BLE_OBSERVER() in c++ class.*/
typedef struct {
    BleObject::Broadcaster* instance;
} BroadcasterImpl;
static BroadcasterImpl broadcasterImpl;

BleObject::Broadcaster::Broadcaster(BleObject* obj) : bleObj_(obj) {
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
    /* Default TX power. */
    txPower_ = 0;
}

int BleObject::Broadcaster::init(void) {
    int ret = configure(&advParams_);
    CHECK_RETURN(ret, nullptr);
    setTxPower(txPower_);
    broadcasterImpl.instance = this;
    NRF_SDH_BLE_OBSERVER(bleBroadcaster, 1, processBroadcasterEvents, &broadcasterImpl);
    return SYSTEM_ERROR_NONE;
}

bool BleObject::Broadcaster::advertising(void) const {
    return isAdvertising_;
}

int BleObject::Broadcaster::setAdvertisingParams(const hal_ble_adv_params_t* params) {
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

int BleObject::Broadcaster::getAdvertisingParams(hal_ble_adv_params_t* params) const {
    if (params == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    memcpy(params, &advParams_, sizeof(hal_ble_adv_params_t));
    return SYSTEM_ERROR_NONE;
}

int BleObject::Broadcaster::setAdvertisingData(const uint8_t* buf, size_t len) {
    // It is invalid to provide the same data buffers while advertising.
    int ret = suspend();
    CHECK_RETURN(ret, nullptr);
    if (buf != nullptr) {
        len = MIN(len, BLE_MAX_ADV_DATA_LEN);
        memcpy(advData_, buf, len);
    } else {
        // It should at least contain the Flags in advertising data.
        len = 3;
    }
    advDataLen_ = len;
    ret = configure(nullptr);
    CHECK_RETURN(ret, nullptr);
    return resume();
}

size_t BleObject::Broadcaster::getAdvertisingData(uint8_t* buf, size_t len) const {
    if (buf == nullptr) {
        return 0;
    }
    len = MIN(len, advDataLen_);
    memcpy(buf, advData_, len);
    return len;
}

int BleObject::Broadcaster::setScanResponseData(const uint8_t* buf, size_t len) {
    // It is invalid to provide the same data buffers while advertising.
    int ret = suspend();
    CHECK_RETURN(ret, nullptr);
    if (buf != nullptr) {
        len = MIN(len, BLE_MAX_ADV_DATA_LEN);
        memcpy(scanRespData_, buf, len);
    } else {
        len = 0;
    }
    scanRespDataLen_ = len;
    ret = configure(nullptr);
    CHECK_RETURN(ret, nullptr);
    return resume();
}

size_t BleObject::Broadcaster::getScanResponseData(uint8_t* buf, size_t len) const {
    if (buf == nullptr) {
        return 0;
    }
    len = MIN(len, scanRespDataLen_);
    memcpy(buf, scanRespData_, len);
    return len;
}

int BleObject::Broadcaster::setTxPower(int8_t val) {
    int ret = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, advHandle_, roundTxPower(val));
    NRF_CHECK_RETURN(ret, "sd_ble_gap_tx_power_set");
    return SYSTEM_ERROR_NONE;
}

int8_t BleObject::Broadcaster::getTxPower(void) const {
    return txPower_;
}

int BleObject::Broadcaster::startAdvertising(void) {
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

int BleObject::Broadcaster::stopAdvertising(void) {
    if (isAdvertising_) {
        int ret = sd_ble_gap_adv_stop(advHandle_);
        NRF_CHECK_RETURN(ret, "sd_ble_gap_adv_stop");
        isAdvertising_ = false;
    }
    return SYSTEM_ERROR_NONE;
}

int BleObject::Broadcaster::suspend(void) {
    if (isAdvertising_) {
        int ret = ble_gap_stop_advertising();
        CHECK_RETURN(ret, "ble_gap_stop_advertising");
        advPending_ = true;
    }
    return SYSTEM_ERROR_NONE;
}

int BleObject::Broadcaster::resume(void) {
    if (advPending_) {
        int ret = startAdvertising();
        CHECK_RETURN(ret, nullptr);
        advPending_ = false;
    }
    return SYSTEM_ERROR_NONE;
}

void BleObject::Broadcaster::toPlatformAdvParams(ble_gap_adv_params_t* params, const hal_ble_adv_params_t* halParams) const {
    memset(params, 0x00, sizeof(ble_gap_adv_params_t));
    params->properties.type = halParams->type;
    params->properties.include_tx_power = false; // FIXME: for extended advertising packet
    params->p_peer_addr = NULL;
    params->interval = halParams->interval;
    params->duration = halParams->timeout;
    params->filter_policy = halParams->filter_policy;
    params->primary_phy = BLE_GAP_PHY_1MBPS;
}

void BleObject::Broadcaster::toPlatformAdvData(ble_gap_adv_data_t* data) {
    data->adv_data.p_data = advData_;
    data->adv_data.len = advDataLen_;
    data->scan_rsp_data.p_data = scanRespData_;
    data->scan_rsp_data.len = scanRespDataLen_;
}

int BleObject::Broadcaster::configure(const hal_ble_adv_params_t* params) {
    int ret;
    ble_gap_adv_data_t bleGapAdvData;
    toPlatformAdvData(&bleGapAdvData);
    if (params == nullptr) {
        ret = sd_ble_gap_adv_set_configure(&advHandle_, &bleGapAdvData, NULL);
    } else {
        ble_gap_adv_params_t bleGapAdvParams;
        toPlatformAdvParams(&bleGapAdvParams, params);
        LOG_DEBUG(TRACE, "BLE advertising interval: %.3fms, timeout: %dms.",
                  bleGapAdvParams.interval*0.625, bleGapAdvParams.duration*10);
        ret = sd_ble_gap_adv_set_configure(&advHandle_, &bleGapAdvData, &bleGapAdvParams);
    }
    NRF_CHECK_RETURN(ret, "sd_ble_gap_adv_set_configure");
    return SYSTEM_ERROR_NONE;
}

int8_t BleObject::Broadcaster::roundTxPower(int8_t value) {
    uint8_t i = 0;
    for (i = 0; i < sizeof(validTxPower_); i++) {
        if (value == validTxPower_[i]) {
            return value;
        } else {
            if (value < validTxPower_[i]) {
                return validTxPower_[i];
            }
        }
    }
    return validTxPower_[i - 1];
}

void BleObject::Broadcaster::processBroadcasterEvents(const ble_evt_t* event, void* context) {
    Broadcaster* broadcaster = static_cast<BroadcasterImpl*>(context)->instance;
    switch (event->header.evt_id) {
        case BLE_GAP_EVT_ADV_SET_TERMINATED: {
            LOG_DEBUG(TRACE, "BLE GAP event: advertising stopped.");
            broadcaster->isAdvertising_ = false;
            BleObject::BleEventDispatcher::EventMessage msg;
            msg.evtType = BLE_EVT_ADV_STOPPED;
            msg.handler = (BleObject::BleEventDispatcher::BleEvtHandler)broadcaster->bleObj_->genericEvthandler_;
            msg.context = broadcaster->bleObj_->genericEvtContext_;
            hal_ble_gap_on_adv_stopped_evt_t* evt = (hal_ble_gap_on_adv_stopped_evt_t*)broadcaster->bleObj_->dispatcher->alloc(sizeof(hal_ble_gap_on_adv_stopped_evt_t));
            LOG_DEBUG(TRACE, "Event data address: %u, len: %u", (unsigned)evt, sizeof(hal_ble_gap_on_adv_stopped_evt_t));
            if (evt) {
                evt->version = 0x01;
                evt->reserved = NULL;
                msg.evtData = evt;
                broadcaster->bleObj_->dispatcher->enqueue(msg);
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
                broadcaster->connHandle_ = BLE_INVALID_CONN_HANDLE;
            }
        } break;
        default: break;
    }
}

const int8_t BleObject::Broadcaster::validTxPower_[8] = { -20, -16, -12, -8, -4, 0, 4, 8 };

typedef struct {
    BleObject::Observer* instance;
} ObserverImpl;
static ObserverImpl observerImpl;

BleObject::Observer::Observer(BleObject* obj) : bleObj_(obj) {
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
}

int BleObject::Observer::init(void) {
    observerImpl.instance = this;
    NRF_SDH_BLE_OBSERVER(bleObserver, 1, processObserverEvents, &observerImpl);
    return SYSTEM_ERROR_NONE;
}

bool BleObject::Observer::scanning(void) {
    return isScanning_;
}

int BleObject::Observer::setScanParams(const hal_ble_scan_params_t* params) {
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

int BleObject::Observer::getScanParams(hal_ble_scan_params_t* params) const {
    if (params == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    memcpy(params, &scanParams_, sizeof(hal_ble_scan_params_t));
    return SYSTEM_ERROR_NONE;
}

int BleObject::Observer::startScanning(bool restart) {
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

int BleObject::Observer::stopScanning(void) {
    if (!isScanning_) {
        return SYSTEM_ERROR_NONE;
    }
    int ret = sd_ble_gap_scan_stop();
    NRF_CHECK_RETURN(ret, "sd_ble_gap_scan_stop");
    os_semaphore_give(scanSemaphore_, false);
    isScanning_ = false;
    return SYSTEM_ERROR_NONE;
}

void BleObject::Observer::toPlatformScanParams(ble_gap_scan_params_t* params) const {
    memset(params, 0x00, sizeof(ble_gap_scan_params_t));
    params->extended = 0;
    params->active = scanParams_.active;
    params->interval = scanParams_.interval;
    params->window = scanParams_.window;
    params->timeout = scanParams_.timeout;
    params->scan_phys = BLE_GAP_PHY_1MBPS;
    params->filter_policy = scanParams_.filter_policy;
}

bool BleObject::Observer::chacheDevice(const hal_ble_addr_t& newAddr) {
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

void BleObject::Observer::processObserverEvents(const ble_evt_t* event, void* context) {
    Observer* observer = static_cast<ObserverImpl*>(context)->instance;
//        HalBleEvtMsg_t msg;
//        memset(&msg, 0x00, sizeof(HalBleEvtMsg_t));
    switch (event->header.evt_id) {
        case BLE_GAP_EVT_ADV_REPORT: {
            const ble_gap_evt_adv_report_t& advReport = event->evt.gap_evt.params.adv_report;
            LOG_DEBUG(TRACE, "BLE GAP event: advertising report.");
            hal_ble_addr_t newAddr;
            newAddr.addr_type = advReport.peer_addr.addr_type;
            memcpy(newAddr.addr, advReport.peer_addr.addr, BLE_SIG_ADDR_LEN);
            if (observer->chacheDevice(newAddr)) {
                // The new scanned device is not in the cached device list.
//                    msg.arg.type = BLE_EVT_SCAN_RESULT;
//                    msg.arg.params.scan_result.version = 0x01;
//                    msg.arg.params.scan_result.type.connectable = advReport.type.connectable;
//                    msg.arg.params.scan_result.type.scannable = advReport.type.scannable;
//                    msg.arg.params.scan_result.type.directed = advReport.type.directed;
//                    msg.arg.params.scan_result.type.scan_response = advReport.type.scan_response;
//                    msg.arg.params.scan_result.type.extended_pdu = advReport.type.extended_pdu;
//                    msg.arg.params.scan_result.rssi = advReport.rssi;
//                    msg.arg.params.scan_result.data_len = advReport.data.len;
//                    memcpy(msg.arg.params.scan_result.data, advReport.data.p_data, msg.arg.params.scan_result.data_len);
//                    msg.arg.params.scan_result.peer_addr.addr_type = newAddr.addr_type;
//                    memcpy(msg.arg.params.scan_result.peer_addr.addr, newAddr.addr, BLE_SIG_ADDR_LEN);
//                    if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
//                        LOG(ERROR, "os_queue_put() failed.");
//                    }
            }
            // Continue scanning, scanning parameters pointer must be set to NULL.
            observer->startScanning(true);
        } break;
        case BLE_GAP_EVT_TIMEOUT: {
            if (event->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_SCAN) {
                LOG_DEBUG(TRACE, "BLE GAP event: Scanning timeout");
                observer->isScanning_ = false;
//                    msg.arg.type = BLE_EVT_SCAN_STOPPED;
//                    msg.arg.params.scan_stopped.version = 0x01;
//                    msg.arg.params.scan_stopped.reserved = NULL;
//                    if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
//                        LOG(ERROR, "os_queue_put() failed.");
//                    }
                os_semaphore_give(observer->scanSemaphore_, false);
            }
        } break;
        default: break;
    }
}

typedef struct {
    BleObject::ConnectionsManager* instance;
} ConnectionsManagerImpl;
static ConnectionsManagerImpl connMgrImpl;

BleObject::ConnectionsManager::ConnectionsManager(BleObject* obj) : bleObj_(obj) {
    isConnecting_ = 0;
    memset(&connectingAddr_, 0x00, sizeof(hal_ble_addr_t));
    connParamsUpdateAttempts_ = 0;
    disconnectingHandle_ = BLE_INVALID_CONN_HANDLE;
    connParamUpdateHandle_ = BLE_INVALID_CONN_HANDLE;
    connectSemaphore_ = nullptr;
    disconnectSemaphore_ = nullptr;
    connParamsUpdateSemaphore_ = nullptr;
    connParamsUpdateTimer_ = nullptr;
}

int BleObject::ConnectionsManager::init(void) {
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
    if (os_timer_create(&connParamsUpdateTimer_, BLE_CONN_PARAMS_UPDATE_DELAY_MS, onConnParamsUpdateTimerExpired, this, true, nullptr)) {
        LOG(ERROR, "os_timer_create() failed.");
        return SYSTEM_ERROR_INTERNAL;
    }
    connMgrImpl.instance = this;
    NRF_SDH_BLE_OBSERVER(bleConnectionManager, 1, processConnectionEvents, &connMgrImpl);
    return SYSTEM_ERROR_NONE;
}

int BleObject::ConnectionsManager::setPPCP(const hal_ble_conn_params_t* ppcp) {
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

int BleObject::ConnectionsManager::getPPCP(hal_ble_conn_params_t* ppcp) const {
    if (ppcp == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    ble_gap_conn_params_t bleGapConnParams;
    int ret = sd_ble_gap_ppcp_get(&bleGapConnParams);
    NRF_CHECK_RETURN(ret, "sd_ble_gap_ppcp_get");
    toHalConnParams(ppcp, &bleGapConnParams);
    return SYSTEM_ERROR_NONE;
}

bool BleObject::ConnectionsManager::connecting(void) const {
    return isConnecting_;
}

bool BleObject::ConnectionsManager::connected(const hal_ble_addr_t* address) {
    if (address == nullptr) {
        return connections_.size() > 0;
    } else if (fetchConnection(address)) {
        return true;
    }
    return false;
}

int BleObject::ConnectionsManager::connect(const hal_ble_addr_t* address) {
    // Stop scanning first to give the scanning semaphore if possible.
    int ret = bleObj_->observer->stopScanning();
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
    bleObj_->observer->toPlatformScanParams(&bleGapScanParams);
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

int BleObject::ConnectionsManager::connectCancel(void) {
    if (isConnecting_) {
        int ret = sd_ble_gap_connect_cancel();
        NRF_CHECK_RETURN(ret, "sd_ble_gap_connect_cancel");
        isConnecting_ = false;
    }
    return SYSTEM_ERROR_NONE;
}

int BleObject::ConnectionsManager::disconnect(uint16_t connHandle) {
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

int BleObject::ConnectionsManager::updateConnectionParams(uint16_t connHandle, const hal_ble_conn_params_t* params) {
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

int BleObject::ConnectionsManager::getConnectionParams(uint16_t connHandle, hal_ble_conn_params_t* params) {
    BleConnection_t* connection = fetchConnection(connHandle);
    if (connection == nullptr) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    memcpy(params, &connection->effectiveConnParams, sizeof(hal_ble_conn_params_t));
    return SYSTEM_ERROR_NONE;
}

bool BleObject::ConnectionsManager::valid(uint16_t connHandle) const {
    if (connHandle == BLE_INVALID_CONN_HANDLE) {
        return false;
    }
    for (int i = 0; i < connections_.size(); i++) {
        const BleConnection_t& connection = connections_[i];
        if (connection.connHandle == connHandle) {
            return true;
        }
    }
    return false;
}

void BleObject::ConnectionsManager::toPlatformConnParams(ble_gap_conn_params_t* params, const hal_ble_conn_params_t* halConnParams) const {
    memset(params, 0x00, sizeof(ble_gap_conn_params_t));
    params->min_conn_interval = halConnParams->min_conn_interval;
    params->max_conn_interval = halConnParams->max_conn_interval;
    params->slave_latency = halConnParams->slave_latency;
    params->conn_sup_timeout = halConnParams->conn_sup_timeout;
}

void BleObject::ConnectionsManager::toHalConnParams(hal_ble_conn_params_t* halConnParams, const ble_gap_conn_params_t* params) const {
    memset(halConnParams, 0x00, sizeof(hal_ble_conn_params_t));
    halConnParams->min_conn_interval = params->min_conn_interval;
    halConnParams->max_conn_interval = params->max_conn_interval;
    halConnParams->slave_latency = params->slave_latency;
    halConnParams->conn_sup_timeout = params->conn_sup_timeout;
}

BleObject::ConnectionsManager::BleConnection_t* BleObject::ConnectionsManager::fetchConnection(uint16_t connHandle) {
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

BleObject::ConnectionsManager::BleConnection_t* BleObject::ConnectionsManager::fetchConnection(const hal_ble_addr_t* address) {
    if (address == nullptr) {
        return nullptr;
    }
    for (int i = 0; i < connections_.size(); i++) {
        BleConnection_t& connection = connections_.at(i);
        hal_ble_addr_t& peer = connection.peer;
        if (peer.addr_type == address->addr_type && !memcmp(peer.addr, address->addr, BLE_SIG_ADDR_LEN)) {
            return &connection;
        }
    }
    return nullptr;
}

void BleObject::ConnectionsManager::addConnection(const BleConnection_t& connection) {
    removeConnection(connection.connHandle);
    connections_.append(connection);
}

void BleObject::ConnectionsManager::removeConnection(uint16_t connHandle) {
    for (int i = 0; i < connections_.size(); i++) {
        const BleConnection_t& connection = connections_[i];
        if (connection.connHandle == connHandle) {
            connections_.removeAt(i);
        }
    }
}

void BleObject::ConnectionsManager::initiateConnParamsUpdateIfNeeded(const BleConnection_t* connection) {
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
        } else {
            int ret = sd_ble_gap_disconnect(connection->connHandle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
            NRF_CHECK_RETURN_VOID(ret, "sd_ble_gap_disconnect");
            LOG_DEBUG(TRACE, "Disconnecting. Update BLE connection parameters failed.");
        }
    }
}

bool BleObject::ConnectionsManager::isConnParamsFeeded(const hal_ble_conn_params_t* params) const {
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

void BleObject::ConnectionsManager::onConnParamsUpdateTimerExpired(os_timer_t timer) {
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

void BleObject::ConnectionsManager::processConnectionEvents(const ble_evt_t* event, void* context) {
    ConnectionsManager* connMgr = static_cast<ConnectionsManagerImpl*>(context)->instance;
    switch (event->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED: {
            const ble_gap_evt_connected_t& connected = event->evt.gap_evt.params.connected;
            LOG_DEBUG(TRACE, "BLE GAP event: connected.");
            BleConnection_t newConnection;
            newConnection.role = connected.role;
            newConnection.connHandle = event->evt.gap_evt.conn_handle;
            connMgr->toHalConnParams(&newConnection.effectiveConnParams, &connected.conn_params);
            newConnection.peer.addr_type = connected.peer_addr.addr_type;
            memcpy(newConnection.peer.addr, connected.peer_addr.addr, BLE_SIG_ADDR_LEN);
            connMgr->addConnection(newConnection);
            LOG_DEBUG(TRACE, "BLE role: %d, connection handle: %d", newConnection.role, newConnection.connHandle);
            LOG_DEBUG(TRACE, "| interval(ms)  latency  timeout(ms) |");
            LOG_DEBUG(TRACE, "  %.2f          %d       %d", newConnection.effectiveConnParams.max_conn_interval*1.25,
                    newConnection.effectiveConnParams.slave_latency, newConnection.effectiveConnParams.conn_sup_timeout*10);
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
            BleObject::BleEventDispatcher::EventMessage msg;
            msg.evtType = BLE_EVT_CONNECTED;
            msg.handler = (BleObject::BleEventDispatcher::BleEvtHandler)connMgr->bleObj_->genericEvthandler_;
            msg.context = connMgr->bleObj_->genericEvtContext_;
            hal_ble_gap_on_connected_evt_t* evt = (hal_ble_gap_on_connected_evt_t*)connMgr->bleObj_->dispatcher->alloc(sizeof(hal_ble_gap_on_connected_evt_t));
            LOG_DEBUG(TRACE, "Event data address: %u, len: %u", (unsigned)evt, sizeof(hal_ble_gap_on_connected_evt_t));
            if (evt) {
                evt->version = 0x01;
                evt->role = newConnection.role;
                evt->conn_handle = newConnection.connHandle;
                evt->conn_interval = newConnection.effectiveConnParams.max_conn_interval;
                evt->slave_latency = newConnection.effectiveConnParams.slave_latency;
                evt->conn_sup_timeout = newConnection.effectiveConnParams.conn_sup_timeout;
                evt->peer_addr.addr_type = newConnection.peer.addr_type;
                memcpy(evt->peer_addr.addr, newConnection.peer.addr, BLE_SIG_ADDR_LEN);
                msg.evtData = evt;
                connMgr->bleObj_->dispatcher->enqueue(msg);
            }
        } break;
        case BLE_GAP_EVT_DISCONNECTED: {
            const ble_gap_evt_disconnected_t& disconnected = event->evt.gap_evt.params.disconnected;
            LOG_DEBUG(TRACE, "BLE GAP event: disconnected, handle: 0x%04X, reason: %d", event->evt.gap_evt.conn_handle, disconnected.reason);
            BleConnection_t* connection = connMgr->fetchConnection(event->evt.gap_evt.conn_handle);
            if (connection != nullptr) {
                // Cancel the on-going connection parameters update procedure.
                if (connMgr->connParamUpdateHandle_ == event->evt.gap_evt.conn_handle) {
                    if (connection->role == BLE_ROLE_PERIPHERAL) {
                        connMgr->connParamUpdateHandle_ = BLE_INVALID_CONN_HANDLE;
                        if (!os_timer_is_active(connMgr->connParamsUpdateTimer_, NULL)) {
                            os_timer_change(connMgr->connParamsUpdateTimer_, OS_TIMER_CHANGE_STOP, true, 0, 0, NULL);
                        }
                    } else {
                        os_semaphore_give(connMgr->connParamsUpdateSemaphore_, false);
                    }
                }
                if (connMgr->disconnectingHandle_ == event->evt.gap_evt.conn_handle) {
                    os_semaphore_give(connMgr->disconnectSemaphore_, false);
                }
                connMgr->removeConnection(event->evt.gap_evt.conn_handle);
            } else {
                LOG(ERROR, "Connection not found.");
                return;
            }
            BleObject::BleEventDispatcher::EventMessage msg;
            msg.evtType = BLE_EVT_DISCONNECTED;
            msg.handler = (BleObject::BleEventDispatcher::BleEvtHandler)connMgr->bleObj_->genericEvthandler_;
            msg.context = connMgr->bleObj_->genericEvtContext_;
            hal_ble_gap_on_disconnected_evt_t* evt = (hal_ble_gap_on_disconnected_evt_t*)connMgr->bleObj_->dispatcher->alloc(sizeof(hal_ble_gap_on_disconnected_evt_t));
            LOG_DEBUG(TRACE, "Event data address: %u, len: %u", (unsigned)evt, sizeof(hal_ble_gap_on_disconnected_evt_t));
            if (evt) {
                evt->version = 0x01;
                evt->reason = disconnected.reason;
                evt->conn_handle = event->evt.gap_evt.conn_handle;
                msg.evtData = evt;
                connMgr->bleObj_->dispatcher->enqueue(msg);
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
            const ble_gap_evt_conn_param_update_t& connParaUpdate = event->evt.gap_evt.params.conn_param_update;
            LOG_DEBUG(TRACE, "BLE GAP event: connection parameter updated.");
            BleConnection_t* connection = connMgr->fetchConnection(event->evt.gap_evt.conn_handle);
            if (connection != nullptr) {
                connMgr->toHalConnParams(&connection->effectiveConnParams, &connParaUpdate.conn_params);
                if (connection->role == BLE_ROLE_PERIPHERAL) {
                    connMgr->initiateConnParamsUpdateIfNeeded(connection);
                } else {
                    os_semaphore_give(connMgr->connParamsUpdateSemaphore_, false);
                }
            } else {
                LOG(ERROR, "Connection not found.");
                return;
            }
            LOG_DEBUG(TRACE, "| interval(ms)  latency  timeout(ms) |");
            LOG_DEBUG(TRACE, "  %.2f          %d       %d", connection->effectiveConnParams.max_conn_interval*1.25,
                    connection->effectiveConnParams.slave_latency, connection->effectiveConnParams.conn_sup_timeout*10);
            BleObject::BleEventDispatcher::EventMessage msg;
            msg.evtType = BLE_EVT_CONN_PARAMS_UPDATED;
            msg.handler = (BleObject::BleEventDispatcher::BleEvtHandler)connMgr->bleObj_->genericEvthandler_;
            msg.context = connMgr->bleObj_->genericEvtContext_;
            hal_ble_gap_on_conn_params_evt_t* evt = (hal_ble_gap_on_conn_params_evt_t*)connMgr->bleObj_->dispatcher->alloc(sizeof(hal_ble_gap_on_conn_params_evt_t));
            LOG_DEBUG(TRACE, "Event data address: %u, len: %u", (unsigned)evt, sizeof(hal_ble_gap_on_conn_params_evt_t));
            if (evt) {
                evt->version = 0x01;
                evt->conn_handle = event->evt.gap_evt.conn_handle;
                evt->conn_interval = connParaUpdate.conn_params.max_conn_interval;
                evt->slave_latency = connParaUpdate.conn_params.slave_latency;
                evt->conn_sup_timeout = connParaUpdate.conn_params.conn_sup_timeout;
                msg.evtData = evt;
                connMgr->bleObj_->dispatcher->enqueue(msg);
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

typedef struct {
    BleObject::GattServer* instance;
} GattServerImpl;
static GattServerImpl gattsImpl;

BleObject::GattServer::GattServer(BleObject* obj) : bleObj_(obj) {
    hvxSemaphore_ = nullptr;
}

int BleObject::GattServer::init(void) {
    // Allocate additional 512 bytes, since each block will occupy several bytes(12).
    pool_.init(BLE_MAX_CHAR_COUNT * BLE_MAX_ATTR_VALUE_PACKET_SIZE + 512);
    gattsImpl.instance = this;
    NRF_SDH_BLE_OBSERVER(bleGattServer, 1, processGattServerEvents, &gattsImpl);
    return SYSTEM_ERROR_NONE;
}

int BleObject::GattServer::addService(uint8_t type, const hal_ble_uuid_t* uuid, uint16_t* handle) {
    ble_uuid_t svcUuid;
    if (uuid == nullptr || handle == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    int ret = toPlatformUUID(uuid, &svcUuid);
    CHECK_RETURN(ret, nullptr);
    ret = sd_ble_gatts_service_add(type, &svcUuid, handle);
    NRF_CHECK_RETURN(ret, "sd_ble_gatts_service_add");
    services_.append(*handle);
    LOG_DEBUG(TRACE, "Service handle: %d.", *handle);
    return SYSTEM_ERROR_NONE;
}

int BleObject::GattServer::addCharacteristic(uint16_t svcHandle, uint8_t properties, const hal_ble_uuid_t* uuid, const char* desc, hal_ble_char_handles_t* handles) {
    if (handles == nullptr || uuid == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (characteristics_.size() >= BLE_MAX_CHAR_COUNT) {
        return SYSTEM_ERROR_LIMIT_EXCEEDED;
    }
    if (findService(svcHandle)) {
        ble_uuid_t charUuid;
        ble_gatts_char_md_t charMd;
        ble_gatts_attr_md_t valueAttrMd;
        ble_gatts_attr_md_t userDescAttrMd;
        ble_gatts_attr_md_t cccdAttrMd;
        ble_gatts_attr_t charValueAttr;
        ble_gatts_char_handles_t charHandles;
        int ret = toPlatformUUID(uuid, &charUuid);
        CHECK_RETURN(ret, nullptr);
        memset(&charMd, 0, sizeof(charMd));
        toPlatformCharProps(properties, &charMd.char_props);
        // User Description Descriptor attribute metadata
        if (desc != nullptr) {
            memset(&userDescAttrMd, 0, sizeof(userDescAttrMd));
            BLE_GAP_CONN_SEC_MODE_SET_OPEN(&userDescAttrMd.read_perm);
            BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&userDescAttrMd.write_perm);
            userDescAttrMd.vloc = BLE_GATTS_VLOC_STACK;
            userDescAttrMd.rd_auth = 0;
            userDescAttrMd.wr_auth = 0;
            userDescAttrMd.vlen = 0;
            charMd.p_char_user_desc = (const uint8_t *)desc;
            charMd.char_user_desc_max_size = strlen(desc);
            charMd.char_user_desc_size = strlen(desc);
            charMd.p_user_desc_md = &userDescAttrMd;
        } else {
            charMd.p_char_user_desc = nullptr;
            charMd.p_user_desc_md = nullptr;
        }
        // Client Characteristic Configuration Descriptor attribute metadata
        if (charMd.char_props.notify || charMd.char_props.indicate) {
            memset(&cccdAttrMd, 0, sizeof(cccdAttrMd));
            BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccdAttrMd.read_perm);
            BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccdAttrMd.write_perm);
            cccdAttrMd.vloc = BLE_GATTS_VLOC_STACK;
            charMd.p_cccd_md = &cccdAttrMd;
        }
        // TODO:
        charMd.p_char_pf = nullptr;
        charMd.p_sccd_md = nullptr;
        // Characteristic value attribute metadata
        memset(&valueAttrMd, 0, sizeof(valueAttrMd));
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&valueAttrMd.read_perm);
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&valueAttrMd.write_perm);
        valueAttrMd.vloc = BLE_GATTS_VLOC_USER;
        valueAttrMd.rd_auth = 0;
        valueAttrMd.wr_auth = 0;
        valueAttrMd.vlen = 1;
        // Characteristic value attribute
        uint8_t* charValue = (uint8_t*)pool_.alloc(BLE_MAX_ATTR_VALUE_PACKET_SIZE);
        if (charValue == nullptr) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        memset(&charValueAttr, 0, sizeof(charValueAttr));
        charValueAttr.p_uuid = &charUuid;
        charValueAttr.p_attr_md = &valueAttrMd;
        charValueAttr.init_len = 0;
        charValueAttr.init_offs = 0;
        charValueAttr.max_len = BLE_MAX_ATTR_VALUE_PACKET_SIZE;
        charValueAttr.p_value = charValue;
        ret = sd_ble_gatts_characteristic_add(svcHandle, &charMd, &charValueAttr, &charHandles);
        if (ret != NRF_SUCCESS) {
            pool_.free(charValue);
            NRF_CHECK_RETURN(ret, "sd_ble_gatts_characteristic_add");
        }
        BleCharacteristic characteristic;
        characteristic.properties = properties;
        characteristic.svcHandle = svcHandle;
        characteristic.handles.decl_handle = charHandles.value_handle - 1;
        characteristic.handles.value_handle = charHandles.value_handle;
        characteristic.handles.user_desc_handle = charHandles.user_desc_handle;
        characteristic.handles.cccd_handle = charHandles.cccd_handle;
        characteristic.handles.sccd_handle = charHandles.sccd_handle;
        characteristics_.append(characteristic);
        *handles = characteristic.handles;
        LOG_DEBUG(TRACE, "Characteristic value handle: %d.", charHandles.value_handle);
        LOG_DEBUG(TRACE, "Characteristic cccd handle: %d.", charHandles.cccd_handle);
        return SYSTEM_ERROR_NONE;
    }
    else {
        return SYSTEM_ERROR_NOT_FOUND;
    }
}

int BleObject::GattServer::addDescriptor(uint16_t charHandle, const hal_ble_uuid_t* uuid, uint8_t* descriptor, uint16_t len, uint16_t* handle) {
    if (uuid == nullptr || descriptor== nullptr || handle == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (findCharacteristic(charHandle)) {
        ble_gatts_attr_t descAttr;
        ble_gatts_attr_md_t descAttrMd;
        ble_uuid_t descUuid;
        int ret = toPlatformUUID(uuid, &descUuid);
        CHECK_RETURN(ret, nullptr);
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
        // FIXME: validate the descriptor to be added
        ret = sd_ble_gatts_descriptor_add(charHandle, &descAttr, handle);
        // TODO: assigne the handle to corresponding characteristic.
        NRF_CHECK_RETURN(ret, "sd_ble_gatts_descriptor_add");
        return SYSTEM_ERROR_NONE;
    }
    else {
        return SYSTEM_ERROR_NOT_FOUND;
    }
}

size_t BleObject::GattServer::setValue(uint16_t handle, const uint8_t* buf, size_t len) {
    if (handle == BLE_INVALID_ATTR_HANDLE || buf == NULL) {
        return 0;
    }
    BleCharacteristic* characteristic = findCharacteristic(handle);
    if (characteristic != nullptr && handle == characteristic->handles.value_handle) {
        len = MIN(len, BLE_MAX_ATTR_VALUE_PACKET_SIZE);
        ble_gatts_value_t gattValue;
        gattValue.len = len;
        gattValue.offset = 0;
        gattValue.p_value = (uint8_t*)buf;
        ret_code_t ret = sd_ble_gatts_value_set(BLE_CONN_HANDLE_INVALID, handle, &gattValue);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "sd_ble_gatts_value_set() failed: %u", (unsigned)ret);
            return 0;
        }
        // Notify or indicate the value if possible.
        if ((characteristic->properties & BLE_SIG_CHAR_PROP_NOTIFY) || (characteristic->properties & BLE_SIG_CHAR_PROP_INDICATE)) {
            for (int i = 0; i < characteristic->cccdConnections.size(); i++) {
                if (os_semaphore_create(&hvxSemaphore_, 1, 0)) {
                    LOG(ERROR, "os_semaphore_create() failed");
                    break;
                }
                ble_gatts_hvx_params_t hvxParams;
                uint16_t hvxLen = len;
                if (characteristic->properties & BLE_SIG_CHAR_PROP_NOTIFY) {
                    hvxParams.type = BLE_GATT_HVX_NOTIFICATION;
                } else if (characteristic->properties & BLE_SIG_CHAR_PROP_INDICATE) {
                    hvxParams.type = BLE_GATT_HVX_INDICATION;
                }
                hvxParams.handle = handle;
                hvxParams.offset = 0;
                hvxParams.p_data = buf;
                hvxParams.p_len = &hvxLen;
                ret_code_t ret = sd_ble_gatts_hvx(characteristic->cccdConnections[i], &hvxParams);
                if (ret != NRF_SUCCESS) {
                    LOG(ERROR, "sd_ble_gatts_hvx() failed: %u", (unsigned)ret);
                    os_semaphore_destroy(hvxSemaphore_);
                    break;
                }
                if (os_semaphore_take(hvxSemaphore_, BLE_HVX_PROCEDURE_TIMEOUT_MS, false)) {
                    os_semaphore_destroy(hvxSemaphore_);
                    break;
                }
                os_semaphore_destroy(hvxSemaphore_);
            }
        }
        return len;
    }
    else {
        return 0;
    }
}

size_t BleObject::GattServer::getValue(uint16_t handle, uint8_t* buf, size_t len) {
    if (handle == BLE_INVALID_ATTR_HANDLE || buf == NULL) {
        return 0;
    }
    BleCharacteristic* characteristic = findCharacteristic(handle);
    if (characteristic != nullptr && handle == characteristic->handles.value_handle) {
        len = MIN(len, BLE_MAX_ATTR_VALUE_PACKET_SIZE);
        ble_gatts_value_t gattValue;
        gattValue.len = len;
        gattValue.offset = 0;
        gattValue.p_value = buf;
        int ret = sd_ble_gatts_value_get(BLE_CONN_HANDLE_INVALID, handle, &gattValue);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "sd_ble_gatts_value_get() failed: %u", (unsigned)ret);
            return 0;
        }
        return gattValue.len;
    } else {
        return 0;
    }
}

bool BleObject::GattServer::findService(uint16_t handle) const {
    for (int i = 0; i < services_.size(); i++) {
        const uint16_t& svcHandle = services_[i];
        if (svcHandle == handle) {
            return true;
        }
    }
    return false;
}

BleObject::GattServer::BleCharacteristic* BleObject::GattServer::findCharacteristic(uint16_t handle) {
    for (int i = 0; i < characteristics_.size(); i++) {
        BleCharacteristic& characteristic = characteristics_[i];
        if (characteristic.handles.value_handle == handle ||
                characteristic.handles.decl_handle == handle ||
                characteristic.handles.user_desc_handle == handle ||
                characteristic.handles.cccd_handle == handle ||
                characteristic.handles.sccd_handle == handle) {
            return &characteristic;
        }
    }
    return nullptr;
}

void BleObject::GattServer::addToCCCDList(BleCharacteristic* characteristic, uint16_t connHandle) {
    characteristic->cccdConnections.append(connHandle);
}

void BleObject::GattServer::removeFromCCCDList(BleCharacteristic* characteristic, uint16_t connHandle) {
    for (int i = 0; i < characteristic->cccdConnections.size(); i++) {
        uint16_t& handle = characteristic->cccdConnections[i];
        if (handle == connHandle) {
            characteristic->cccdConnections.removeAt(i);
        }
    }
}

void BleObject::GattServer::removeFromAllCCCDList(uint16_t connHandle) {
    for (int i = 0; i < characteristics_.size(); i++) {
        BleCharacteristic& characteristic = characteristics_[i];
        removeFromCCCDList(&characteristic, connHandle);
    }
}

void BleObject::GattServer::processGattServerEvents(const ble_evt_t* event, void* context) {
    GattServer* gatts = static_cast<GattServerImpl*>(context)->instance;
//        HalBleEvtMsg_t msg;
//        memset(&msg, 0x00, sizeof(HalBleEvtMsg_t));
    switch (event->header.evt_id) {
        case BLE_GAP_EVT_DISCONNECTED: {
            gatts->removeFromAllCCCDList(event->evt.gap_evt.conn_handle);
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
            uint16_t effectMtu = MIN(clientMtu, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
            int ret = sd_ble_gatts_exchange_mtu_reply(event->evt.gatts_evt.conn_handle, effectMtu);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gatts_exchange_mtu_reply() failed: %u", (unsigned)ret);
            }
            LOG_DEBUG(TRACE, "Reply with Server RX MTU: %d.", effectMtu);
//                msg.arg.type = BLE_EVT_GATT_PARAMS_UPDATED;
//                msg.arg.params.gatt_params_updated.version = 0x01;
//                msg.arg.params.gatt_params_updated.conn_handle = event->evt.gatts_evt.conn_handle;
//                msg.arg.params.gatt_params_updated.att_mtu_size = effectMtu;
//                if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
//                    LOG(ERROR, "os_queue_put() failed.");
//                }
        } break;
        case BLE_GATTS_EVT_WRITE: {
            const ble_gatts_evt_write_t& write = event->evt.gatts_evt.params.write;
            LOG_DEBUG(TRACE, "BLE GATT Server event: write characteristic, handle: %d, len: %d.", write.handle, write.len);
            BleCharacteristic* characteristic = gatts->findCharacteristic(write.handle);
            if (characteristic != nullptr) {
                if (characteristic->handles.value_handle == event->evt.gatts_evt.params.write.handle) {
                    // TODO: deal with different write commands.
//                        msg.arg.type = BLE_EVT_DATA_WRITTEN;
//                        msg.arg.params.data_rec.version = 0x01;
//                        msg.arg.params.data_rec.conn_handle = event->evt.gatts_evt.conn_handle;
//                        msg.arg.params.data_rec.attr_handle = write.handle;
//                        msg.arg.params.data_rec.offset = write.offset;
//                        msg.arg.params.data_rec.data_len = write.len;
//                        memcpy(msg.arg.params.data_rec.data, write.data, msg.arg.params.data_rec.data_len);
//                        if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
//                            LOG(ERROR, "os_queue_put() failed.");
//                        }
                } else if (characteristic->handles.cccd_handle == write.handle) {
                    if (event->evt.gatts_evt.params.write.data[0] > 0) {
                        gatts->addToCCCDList(characteristic, event->evt.gatts_evt.conn_handle);
                    } else {
                        gatts->removeFromCCCDList(characteristic, event->evt.gatts_evt.conn_handle);
                    }
                }
            }
        } break;
        case BLE_GATTS_EVT_HVN_TX_COMPLETE: {
            LOG_DEBUG(TRACE, "BLE GATT Server event: notification sent.");
            os_semaphore_give(gatts->hvxSemaphore_, false);
        } break;
        case BLE_GATTS_EVT_HVC: {
            LOG_DEBUG(TRACE, "BLE GATT Server event: indication confirmed.");
            os_semaphore_give(gatts->hvxSemaphore_, false);
        } break;
        case BLE_GATTS_EVT_TIMEOUT: {
            LOG_DEBUG(TRACE, "BLE GATT Server event: timeout, source: %d.", event->evt.gatts_evt.params.timeout.src);
            // Disconnect on GATT Server timeout event.
            int ret = sd_ble_gap_disconnect(event->evt.gatts_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gap_disconnect() failed: %u", (unsigned)ret);
            }
        } break;
        default: break;
    }
}

typedef struct {
    BleObject::GattClient* instance;
} GattClientImpl;
static GattClientImpl gattcImpl;

BleObject::GattClient::GattClient(BleObject* obj) : bleObj_(obj) {
    readAttrHandle_ = BLE_INVALID_ATTR_HANDLE;
    readBuf_ = nullptr;
    readLen_ = 0;
    readWriteSemaphore_ = nullptr;
    resetDiscoveryState();
}

int BleObject::GattClient::init(void) {
    gattcPool_.init(GATT_CLIENT_POOL_SIZE); // Just a guess
    gattcImpl.instance = this;
    NRF_SDH_BLE_OBSERVER(bleGattClient, 1, processGattClientEvents, &gattcImpl);
    return SYSTEM_ERROR_NONE;
}

bool BleObject::GattClient::discovering(void) const {
    return isDiscovering_;
}

int BleObject::GattClient::discoverServices(uint16_t connHandle, const hal_ble_uuid_t* uuid) {
    int ret = bleObj_->connectionsMgr->valid(connHandle);
    CHECK_RETURN(ret, nullptr);
    if (isDiscovering_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (os_semaphore_create(&discoverySemaphore_, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }
    currDiscConnHandle_ = connHandle;
    currDiscProcedure_ = BLE_DISCOVERY_PROCEDURE_SERVICES;
    if (uuid == nullptr) {
        discoverAll_ = true;
        ret = sd_ble_gattc_primary_services_discover(connHandle, SERVICES_BASE_START_HANDLE, nullptr);
    } else {
        ble_uuid_t svcUUID;
        toPlatformUUID(uuid, &svcUUID);
        ret = sd_ble_gattc_primary_services_discover(connHandle, SERVICES_BASE_START_HANDLE, &svcUUID);
    }
    if (ret != NRF_SUCCESS) {
        resetDiscoveryState();
        os_semaphore_destroy(discoverySemaphore_);
        NRF_CHECK_RETURN(ret, "sd_ble_gattc_primary_services_discover");
    }
    isDiscovering_ = true;
    ret = SYSTEM_ERROR_NONE;
    if (os_semaphore_take(discoverySemaphore_, BLE_DICOVERY_PROCEDURE_TIMEOUT_MS, false)) {
        ret = SYSTEM_ERROR_TIMEOUT;
    }
    os_semaphore_destroy(discoverySemaphore_);
    hal_ble_svc_t* services = (hal_ble_svc_t*)gattcPool_.alloc(discoveredServices_.size() * sizeof(hal_ble_svc_t));
    if (services) {
//        HalBleEvtMsg_t msg;
//        memset(&msg, 0x00, sizeof(HalBleEvtMsg_t));
//        msg.arg.type = BLE_EVT_SVC_DISCOVERED;
//        msg.arg.params.svc_disc.version = 0x01;
//        msg.arg.params.svc_disc.conn_handle = currDiscConnHandle_;
//        msg.arg.params.svc_disc.count = discoveredServices_.size();
//        for (int i = 0; i < discoveredServices_.size(); i++) {
//            const hal_ble_svc_t& service = discoveredServices_[i];
//            memcpy(services + i, &service, sizeof(hal_ble_svc_t));
//        }
//        msg.arg.params.svc_disc.services = services;
//        // TODO: Free the memory after popping the event.
//        if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
//            LOG(ERROR, "os_queue_put() failed.");
//            gattcPool_.free(services);
//        }
    }
    resetDiscoveryState();
    return ret;
}

int BleObject::GattClient::discoverCharacteristics(uint16_t connHandle, const hal_ble_svc_t* service) {
    int ret = bleObj_->connectionsMgr->valid(connHandle);
    CHECK_RETURN(ret, nullptr);
    if (isDiscovering_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (os_semaphore_create(&discoverySemaphore_, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }
    ble_gattc_handle_range_t handleRange;
    handleRange.start_handle = service->start_handle;
    handleRange.end_handle = service->end_handle;
    currDiscConnHandle_ = connHandle;
    currDiscSvc_ = *service;
    currDiscProcedure_ = BLE_DISCOVERY_PROCEDURE_CHARACTERISTICS;
    if (sd_ble_gattc_characteristics_discover(connHandle, &handleRange) != NRF_SUCCESS) {
        resetDiscoveryState();
        os_semaphore_destroy(discoverySemaphore_);
        NRF_CHECK_RETURN(ret, "sd_ble_gattc_characteristics_discover");
    }
    isDiscovering_ = true;
    ret = SYSTEM_ERROR_NONE;
    os_semaphore_take(discoverySemaphore_, BLE_DICOVERY_PROCEDURE_TIMEOUT_MS, false);
    // Now discover characteristic descriptors
    currDiscProcedure_ = BLE_DISCOVERY_PROCEDURE_DESCRIPTORS;
    if (sd_ble_gattc_descriptors_discover(currDiscConnHandle_, &handleRange) != NRF_SUCCESS) {
        os_semaphore_destroy(discoverySemaphore_);
        LOG(ERROR, "sd_ble_gattc_descriptors_discover() failed: %u", (unsigned)ret);
    } else {
        ret = SYSTEM_ERROR_NONE;
        if (os_semaphore_take(discoverySemaphore_, BLE_DICOVERY_PROCEDURE_TIMEOUT_MS, false)) {
            ret = SYSTEM_ERROR_TIMEOUT;
        }
    }
    os_semaphore_destroy(discoverySemaphore_);
    hal_ble_char_t* characteristics = (hal_ble_char_t*)gattcPool_.alloc(discoveredCharacteristics_.size() * sizeof(hal_ble_char_t));
    if (characteristics) {
//        HalBleEvtMsg_t msg;
//        memset(&msg, 0x00, sizeof(HalBleEvtMsg_t));
//        msg.arg.type = BLE_EVT_CHAR_DISCOVERED;
//        msg.arg.params.char_disc.version = 0x01;
//        msg.arg.params.char_disc.conn_handle = currDiscConnHandle_;
//        msg.arg.params.char_disc.count = discoveredCharacteristics_.size();
//        for (int i = 0; i < discoveredCharacteristics_.size(); i++) {
//            const hal_ble_char_t& characteristic = discoveredCharacteristics_[i];
//            memcpy(characteristics + i, &characteristic, sizeof(hal_ble_char_t));
//        }
//        msg.arg.params.char_disc.characteristics = characteristics;
//        // TODO: Free the memory after popping the event.
//        if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
//            LOG(ERROR, "os_queue_put() failed.");
//        }
    }
    resetDiscoveryState();
    return ret;
}

size_t BleObject::GattClient::writeAttribute(uint16_t connHandle, uint16_t attrHandle, const uint8_t* buf, size_t len, bool response) {
    if (buf == nullptr) {
        return 0;
    }
    int ret = bleObj_->connectionsMgr->valid(connHandle);
    CHECK_RETURN(ret, nullptr);
    ble_gattc_write_params_t writeParams;
    if (os_semaphore_create(&readWriteSemaphore_, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }
    if (response) {
        writeParams.write_op = BLE_GATT_OP_WRITE_REQ;
    } else {
        writeParams.write_op = BLE_GATT_OP_WRITE_CMD;
    }
    len = MIN(len, BLE_MAX_ATTR_VALUE_PACKET_SIZE);
    writeParams.flags = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE;
    writeParams.handle = attrHandle;
    writeParams.offset = 0;
    writeParams.len = len;
    writeParams.p_value = buf;
    if (sd_ble_gattc_write(connHandle, &writeParams) != NRF_SUCCESS) {
        os_semaphore_destroy(readWriteSemaphore_);
        LOG(ERROR, "sd_ble_gattc_write() failed: %u", (unsigned)ret);
        return 0;
    }
    if (os_semaphore_take(readWriteSemaphore_, BLE_READ_WRITE_PROCEDURE_TIMEOUT_MS, false)) {
        len = 0;
    }
    os_semaphore_destroy(readWriteSemaphore_);
    return len;
}

// FIXME: Multi-link read
size_t BleObject::GattClient::readAttribute(uint16_t connHandle, uint16_t attrHandle, uint8_t* buf, size_t len) {
    if (buf == nullptr) {
        return 0;
    }
    int ret = bleObj_->connectionsMgr->valid(connHandle);
    CHECK_RETURN(ret, nullptr);
    if (os_semaphore_create(&readWriteSemaphore_, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }
    readAttrHandle_ = attrHandle;
    readBuf_ = buf;
    readLen_ = MIN(len, BLE_MAX_ATTR_VALUE_PACKET_SIZE);
    ret = sd_ble_gattc_read(connHandle, attrHandle, 0);
    if (ret != NRF_SUCCESS) {
        readBuf_ = nullptr;
        readAttrHandle_ = BLE_INVALID_ATTR_HANDLE;
        LOG(ERROR, "sd_ble_gattc_read() failed: %u", (unsigned)ret);
        os_semaphore_destroy(readWriteSemaphore_);
        return 0;
    }
    if (os_semaphore_take(readWriteSemaphore_, BLE_READ_WRITE_PROCEDURE_TIMEOUT_MS, false)) {
        return 0;
    }
    os_semaphore_destroy(readWriteSemaphore_);
    return readLen_;
}

void BleObject::GattClient::resetDiscoveryState(void) {
    discoverAll_ = false;
    isDiscovering_ = false;
    currDiscConnHandle_ = BLE_INVALID_CONN_HANDLE;
    currDiscProcedure_ = BLE_DISCOVERY_PROCEDURE_IDLE;
    discoveredServices_.clear();
    discoveredCharacteristics_.clear();
    discoverySemaphore_ = nullptr;
}

bool BleObject::GattClient::readServiceUUID128IfNeeded(void) {
    for (int i = 0;  i < discoveredServices_.size(); i++) {
        hal_ble_svc_t& service = discoveredServices_[i];
        if (service.uuid.type == BLE_UUID_TYPE_128BIT_SHORTED) {
            return (sd_ble_gattc_read(currDiscConnHandle_, service.start_handle, 0) == NRF_SUCCESS);
        }
    }
    return false;
}

bool BleObject::GattClient::readCharacteristicUUID128IfNeeded(void) {
    for (int i = 0;  i < discoveredCharacteristics_.size(); i++) {
        hal_ble_char_t& characteristic = discoveredCharacteristics_[i];
        if (characteristic.uuid.type == BLE_UUID_TYPE_128BIT_SHORTED) {
            return (sd_ble_gattc_read(currDiscConnHandle_, characteristic.decl_handle, 0) == NRF_SUCCESS);
        }
    }
    return false;
}

hal_ble_svc_t* BleObject::GattClient::findDiscoveredService(uint16_t handle) {
    for (int i = 0;  i < discoveredServices_.size(); i++) {
        hal_ble_svc_t& service = discoveredServices_[i];
        if (service.start_handle <= handle && handle <= service.end_handle) {
            return &service;
        }
    }
    return nullptr;
}

hal_ble_char_t* BleObject::GattClient::findDiscoveredCharacteristic(uint16_t handle) {
    hal_ble_char_t* foundChar = nullptr;
    for (int i = 0;  i < discoveredCharacteristics_.size(); i++) {
        hal_ble_char_t& characteristic = discoveredCharacteristics_[i];
        if (characteristic.decl_handle <= handle) {
            foundChar = &characteristic;
        }
    }
    return foundChar;
}

void BleObject::GattClient::processGattClientEvents(const ble_evt_t* event, void* context) {
    GattClient* gattc = static_cast<GattClientImpl*>(context)->instance;
//        HalBleEvtMsg_t msg;
//        memset(&msg, 0x00, sizeof(HalBleEvtMsg_t));
    switch (event->header.evt_id) {
        case BLE_GAP_EVT_DISCONNECTED: {
            if (gattc->isDiscovering_ && event->evt.gap_evt.conn_handle == gattc->currDiscConnHandle_) {
                os_semaphore_destroy(gattc->discoverySemaphore_);
                gattc->resetDiscoveryState();
            }
        } break;
        case BLE_GATTC_EVT_EXCHANGE_MTU_RSP: {
            LOG_DEBUG(TRACE, "BLE GATT Client event: exchange ATT MTU response.");
            uint16_t serverMtu = event->evt.gattc_evt.params.exchange_mtu_rsp.server_rx_mtu;
            LOG_DEBUG(TRACE, "Effective Server RX MTU: %d.", serverMtu);
//            uint16_t effectMtu = MIN(serverMtu, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
//                msg.arg.type = BLE_EVT_GATT_PARAMS_UPDATED;
//                msg.arg.params.gatt_params_updated.version = 0x01;
//                msg.arg.params.gatt_params_updated.conn_handle = event->evt.gattc_evt.conn_handle;
//                msg.arg.params.gatt_params_updated.att_mtu_size = effectMtu;
//                if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
//                    LOG(ERROR, "os_queue_put() failed.");
//                }
        } break;
        case BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP: {
            const ble_gattc_evt_prim_srvc_disc_rsp_t& primSvcDiscRsp = event->evt.gattc_evt.params.prim_srvc_disc_rsp;
            LOG_DEBUG(TRACE, "BLE GATT Client event: %d primary service discovered.", primSvcDiscRsp.count);
            if (gattc->isDiscovering_ && gattc->currDiscProcedure_ == BLE_DISCOVERY_PROCEDURE_SERVICES &&
                    event->evt.gattc_evt.conn_handle == gattc->currDiscConnHandle_) {
                if (event->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_SUCCESS) {
                    for (uint8_t i = 0; i < primSvcDiscRsp.count; i++) {
                        hal_ble_svc_t service;
                        service.start_handle = primSvcDiscRsp.services[i].handle_range.start_handle;
                        service.end_handle = primSvcDiscRsp.services[i].handle_range.end_handle;
                        toHalUUID(&primSvcDiscRsp.services[i].uuid, &service.uuid);
                        gattc->discoveredServices_.append(service);
                    }
                    uint16_t currEndHandle = primSvcDiscRsp.services[primSvcDiscRsp.count - 1].handle_range.end_handle;
                    if ((gattc->discoverAll_) && (currEndHandle < SERVICES_TOP_END_HANDLE)) {
                        int ret = sd_ble_gattc_primary_services_discover(gattc->currDiscConnHandle_, currEndHandle + 1, NULL);
                        if (ret == NRF_SUCCESS) {
                            return;
                        }
                        LOG(ERROR, "sd_ble_gattc_primary_services_discover() failed: %u", (unsigned)ret);
                    }
                } else {
                    LOG(ERROR, "BLE service discovery failed: %d, handle: %d.", event->evt.gattc_evt.gatt_status, event->evt.gattc_evt.error_handle);
                }
                if (gattc->readServiceUUID128IfNeeded()) {
                    return;
                }
                os_semaphore_give(gattc->discoverySemaphore_, false);
            }
        } break;
        case BLE_GATTC_EVT_CHAR_DISC_RSP: {
            const ble_gattc_evt_char_disc_rsp_t& charDiscRsp = event->evt.gattc_evt.params.char_disc_rsp;
            LOG_DEBUG(TRACE, "BLE GATT Client event: %d characteristic discovered.", charDiscRsp.count);
            if (gattc->isDiscovering_ && gattc->currDiscProcedure_ == BLE_DISCOVERY_PROCEDURE_CHARACTERISTICS &&
                    event->evt.gattc_evt.conn_handle == gattc->currDiscConnHandle_) {
                if (event->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_SUCCESS) {
                    for (uint8_t i = 0; i < charDiscRsp.count; i++) {
                        hal_ble_char_t characteristic;
                        characteristic.char_ext_props = charDiscRsp.chars[i].char_ext_props;
                        characteristic.properties = toHalCharProps(charDiscRsp.chars[i].char_props);
                        characteristic.decl_handle = charDiscRsp.chars[i].handle_decl;
                        characteristic.value_handle = charDiscRsp.chars[i].handle_value;
                        toHalUUID(&charDiscRsp.chars[i].uuid, &characteristic.uuid);
                        gattc->discoveredCharacteristics_.append(characteristic);
                    }
                    uint16_t currEndHandle = charDiscRsp.chars[charDiscRsp.count - 1].handle_value;
                    if (currEndHandle < gattc->currDiscSvc_.end_handle) {
                        ble_gattc_handle_range_t handleRange;
                        handleRange.start_handle = currEndHandle + 1;
                        handleRange.end_handle = gattc->currDiscSvc_.end_handle;
                        int ret = sd_ble_gattc_characteristics_discover(gattc->currDiscConnHandle_, &handleRange);
                        if (ret == NRF_SUCCESS) {
                            return;
                        }
                        LOG(ERROR, "sd_ble_gattc_characteristics_discover() failed: %u", (unsigned)ret);
                    }
                } else {
                    LOG(ERROR, "BLE characteristic discovery failed: %d, handle: %d.", event->evt.gattc_evt.gatt_status, event->evt.gattc_evt.error_handle);
                }
                if (gattc->readCharacteristicUUID128IfNeeded()) {
                    return;
                }
                os_semaphore_give(gattc->discoverySemaphore_, false);
            }
        } break;
        case BLE_GATTC_EVT_DESC_DISC_RSP: {
            const ble_gattc_evt_desc_disc_rsp_t& descDiscRsp = event->evt.gattc_evt.params.desc_disc_rsp;
            LOG_DEBUG(TRACE, "BLE GATT Client event: %d descriptors discovered.", descDiscRsp.count);
            if (gattc->isDiscovering_ && gattc->currDiscProcedure_ == BLE_DISCOVERY_PROCEDURE_DESCRIPTORS &&
                    event->evt.gattc_evt.conn_handle == gattc->currDiscConnHandle_) {
                if (event->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_SUCCESS) {
                    for (uint8_t i = 0; i < descDiscRsp.count; i++) {
                        // It will report all attributes with 16-bits UUID, filter descriptors.
                        hal_ble_char_t* characteristic = gattc->findDiscoveredCharacteristic(descDiscRsp.descs[i].handle);
                        if (characteristic) {
                            switch (descDiscRsp.descs[i].uuid.uuid) {
                                case BLE_SIG_UUID_CHAR_USER_DESCRIPTION_DESC: {
                                    characteristic->user_desc_handle = descDiscRsp.descs[i].handle;
                                } break;
                                case BLE_SIG_UUID_CLIENT_CHAR_CONFIG_DESC: {
                                    characteristic->cccd_handle = descDiscRsp.descs[i].handle;
                                } break;
                                case BLE_SIG_UUID_SERVER_CHAR_CONFIG_DESC: {
                                    characteristic->sccd_handle = descDiscRsp.descs[i].handle;
                                } break;
                                case BLE_SIG_UUID_CHAR_EXTENDED_PROPERTIES_DESC:
                                case BLE_SIG_UUID_CHAR_PRESENT_FORMAT_DESC:
                                case BLE_SIG_UUID_CHAR_AGGREGATE_FORMAT:
                                default: break;
                            }
                        }
                    }
                    uint16_t currEndHandle = descDiscRsp.descs[descDiscRsp.count - 1].handle;
                    if (currEndHandle < gattc->currDiscSvc_.end_handle) {
                        ble_gattc_handle_range_t handleRange;
                        handleRange.start_handle = currEndHandle + 1;
                        handleRange.end_handle = gattc->currDiscSvc_.end_handle;
                        int ret = sd_ble_gattc_descriptors_discover(gattc->currDiscConnHandle_, &handleRange);
                        if (ret == NRF_SUCCESS) {
                            return;
                        }
                        LOG(ERROR, "sd_ble_gattc_descriptors_discover() failed: %u", (unsigned)ret);
                    }
                }
                os_semaphore_give(gattc->discoverySemaphore_, false);
            }
        } break;
        case BLE_GATTC_EVT_READ_RSP: {
            const ble_gattc_evt_read_rsp_t& readRsp = event->evt.gattc_evt.params.read_rsp;
            LOG_DEBUG(TRACE, "BLE GATT Client event: read response.");
            if (gattc->isDiscovering_ && event->evt.gattc_evt.conn_handle == gattc->currDiscConnHandle_) {
                if (event->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_SUCCESS) {
                    if (gattc->currDiscProcedure_ == BLE_DISCOVERY_PROCEDURE_SERVICES) {
                        hal_ble_svc_t* service = gattc->findDiscoveredService(readRsp.handle);
                        if (service) {
                            service->uuid.type = BLE_UUID_TYPE_128BIT;
                            memcpy(service->uuid.uuid128, readRsp.data, BLE_SIG_UUID_128BIT_LEN);
                            if (gattc->readServiceUUID128IfNeeded()) {
                                return;
                            }
                        }
                    } else if (gattc->currDiscProcedure_ == BLE_DISCOVERY_PROCEDURE_CHARACTERISTICS) {
                        hal_ble_char_t* characteristic = gattc->findDiscoveredCharacteristic(readRsp.handle);
                        if (characteristic) {
                            characteristic->uuid.type = BLE_UUID_TYPE_128BIT;
                            memcpy(characteristic->uuid.uuid128, &readRsp.data[3], BLE_SIG_UUID_128BIT_LEN);
                            if (gattc->readCharacteristicUUID128IfNeeded()) {
                                return;
                            }
                        }
                    }
                } else {
                    LOG(ERROR, "BLE read characteristic failed: %d, handle: %d.", event->evt.gattc_evt.gatt_status, event->evt.gattc_evt.error_handle);
                }
                os_semaphore_give(gattc->discoverySemaphore_, false);
            } else if (event->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_SUCCESS) {
                if (gattc->readAttrHandle_ == readRsp.handle && gattc->readBuf_ != nullptr) {
                    gattc->readLen_ = MIN(gattc->readLen_, readRsp.len);
                    memcpy(gattc->readBuf_, readRsp.data, gattc->readLen_);
                }
                os_semaphore_give(gattc->readWriteSemaphore_, false);
            } else {
                LOG(ERROR, "BLE read characteristic failed: %d, handle: %d.", event->evt.gattc_evt.gatt_status, event->evt.gattc_evt.error_handle);
            }
        } break;
        case BLE_GATTC_EVT_WRITE_RSP: {
            LOG_DEBUG(TRACE, "BLE GATT Client event: write with response completed.");
            os_semaphore_give(gattc->readWriteSemaphore_, false);
        } break;
        case BLE_GATTC_EVT_WRITE_CMD_TX_COMPLETE: {
            LOG_DEBUG(TRACE, "BLE GATT Client event: write without response completed.");
            os_semaphore_give(gattc->readWriteSemaphore_, false);
        } break;
        case BLE_GATTC_EVT_HVX: {
            const ble_gattc_evt_hvx_t& hvx = event->evt.gattc_evt.params.hvx;
            LOG_DEBUG(TRACE, "BLE GATT Client event: data received. conn_handle: %d, attr_handle: %d, len: %d, type: %d",
                    event->evt.gattc_evt.conn_handle, hvx.handle, hvx.len, hvx.type);
            if (hvx.type == BLE_GATT_HVX_INDICATION) {
                int ret = sd_ble_gattc_hv_confirm(event->evt.gattc_evt.conn_handle, hvx.handle);
                if (ret != NRF_SUCCESS) {
                    LOG(ERROR, "sd_ble_gattc_hv_confirm() failed: %u", (unsigned)ret);
                }
            }
//                msg.arg.type = BLE_EVT_DATA_NOTIFIED;
//                msg.arg.params.data_rec.version = 0x01;
//                msg.arg.params.data_rec.conn_handle = event->evt.gattc_evt.conn_handle;
//                msg.arg.params.data_rec.attr_handle = hvx.handle;
//                msg.arg.params.data_rec.offset = 0;
//                msg.arg.params.data_rec.data_len = hvx.len;
//                memcpy(msg.arg.params.data_rec.data, hvx.data, hvx.len);
//                if (os_queue_put(s_bleInstance.evtQueue, &msg, 0, NULL)) {
//                    LOG(ERROR, "os_queue_put() failed.");
//                }
        } break;
        case BLE_GATTC_EVT_TIMEOUT: {
            LOG_DEBUG(TRACE, "BLE GATT Client event: timeout, conn handle: %d, source: %d",
                    event->evt.gattc_evt.conn_handle, event->evt.gattc_evt.params.timeout.src);
            // Disconnect on GATT Client timeout event.
            int ret = sd_ble_gap_disconnect(event->evt.gattc_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gap_disconnect() failed: %u", (unsigned)ret);
            }
        } break;
        default: break;
    }
}

bool BleObject::initialized_ = false;

BleObject::BleObject()
    : dispatcher(std::make_unique<BleEventDispatcher>(this)),
      gap(std::make_unique<BleGap>(this)),
      broadcaster(std::make_unique<Broadcaster>(this)),
      observer(std::make_unique<Observer>(this)),
      connectionsMgr(std::make_unique<ConnectionsManager>(this)),
      gatts(std::make_unique<GattServer>(this)),
      gattc(std::make_unique<GattClient>(this)) {
    init();
}

BleObject& BleObject::getInstance(void) {
    static BleObject instance;
    return instance;
}

int BleObject::init(void) {
    if (!initialized_) {
        // Configure the BLE stack using the default settings.
        // Fetch the start address of the application RAM.
        uint32_t ramStart = 0;
        int ret = nrf_sdh_ble_default_cfg_set(BLE_CONN_CFG_TAG, &ramStart);
        NRF_CHECK_RETURN(ret, "nrf_sdh_ble_default_cfg_set");
        LOG_DEBUG(TRACE, "RAM start: 0x%08x", (unsigned)ramStart);
        // Enable the stack
        ret = nrf_sdh_ble_enable(&ramStart);
        NRF_CHECK_RETURN(ret, "nrf_sdh_ble_enable");
        dispatcher->init();
        gap->init();
        broadcaster->init();
        observer->init();
        connectionsMgr->init();
        gatts->init();
        gattc->init();
        genericEvthandler_ = nullptr;
        genericEvtContext_ = nullptr;
        initialized_ = true;
    }
    return SYSTEM_ERROR_NONE;
}

void BleObject::onGenericEventCallback(on_ble_evt_cb_t cb, void* context) {
    genericEvthandler_ = cb;
    genericEvtContext_ = context;
}


/**********************************************
 * Particle BLE APIs
 */
bool ble_stack_is_initialized(void) {
    return true;
}

int ble_stack_init(void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    /* The SoftDevice has been enabled in core_hal.c */
    LOG_DEBUG(TRACE, "ble_stack_init().");
    return BleObject::getInstance().init();
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
    BleObject::getInstance().onGenericEventCallback(callback, context);
    return SYSTEM_ERROR_NONE;
}


/**********************************************
 * BLE GAP APIs
 */
int ble_gap_set_device_address(const hal_ble_addr_t* address) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_set_device_address().");
    return BleObject::getInstance().gap->setDeviceAddress(address);
}

int ble_gap_get_device_address(hal_ble_addr_t* address) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_get_device_address().");
    return BleObject::getInstance().gap->getDeviceAddress(address);
}

int ble_gap_set_device_name(const uint8_t* device_name, uint16_t len) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_set_device_name().");
    return BleObject::getInstance().gap->setDeviceName(device_name, len);
}

int ble_gap_get_device_name(uint8_t* device_name, uint16_t* len) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_get_device_name().");
    return BleObject::getInstance().gap->getDeviceName(device_name, len);
}

int ble_gap_set_appearance(uint16_t appearance) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_set_appearance().");
    return BleObject::getInstance().gap->setAppearance(appearance);
}

int ble_gap_get_appearance(uint16_t* appearance) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_get_appearance().");
    return BleObject::getInstance().gap->getAppearance(appearance);
}

int ble_gap_set_ppcp(const hal_ble_conn_params_t* ppcp, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_set_ppcp().");
    return BleObject::getInstance().connectionsMgr->setPPCP(ppcp);
}

int ble_gap_get_ppcp(hal_ble_conn_params_t* ppcp, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_get_ppcp().");
    return BleObject::getInstance().connectionsMgr->getPPCP(ppcp);
}

int ble_gap_add_whitelist(const hal_ble_addr_t* addr_list, uint8_t len, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_add_whitelist().");
    return BleObject::getInstance().gap->addWhitelist(addr_list, len);
}

int ble_gap_delete_whitelist(void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_delete_whitelist().");
    return BleObject::getInstance().gap->deleteWhitelist();
}

int ble_gap_set_tx_power(int8_t value) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_set_tx_power().");
    return BleObject::getInstance().broadcaster->setTxPower(value);
}

int8_t ble_gap_get_tx_power(void) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_get_tx_power().");
    return BleObject::getInstance().broadcaster->getTxPower();
}

int ble_gap_set_advertising_parameters(const hal_ble_adv_params_t* adv_params, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_set_advertising_parameters().");
    return BleObject::getInstance().broadcaster->setAdvertisingParams(adv_params);
}

int ble_gap_get_advertising_parameters(hal_ble_adv_params_t* adv_params, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_get_advertising_parameters().");
    return BleObject::getInstance().broadcaster->getAdvertisingParams(adv_params);
}

int ble_gap_set_advertising_data(const uint8_t* buf, uint16_t len, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_set_advertising_data().");
    return BleObject::getInstance().broadcaster->setAdvertisingData(buf, len);
}

size_t ble_gap_get_advertising_data(uint8_t* buf, uint16_t len, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_get_advertising_data().");
    return BleObject::getInstance().broadcaster->getAdvertisingData(buf, len);
}

int ble_gap_set_scan_response_data(const uint8_t* buf, uint16_t len, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_set_scan_response_data().");
    return BleObject::getInstance().broadcaster->setScanResponseData(buf, len);
}

size_t ble_gap_get_scan_response_data(uint8_t* buf, uint16_t len, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_get_scan_response_data().");
    return BleObject::getInstance().broadcaster->getScanResponseData(buf, len);
}

int ble_gap_start_advertising(void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_start_advertising().");
    return BleObject::getInstance().broadcaster->startAdvertising();
}

int ble_gap_stop_advertising(void) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_stop_advertising().");
    return BleObject::getInstance().broadcaster->stopAdvertising();
}

bool ble_gap_is_advertising(void) {
    return BleObject::getInstance().broadcaster->advertising();
}

int ble_gap_set_scan_parameters(const hal_ble_scan_params_t* scan_params, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_set_scan_parameters().");
    return BleObject::getInstance().observer->setScanParams(scan_params);
}

int ble_gap_get_scan_parameters(hal_ble_scan_params_t* scan_params, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_set_scan_parameters().");
    return BleObject::getInstance().observer->getScanParams(scan_params);
}

int ble_gap_start_scan(void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_start_scan().");
    return BleObject::getInstance().observer->startScanning(false);
}

bool ble_gap_is_scanning(void) {
    return BleObject::getInstance().observer->scanning();
}

int ble_gap_stop_scan(void) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_stop_scan().");
    return BleObject::getInstance().observer->stopScanning();
}

int ble_gap_connect(const hal_ble_addr_t* address, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_connect().");
    return BleObject::getInstance().connectionsMgr->connect(address);
}

bool ble_gap_is_connecting(void) {
    return BleObject::getInstance().connectionsMgr->connecting();
}

bool ble_gap_is_connected(const hal_ble_addr_t* address) {
    return BleObject::getInstance().connectionsMgr->connected(address);
}

int ble_gap_connect_cancel(void) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_connect_cancel().");
    return BleObject::getInstance().connectionsMgr->connectCancel();
}

int ble_gap_disconnect(uint16_t conn_handle, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_disconnect().");
    return BleObject::getInstance().connectionsMgr->disconnect(conn_handle);
}

int ble_gap_update_connection_params(uint16_t conn_handle, const hal_ble_conn_params_t* conn_params, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_update_connection_params().");
    return BleObject::getInstance().connectionsMgr->updateConnectionParams(conn_handle, conn_params);
}

int ble_gap_get_connection_params(uint16_t conn_handle, hal_ble_conn_params_t* conn_params, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_update_connection_params().");
    return BleObject::getInstance().connectionsMgr->getConnectionParams(conn_handle, conn_params);
}

int ble_gap_get_rssi(uint16_t conn_handle, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gap_get_rssi().");
    return 0;
}


/**********************************************
 * BLE GATT Server APIs
 */
int ble_gatt_server_add_service(uint8_t type, const hal_ble_uuid_t* uuid, uint16_t* handle, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gatt_server_add_service().");
    return BleObject::getInstance().gatts->addService(type, uuid, handle);
}

int ble_gatt_server_add_characteristic(const hal_ble_char_init_t* char_init, hal_ble_char_handles_t* handles, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gatt_server_add_characteristic().");
    return BleObject::getInstance().gatts->addCharacteristic(char_init->service_handle, char_init->properties, &char_init->uuid, char_init->description, handles);
}

int ble_gatt_server_add_descriptor(const hal_ble_desc_init_t* desc_init, uint16_t* handle, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gatt_server_add_descriptor().");
    return BleObject::getInstance().gatts->addDescriptor(desc_init->char_handle, &desc_init->uuid, desc_init->descriptor, desc_init->len, handle);
}

size_t ble_gatt_server_set_characteristic_value(uint16_t value_handle, const uint8_t* buf, size_t len, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gatt_server_set_characteristic_value().");
    return BleObject::getInstance().gatts->setValue(value_handle, buf, len);
}

size_t ble_gatt_server_get_characteristic_value(uint16_t value_handle, uint8_t* buf, size_t len, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gatt_server_get_characteristic_value().");
    return BleObject::getInstance().gatts->getValue(value_handle, buf, len);
}


/**********************************************
 * BLE GATT Client APIs
 */
int ble_gatt_client_discover_all_services(uint16_t conn_handle, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gatt_client_discover_all_services().");
    return BleObject::getInstance().gattc->discoverServices(conn_handle, nullptr);
}

int ble_gatt_client_discover_service_by_uuid(uint16_t conn_handle, const hal_ble_uuid_t* uuid, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gatt_client_discover_service_by_uuid().");
    return BleObject::getInstance().gattc->discoverServices(conn_handle, uuid);
}

int ble_gatt_client_discover_characteristics(uint16_t conn_handle, const hal_ble_svc_t* service, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gatt_client_discover_characteristics().");
    return BleObject::getInstance().gattc->discoverCharacteristics(conn_handle, service);
}

int ble_gatt_client_discover_characteristics_by_uuid(uint16_t conn_handle, const hal_ble_svc_t* service, const hal_ble_uuid_t* uuid, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

bool ble_gatt_client_is_discovering(void) {
    return BleObject::getInstance().gattc->discovering();
}

int ble_gatt_client_configure_cccd(uint16_t conn_handle, uint16_t cccd_handle, uint8_t cccd_value, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gatt_client_configure_cccd().");
    if (conn_handle == BLE_CONN_HANDLE_INVALID || cccd_handle == BLE_INVALID_ATTR_HANDLE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (cccd_value > BLE_SIG_CCCD_VAL_INDICATION) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    uint8_t buf[2] = {0x00, 0x00};
    buf[0] = cccd_value;
    if (BleObject::getInstance().gattc->writeAttribute(conn_handle, cccd_handle, buf, 2, true) == 0) {
        return SYSTEM_ERROR_INTERNAL;
    }
    return SYSTEM_ERROR_NONE;
}

size_t ble_gatt_client_write_with_response(uint16_t conn_handle, uint16_t value_handle, const uint8_t* buf, size_t len, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gatt_client_write_with_response().");
    return BleObject::getInstance().gattc->writeAttribute(conn_handle, value_handle, buf, len, true);
}

size_t ble_gatt_client_write_without_response(uint16_t conn_handle, uint16_t value_handle, const uint8_t* buf, size_t len, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gatt_client_write_without_response().");
    return BleObject::getInstance().gattc->writeAttribute(conn_handle, value_handle, buf, len, false);
}

size_t ble_gatt_client_read(uint16_t conn_handle, uint16_t value_handle, uint8_t* buf, size_t len, void* reserved) {
    std::lock_guard<bleLock> lk(bleLock());
    LOG_DEBUG(TRACE, "ble_gatt_client_read().");
    return BleObject::getInstance().gattc->readAttribute(conn_handle, value_handle, buf, len);
}

#endif // HAL_PLATFORM_BLE
