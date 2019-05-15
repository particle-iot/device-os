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
#define LOG_CHECKED_ERRORS 1

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
#include "check_nrf.h"
#include "check.h"
#include "scope_guard.h"

using namespace particle;
#include "intrusive_list.h"

static_assert(NRF_SDH_BLE_PERIPHERAL_LINK_COUNT == 1, "Multiple simultaneous peripheral connections are not supported");
static_assert(NRF_SDH_BLE_TOTAL_LINK_COUNT <= 20, "Maximum supported number of concurrent connections in the peripheral and central roles combined exceeded");

using spark::Vector;
using namespace particle::ble;

#define BLE_CONN_CFG_TAG 1

StaticRecursiveMutex s_bleMutex;

//anonymous namespace
namespace {

// BLE service base start handle.
const hal_ble_attr_handle_t SERVICES_BASE_START_HANDLE = 0x0001;
// BLE service top end handle.
const hal_ble_attr_handle_t SERVICES_TOP_END_HANDLE = 0xFFFF;

// Pool for storing scan result pure data.
const size_t OBSERVER_SCANNED_DATA_POOL_SIZE = 1024;
// Pool for scan result cached device.
const size_t OBSERVER_CACHED_DEVICE_POOL_SIZE = 1024;
// Pool for scan pending results.
const size_t OBSERVER_PENDING_RESULTS_POOL_SIZE = 1024;
// Pool for storing on-going connections.
const size_t CONNECTIONS_POOL_SIZE = 256;
// Pool for GATT Server to store received data.
const size_t GATT_SERVER_DATA_REC_POOL_SIZE = 1024;
// Pool for storing discovered service or characteristics.
const size_t GATT_CLIENT_DISC_SVC_CHAR_POOL_SIZE = 2048;
// Pool for GATT Client to store received data.
const size_t GATT_CLIENT_DATA_REC_POOL_SIZE = 1024;
// Pool for link att_mtu list.
const size_t GATT_LINK_ATT_MTU_LIST_POOL_SIZE = 256;

// Timeout for GAP connection operation.
const uint32_t CONNECTION_OPERATION_TIMEOUT_MS = 5000;
// Timeout for sending notification/indication.
const uint32_t BLE_HVX_PROCEDURE_TIMEOUT_MS = 5000;
// Timeout for BLE service discovery procedure.
const uint32_t BLE_DICOVERY_PROCEDURE_TIMEOUT_MS = 10000;
// Timeout for data read write procedure.
const uint32_t BLE_READ_WRITE_PROCEDURE_TIMEOUT_MS = 5000;
// Delay for GATT Client to send the ATT MTU exchanging request.
const uint32_t BLE_ATT_MTU_EXCHANGE_DELAY_MS = 800;

static const uint8_t BleAdvEvtTypeMap[] = {
    BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED,
    BLE_GAP_ADV_TYPE_EXTENDED_CONNECTABLE_NONSCANNABLE_UNDIRECTED,
    BLE_GAP_ADV_TYPE_CONNECTABLE_NONSCANNABLE_DIRECTED,
    BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED,
    BLE_GAP_ADV_TYPE_EXTENDED_NONCONNECTABLE_NONSCANNABLE_DIRECTED,
    BLE_GAP_ADV_TYPE_NONCONNECTABLE_SCANNABLE_UNDIRECTED,
    BLE_GAP_ADV_TYPE_EXTENDED_NONCONNECTABLE_SCANNABLE_DIRECTED
};

system_error_t sysError(uint32_t error) {
    switch (error) {
        case NRF_SUCCESS:
            return SYSTEM_ERROR_NONE;
        case NRF_ERROR_SVC_HANDLER_MISSING:
        case NRF_ERROR_NOT_SUPPORTED:
            return SYSTEM_ERROR_NOT_SUPPORTED;
        case NRF_ERROR_SOFTDEVICE_NOT_ENABLED:
        case NRF_ERROR_INVALID_STATE:
            return SYSTEM_ERROR_INVALID_STATE;
        case NRF_ERROR_INTERNAL:
            return SYSTEM_ERROR_INTERNAL;
        case NRF_ERROR_NO_MEM:
            return SYSTEM_ERROR_NO_MEMORY;
        case NRF_ERROR_NOT_FOUND:
            return SYSTEM_ERROR_NOT_FOUND;
        case NRF_ERROR_INVALID_PARAM:
        case NRF_ERROR_INVALID_LENGTH:
        case NRF_ERROR_INVALID_FLAGS:
        case NRF_ERROR_INVALID_DATA:
        case NRF_ERROR_DATA_SIZE:
        case NRF_ERROR_NULL:
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        case NRF_ERROR_TIMEOUT:
            return SYSTEM_ERROR_TIMEOUT;
        case NRF_ERROR_FORBIDDEN:
        case NRF_ERROR_INVALID_ADDR:
            return SYSTEM_ERROR_NOT_ALLOWED;
        case NRF_ERROR_BUSY:
            return SYSTEM_ERROR_BUSY;
        case NRF_ERROR_CONN_COUNT:
            return SYSTEM_ERROR_LIMIT_EXCEEDED;
        case NRF_ERROR_RESOURCES:
            return SYSTEM_ERROR_ABORTED;
        default:
            return SYSTEM_ERROR_UNKNOWN;
    }
}

bool addressEqual(const hal_ble_addr_t& srcAddr, const hal_ble_addr_t& destAddr) {
    return (srcAddr.addr_type == destAddr.addr_type && !memcmp(srcAddr.addr, destAddr.addr, BLE_SIG_ADDR_LEN));
}

} //anonymous namespace

class GattBase {
public:
    /*
     * A device's Exchange MTU Request shall contain the same MTU as the
     * device's Exchange MTU Response (i.e. the MTU shall be symmetric).
     */
    struct LinkAttMtu {
        LinkAttMtu* next;
        hal_ble_conn_handle_t connHandle;
        size_t effective;
        bool isExchanged;
    };

    GattBase() {
        if (!poolInitialized) {
            attMtuListPool_.init(GATT_LINK_ATT_MTU_LIST_POOL_SIZE);
            poolInitialized = true;
        }
    }
    ~GattBase() = default;

protected:
    size_t getAttMtu(hal_ble_conn_handle_t connHandle) const {
        auto attMtu = attMtuList_.front();
        while (attMtu) {
            if (attMtu->connHandle == connHandle) {
                return attMtu->effective;
            }
            attMtu = attMtu->next;
        }
        return BLE_MIN_ATT_MTU_SIZE;
    }

    int configureAttMtu(hal_ble_conn_handle_t connHandle, size_t effective, bool exchanged = false) {
        // Update the effective ATT_MTU if existed.
        auto attMtu = attMtuList_.front();
        while (attMtu) {
            if (attMtu->connHandle == connHandle) {
                attMtu->effective = effective;
                attMtu->isExchanged = exchanged;
                return SYSTEM_ERROR_NONE;
            }
            attMtu = attMtu->next;
        }
        // Set ATT_MTU for new link.
        LinkAttMtu* effectAttMtu = (LinkAttMtu*)attMtuListPool_.alloc(sizeof(LinkAttMtu));
        if (effectAttMtu) {
            effectAttMtu->connHandle = connHandle;
            effectAttMtu->effective = effective;
            effectAttMtu->isExchanged = exchanged;
            attMtuList_.pushFront(effectAttMtu);
            return SYSTEM_ERROR_NONE;
        }
        return SYSTEM_ERROR_NO_MEMORY;
    }

    bool attMtuExchanged(hal_ble_conn_handle_t connHandle) const {
        auto attMtu = attMtuList_.front();
        while (attMtu) {
            if (attMtu->connHandle == connHandle) {
                return attMtu->isExchanged;
            }
            attMtu = attMtu->next;
        }
        return false;
    }

    void clearAttMtu(hal_ble_conn_handle_t connHandle) {
        auto attMtu = attMtuList_.front();
        LinkAttMtu* prev = nullptr;
        while (attMtu) {
            if (attMtu->connHandle == connHandle) {
                LinkAttMtu* curr = attMtuList_.pop(attMtu, prev);
                attMtu = curr->next;
                attMtuListPool_.free(curr);
                continue;
            }
            prev = attMtu;
            attMtu = attMtu->next;
        }
    }

    static uint8_t toHalCharProps(ble_gatt_char_props_t properties) {
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

    static ble_gatt_char_props_t toPlatformCharProps(uint8_t halProperties) {
        ble_gatt_char_props_t properties = {};
        properties.broadcast = (halProperties & BLE_SIG_CHAR_PROP_BROADCAST) ? 1 : 0;
        properties.read = (halProperties & BLE_SIG_CHAR_PROP_READ) ? 1 : 0;
        properties.write_wo_resp = (halProperties & BLE_SIG_CHAR_PROP_WRITE_WO_RESP) ? 1 : 0;
        properties.write = (halProperties & BLE_SIG_CHAR_PROP_WRITE) ? 1 : 0;
        properties.notify = (halProperties & BLE_SIG_CHAR_PROP_NOTIFY) ? 1 : 0;
        properties.indicate = (halProperties & BLE_SIG_CHAR_PROP_INDICATE) ? 1 : 0;
        properties.auth_signed_wr = (halProperties & BLE_SIG_CHAR_PROP_AUTH_SIGN_WRITES) ? 1 : 0;
        return properties;
    }

    // GATT Server and GATT client share the same ATT_MTU.
    static size_t desiredAttMtu_;
    static AtomicIntrusiveList<LinkAttMtu> attMtuList_;
    static AtomicAllocedPool attMtuListPool_;
    static bool poolInitialized;
};

size_t GattBase::desiredAttMtu_ = BLE_MAX_ATT_MTU_SIZE;
AtomicIntrusiveList<GattBase::LinkAttMtu> GattBase::attMtuList_;
AtomicAllocedPool GattBase::attMtuListPool_;
bool GattBase::poolInitialized = false;

class BleObject {
public:
    class BleEventDispatcher;
    class BleGap;
    class Broadcaster;
    class Observer;
    class ConnectionsManager;
    class GattServer;
    class GattClient;

    static BleObject& getInstance();
    int init();
    int selectAntenna(hal_ble_ant_type_t antenna) const;

    BleEventDispatcher* dispatcher() { return dispatcher_.get(); }
    BleGap* gap() { return gap_.get(); }
    Broadcaster* broadcaster() { return broadcaster_.get(); }
    Observer* observer() { return observer_.get(); }
    ConnectionsManager* connMgr() { return connectionsMgr_.get(); }
    GattServer* gatts() { return gatts_.get(); }
    GattClient* gattc() { return gattc_.get(); }

private:
    BleObject()
            : dispatcher_(std::make_unique<BleEventDispatcher>()),
              gap_(std::make_unique<BleGap>()),
              broadcaster_(std::make_unique<Broadcaster>()),
              observer_(std::make_unique<Observer>()),
              connectionsMgr_(std::make_unique<ConnectionsManager>()),
              gatts_(std::make_unique<GattServer>()),
              gattc_(std::make_unique<GattClient>()) {
        init();
    }
    ~BleObject() = default;
    static int toPlatformUUID(const hal_ble_uuid_t* halUuid, ble_uuid_t* uuid);
    static int toHalUUID(const ble_uuid_t* uuid, hal_ble_uuid_t* halUuid);

    std::unique_ptr<BleEventDispatcher> dispatcher_;         /**< BLE event dispatcher. */
    std::unique_ptr<BleGap> gap_;                            /**< BLE GAP instance. */
    std::unique_ptr<Broadcaster> broadcaster_;               /**< BLE Broadcaster instance. */
    std::unique_ptr<Observer> observer_;                     /**< BLE Observer instance. */
    std::unique_ptr<ConnectionsManager> connectionsMgr_;     /**< BLE connections manager instance. */
    std::unique_ptr<GattServer> gatts_;                      /**< BLE GATT server instance. */
    std::unique_ptr<GattClient> gattc_;                      /**< BLE GATT client instance. */
    static bool initialized_;
};

class BleObject::BleEventDispatcher {
public:
    typedef void (*BleSpecificEventHandler)(void* data, void* context);

    class EventMessage {
    public:
        EventMessage()
                : handler(nullptr),
                  context(nullptr),
                  hook(nullptr),
                  hookContext(nullptr) {
            evt = {};
        }
        ~EventMessage() = default;

        hal_ble_evts_t evt;                 /**< BLE event data. */
        BleSpecificEventHandler handler;    /**< BLE specific event handler. */
        void* context;                      /**< BLE specific event context. */
        on_ble_evt_cb_t hook;               /**< Internal hook function after event being dispatched. */
        void* hookContext;                  /**< Internal hook function context. */
    };

    BleEventDispatcher()
            : evtQueue_(nullptr),
              evtThread_(nullptr) {
    }
    ~BleEventDispatcher() = default;
    int init();
    int enqueue(EventMessage& msg);
    void onGenericEventCallback(on_ble_evt_cb_t cb, void* context);

private:
    class BleGenericEventHandler {
    public:
        BleGenericEventHandler() : handler(nullptr), context(nullptr) {}
        ~BleGenericEventHandler() = default;

        on_ble_evt_cb_t handler;
        void* context;
    };

    static os_thread_return_t bleEventDispatch(void* param);

    os_queue_t evtQueue_;                                   /**< BLE event queue. */
    os_thread_t evtThread_;                                 /**< BLE event thread. */
    Vector<BleGenericEventHandler> genericEventHandlers_;   /**< BLE generic event handlers. */
};

class BleObject::BleGap {
public:
    BleGap() = default;
    ~BleGap() = default;
    int init();
    int setDeviceName(const char* deviceName, size_t len);
    int getDeviceName(char* deviceName, size_t len);
    int setDeviceAddress(const hal_ble_addr_t* address);
    int getDeviceAddress(hal_ble_addr_t* address);
    int setAppearance(ble_sig_appearance_t appearance);
    int getAppearance(ble_sig_appearance_t* appearance);
    int addWhitelist(const hal_ble_addr_t* addrList, size_t len);
    int deleteWhitelist();

private:
    static void processBleGapEvents(const ble_evt_t* event, void* context);
};

class BleObject::Broadcaster {
public:
    Broadcaster();
    ~Broadcaster() = default;
    int init();
    bool advertising() const;
    int setAdvertisingParams(const hal_ble_adv_params_t* params);
    int getAdvertisingParams(hal_ble_adv_params_t* params) const;
    int setAdvertisingData(const uint8_t* buf, size_t len);
    ssize_t getAdvertisingData(uint8_t* buf, size_t len) const;
    int setScanResponseData(const uint8_t* buf, size_t len);
    ssize_t getScanResponseData(uint8_t* buf, size_t len) const;
    int setTxPower(int8_t val);
    int getTxPower(int8_t* txPower) const;
    int startAdvertising();
    int stopAdvertising();
    int setAutoAdvertiseScheme(hal_ble_auto_adv_cfg_t config);
    hal_ble_auto_adv_cfg_t getAutoAdvertiseScheme();

private:
    int suspend();
    int resume();
    ble_gap_adv_data_t toPlatformAdvData(void);
    int configure(const hal_ble_adv_params_t* params);
    static int8_t roundTxPower(int8_t value);
    static ble_gap_adv_params_t toPlatformAdvParams(const hal_ble_adv_params_t* halParams);
    static void processBroadcasterEvents(const ble_evt_t* event, void* context);

    volatile bool isAdvertising_;                   /**< If it is advertising or not. */
    volatile hal_ble_auto_adv_cfg_t autoAdvCfg_;    /**< Automatic advertising configuration. */
    uint8_t advHandle_;                             /**< Advertising handle. */
    hal_ble_adv_params_t advParams_;                /**< Current advertising parameters. */
    uint8_t advData_[BLE_MAX_ADV_DATA_LEN];         /**< Current advertising data. */
    size_t advDataLen_;                             /**< Current advertising data length. */
    uint8_t scanRespData_[BLE_MAX_ADV_DATA_LEN];    /**< Current scan response data. */
    size_t scanRespDataLen_;                        /**< Current scan response data length. */
    int8_t txPower_;                                /**< TX Power. */
    bool advPending_;                               /**< Advertising is pending. */
    bool connectedAdvParams_;                       /**< Whether it is using the advertising parameters being set when connected as Peripheral. */
    volatile hal_ble_conn_handle_t connHandle_;     /**< Connection handle. It is assigned once device is connected as Peripheral. It is used for re-start advertising. */
    static const int8_t validTxPower_[8];           /**< Valid TX power values. */
};

class BleObject::Observer {
public:
    Observer()
            : isScanning_(false),
              scanSemaphore_(nullptr),
              scanResultCallBack_(nullptr),
              context_(nullptr) {
        scanParams_.version = 0x01;
        scanParams_.active = true;
        scanParams_.filter_policy = BLE_SCAN_FP_ACCEPT_ALL;
        scanParams_.interval = BLE_DEFAULT_SCANNING_INTERVAL;
        scanParams_.window = BLE_DEFAULT_SCANNING_WINDOW;
        scanParams_.timeout = BLE_DEFAULT_SCANNING_TIMEOUT;
        bleScanData_.p_data = scanReportBuff_;
        bleScanData_.len = sizeof(scanReportBuff_);
        scanCacheDevicesPool_.init(OBSERVER_CACHED_DEVICE_POOL_SIZE);
        scanPendingResultsPool_.init(OBSERVER_PENDING_RESULTS_POOL_SIZE);
    }
    ~Observer() = default;
    int init();
    bool scanning();
    int setScanParams(const hal_ble_scan_params_t* params);
    int getScanParams(hal_ble_scan_params_t* params) const;
    int startScanning(on_ble_scan_result_cb_t callback, void* context);
    int stopScanning();
    ble_gap_scan_params_t toPlatformScanParams(void) const;

private:
    struct cachedDevice {
        cachedDevice* next;
        hal_ble_addr_t addr;
    };

    struct pendingResult {
        pendingResult* next;
        hal_ble_gap_on_scan_result_evt_t resultEvt;
    };

    bool isCachedDevice(const hal_ble_addr_t& address) const;
    int addCachedDevice(const hal_ble_addr_t& address);
    void clearCachedDevice();
    hal_ble_gap_on_scan_result_evt_t* getPendingResult(const hal_ble_addr_t& address);
    int addPendingResult(const hal_ble_gap_on_scan_result_evt_t& resultEvt);
    void removePendingResult(const hal_ble_addr_t& address);
    void clearPendingResult();
    int continueScanning();
    static void observerEventProcessedHook(const hal_ble_evts_t *event, void* context);
    static void processObserverEvents(const ble_evt_t* event, void* context);

    bool isScanning_;                                       /**< If it is scanning or not. */
    hal_ble_scan_params_t scanParams_;                      /**< BLE scanning parameters. */
    os_semaphore_t scanSemaphore_;                          /**< Semaphore to wait until the scan procedure completed. */
    uint8_t scanReportBuff_[BLE_MAX_SCAN_REPORT_BUF_LEN];   /**< Buffer to hold the scanned report data. */
    ble_data_t bleScanData_;                                /**< BLE scanned data. */
    AtomicAllocedPool observerScannedDataPool_;             /**< Pool to allocate memory for scanned data. */
    on_ble_scan_result_cb_t scanResultCallBack_;            /**< Callback function on scan result. */
    void* context_;                                         /**< Context of the scan result callback function. */
    AtomicIntrusiveList<cachedDevice> cachedDevicesList_;   /**< Cached address of scanned devices to filter-out duplicated result. */
    AtomicIntrusiveList<pendingResult> pendingResultsList_; /**< Caches the scanned advertising data until the scan response data is captured. */
    AtomicAllocedPool scanCacheDevicesPool_;
    AtomicAllocedPool scanPendingResultsPool_;
};

class BleObject::ConnectionsManager {
public:
    struct BleConnection {
        BleConnection* next;
        hal_ble_role_t role;
        hal_ble_conn_handle_t connHandle;
        hal_ble_conn_params_t effectiveConnParams;
        hal_ble_addr_t peer;
    };

    ConnectionsManager()
            : isConnecting_(false),
              disconnectingHandle_(BLE_INVALID_CONN_HANDLE),
              connParamUpdateHandle_(BLE_INVALID_CONN_HANDLE),
              connParamsUpdateAttempts_(0),
              connParamsUpdateTimer_(nullptr),
              connParamsUpdateSemaphore_(nullptr),
              connectSemaphore_(nullptr),
              disconnectSemaphore_(nullptr) {
        connectingAddr_ = {};
        connectionsPool_.init(CONNECTIONS_POOL_SIZE);
    }
    ~ConnectionsManager() = default;
    int init();
    int setPpcp(const hal_ble_conn_params_t* ppcp);
    int getPpcp(hal_ble_conn_params_t* ppcp) const;
    bool connecting(const hal_ble_addr_t* address) const;
    bool connected(const hal_ble_addr_t* address = nullptr);
    int connect(const hal_ble_addr_t* address);
    int connectCancel(const hal_ble_addr_t* address);
    int disconnect(hal_ble_conn_handle_t connHandle);
    int updateConnectionParams(hal_ble_conn_handle_t connHandle, const hal_ble_conn_params_t* params);
    int getConnectionParams(hal_ble_conn_handle_t connHandle, hal_ble_conn_params_t* params);
    bool valid(hal_ble_conn_handle_t connHandle);

private:
    BleConnection* fetchConnection(hal_ble_conn_handle_t connHandle);
    BleConnection* fetchConnection(const hal_ble_addr_t* address);
    int addConnection(const BleConnection& connection);
    void removeConnection(hal_ble_conn_handle_t connHandle);
    void initiateConnParamsUpdateIfNeeded(const BleConnection* connection);
    bool isConnParamsFeeded(const hal_ble_conn_params_t* params) const;
    static ble_gap_conn_params_t toPlatformConnParams(const hal_ble_conn_params_t* halConnParams);
    static hal_ble_conn_params_t toHalConnParams(const ble_gap_conn_params_t* params);
    static void onConnParamsUpdateTimerExpired(os_timer_t timer);
    static void processConnectionEvents(const ble_evt_t* event, void* context);

    bool isConnecting_;                                         /**< If it is connecting or not. */
    hal_ble_addr_t connectingAddr_;                             /**< Address of peer the Central is connecting to. */
    volatile hal_ble_conn_handle_t disconnectingHandle_;        /**< Handle of connection to be disconnected. */
    volatile hal_ble_conn_handle_t connParamUpdateHandle_;      /**< Handle of the connection that is to send peripheral connection update request. */
    uint8_t connParamsUpdateAttempts_;                          /**< Attempts for peripheral to update connection parameters. */
    os_timer_t connParamsUpdateTimer_;                          /**< Timer used for sending peripheral connection update request after connection established. */
    os_semaphore_t connParamsUpdateSemaphore_;                  /**< Semaphore to wait until connection parameters updated. */
    os_semaphore_t connectSemaphore_;                           /**< Semaphore to wait until connection established. */
    os_semaphore_t disconnectSemaphore_;                        /**< Semaphore to wait until connection disconnected. */
    AtomicIntrusiveList<BleConnection> connectionsList_;        /**< Current on-going connections. */
    AtomicAllocedPool connectionsPool_;
};

class BleObject::GattServer : public GattBase {
public:
    GattServer() : hvxSemaphore_(nullptr) {}
    ~GattServer() = default;
    int init();
    int addService(uint8_t type, const hal_ble_uuid_t* uuid, hal_ble_attr_handle_t* svcHandle);
    int addCharacteristic(hal_ble_attr_handle_t svcHandle, uint8_t properties, const hal_ble_uuid_t* uuid, const char* desc, hal_ble_char_handles_t* charHandles);
    int addDescriptor(hal_ble_attr_handle_t charHandle, const hal_ble_uuid_t* uuid, uint8_t* descriptor, size_t len, hal_ble_attr_handle_t* descHandle);
    ssize_t setValue(hal_ble_attr_handle_t attrHandle, const uint8_t* buf, size_t len);
    ssize_t getValue(hal_ble_attr_handle_t attrHandle, uint8_t* buf, size_t len);

private:
    class BleCharacteristic {
        public:
            BleCharacteristic() {
                for (auto& cccdConnection : cccdConnections) {
                    cccdConnection = BLE_INVALID_CONN_HANDLE;
                }
            }
            ~BleCharacteristic() = default;

            uint8_t properties;
            hal_ble_attr_handle_t svcHandle;
            hal_ble_char_handles_t charHandles;
            hal_ble_conn_handle_t cccdConnections[BLE_MAX_LINK_COUNT];
    };

    bool findService(hal_ble_attr_handle_t svcHandle) const;
    BleCharacteristic* findCharacteristic(hal_ble_attr_handle_t attrHandle);
    void addToCccdList(BleCharacteristic* characteristic, hal_ble_conn_handle_t connHandle);
    void removeFromCCCDList(BleCharacteristic* characteristic, hal_ble_conn_handle_t connHandle);
    void removeFromAllCCCDList(hal_ble_conn_handle_t connHandle);
    static void gattsEventProcessedHook(const hal_ble_evts_t *event, void* context);
    static void processGattServerEvents(const ble_evt_t* event, void* context);

    os_semaphore_t hvxSemaphore_;                   /**< Semaphore to wait until the HVX operation completed. */
    Vector<hal_ble_attr_handle_t> services_;        /**< Added services. */
    Vector<BleCharacteristic> characteristics_;     /**< Added characteristic. */
    AtomicAllocedPool gattsDataRecPool_;            /**< Pool to allocate memory for received data. */
};

class BleObject::GattClient : public GattBase {
public:
    GattClient()
            : isDiscovering_(false),
              currDiscConnHandle_(BLE_INVALID_CONN_HANDLE),
              currDiscProcedure_(DiscoveryProcedure::BLE_DISCOVERY_PROCEDURE_IDLE),
              discoverySemaphore_(nullptr),
              readWriteSemaphore_(nullptr),
              readAttrHandle_(BLE_INVALID_ATTR_HANDLE),
              readBuf_(nullptr),
              readLen_(0),
              attMtuExchangeConnHandle_(BLE_INVALID_CONN_HANDLE),
              attMtuExchangeTimer_(nullptr) {
        resetDiscoveryState();
    }
    ~GattClient() = default;
    int init();
    bool discovering(hal_ble_conn_handle_t connHandle) const;
    int discoverServices(hal_ble_conn_handle_t connHandle, const hal_ble_uuid_t* uuid, on_ble_disc_service_cb_t callback, void* context);
    int discoverCharacteristics(hal_ble_conn_handle_t connHandle, const hal_ble_svc_t* service, on_ble_disc_char_cb_t callback, void* context);
    int setDesiredAttMtu(size_t attMtu);
    ssize_t writeAttribute(hal_ble_conn_handle_t connHandle, hal_ble_attr_handle_t attrHandle, const uint8_t* buf, size_t len, bool response);
    ssize_t readAttribute(hal_ble_conn_handle_t connHandle, hal_ble_attr_handle_t attrHandle, uint8_t* buf, size_t len);

private:
    enum class DiscoveryProcedure {
        BLE_DISCOVERY_PROCEDURE_IDLE = 0,
        BLE_DISCOVERY_PROCEDURE_SERVICES = 1,
        BLE_DISCOVERY_PROCEDURE_CHARACTERISTICS = 2,
        BLE_DISCOVERY_PROCEDURE_DESCRIPTORS = 3,
    };

    struct DiscoveredService {
        DiscoveredService* next;
        hal_ble_svc_t service;
    };

    struct DiscoveredCharacteristic {
        DiscoveredCharacteristic* next;
        hal_ble_char_t characteristic;
    };

    void resetDiscoveryState();
    int addDiscoveredService(const hal_ble_svc_t& service);
    int addDiscoveredCharacteristic(const hal_ble_char_t& characteristic);
    bool readServiceUUID128IfNeeded() const;
    bool readCharacteristicUUID128IfNeeded() const;
    hal_ble_svc_t* findDiscoveredService(hal_ble_attr_handle_t attrHandle);
    hal_ble_char_t* findDiscoveredCharacteristic(hal_ble_attr_handle_t attrHandle);
    static void onAttMtuExchangeTimerExpired(os_timer_t timer);
    static void gattcEventProcessedHook(const hal_ble_evts_t *event, void* context);
    static void processGattClientEvents(const ble_evt_t* event, void* context);

    bool discoverAll_;                                              /**< If it is going to discover all service during the service discovery procedure. */
    bool isDiscovering_;                                            /**< If there is on-going discovery procedure. */
    hal_ble_conn_handle_t currDiscConnHandle_;                      /**< Current connection handle under which the service and characteristics to be discovered. */
    DiscoveryProcedure currDiscProcedure_;                          /**< Current discovery procedure. */
    hal_ble_svc_t currDiscSvc_;                                     /**< Current service to be discovered for the characteristics. */
    os_semaphore_t discoverySemaphore_;                             /**< Semaphore to wait until the discovery procedure completed. */
    os_semaphore_t readWriteSemaphore_;                             /**< Semaphore to wait until the read or write operation completed. */
    AtomicIntrusiveList<DiscoveredService> discServicesList_;       /**< Discover services. */
    AtomicIntrusiveList<DiscoveredCharacteristic> discCharacteristicsList_; /**< Discovered characteristics. */
    volatile size_t discServiceCount_;                              /**< Discovered service count. */
    volatile size_t discCharacteristicCount_;                       /**< Discovered characteristic count. */
    hal_ble_attr_handle_t readAttrHandle_;                          /**< Current handle of which attribute to be read. */
    uint8_t* readBuf_;                                              /**< Current buffer to be filled for the read data. */
    size_t readLen_;                                                /**< Length of read data. */
    AtomicAllocedPool gattcDataRecPool_;                            /**< Pool to allocate memory for received data. */
    AtomicAllocedPool gattcDiscPool_;                               /**< Pool to allocate memory for service/characteristic discovery. */
    volatile hal_ble_conn_handle_t attMtuExchangeConnHandle_;       /**< Current handle of the connection to execute ATT_MTU exchange procedure. */
    os_timer_t attMtuExchangeTimer_;                                /**< Timer used for sending the exchanging ATT_MTU request after connection established. */
};

int BleObject::BleEventDispatcher::init() {
    genericEventHandlers_.clear();
    if (os_queue_create(&evtQueue_, sizeof(EventMessage), BLE_EVENT_QUEUE_ITEM_COUNT, nullptr)) {
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

int BleObject::BleEventDispatcher::enqueue(EventMessage& msg) {
    if (os_queue_put(evtQueue_, &msg, 0, nullptr)) {
        LOG(ERROR, "os_queue_put() failed.");
        // Free the memory if needed.
        if (msg.hook && msg.hookContext) {
            msg.hook(&msg.evt, msg.hookContext);
        }
        return SYSTEM_ERROR_NO_MEMORY;
    }
    return SYSTEM_ERROR_NONE;
}

void BleObject::BleEventDispatcher::onGenericEventCallback(on_ble_evt_cb_t cb, void* context) {
    BleGenericEventHandler evtHandler;
    evtHandler.handler = cb;
    evtHandler.context = context;
    genericEventHandlers_.append(evtHandler);
}

os_thread_return_t BleObject::BleEventDispatcher::bleEventDispatch(void* param) {
    BleEventDispatcher* dispatcher = static_cast<BleEventDispatcher*>(param);
    while (1) {
        EventMessage msg;
        if (!os_queue_take(dispatcher->evtQueue_, &msg, CONCURRENT_WAIT_FOREVER, nullptr)) {
            if (msg.handler) {
                // Invoke the handler registered by specific application.
                if (msg.evt.type == BLE_EVT_SCAN_RESULT) {
                    msg.handler(&msg.evt.params.scan_result, msg.context);
                } else if (msg.evt.type == BLE_EVT_SVC_DISCOVERED) {
                    msg.handler(&msg.evt.params.svc_disc, msg.context);
                } else if (msg.evt.type == BLE_EVT_CHAR_DISCOVERED) {
                    msg.handler(&msg.evt.params.char_disc, msg.context);
                }
            } else {
                // Just dispatch the event to the application those have subscribed the generic events.
                for (auto& evtHandler : dispatcher->genericEventHandlers_) {
                    if (evtHandler.handler) {
                        evtHandler.handler(&msg.evt, evtHandler.context);
                    }
                }
            }
            // Free the memory if needed.
            if (msg.hook) {
                msg.hook(&msg.evt, msg.hookContext);
            }
        } else {
            LOG(ERROR, "BLE event thread exited.");
            break;
        }
    }
    os_thread_exit(dispatcher->evtThread_);
}

struct BleGapImpl {
    BleObject::BleGap* instance;
};
static BleGapImpl bleGapImpl;

int BleObject::BleGap::init() {
    // Set the default device name
    char devName[32] = {};
    get_device_name(devName, sizeof(devName));
    setDeviceName(devName, strlen(devName));
    bleGapImpl.instance = this;
    NRF_SDH_BLE_OBSERVER(bleGap, 1, processBleGapEvents, &bleGapImpl);
    return SYSTEM_ERROR_NONE;
}

int BleObject::BleGap::setDeviceName(const char* deviceName, size_t len) {
    if (deviceName == nullptr || len == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    ble_gap_conn_sec_mode_t secMode;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&secMode);
    int ret = sd_ble_gap_device_name_set(&secMode, (const uint8_t*)deviceName, len);
    CHECK_NRF_RETURN(ret, sysError(ret));
    return SYSTEM_ERROR_NONE;
}

int BleObject::BleGap::getDeviceName(char* deviceName, size_t len) {
    if (deviceName == nullptr || len == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    // non NULL-terminated string returned.
    uint16_t nameLen = len - 1;
    int ret = sd_ble_gap_device_name_get((uint8_t*)deviceName, &nameLen);
    nameLen = std::min(len - 1, (size_t)nameLen);
    deviceName[nameLen] = '\0';
    CHECK_NRF_RETURN(ret, sysError(ret));
    return SYSTEM_ERROR_NONE;
}

int BleObject::BleGap::setDeviceAddress(const hal_ble_addr_t* address) {
    if (BleObject::getInstance().broadcaster()->advertising() ||
            BleObject::getInstance().observer()->scanning() ||
            BleObject::getInstance().connMgr()->connecting(address)) {
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
    CHECK_NRF_RETURN(ret, sysError(ret));
    return SYSTEM_ERROR_NONE;
}

int BleObject::BleGap::getDeviceAddress(hal_ble_addr_t* address) {
    if (address == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    ble_gap_addr_t localAddr;
    int ret = sd_ble_gap_addr_get(&localAddr);
    CHECK_NRF_RETURN(ret, sysError(ret));
    address->addr_type = (ble_sig_addr_type_t)localAddr.addr_type;
    memcpy(address->addr, localAddr.addr, BLE_SIG_ADDR_LEN);
    return SYSTEM_ERROR_NONE;
}

int BleObject::BleGap::setAppearance(ble_sig_appearance_t appearance) {
    int ret = sd_ble_gap_appearance_set((uint16_t)appearance);
    CHECK_NRF_RETURN(ret, sysError(ret));
    return SYSTEM_ERROR_NONE;
}

int BleObject::BleGap::getAppearance(ble_sig_appearance_t* appearance) {
    if (appearance == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    int ret = sd_ble_gap_appearance_get((uint16_t*)appearance);
    CHECK_NRF_RETURN(ret, sysError(ret));
    return SYSTEM_ERROR_NONE;
}

int BleObject::BleGap::addWhitelist(const hal_ble_addr_t* addrList, size_t len) {
    if (addrList == nullptr || len == 0 || len > BLE_MAX_WHITELIST_ADDR_COUNT) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    ble_gap_addr_t* whitelist = (ble_gap_addr_t*)malloc(sizeof(ble_gap_addr_t) * len);
    SCOPE_GUARD ({
        free(whitelist);
    });
    ble_gap_addr_t const* whitelistPointer[BLE_MAX_WHITELIST_ADDR_COUNT];
    for (size_t i = 0; i < len; i++) {
        whitelist[i].addr_id_peer = true;
        whitelist[i].addr_type = addrList[i].addr_type;
        memcpy(whitelist[i].addr, addrList[i].addr, BLE_SIG_ADDR_LEN);
        whitelistPointer[i] = &whitelist[i];
    }
    int ret = sd_ble_gap_whitelist_set(whitelistPointer, len);
    CHECK_NRF_RETURN(ret, sysError(ret));
    return SYSTEM_ERROR_NONE;
}

int BleObject::BleGap::deleteWhitelist() {
    int ret = sd_ble_gap_whitelist_set(nullptr, 0);
    CHECK_NRF_RETURN(ret, sysError(ret));
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
            break;
        }
        case BLE_GAP_EVT_PHY_UPDATE: {
            LOG_DEBUG(TRACE, "BLE GAP event: physical updated.");
            break;
        }
        case BLE_GAP_EVT_DATA_LENGTH_UPDATE: {
            LOG_DEBUG(TRACE, "BLE GAP event: gap data length updated.");
            LOG_DEBUG(TRACE, "| txo    rxo     txt(us)     rxt(us) |.");
            LOG_DEBUG(TRACE, "  %d    %d     %d        %d",
                    event->evt.gap_evt.params.data_length_update.effective_params.max_tx_octets, event->evt.gap_evt.params.data_length_update.effective_params.max_rx_octets,
                    event->evt.gap_evt.params.data_length_update.effective_params.max_tx_time_us, event->evt.gap_evt.params.data_length_update.effective_params.max_rx_time_us);
            break;
        }
        case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST: {
            LOG_DEBUG(TRACE, "BLE GAP event: gap data length update request.");
            ble_gap_data_length_params_t gapDataLenParams;
            gapDataLenParams.max_tx_octets  = BLE_GAP_DATA_LENGTH_AUTO;
            gapDataLenParams.max_rx_octets  = BLE_GAP_DATA_LENGTH_AUTO;
            gapDataLenParams.max_tx_time_us = BLE_GAP_DATA_LENGTH_AUTO;
            gapDataLenParams.max_rx_time_us = BLE_GAP_DATA_LENGTH_AUTO;
            ret = sd_ble_gap_data_length_update(event->evt.gap_evt.conn_handle, &gapDataLenParams, nullptr);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gap_data_length_update() failed: %u", (unsigned)ret);
            }
            break;
        }
        case BLE_GAP_EVT_SEC_PARAMS_REQUEST: {
            LOG_DEBUG(TRACE, "BLE GAP event: security parameters request.");
            // Pairing is not supported
            ret = sd_ble_gap_sec_params_reply(event->evt.gap_evt.conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, nullptr, nullptr);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gap_sec_params_reply() failed: %u", (unsigned)ret);
            }
            break;
        }
        case BLE_GAP_EVT_AUTH_STATUS: {
            LOG_DEBUG(TRACE, "BLE GAP event: authentication status updated.");
            if (event->evt.gap_evt.params.auth_status.auth_status == BLE_GAP_SEC_STATUS_SUCCESS) {
                LOG_DEBUG(TRACE, "Authentication succeeded");
            } else {
                LOG_DEBUG(WARN, "Authentication failed, status: %d", (int)event->evt.gap_evt.params.auth_status.auth_status);
            }
            break;
        }
        case BLE_GAP_EVT_TIMEOUT: {
            if (event->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_AUTH_PAYLOAD) {
                LOG_DEBUG(ERROR, "BLE GAP event: Authenticated payload timeout");
            }
            break;
        }
        default: {
            break;
        }
    }
}

/* FIXME: It causes a section type conflict if using NRF_SDH_BLE_OBSERVER() in c++ class.*/
struct BroadcasterImpl {
    BleObject::Broadcaster* instance;
};
static BroadcasterImpl broadcasterImpl;

BleObject::Broadcaster::Broadcaster()
        : isAdvertising_(false),
          autoAdvCfg_(BLE_AUTO_ADV_ALWAYS),
          advHandle_(BLE_GAP_ADV_SET_HANDLE_NOT_SET),
          txPower_(0),
          advPending_(false),
          connectedAdvParams_(false),
          connHandle_(BLE_INVALID_CONN_HANDLE) {
    /* Default advertising parameters. */
    advParams_.version = BLE_API_VERSION;
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
    advData_[advDataLen_++] = 0x02; // Length field of an AD structure, it is the length of the rest AD structure data.
    advData_[advDataLen_++] = BLE_SIG_AD_TYPE_FLAGS; // Type field of an AD structure.
    advData_[advDataLen_++] = BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE; // Payload field of an AD structure.
}

int BleObject::Broadcaster::init() {
    CHECK(configure(&advParams_));
    setTxPower(txPower_);
    broadcasterImpl.instance = this;
    NRF_SDH_BLE_OBSERVER(bleBroadcaster, 1, processBroadcasterEvents, &broadcasterImpl);
    return SYSTEM_ERROR_NONE;
}

bool BleObject::Broadcaster::advertising() const {
    return isAdvertising_;
}

int BleObject::Broadcaster::setAdvertisingParams(const hal_ble_adv_params_t* params) {
    if (params == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    CHECK(suspend());
    if (connHandle_ != BLE_INVALID_CONN_HANDLE) {
        hal_ble_adv_params_t connectedAdvParams = *params;
        connectedAdvParams.type = BLE_ADV_SCANABLE_UNDIRECTED_EVT;
        CHECK(configure(&connectedAdvParams));
        advParams_.size = sizeof(hal_ble_adv_params_t);
        memcpy(&advParams_, params, advParams_.size);
        advParams_.version = BLE_API_VERSION;
        connectedAdvParams_ = true; // Set the flag after the advParams_ being updated.
    } else {
        CHECK(configure(params));
        advParams_.size = sizeof(hal_ble_adv_params_t);
        memcpy(&advParams_, params, advParams_.size);
        advParams_.version = BLE_API_VERSION;
    }
    return resume();
}

int BleObject::Broadcaster::getAdvertisingParams(hal_ble_adv_params_t* params) const {
    if (params == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    memcpy(params, &advParams_, params->size);
    return SYSTEM_ERROR_NONE;
}

int BleObject::Broadcaster::setAdvertisingData(const uint8_t* buf, size_t len) {
    // It is invalid to provide the same data buffers while advertising.
    CHECK(suspend());
    if (buf != nullptr) {
        len = std::min(len, (size_t)BLE_MAX_ADV_DATA_LEN);
        memcpy(advData_, buf, len);
    } else {
        len = 0;
    }
    advDataLen_ = len;
    CHECK(configure(nullptr));
    return resume();
}

ssize_t BleObject::Broadcaster::getAdvertisingData(uint8_t* buf, size_t len) const {
    if (buf == nullptr) {
        return advDataLen_;
    }
    len = std::min(len, advDataLen_);
    memcpy(buf, advData_, len);
    return len;
}

int BleObject::Broadcaster::setScanResponseData(const uint8_t* buf, size_t len) {
    // It is invalid to provide the same data buffers while advertising.
    CHECK(suspend());
    if (buf != nullptr) {
        len = std::min(len, (size_t)BLE_MAX_ADV_DATA_LEN);
        memcpy(scanRespData_, buf, len);
    } else {
        len = 0;
    }
    scanRespDataLen_ = len;
    CHECK(configure(nullptr));
    return resume();
}

ssize_t BleObject::Broadcaster::getScanResponseData(uint8_t* buf, size_t len) const {
    if (buf == nullptr) {
        return scanRespDataLen_;
    }
    len = std::min(len, scanRespDataLen_);
    memcpy(buf, scanRespData_, len);
    return len;
}

int BleObject::Broadcaster::setTxPower(int8_t val) {
    val = roundTxPower(val);
    int ret = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, advHandle_, val);
    CHECK_NRF_RETURN(ret, sysError(ret));
    txPower_ = val;
    return SYSTEM_ERROR_NONE;
}

int BleObject::Broadcaster::getTxPower(int8_t* txPower) const {
    if (txPower == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    *txPower = txPower_;
    return SYSTEM_ERROR_NONE;
}

int BleObject::Broadcaster::startAdvertising() {
    CHECK(stopAdvertising());
    if (connHandle_ != BLE_INVALID_CONN_HANDLE) {
        // It is connected as Peripheral, the advertising event should be set to non-connectable.
        if (!connectedAdvParams_) {
            hal_ble_adv_params_t connectedAdvParams = advParams_;
            connectedAdvParams.type = BLE_ADV_SCANABLE_UNDIRECTED_EVT;
            CHECK(configure(&connectedAdvParams));
            connectedAdvParams_ = true;
        }
    } else {
        // Restores the advertising parameters set by user.
        if (connectedAdvParams_) {
            CHECK(configure(&advParams_));
            connectedAdvParams_ = false;
        }
    }
    int ret = sd_ble_gap_adv_start(advHandle_, BLE_CONN_CFG_TAG);
    CHECK_NRF_RETURN(ret, sysError(ret));
    isAdvertising_ = true;
    return SYSTEM_ERROR_NONE;
}

int BleObject::Broadcaster::stopAdvertising() {
    if (isAdvertising_) {
        int ret = sd_ble_gap_adv_stop(advHandle_);
        CHECK_NRF_RETURN(ret, sysError(ret));
        isAdvertising_ = false;
    }
    return SYSTEM_ERROR_NONE;
}

int BleObject::Broadcaster::setAutoAdvertiseScheme(hal_ble_auto_adv_cfg_t config) {
    autoAdvCfg_ = config;
    if (connHandle_ == BLE_INVALID_CONN_HANDLE && autoAdvCfg_ == BLE_AUTO_ADV_SINCE_NEXT_CONN) {
        // If it is not connected as Peripheral currently.
        autoAdvCfg_ = BLE_AUTO_ADV_ALWAYS;
    }
    return SYSTEM_ERROR_NONE;
}

hal_ble_auto_adv_cfg_t BleObject::Broadcaster::getAutoAdvertiseScheme() {
    return autoAdvCfg_;
}

int BleObject::Broadcaster::suspend() {
    advPending_ = false;
    if (isAdvertising_) {
        CHECK(stopAdvertising());
        advPending_ = true;
    }
    return SYSTEM_ERROR_NONE;
}

int BleObject::Broadcaster::resume() {
    if (advPending_) {
        CHECK(startAdvertising());
    }
    advPending_ = false;
    return SYSTEM_ERROR_NONE;
}

ble_gap_adv_params_t BleObject::Broadcaster::toPlatformAdvParams(const hal_ble_adv_params_t* halParams) {
    ble_gap_adv_params_t params = {};
    params.properties.type = BleAdvEvtTypeMap[halParams->type];
    params.properties.include_tx_power = false; // FIXME: for extended advertising packet
    params.p_peer_addr = nullptr;
    params.interval = halParams->interval;
    params.duration = halParams->timeout;
    params.filter_policy = halParams->filter_policy;
    params.primary_phy = BLE_GAP_PHY_1MBPS;
    return params;
}

ble_gap_adv_data_t BleObject::Broadcaster::toPlatformAdvData(void) {
    ble_gap_adv_data_t advData = {};
    advData.adv_data.p_data = advData_;
    advData.adv_data.len = advDataLen_;
    advData.scan_rsp_data.p_data = scanRespData_;
    advData.scan_rsp_data.len = scanRespDataLen_;
    return advData;
}

int BleObject::Broadcaster::configure(const hal_ble_adv_params_t* params) {
    int ret;
    ble_gap_adv_data_t bleGapAdvData = toPlatformAdvData();
    if (params == nullptr) {
        ret = sd_ble_gap_adv_set_configure(&advHandle_, &bleGapAdvData, nullptr);
    } else {
        ble_gap_adv_params_t bleGapAdvParams = toPlatformAdvParams(params);
        LOG_DEBUG(TRACE, "BLE advertising interval: %.3fms, timeout: %dms.",
                  bleGapAdvParams.interval*0.625, bleGapAdvParams.duration*10);
        ret = sd_ble_gap_adv_set_configure(&advHandle_, &bleGapAdvData, &bleGapAdvParams);
    }
    CHECK_NRF_RETURN(ret, sysError(ret));
    return SYSTEM_ERROR_NONE;
}

int8_t BleObject::Broadcaster::roundTxPower(int8_t value) {
    for (const auto& txPower : validTxPower_) {
        if (value <= txPower) {
            return txPower;
        }
    }
    return BLE_MAX_TX_POWER;
}

void BleObject::Broadcaster::processBroadcasterEvents(const ble_evt_t* event, void* context) {
    Broadcaster* broadcaster = static_cast<BroadcasterImpl*>(context)->instance;
    switch (event->header.evt_id) {
        case BLE_GAP_EVT_ADV_SET_TERMINATED: {
            LOG_DEBUG(TRACE, "BLE GAP event: advertising stopped.");
            broadcaster->isAdvertising_ = false;
            BleObject::BleEventDispatcher::EventMessage msg;
            msg.evt.version = BLE_API_VERSION;
            msg.evt.size = sizeof(hal_ble_evts_t);
            msg.evt.type = BLE_EVT_ADV_STOPPED;
            msg.evt.params.adv_stopped.reserved = nullptr;
            BleObject::getInstance().dispatcher()->enqueue(msg);
            break;
        }
        case BLE_GAP_EVT_CONNECTED: {
            // When connected as Peripheral, stop advertising automatically.
            // When connected as Central, nothing should change.
            if (event->evt.gap_evt.params.connected.role == BLE_ROLE_PERIPHERAL) {
                broadcaster->connHandle_ = event->evt.gap_evt.conn_handle;
                broadcaster->isAdvertising_ = false;
            }
            break;
        }
        case BLE_GAP_EVT_DISCONNECTED: {
            if (broadcaster->connHandle_ == event->evt.gap_evt.conn_handle) {
                // The connection handle must be cleared before re-start advertising.
                // Otherwise, it cannot restore the normal advertising parameters.
                broadcaster->connHandle_ = BLE_INVALID_CONN_HANDLE;
                if (broadcaster->isAdvertising_) {
                    LOG_DEBUG(TRACE, "Restart BLE advertising.");
                    broadcaster->startAdvertising();
                } else {
                    if (broadcaster->autoAdvCfg_ == BLE_AUTO_ADV_FORBIDDEN) {
                        return;
                    } else if (broadcaster->autoAdvCfg_ == BLE_AUTO_ADV_SINCE_NEXT_CONN) {
                        broadcaster->autoAdvCfg_ = BLE_AUTO_ADV_ALWAYS;
                        return;
                    } else {
                        LOG_DEBUG(TRACE, "Restart BLE advertising.");
                        broadcaster->startAdvertising();
                    }
                }
            }
            break;
        }
        default: {
            break;
        }
    }
}

const int8_t BleObject::Broadcaster::validTxPower_[8] = { -20, -16, -12, -8, -4, 0, 4, BLE_MAX_TX_POWER };

struct ObserverImpl {
    BleObject::Observer* instance;
};
static ObserverImpl observerImpl;

int BleObject::Observer::init() {
    scanParams_.size = sizeof(hal_ble_scan_params_t);
    observerScannedDataPool_.init(OBSERVER_SCANNED_DATA_POOL_SIZE);
    observerImpl.instance = this;
    NRF_SDH_BLE_OBSERVER(bleObserver, 1, processObserverEvents, &observerImpl);
    return SYSTEM_ERROR_NONE;
}

bool BleObject::Observer::scanning() {
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
    scanParams_.size = sizeof(hal_ble_scan_params_t);
    memcpy(&scanParams_, params, scanParams_.size);
    scanParams_.version = BLE_API_VERSION;
    return SYSTEM_ERROR_NONE;
}

int BleObject::Observer::getScanParams(hal_ble_scan_params_t* params) const {
    if (params == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    memcpy(params, &scanParams_, params->size);
    return SYSTEM_ERROR_NONE;
}

int BleObject::Observer::continueScanning() {
    int ret = sd_ble_gap_scan_start(nullptr, &bleScanData_);
    CHECK_NRF_RETURN(ret, sysError(ret));
    return SYSTEM_ERROR_NONE;
}

int BleObject::Observer::startScanning(on_ble_scan_result_cb_t callback, void* context) {
    if (isScanning_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (os_semaphore_create(&scanSemaphore_, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }
    SCOPE_GUARD ({
        os_semaphore_destroy(scanSemaphore_);
        scanSemaphore_ = nullptr;
    });
    ble_gap_scan_params_t bleGapScanParams = toPlatformScanParams();
    LOG_DEBUG(TRACE, "| interval(ms)   window(ms)   timeout(ms) |");
    LOG_DEBUG(TRACE, "  %.3f        %.3f      %d",
            bleGapScanParams.interval*0.625, bleGapScanParams.window*0.625, bleGapScanParams.timeout*10);
    scanResultCallBack_ = callback;
    context_ = context;
    int ret = sd_ble_gap_scan_start(&bleGapScanParams, &bleScanData_);
    CHECK_NRF_RETURN(ret, sysError(ret));
    isScanning_ = true;
    ret = SYSTEM_ERROR_NONE;
    if (os_semaphore_take(scanSemaphore_, CONCURRENT_WAIT_FOREVER, false)) {
        ret = SYSTEM_ERROR_TIMEOUT;
    }
    isScanning_ = false;
    clearCachedDevice();
    clearPendingResult();
    return ret;
}

int BleObject::Observer::stopScanning() {
    if (!isScanning_) {
        return SYSTEM_ERROR_NONE;
    }
    int ret = sd_ble_gap_scan_stop();
    CHECK_NRF_RETURN(ret, sysError(ret));
    if (scanSemaphore_) {
        os_semaphore_give(scanSemaphore_, false);
    }
    return SYSTEM_ERROR_NONE;
}

ble_gap_scan_params_t BleObject::Observer::toPlatformScanParams(void) const {
    ble_gap_scan_params_t params = {};
    params.extended = 0;
    params.active = scanParams_.active;
    params.interval = scanParams_.interval;
    params.window = scanParams_.window;
    params.timeout = scanParams_.timeout;
    params.scan_phys = BLE_GAP_PHY_1MBPS;
    params.filter_policy = scanParams_.filter_policy;
    return params;
}

bool BleObject::Observer::isCachedDevice(const hal_ble_addr_t& address) const {
    auto device = cachedDevicesList_.front();
    while (device) {
        if (addressEqual(device->addr, address)) {
            return true;
        }
        device = device->next;
    }
    return false;
}

int BleObject::Observer::addCachedDevice(const hal_ble_addr_t& address) {
    cachedDevice* device = (cachedDevice*)scanCacheDevicesPool_.alloc(sizeof(cachedDevice));
    if (device) {
        device->addr = address;
        cachedDevicesList_.pushFront(device);
        return SYSTEM_ERROR_NONE;
    }
    return SYSTEM_ERROR_NO_MEMORY;
}

void BleObject::Observer::clearCachedDevice() {
    cachedDevice* device = nullptr;
    do {
        device = cachedDevicesList_.popFront();
        if (device) {
            scanCacheDevicesPool_.free(device);
        }
    } while (device);
}

hal_ble_gap_on_scan_result_evt_t* BleObject::Observer::getPendingResult(const hal_ble_addr_t& address) {
    auto result = pendingResultsList_.front();
    while (result) {
        if (addressEqual(result->resultEvt.peer_addr, address)) {
            return &result->resultEvt;
        }
        result = result->next;
    }
    return nullptr;
}

int BleObject::Observer::addPendingResult(const hal_ble_gap_on_scan_result_evt_t& resultEvt) {
    pendingResult* result = (pendingResult*)scanPendingResultsPool_.alloc(sizeof(pendingResult));
    if (result) {
        memcpy(&result->resultEvt, &resultEvt, sizeof(hal_ble_gap_on_scan_result_evt_t));
        pendingResultsList_.pushFront(result);
        return SYSTEM_ERROR_NONE;
    }
    return SYSTEM_ERROR_NO_MEMORY;
}

void BleObject::Observer::removePendingResult(const hal_ble_addr_t& address) {
    auto result = pendingResultsList_.front();
    pendingResult* prev = nullptr;
    while (result) {
        if (addressEqual(result->resultEvt.peer_addr, address)) {
            pendingResult* curr = pendingResultsList_.pop(result, prev);
            result = curr->next;
            scanPendingResultsPool_.free(curr);
            return;
        }
        prev = result;
        result = result->next;
    }
}

void BleObject::Observer::clearPendingResult() {
    pendingResult* result = nullptr;
    do {
        result = pendingResultsList_.popFront();
        if (result) {
            if (result->resultEvt.adv_data != nullptr) {
                observerScannedDataPool_.free(result->resultEvt.adv_data);
                LOG_DEBUG(TRACE, "clearPendingResult: Free Scanned adv_data memory: %u", (unsigned)result->resultEvt.adv_data);
            }
            scanPendingResultsPool_.free(result);
        }
    } while (result);
}

void BleObject::Observer::observerEventProcessedHook(const hal_ble_evts_t *event, void* context) {
    Observer* observer = static_cast<Observer*>(context);
    if (observer && event && event->type == BLE_EVT_SCAN_RESULT) {
        if (event->params.scan_result.adv_data != nullptr) {
            observer->observerScannedDataPool_.free(event->params.scan_result.adv_data);
            LOG_DEBUG(TRACE, "Hook: Free Scanned adv_data memory: %u", (unsigned)event->params.scan_result.adv_data);
        }
        if (event->params.scan_result.sr_data != nullptr) {
            observer->observerScannedDataPool_.free(event->params.scan_result.sr_data);
            LOG_DEBUG(TRACE, "Hook: Free Scanned sr_data memory: %u", (unsigned)event->params.scan_result.sr_data);
        }
    }
}

void BleObject::Observer::processObserverEvents(const ble_evt_t* event, void* context) {
    Observer* observer = static_cast<ObserverImpl*>(context)->instance;
    switch (event->header.evt_id) {
        case BLE_GAP_EVT_ADV_REPORT: {
            const ble_gap_evt_adv_report_t& advReport = event->evt.gap_evt.params.adv_report;
            hal_ble_addr_t newAddr;
            newAddr.addr_type = (ble_sig_addr_type_t)advReport.peer_addr.addr_type;
            memcpy(newAddr.addr, advReport.peer_addr.addr, BLE_SIG_ADDR_LEN);
            if (!observer->isCachedDevice(newAddr)) {
                if (!observer->scanParams_.active || !advReport.type.scannable) {
                    if (observer->addCachedDevice(newAddr) != SYSTEM_ERROR_NONE) {
                        LOG(ERROR, "There will have no memory to cache more devices.");
                    }
                    uint8_t* advData = (uint8_t*)observer->observerScannedDataPool_.alloc(advReport.data.len);
                    if (advData) {
                        LOG_DEBUG(TRACE, "Scanned adv_data address: %u, len: %u", (unsigned)advData, advReport.data.len);
                        BleObject::BleEventDispatcher::EventMessage msg;
                        msg.evt.type = BLE_EVT_SCAN_RESULT;
                        msg.evt.version = BLE_API_VERSION;
                        msg.evt.size = sizeof(hal_ble_evts_t);
                        msg.evt.params.scan_result.type.connectable = advReport.type.connectable;
                        msg.evt.params.scan_result.type.scannable = advReport.type.scannable;
                        msg.evt.params.scan_result.type.directed = advReport.type.directed;
                        msg.evt.params.scan_result.type.extended_pdu = advReport.type.extended_pdu;
                        msg.evt.params.scan_result.rssi = advReport.rssi;
                        msg.evt.params.scan_result.peer_addr.addr_type = (ble_sig_addr_type_t)advReport.peer_addr.addr_type;
                        memcpy(msg.evt.params.scan_result.peer_addr.addr, advReport.peer_addr.addr, BLE_SIG_ADDR_LEN);
                        msg.evt.params.scan_result.adv_data_len = advReport.data.len;
                        memcpy(advData, advReport.data.p_data, advReport.data.len);
                        msg.evt.params.scan_result.adv_data = advData;
                        msg.handler = (BleObject::BleEventDispatcher::BleSpecificEventHandler)observer->scanResultCallBack_;
                        msg.context = observer->context_;
                        msg.hook = observer->observerEventProcessedHook;
                        msg.hookContext = observer;
                        BleObject::getInstance().dispatcher()->enqueue(msg);
                    }
                } else if (advReport.type.scannable) {
                    if (!advReport.type.scan_response) {
                        if (observer->getPendingResult(newAddr) == nullptr) {
                            uint8_t* advData = (uint8_t*)observer->observerScannedDataPool_.alloc(advReport.data.len);
                            if (advData) {
                                LOG_DEBUG(TRACE, "Cached adv_data address: %u, len: %u", (unsigned)advData, advReport.data.len);
                                hal_ble_gap_on_scan_result_evt_t result = {};
                                result.type.connectable = advReport.type.connectable;
                                result.type.scannable = advReport.type.scannable;
                                result.type.directed = advReport.type.directed;
                                result.type.extended_pdu = advReport.type.extended_pdu;
                                result.rssi = advReport.rssi;
                                result.peer_addr.addr_type = newAddr.addr_type;
                                memcpy(result.peer_addr.addr, newAddr.addr, BLE_SIG_ADDR_LEN);
                                result.adv_data_len = advReport.data.len;
                                memcpy(advData, advReport.data.p_data, advReport.data.len);
                                result.adv_data = advData;
                                if (observer->addPendingResult(result) != SYSTEM_ERROR_NONE) {
                                    LOG(ERROR, "There will have no memory to store pending results.");
                                }
                            }
                        }
                    } else {
                        hal_ble_gap_on_scan_result_evt_t* result = observer->getPendingResult(newAddr);
                        if (result) {
                            result->rssi = advReport.rssi;
                            result->sr_data = nullptr;
                            result->sr_data_len = advReport.data.len;
                            if (advReport.data.len > 0) {
                                uint8_t* srData = (uint8_t*)observer->observerScannedDataPool_.alloc(advReport.data.len);
                                if (srData) {
                                    LOG_DEBUG(TRACE, "Scanned sr_data address: %u, len: %u", (unsigned)srData, advReport.data.len);
                                    memcpy(srData, advReport.data.p_data, advReport.data.len);
                                    result->sr_data = srData;
                                } else {
                                    result->sr_data_len = 0;
                                }
                            }
                            BleObject::BleEventDispatcher::EventMessage msg;
                            msg.evt.type = BLE_EVT_SCAN_RESULT;
                            msg.evt.params.scan_result = *result;
                            msg.handler = (BleObject::BleEventDispatcher::BleSpecificEventHandler)observer->scanResultCallBack_;
                            msg.context = observer->context_;
                            msg.hook = observer->observerEventProcessedHook;
                            msg.hookContext = observer;
                            BleObject::getInstance().dispatcher()->enqueue(msg);
                            observer->removePendingResult(newAddr);
                            if (observer->addCachedDevice(newAddr) != SYSTEM_ERROR_NONE) {
                                LOG(ERROR, "There will have no memory to cache more devices.");
                            }
                        }
                    }
                }
            }
            // Continue scanning
            observer->continueScanning();
            break;
        }
        case BLE_GAP_EVT_TIMEOUT: {
            if (event->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_SCAN) {
                LOG_DEBUG(TRACE, "BLE GAP event: Scanning timeout");
                if (observer->scanSemaphore_) {
                    os_semaphore_give(observer->scanSemaphore_, false);
                }
            }
            break;
        }
        default: {
            break;
        }
    }
}

struct ConnectionsManagerImpl {
    BleObject::ConnectionsManager* instance;
};
static ConnectionsManagerImpl connMgrImpl;

int BleObject::ConnectionsManager::init() {
    /*
     * Default Peripheral Preferred Connection Parameters.
     * For BLE Central, this is the initial connection parameters.
     * For BLE Peripheral, this is the Peripheral Preferred Connection Parameters.
     */
    ble_gap_conn_params_t bleGapConnParams = {};
    bleGapConnParams.min_conn_interval = BLE_DEFAULT_MIN_CONN_INTERVAL;
    bleGapConnParams.max_conn_interval = BLE_DEFAULT_MAX_CONN_INTERVAL;
    bleGapConnParams.slave_latency = BLE_DEFAULT_SLAVE_LATENCY;
    bleGapConnParams.conn_sup_timeout = BLE_DEFAULT_CONN_SUP_TIMEOUT;
    int ret = sd_ble_gap_ppcp_set(&bleGapConnParams);
    CHECK_NRF_RETURN(ret, sysError(ret));
    if (os_timer_create(&connParamsUpdateTimer_, BLE_CONN_PARAMS_UPDATE_DELAY_MS, onConnParamsUpdateTimerExpired, this, true, nullptr)) {
        LOG(ERROR, "os_timer_create() failed.");
        connParamsUpdateTimer_ = nullptr;
        return SYSTEM_ERROR_INTERNAL;
    }
    connMgrImpl.instance = this;
    NRF_SDH_BLE_OBSERVER(bleConnectionManager, 1, processConnectionEvents, &connMgrImpl);
    return SYSTEM_ERROR_NONE;
}

int BleObject::ConnectionsManager::setPpcp(const hal_ble_conn_params_t* ppcp) {
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
    ble_gap_conn_params_t bleGapConnParams = toPlatformConnParams(ppcp);
    int ret = sd_ble_gap_ppcp_set(&bleGapConnParams);
    CHECK_NRF_RETURN(ret, sysError(ret));
    return SYSTEM_ERROR_NONE;
}

int BleObject::ConnectionsManager::getPpcp(hal_ble_conn_params_t* ppcp) const {
    if (ppcp == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    ble_gap_conn_params_t bleGapConnParams = {};
    int ret = sd_ble_gap_ppcp_get(&bleGapConnParams);
    CHECK_NRF_RETURN(ret, sysError(ret));
    *ppcp = toHalConnParams(&bleGapConnParams);
    return SYSTEM_ERROR_NONE;
}

bool BleObject::ConnectionsManager::connecting(const hal_ble_addr_t* address) const {
    // TODO: several links are being established concurrently.
    return isConnecting_;
}

bool BleObject::ConnectionsManager::connected(const hal_ble_addr_t* address) {
    if (address == nullptr) {
        // Check if local device is connected as BLE Peripheral.
        auto connection = connectionsList_.front();
        while (connection) {
            if (connection->role == BLE_ROLE_PERIPHERAL) {
                return true;
            }
            connection = connection->next;
        }
        return false;
    } else if (fetchConnection(address)) {
        return true;
    }
    return false;
}

int BleObject::ConnectionsManager::connect(const hal_ble_addr_t* address) {
    // Stop scanning first to give the scanning semaphore if possible.
    int ret;
    CHECK(BleObject::getInstance().observer()->stopScanning());
    if (os_semaphore_create(&connectSemaphore_, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }
    SCOPE_GUARD ({
        os_semaphore_destroy(connectSemaphore_);
        connectSemaphore_ = nullptr;
    });
    ble_gap_addr_t bleDevAddr = {};
    bleDevAddr.addr_type = address->addr_type;
    memcpy(bleDevAddr.addr, address->addr, BLE_SIG_ADDR_LEN);
    ble_gap_scan_params_t bleGapScanParams = BleObject::getInstance().observer()->toPlatformScanParams();
    ble_gap_conn_params_t bleGapConnParams = {};
    ret = sd_ble_gap_ppcp_get(&bleGapConnParams);
    CHECK_NRF_RETURN(ret, sysError(ret));
    ret = sd_ble_gap_connect(&bleDevAddr, &bleGapScanParams, &bleGapConnParams, BLE_CONN_CFG_TAG);
    if (ret != NRF_SUCCESS) {
        CHECK_NRF_RETURN(ret, sysError(ret));
    }
    isConnecting_ = true;
    memcpy(&connectingAddr_, address, sizeof(hal_ble_addr_t));
    ret = SYSTEM_ERROR_NONE;
    if (os_semaphore_take(connectSemaphore_, CONNECTION_OPERATION_TIMEOUT_MS, false)) {
        ret = SYSTEM_ERROR_TIMEOUT;
    }
    isConnecting_ = false;
    connectingAddr_ = {};
    return ret;
}

int BleObject::ConnectionsManager::connectCancel(const hal_ble_addr_t* address) {
    if (isConnecting_) {
        int ret = sd_ble_gap_connect_cancel();
        CHECK_NRF_RETURN(ret, sysError(ret));
        if (connectSemaphore_) {
            os_semaphore_give(connectSemaphore_, false);
        }
    }
    return SYSTEM_ERROR_NONE;
}

int BleObject::ConnectionsManager::disconnect(hal_ble_conn_handle_t connHandle) {
    if (fetchConnection(connHandle) == nullptr) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    if (os_semaphore_create(&disconnectSemaphore_, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }
    SCOPE_GUARD ({
        os_semaphore_destroy(disconnectSemaphore_);
        disconnectSemaphore_ = nullptr;
    });
    int ret = sd_ble_gap_disconnect(connHandle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    if (ret != NRF_SUCCESS) {
        CHECK_NRF_RETURN(ret, sysError(ret));
    }
    disconnectingHandle_ = connHandle;
    ret = SYSTEM_ERROR_NONE;
    if (os_semaphore_take(disconnectSemaphore_, CONNECTION_OPERATION_TIMEOUT_MS, false)) {
        ret = SYSTEM_ERROR_TIMEOUT;
    }
    disconnectingHandle_ = BLE_INVALID_CONN_HANDLE;
    return ret;
}

int BleObject::ConnectionsManager::updateConnectionParams(hal_ble_conn_handle_t connHandle, const hal_ble_conn_params_t* params) {
    int ret;
    if (fetchConnection(connHandle) == nullptr) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    if (os_semaphore_create(&connParamsUpdateSemaphore_, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }
    SCOPE_GUARD ({
        os_semaphore_destroy(connParamsUpdateSemaphore_);
        connParamsUpdateSemaphore_ = nullptr;
    });
    ble_gap_conn_params_t bleGapConnParams = {};
    if (params == nullptr) {
        // Use the PPCP characteristic value as the connection parameters.
        ret = sd_ble_gap_ppcp_get(&bleGapConnParams);
        CHECK_NRF_RETURN(ret, sysError(ret));
    } else {
        bleGapConnParams = toPlatformConnParams(params);
    }
    // For Central role, this will initiate the connection parameter update procedure.
    // For Peripheral role, this will use the passed in parameters and send the request to central.
    ret = sd_ble_gap_conn_param_update(connHandle, &bleGapConnParams);
    if (ret != NRF_SUCCESS) {
        CHECK_NRF_RETURN(ret, sysError(ret));
    }
    connParamUpdateHandle_ = connHandle;
    ret = SYSTEM_ERROR_NONE;
    if (os_semaphore_take(connParamsUpdateSemaphore_, CONNECTION_OPERATION_TIMEOUT_MS, false)) {
        ret = SYSTEM_ERROR_TIMEOUT;
    }
    connParamUpdateHandle_ = BLE_INVALID_CONN_HANDLE;
    return ret;
}

int BleObject::ConnectionsManager::getConnectionParams(hal_ble_conn_handle_t connHandle, hal_ble_conn_params_t* params) {
    const BleConnection* connection = fetchConnection(connHandle);
    if (connection == nullptr) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    memcpy(params, &connection->effectiveConnParams, params->size);
    return SYSTEM_ERROR_NONE;
}

bool BleObject::ConnectionsManager::valid(hal_ble_conn_handle_t connHandle) {
    const BleConnection* connection = fetchConnection(connHandle);
    return connection != nullptr;
}

ble_gap_conn_params_t BleObject::ConnectionsManager::toPlatformConnParams(const hal_ble_conn_params_t* halConnParams) {
    ble_gap_conn_params_t params = {};
    params.min_conn_interval = halConnParams->min_conn_interval;
    params.max_conn_interval = halConnParams->max_conn_interval;
    params.slave_latency = halConnParams->slave_latency;
    params.conn_sup_timeout = halConnParams->conn_sup_timeout;
    return params;
}

hal_ble_conn_params_t BleObject::ConnectionsManager::toHalConnParams(const ble_gap_conn_params_t* params) {
    hal_ble_conn_params_t halConnParams = {};
    halConnParams.version = BLE_API_VERSION;
    halConnParams.size = sizeof(hal_ble_conn_params_t);
    halConnParams.min_conn_interval = params->min_conn_interval;
    halConnParams.max_conn_interval = params->max_conn_interval;
    halConnParams.slave_latency = params->slave_latency;
    halConnParams.conn_sup_timeout = params->conn_sup_timeout;
    return halConnParams;
}

BleObject::ConnectionsManager::BleConnection* BleObject::ConnectionsManager::fetchConnection(hal_ble_conn_handle_t connHandle) {
    if (connHandle == BLE_INVALID_CONN_HANDLE) {
        return nullptr;
    }
    auto connection = connectionsList_.front();
    while (connection) {
        if (connection->connHandle == connHandle) {
            return connection;
        }
        connection = connection->next;
    }
    return nullptr;
}

BleObject::ConnectionsManager::BleConnection* BleObject::ConnectionsManager::fetchConnection(const hal_ble_addr_t* address) {
    if (address == nullptr) {
        return nullptr;
    }
    auto connection = connectionsList_.front();
    while (connection) {
        if (addressEqual(connection->peer, *address)) {
            return connection;
        }
        connection = connection->next;
    }
    return nullptr;
}

int BleObject::ConnectionsManager::addConnection(const BleConnection& connection) {
    removeConnection(connection.connHandle);
    BleConnection* conn = (BleConnection*)connectionsPool_.alloc(sizeof(BleConnection));
    if (conn) {
        *conn = connection;
        connectionsList_.pushFront(conn);
        return SYSTEM_ERROR_NONE;
    }
    return SYSTEM_ERROR_NO_MEMORY;
}

void BleObject::ConnectionsManager::removeConnection(hal_ble_conn_handle_t connHandle) {
    auto connection = connectionsList_.front();
    BleConnection* prev = nullptr;
    while (connection) {
        if (connection->connHandle == connHandle) {
            BleConnection* curr = connectionsList_.pop(connection, prev);
            connection = curr->next;
            connectionsPool_.free(curr);
            continue;
        }
        prev = connection;
        connection = connection->next;
    }
}

void BleObject::ConnectionsManager::initiateConnParamsUpdateIfNeeded(const BleConnection* connection) {
    if (connParamsUpdateTimer_ == nullptr) {
        return;
    }
    if (!isConnParamsFeeded(&connection->effectiveConnParams)) {
        if (connParamsUpdateAttempts_ < BLE_CONN_PARAMS_UPDATE_ATTEMPS) {
            if (!os_timer_change(connParamsUpdateTimer_, OS_TIMER_CHANGE_START, true, 0, 0, nullptr)) {
                LOG_DEBUG(TRACE, "Attempts to update BLE connection parameters, try: %d after %d ms",
                        connParamsUpdateAttempts_, BLE_CONN_PARAMS_UPDATE_DELAY_MS);
                connParamUpdateHandle_ = connection->connHandle;
                return;
            }
        } else {
            int ret = sd_ble_gap_disconnect(connection->connHandle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
            if (ret != NRF_SUCCESS) {
                LOG_DEBUG(TRACE, "sd_ble_gap_disconnect() failed: %d", ret);
                return;
            }
            LOG_DEBUG(TRACE, "Disconnecting. Update BLE connection parameters failed.");
        }
    }
}

bool BleObject::ConnectionsManager::isConnParamsFeeded(const hal_ble_conn_params_t* params) const {
    hal_ble_conn_params_t ppcp = {};
    if (getPpcp(&ppcp) == SYSTEM_ERROR_NONE) {
        uint16_t minAcceptedSl = ppcp.slave_latency - std::min((uint16_t)BLE_CONN_PARAMS_SLAVE_LATENCY_ERR, ppcp.slave_latency);
        uint16_t maxAcceptedSl = ppcp.slave_latency + BLE_CONN_PARAMS_SLAVE_LATENCY_ERR;
        uint16_t minAcceptedTo = ppcp.conn_sup_timeout - std::min((uint16_t)BLE_CONN_PARAMS_TIMEOUT_ERR, ppcp.conn_sup_timeout);
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
        return;
    }
    connMgr->connParamsUpdateAttempts_++;
}

void BleObject::ConnectionsManager::processConnectionEvents(const ble_evt_t* event, void* context) {
    ConnectionsManager* connMgr = static_cast<ConnectionsManagerImpl*>(context)->instance;
    switch (event->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED: {
            const ble_gap_evt_connected_t& connected = event->evt.gap_evt.params.connected;
            LOG_DEBUG(TRACE, "BLE GAP event: connected.");
            BleConnection newConnection = {};
            newConnection.role = (hal_ble_role_t)connected.role;
            newConnection.connHandle = event->evt.gap_evt.conn_handle;
            newConnection.effectiveConnParams = connMgr->toHalConnParams(&connected.conn_params);
            newConnection.peer.addr_type = (ble_sig_addr_type_t)connected.peer_addr.addr_type;
            memcpy(newConnection.peer.addr, connected.peer_addr.addr, BLE_SIG_ADDR_LEN);
            connMgr->addConnection(newConnection);
            LOG_DEBUG(TRACE, "BLE role: %d, connection handle: %d", newConnection.role, newConnection.connHandle);
            LOG_DEBUG(TRACE, "| interval(ms)  latency  timeout(ms) |");
            LOG_DEBUG(TRACE, "  %.2f          %d       %d", newConnection.effectiveConnParams.max_conn_interval*1.25,
                    newConnection.effectiveConnParams.slave_latency, newConnection.effectiveConnParams.conn_sup_timeout*10);
            // If the connection is initiated by Central.
            if (connMgr->isConnecting_ && addressEqual(newConnection.peer, connMgr->connectingAddr_)) {
                if (connMgr->connectSemaphore_) {
                    os_semaphore_give(connMgr->connectSemaphore_, false);
                }
            }
            // Update connection parameters if needed.
            if (newConnection.role == BLE_ROLE_PERIPHERAL) {
                connMgr->connParamsUpdateAttempts_ = 0;
                connMgr->initiateConnParamsUpdateIfNeeded(&newConnection);
            }
            BleObject::BleEventDispatcher::EventMessage msg;
            msg.evt.type = BLE_EVT_CONNECTED;
            msg.evt.version = BLE_API_VERSION;
            msg.evt.size = sizeof(hal_ble_evts_t);
            msg.evt.params.connected.role = newConnection.role;
            msg.evt.params.connected.conn_handle = newConnection.connHandle;
            msg.evt.params.connected.conn_interval = newConnection.effectiveConnParams.max_conn_interval;
            msg.evt.params.connected.slave_latency = newConnection.effectiveConnParams.slave_latency;
            msg.evt.params.connected.conn_sup_timeout = newConnection.effectiveConnParams.conn_sup_timeout;
            msg.evt.params.connected.peer_addr.addr_type = newConnection.peer.addr_type;
            memcpy(msg.evt.params.connected.peer_addr.addr, newConnection.peer.addr, BLE_SIG_ADDR_LEN);
            BleObject::getInstance().dispatcher()->enqueue(msg);
            break;
        }
        case BLE_GAP_EVT_DISCONNECTED: {
            const ble_gap_evt_disconnected_t& disconnected = event->evt.gap_evt.params.disconnected;
            LOG_DEBUG(TRACE, "BLE GAP event: disconnected, handle: 0x%04X, reason: %d", event->evt.gap_evt.conn_handle, disconnected.reason);
            BleConnection* connection = connMgr->fetchConnection(event->evt.gap_evt.conn_handle);
            if (connection != nullptr) {
                // Cancel the on-going connection parameters update procedure.
                if (connMgr->connParamUpdateHandle_ == event->evt.gap_evt.conn_handle) {
                    if (connection->role == BLE_ROLE_PERIPHERAL) {
                        connMgr->connParamUpdateHandle_ = BLE_INVALID_CONN_HANDLE;
                        if (!os_timer_is_active(connMgr->connParamsUpdateTimer_, nullptr)) {
                            os_timer_change(connMgr->connParamsUpdateTimer_, OS_TIMER_CHANGE_STOP, true, 0, 0, nullptr);
                        }
                    } else {
                        if (connMgr->connParamsUpdateSemaphore_) {
                            os_semaphore_give(connMgr->connParamsUpdateSemaphore_, false);
                        }
                    }
                }
                if (connMgr->disconnectingHandle_ == event->evt.gap_evt.conn_handle) {
                    if (connMgr->disconnectSemaphore_) {
                        os_semaphore_give(connMgr->disconnectSemaphore_, false);
                    }
                }
                connMgr->removeConnection(event->evt.gap_evt.conn_handle);
            } else {
                LOG(ERROR, "Connection not found.");
                return;
            }
            BleObject::BleEventDispatcher::EventMessage msg;
            msg.evt.type = BLE_EVT_DISCONNECTED;
            msg.evt.version = BLE_API_VERSION;
            msg.evt.size = sizeof(hal_ble_evts_t);
            msg.evt.params.disconnected.reason = disconnected.reason;
            msg.evt.params.disconnected.conn_handle = event->evt.gap_evt.conn_handle;
            BleObject::getInstance().dispatcher()->enqueue(msg);
            break;
        }
        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST: {
            LOG_DEBUG(TRACE, "BLE GAP event: connection parameter update request.");
            int ret = sd_ble_gap_conn_param_update(event->evt.gap_evt.conn_handle,
                    &event->evt.gap_evt.params.conn_param_update_request.conn_params);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gap_conn_param_update() failed: %u", (unsigned)ret);
            }
            break;
        }
        case BLE_GAP_EVT_CONN_PARAM_UPDATE: {
            const ble_gap_evt_conn_param_update_t& connParaUpdate = event->evt.gap_evt.params.conn_param_update;
            LOG_DEBUG(TRACE, "BLE GAP event: connection parameter updated.");
            BleConnection* connection = connMgr->fetchConnection(event->evt.gap_evt.conn_handle);
            if (connection != nullptr) {
                connection->effectiveConnParams = connMgr->toHalConnParams(&connParaUpdate.conn_params);
                if (connection->role == BLE_ROLE_PERIPHERAL) {
                    connMgr->initiateConnParamsUpdateIfNeeded(connection);
                } else {
                    if (connMgr->connParamsUpdateSemaphore_) {
                        os_semaphore_give(connMgr->connParamsUpdateSemaphore_, false);
                    }
                }
            } else {
                LOG(ERROR, "Connection not found.");
                return;
            }
            LOG_DEBUG(TRACE, "| interval(ms)  latency  timeout(ms) |");
            LOG_DEBUG(TRACE, "  %.2f          %d       %d", connection->effectiveConnParams.max_conn_interval*1.25,
                    connection->effectiveConnParams.slave_latency, connection->effectiveConnParams.conn_sup_timeout*10);
            BleObject::BleEventDispatcher::EventMessage msg;
            msg.evt.type = BLE_EVT_CONN_PARAMS_UPDATED;
            msg.evt.version = BLE_API_VERSION;
            msg.evt.size = sizeof(hal_ble_evts_t);
            msg.evt.params.conn_params_updated.conn_handle = event->evt.gap_evt.conn_handle;
            msg.evt.params.conn_params_updated.conn_interval = connParaUpdate.conn_params.max_conn_interval;
            msg.evt.params.conn_params_updated.slave_latency = connParaUpdate.conn_params.slave_latency;
            msg.evt.params.conn_params_updated.conn_sup_timeout = connParaUpdate.conn_params.conn_sup_timeout;
            BleObject::getInstance().dispatcher()->enqueue(msg);
            break;
        }
        case BLE_GAP_EVT_TIMEOUT: {
            if (event->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN) {
                LOG_DEBUG(ERROR, "BLE GAP event: Connection timeout");
                if (connMgr->isConnecting_) {
                    os_semaphore_give(connMgr->connectSemaphore_, false);
                }
            }
            break;
        }
        default: {
            break;
        }
    }
}

struct GattServerImpl {
    BleObject::GattServer* instance;
};
static GattServerImpl gattsImpl;

int BleObject::GattServer::init() {
    gattsDataRecPool_.init(GATT_SERVER_DATA_REC_POOL_SIZE);
    gattsImpl.instance = this;
    NRF_SDH_BLE_OBSERVER(bleGattServer, 1, processGattServerEvents, &gattsImpl);
    return SYSTEM_ERROR_NONE;
}

int BleObject::GattServer::addService(uint8_t type, const hal_ble_uuid_t* uuid, hal_ble_attr_handle_t* svcHandle) {
    ble_uuid_t svcUuid;
    if (uuid == nullptr || svcHandle == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    int ret;
    CHECK(BleObject::toPlatformUUID(uuid, &svcUuid));
    ret = sd_ble_gatts_service_add(type, &svcUuid, svcHandle);
    CHECK_NRF_RETURN(ret, sysError(ret));
    services_.append(*svcHandle);
    LOG_DEBUG(TRACE, "Service handle: %d.", *svcHandle);
    return SYSTEM_ERROR_NONE;
}

int BleObject::GattServer::addCharacteristic(hal_ble_attr_handle_t svcHandle, uint8_t properties, const hal_ble_uuid_t* uuid, const char* desc, hal_ble_char_handles_t* charHandles) {
    if (charHandles == nullptr || uuid == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (characteristics_.size() >= BLE_MAX_CHAR_COUNT) {
        return SYSTEM_ERROR_LIMIT_EXCEEDED;
    }
    if (findService(svcHandle)) {
        ble_uuid_t charUuid;
        ble_gatts_char_md_t charMd = {};
        ble_gatts_attr_md_t valueAttrMd = {};
        ble_gatts_attr_md_t userDescAttrMd = {};
        ble_gatts_attr_md_t cccdAttrMd = {};
        ble_gatts_attr_t charValueAttr = {};
        ble_gatts_char_handles_t handles = {};
        int ret;
        CHECK(BleObject::toPlatformUUID(uuid, &charUuid));
        charMd.char_props = {};
        charMd.char_props = toPlatformCharProps(properties);
        // User Description Descriptor attribute metadata
        if (desc != nullptr) {
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
            BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccdAttrMd.read_perm);
            BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccdAttrMd.write_perm);
            cccdAttrMd.vloc = BLE_GATTS_VLOC_STACK;
            charMd.p_cccd_md = &cccdAttrMd;
        }
        // TODO:
        charMd.p_char_pf = nullptr;
        charMd.p_sccd_md = nullptr;
        // Characteristic value attribute metadata
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&valueAttrMd.read_perm);
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&valueAttrMd.write_perm);
        valueAttrMd.vloc = BLE_GATTS_VLOC_USER;
        valueAttrMd.rd_auth = 0;
        valueAttrMd.wr_auth = 0;
        valueAttrMd.vlen = 1;
        // Characteristic value attribute
        uint8_t* charValue = (uint8_t*)malloc(BLE_MAX_ATTR_VALUE_PACKET_SIZE);
        if (charValue == nullptr) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        charValueAttr.p_uuid = &charUuid;
        charValueAttr.p_attr_md = &valueAttrMd;
        charValueAttr.init_len = 0;
        charValueAttr.init_offs = 0;
        charValueAttr.max_len = BLE_MAX_ATTR_VALUE_PACKET_SIZE;
        charValueAttr.p_value = charValue;
        ret = sd_ble_gatts_characteristic_add(svcHandle, &charMd, &charValueAttr, &handles);
        if (ret != NRF_SUCCESS) {
            free(charValue);
            CHECK_NRF_RETURN(ret, sysError(ret));
        }
        BleCharacteristic characteristic;
        characteristic.properties = properties;
        characteristic.svcHandle = svcHandle;
        characteristic.charHandles.size = sizeof(hal_ble_char_handles_t);
        characteristic.charHandles.decl_handle = handles.value_handle - 1;
        characteristic.charHandles.value_handle = handles.value_handle;
        characteristic.charHandles.user_desc_handle = handles.user_desc_handle;
        characteristic.charHandles.cccd_handle = handles.cccd_handle;
        characteristic.charHandles.sccd_handle = handles.sccd_handle;
        characteristics_.append(characteristic);
        *charHandles = characteristic.charHandles;
        LOG_DEBUG(TRACE, "Characteristic value handle: %d.", handles.value_handle);
        LOG_DEBUG(TRACE, "Characteristic cccd handle: %d.", handles.cccd_handle);
        return SYSTEM_ERROR_NONE;
    }
    else {
        return SYSTEM_ERROR_NOT_FOUND;
    }
}

int BleObject::GattServer::addDescriptor(hal_ble_attr_handle_t charHandle, const hal_ble_uuid_t* uuid, uint8_t* descriptor, size_t len, hal_ble_attr_handle_t* descHandle) {
    if (uuid == nullptr || descriptor== nullptr || descHandle == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (findCharacteristic(charHandle)) {
        ble_gatts_attr_t descAttr = {};
        ble_gatts_attr_md_t descAttrMd = {};
        ble_uuid_t descUuid;
        int ret;
        CHECK(BleObject::toPlatformUUID(uuid, &descUuid));
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&descAttrMd.read_perm);
        BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&descAttrMd.write_perm);
        descAttrMd.vloc = BLE_GATTS_VLOC_STACK;
        descAttrMd.rd_auth = 0;
        descAttrMd.wr_auth = 0;
        descAttrMd.vlen    = 0;
        descAttr.p_uuid    = &descUuid;
        descAttr.p_attr_md = &descAttrMd;
        descAttr.init_len  = len;
        descAttr.init_offs = 0;
        descAttr.max_len   = len;
        descAttr.p_value   = descriptor;
        // FIXME: validate the descriptor to be added
        ret = sd_ble_gatts_descriptor_add(charHandle, &descAttr, descHandle);
        // TODO: assigne the handle to corresponding characteristic.
        CHECK_NRF_RETURN(ret, sysError(ret));
        return SYSTEM_ERROR_NONE;
    }
    else {
        return SYSTEM_ERROR_NOT_FOUND;
    }
}

ssize_t BleObject::GattServer::setValue(hal_ble_attr_handle_t attrHandle, const uint8_t* buf, size_t len) {
    if (attrHandle == BLE_INVALID_ATTR_HANDLE || buf == nullptr || len == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    BleCharacteristic* characteristic = findCharacteristic(attrHandle);
    if (characteristic != nullptr && attrHandle == characteristic->charHandles.value_handle) {
        len = std::min(len, (size_t)BLE_MAX_ATTR_VALUE_PACKET_SIZE);
        ble_gatts_value_t gattValue = {};
        gattValue.len = len;
        gattValue.offset = 0;
        gattValue.p_value = (uint8_t*)buf;
        int ret = sd_ble_gatts_value_set(BLE_CONN_HANDLE_INVALID, attrHandle, &gattValue);
        CHECK_NRF_RETURN(ret, sysError(ret));
        // Notify or indicate the value if possible.
        if ((characteristic->properties & BLE_SIG_CHAR_PROP_NOTIFY) || (characteristic->properties & BLE_SIG_CHAR_PROP_INDICATE)) {
            for (const auto& cccdConnection : characteristic->cccdConnections) {
                if (cccdConnection != BLE_INVALID_CONN_HANDLE) {
                    if (os_semaphore_create(&hvxSemaphore_, 1, 0)) {
                        LOG(ERROR, "os_semaphore_create() failed");
                        break;
                    }
                    SCOPE_GUARD ({
                        os_semaphore_destroy(hvxSemaphore_);
                        hvxSemaphore_ = nullptr;
                    });
                    ble_gatts_hvx_params_t hvxParams = {};
                    uint16_t hvxLen = std::min(len, (size_t)BLE_ATTR_VALUE_PACKET_SIZE(getAttMtu(cccdConnection)));
                    LOG_DEBUG(TRACE, "Notify data len: %d", hvxLen);
                    if (characteristic->properties & BLE_SIG_CHAR_PROP_NOTIFY) {
                        hvxParams.type = BLE_GATT_HVX_NOTIFICATION;
                    } else if (characteristic->properties & BLE_SIG_CHAR_PROP_INDICATE) {
                        hvxParams.type = BLE_GATT_HVX_INDICATION;
                    }
                    hvxParams.handle = attrHandle;
                    hvxParams.offset = 0;
                    hvxParams.p_data = buf;
                    hvxParams.p_len = &hvxLen;
                    ret_code_t ret = sd_ble_gatts_hvx(cccdConnection, &hvxParams);
                    if (ret != NRF_SUCCESS) {
                        LOG(ERROR, "sd_ble_gatts_hvx() failed: %u", (unsigned)ret);
                        break;
                    }
                    if (os_semaphore_take(hvxSemaphore_, BLE_HVX_PROCEDURE_TIMEOUT_MS, false)) {
                        break;
                    }
                }
            }
        }
        return len;
    }
    else {
        return SYSTEM_ERROR_NOT_FOUND;
    }
}

ssize_t BleObject::GattServer::getValue(hal_ble_attr_handle_t attrHandle, uint8_t* buf, size_t len) {
    if (attrHandle == BLE_INVALID_ATTR_HANDLE || buf == nullptr || len == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    BleCharacteristic* characteristic = findCharacteristic(attrHandle);
    if (characteristic != nullptr && attrHandle == characteristic->charHandles.value_handle) {
        len = std::min(len, (size_t)BLE_MAX_ATTR_VALUE_PACKET_SIZE);
        ble_gatts_value_t gattValue;
        gattValue.len = len;
        gattValue.offset = 0;
        gattValue.p_value = buf;
        int ret = sd_ble_gatts_value_get(BLE_CONN_HANDLE_INVALID, attrHandle, &gattValue);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "sd_ble_gatts_value_get() failed: %u", (unsigned)ret);
            return 0;
        }
        return gattValue.len;
    } else {
        return SYSTEM_ERROR_NOT_FOUND;
    }
}

bool BleObject::GattServer::findService(hal_ble_attr_handle_t svcHandle) const {
    for (const auto& handle : services_) {
        if (handle == svcHandle) {
            return true;
        }
    }
    return false;
}

BleObject::GattServer::BleCharacteristic* BleObject::GattServer::findCharacteristic(hal_ble_attr_handle_t attrHandle) {
    for (auto& characteristic : characteristics_) {
        if (characteristic.charHandles.value_handle == attrHandle ||
                characteristic.charHandles.decl_handle == attrHandle ||
                characteristic.charHandles.user_desc_handle == attrHandle ||
                characteristic.charHandles.cccd_handle == attrHandle ||
                characteristic.charHandles.sccd_handle == attrHandle) {
            return &characteristic;
        }
    }
    return nullptr;
}

void BleObject::GattServer::addToCccdList(BleCharacteristic* characteristic, hal_ble_conn_handle_t connHandle) {
    for (auto& cccdConnection : characteristic->cccdConnections) {
        if (cccdConnection == BLE_INVALID_CONN_HANDLE) {
            cccdConnection = connHandle;
            return;
        }
    }
}

void BleObject::GattServer::removeFromCCCDList(BleCharacteristic* characteristic, hal_ble_conn_handle_t connHandle) {
    for (auto& cccdConnection : characteristic->cccdConnections) {
        if (cccdConnection == connHandle) {
            cccdConnection = BLE_INVALID_CONN_HANDLE;
        }
    }
}

void BleObject::GattServer::removeFromAllCCCDList(hal_ble_conn_handle_t connHandle) {
    for (auto& characteristic : characteristics_) {
        removeFromCCCDList(&characteristic, connHandle);
    }
}

void BleObject::GattServer::gattsEventProcessedHook(const hal_ble_evts_t *event, void* context) {
    GattServer* gatts = static_cast<GattServer*>(context);
    if (gatts && event && event->type == BLE_EVT_DATA_WRITTEN) {
        if (event->params.data_rec.data != nullptr) {
            gatts->gattsDataRecPool_.free(event->params.data_rec.data);
            LOG_DEBUG(TRACE, "Free received data memory: %u", (unsigned)event->params.data_rec.data);
        }
    }
}

void BleObject::GattServer::processGattServerEvents(const ble_evt_t* event, void* context) {
    GattServer* gatts = static_cast<GattServerImpl*>(context)->instance;
    switch (event->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED: {
            // Use the default ATT_MTU on connected.
            gatts->configureAttMtu(event->evt.gap_evt.conn_handle, BLE_DEFAULT_ATT_MTU_SIZE);
            break;
        }
        case BLE_GAP_EVT_DISCONNECTED: {
            gatts->clearAttMtu(event->evt.gap_evt.conn_handle);
            gatts->removeFromAllCCCDList(event->evt.gap_evt.conn_handle);
            break;
        }
        case BLE_GATTS_EVT_SYS_ATTR_MISSING: {
            LOG_DEBUG(TRACE, "BLE GATT Server event: system attribute is missing.");
            // No persistent system attributes
            const uint32_t ret = sd_ble_gatts_sys_attr_set(event->evt.gatts_evt.conn_handle, nullptr, 0, 0);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gatts_sys_attr_set() failed: %u", (unsigned)ret);
            }
            break;
        }
        case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST: {
            const ble_gatts_evt_exchange_mtu_request_t& mtuRequest = event->evt.gatts_evt.params.exchange_mtu_request;
            LOG_DEBUG(TRACE, "BLE GATT Server event: exchange ATT MTU request: %d, desired: %d", mtuRequest.client_rx_mtu, gatts->desiredAttMtu_);
            int ret = sd_ble_gatts_exchange_mtu_reply(event->evt.gatts_evt.conn_handle, gatts->desiredAttMtu_);
            if (ret != NRF_SUCCESS) {
                LOG_DEBUG(TRACE, "sd_ble_gatts_exchange_mtu_reply() failed: %d", ret);
                return;
            }
            size_t effectAttMtu = std::min((size_t)mtuRequest.client_rx_mtu, gatts->desiredAttMtu_);
            effectAttMtu = std::max(effectAttMtu, (size_t)BLE_MIN_ATT_MTU_SIZE);
            gatts->configureAttMtu(event->evt.gatts_evt.conn_handle, effectAttMtu, true);
            LOG_DEBUG(TRACE, "Effective ATT MTU: %d.", effectAttMtu);
            BleObject::BleEventDispatcher::EventMessage msg;
            msg.evt.type = BLE_EVT_GATT_PARAMS_UPDATED;
            msg.evt.version = BLE_API_VERSION;
            msg.evt.size = sizeof(hal_ble_evts_t);
            msg.evt.params.gatt_params_updated.conn_handle = event->evt.gatts_evt.conn_handle;
            msg.evt.params.gatt_params_updated.att_mtu_size = effectAttMtu;
            BleObject::getInstance().dispatcher()->enqueue(msg);
            break;
        }
        case BLE_GATTS_EVT_WRITE: {
            const ble_gatts_evt_write_t& write = event->evt.gatts_evt.params.write;
            LOG_DEBUG(TRACE, "BLE GATT Server event: write characteristic, handle: %d", write.handle);
            BleCharacteristic* characteristic = gatts->findCharacteristic(write.handle);
            if (characteristic != nullptr) {
                if (characteristic->charHandles.cccd_handle == write.handle) {
                    if (event->evt.gatts_evt.params.write.data[0] > 0) {
                        gatts->addToCccdList(characteristic, event->evt.gatts_evt.conn_handle);
                    } else {
                        gatts->removeFromCCCDList(characteristic, event->evt.gatts_evt.conn_handle);
                    }
                }
                // TODO: deal with different write commands.
                uint8_t* data = (uint8_t*)gatts->gattsDataRecPool_.alloc(write.len);
                if (data) {
                    LOG_DEBUG(TRACE, "Received data address: %u, len: %u", (unsigned)data, write.len);
                    BleObject::BleEventDispatcher::EventMessage msg;
                    msg.evt.type = BLE_EVT_DATA_WRITTEN;
                    msg.evt.version = BLE_API_VERSION;
                    msg.evt.size = sizeof(hal_ble_evts_t);
                    msg.evt.params.data_rec.conn_handle = event->evt.gatts_evt.conn_handle;
                    msg.evt.params.data_rec.attr_handle = write.handle;
                    msg.evt.params.data_rec.offset = write.offset;
                    msg.evt.params.data_rec.data_len = write.len;
                    memcpy(data, write.data, write.len);
                    msg.evt.params.data_rec.data = data;
                    msg.hook = gatts->gattsEventProcessedHook;
                    msg.hookContext = gatts;
                    BleObject::getInstance().dispatcher()->enqueue(msg);
                }
            }
            break;
        }
        case BLE_GATTS_EVT_HVN_TX_COMPLETE: {
            LOG_DEBUG(TRACE, "BLE GATT Server event: notification sent.");
            if (gatts->hvxSemaphore_) {
                os_semaphore_give(gatts->hvxSemaphore_, false);
            }
            break;
        }
        case BLE_GATTS_EVT_HVC: {
            LOG_DEBUG(TRACE, "BLE GATT Server event: indication confirmed.");
            if (gatts->hvxSemaphore_) {
                os_semaphore_give(gatts->hvxSemaphore_, false);
            }
            break;
        }
        case BLE_GATTS_EVT_TIMEOUT: {
            LOG_DEBUG(TRACE, "BLE GATT Server event: timeout, source: %d.", event->evt.gatts_evt.params.timeout.src);
            // Disconnect on GATT Server timeout event.
            int ret = sd_ble_gap_disconnect(event->evt.gatts_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gap_disconnect() failed: %u", (unsigned)ret);
            }
            break;
        }
        default: {
            break;
        }
    }
}

struct GattClientImpl {
    BleObject::GattClient* instance;
};
static GattClientImpl gattcImpl;

int BleObject::GattClient::init() {
    gattcDiscPool_.init(GATT_CLIENT_DISC_SVC_CHAR_POOL_SIZE);
    gattcDataRecPool_.init(GATT_CLIENT_DATA_REC_POOL_SIZE);
    if (os_timer_create(&attMtuExchangeTimer_, BLE_ATT_MTU_EXCHANGE_DELAY_MS, onAttMtuExchangeTimerExpired, this, true, nullptr)) {
        LOG(ERROR, "os_timer_create() failed.");
        return SYSTEM_ERROR_INTERNAL;
    }
    gattcImpl.instance = this;
    NRF_SDH_BLE_OBSERVER(bleGattClient, 1, processGattClientEvents, &gattcImpl);
    return SYSTEM_ERROR_NONE;
}

bool BleObject::GattClient::discovering(hal_ble_conn_handle_t connHandle) const {
    // TODO: discovering service and characteristic on multi-links concurrently.
    return isDiscovering_;
}

int BleObject::GattClient::discoverServices(hal_ble_conn_handle_t connHandle, const hal_ble_uuid_t* uuid, on_ble_disc_service_cb_t callback, void* context) {
    int ret;
    if (!BleObject::getInstance().connMgr()->valid(connHandle)) {
        LOG(ERROR, "Connection invalid.");
        return SYSTEM_ERROR_NOT_FOUND;
    }
    if (isDiscovering_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (os_semaphore_create(&discoverySemaphore_, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }
    SCOPE_GUARD ({
        os_semaphore_destroy(discoverySemaphore_);
        discoverySemaphore_ = nullptr;
    });
    currDiscConnHandle_ = connHandle;
    currDiscProcedure_ = DiscoveryProcedure::BLE_DISCOVERY_PROCEDURE_SERVICES;
    if (uuid == nullptr) {
        discoverAll_ = true;
        ret = sd_ble_gattc_primary_services_discover(connHandle, SERVICES_BASE_START_HANDLE, nullptr);
    } else {
        ble_uuid_t svcUUID;
        BleObject::toPlatformUUID(uuid, &svcUUID);
        ret = sd_ble_gattc_primary_services_discover(connHandle, SERVICES_BASE_START_HANDLE, &svcUUID);
    }
    if (ret != NRF_SUCCESS) {
        resetDiscoveryState();
        CHECK_NRF_RETURN(ret, sysError(ret));
    }
    isDiscovering_ = true;
    ret = SYSTEM_ERROR_NONE;
    if (os_semaphore_take(discoverySemaphore_, BLE_DICOVERY_PROCEDURE_TIMEOUT_MS, false)) {
        ret = SYSTEM_ERROR_TIMEOUT;
    }
    hal_ble_svc_t* services = (hal_ble_svc_t*)malloc(discServiceCount_ * sizeof(hal_ble_svc_t));
    if (services) {
        LOG_DEBUG(TRACE, "Discovered services address: %u, len: %u", (unsigned)services, discServiceCount_ * sizeof(hal_ble_svc_t));
        BleObject::BleEventDispatcher::EventMessage msg;
        msg.evt.type = BLE_EVT_SVC_DISCOVERED;
        msg.evt.version = BLE_API_VERSION;
        msg.evt.size = sizeof(hal_ble_evts_t);
        msg.evt.params.svc_disc.conn_handle = currDiscConnHandle_;
        msg.evt.params.svc_disc.count = discServiceCount_;
        DiscoveredService* discService = nullptr;
        size_t i = 0;
        do {
            discService = discServicesList_.popFront();
            if (discService) {
                memcpy(services + i, &discService->service, sizeof(hal_ble_svc_t));
                i++;
                DiscoveredService* curr = discService;
                gattcDiscPool_.free(curr);
            }
        } while (discService);
        msg.evt.params.svc_disc.services = services;
        msg.handler = (BleObject::BleEventDispatcher::BleSpecificEventHandler)callback;
        msg.context = context;
        msg.hook = gattcEventProcessedHook;
        msg.hookContext = this;
        BleObject::getInstance().dispatcher()->enqueue(msg);
    }
    resetDiscoveryState();
    return ret;
}

int BleObject::GattClient::discoverCharacteristics(hal_ble_conn_handle_t connHandle, const hal_ble_svc_t* service, on_ble_disc_char_cb_t callback, void* context) {
    if (!BleObject::getInstance().connMgr()->valid(connHandle)) {
        LOG(ERROR, "Connection invalid.");
        return SYSTEM_ERROR_NOT_FOUND;
    }
    if (isDiscovering_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (os_semaphore_create(&discoverySemaphore_, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }
    SCOPE_GUARD ({
        os_semaphore_destroy(discoverySemaphore_);
        discoverySemaphore_ = nullptr;
    });
    ble_gattc_handle_range_t handleRange = {};
    handleRange.start_handle = service->start_handle;
    handleRange.end_handle = service->end_handle;
    currDiscConnHandle_ = connHandle;
    currDiscSvc_ = *service;
    currDiscProcedure_ = DiscoveryProcedure::BLE_DISCOVERY_PROCEDURE_CHARACTERISTICS;
    int ret = sd_ble_gattc_characteristics_discover(connHandle, &handleRange);
    if (ret != NRF_SUCCESS) {
        resetDiscoveryState();
        CHECK_NRF_RETURN(ret, sysError(ret));
    }
    isDiscovering_ = true;
    ret = SYSTEM_ERROR_NONE;
    os_semaphore_take(discoverySemaphore_, BLE_DICOVERY_PROCEDURE_TIMEOUT_MS, false);
    // Now discover characteristic descriptors
    if (!BleObject::getInstance().connMgr()->valid(connHandle)) {
        LOG(ERROR, "Connection invalid.");
        resetDiscoveryState();
        return SYSTEM_ERROR_NOT_FOUND;
    }
    currDiscProcedure_ = DiscoveryProcedure::BLE_DISCOVERY_PROCEDURE_DESCRIPTORS;
    if (sd_ble_gattc_descriptors_discover(currDiscConnHandle_, &handleRange) != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gattc_descriptors_discover() failed: %u", (unsigned)ret);
    } else {
        ret = SYSTEM_ERROR_NONE;
        if (os_semaphore_take(discoverySemaphore_, BLE_DICOVERY_PROCEDURE_TIMEOUT_MS, false)) {
            ret = SYSTEM_ERROR_TIMEOUT;
        }
    }
    hal_ble_char_t* characteristics = (hal_ble_char_t*)malloc(discCharacteristicCount_ * sizeof(hal_ble_char_t));
    if (characteristics) {
        LOG_DEBUG(TRACE, "Discovered characteristics address: %u, len: %u", (unsigned)characteristics, discCharacteristicCount_ * sizeof(hal_ble_char_t));
        BleObject::BleEventDispatcher::EventMessage msg;
        msg.evt.type = BLE_EVT_CHAR_DISCOVERED;
        msg.evt.version = BLE_API_VERSION;
        msg.evt.size = sizeof(hal_ble_evts_t);
        msg.evt.params.char_disc.conn_handle = currDiscConnHandle_;
        msg.evt.params.char_disc.count = discCharacteristicCount_;
        DiscoveredCharacteristic* discChar = nullptr;
        size_t i = 0;
        do {
            discChar = discCharacteristicsList_.popFront();
            if (discChar) {
                memcpy(characteristics + i, &discChar->characteristic, sizeof(hal_ble_char_t));
                i++;
                DiscoveredCharacteristic* curr = discChar;
                gattcDiscPool_.free(curr);
            }
        } while (discChar);
        msg.evt.params.char_disc.characteristics = characteristics;
        msg.handler = (BleObject::BleEventDispatcher::BleSpecificEventHandler)callback;
        msg.context = context;
        msg.hook = gattcEventProcessedHook;
        msg.hookContext = this;
        BleObject::getInstance().dispatcher()->enqueue(msg);
    }
    resetDiscoveryState();
    return ret;
}

int BleObject::GattClient::setDesiredAttMtu(size_t attMtu) {
    desiredAttMtu_ = std::min(attMtu, (size_t)BLE_MAX_ATT_MTU_SIZE);
    return SYSTEM_ERROR_NONE;
}

ssize_t BleObject::GattClient::writeAttribute(hal_ble_conn_handle_t connHandle, hal_ble_attr_handle_t attrHandle, const uint8_t* buf, size_t len, bool response) {
    if (buf == nullptr || len == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (!BleObject::getInstance().connMgr()->valid(connHandle)) {
        LOG(ERROR, "Connection invalid.");
        return SYSTEM_ERROR_NOT_FOUND;
    }
    ble_gattc_write_params_t writeParams;
    if (os_semaphore_create(&readWriteSemaphore_, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }
    SCOPE_GUARD ({
        os_semaphore_destroy(readWriteSemaphore_);
        readWriteSemaphore_ = nullptr;
    });
    if (response) {
        writeParams.write_op = BLE_GATT_OP_WRITE_REQ;
    } else {
        writeParams.write_op = BLE_GATT_OP_WRITE_CMD;
    }
    len = std::min(len, (size_t)BLE_ATTR_VALUE_PACKET_SIZE(getAttMtu(connHandle)));
    writeParams.flags = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE;
    writeParams.handle = attrHandle;
    writeParams.offset = 0;
    writeParams.len = len;
    writeParams.p_value = buf;
    int ret = sd_ble_gattc_write(connHandle, &writeParams);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "sd_ble_gattc_write() failed: %u", (unsigned)ret);
        return sysError(ret);
    }
    if (os_semaphore_take(readWriteSemaphore_, BLE_READ_WRITE_PROCEDURE_TIMEOUT_MS, false)) {
        return SYSTEM_ERROR_TIMEOUT;
    }
    return len;
}

// FIXME: Multi-link read
ssize_t BleObject::GattClient::readAttribute(hal_ble_conn_handle_t connHandle, hal_ble_attr_handle_t attrHandle, uint8_t* buf, size_t len) {
    if (buf == nullptr || len == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (!BleObject::getInstance().connMgr()->valid(connHandle)) {
        LOG(ERROR, "Connection invalid.");
        return SYSTEM_ERROR_NOT_FOUND;
    }
    if (os_semaphore_create(&readWriteSemaphore_, 1, 0)) {
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }
    SCOPE_GUARD ({
        os_semaphore_destroy(readWriteSemaphore_);
        readWriteSemaphore_ = nullptr;
    });
    readAttrHandle_ = attrHandle;
    readBuf_ = buf;
    readLen_ = std::min(len, (size_t)BLE_ATTR_VALUE_PACKET_SIZE(getAttMtu(connHandle)));
    int ret = sd_ble_gattc_read(connHandle, attrHandle, 0);
    if (ret != NRF_SUCCESS) {
        readBuf_ = nullptr;
        readAttrHandle_ = BLE_INVALID_ATTR_HANDLE;
        LOG(ERROR, "sd_ble_gattc_read() failed: %u", (unsigned)ret);
        return sysError(ret);
    }
    if (os_semaphore_take(readWriteSemaphore_, BLE_READ_WRITE_PROCEDURE_TIMEOUT_MS, false)) {
        return SYSTEM_ERROR_TIMEOUT;
    }
    return readLen_;
}

void BleObject::GattClient::resetDiscoveryState() {
    discoverAll_ = false;
    isDiscovering_ = false;
    currDiscConnHandle_ = BLE_INVALID_CONN_HANDLE;
    currDiscProcedure_ = DiscoveryProcedure::BLE_DISCOVERY_PROCEDURE_IDLE;
    discServiceCount_ = 0;
    discCharacteristicCount_ = 0;
}

int BleObject::GattClient::addDiscoveredService(const hal_ble_svc_t& service) {
    DiscoveredService* discService = (DiscoveredService*)gattcDiscPool_.alloc(sizeof(DiscoveredService));
    if (discService) {
        discService->service = service;
        discServicesList_.pushFront(discService);
        discServiceCount_++;
        return SYSTEM_ERROR_NONE;
    }
    return SYSTEM_ERROR_NO_MEMORY;
}

int BleObject::GattClient::addDiscoveredCharacteristic(const hal_ble_char_t& characteristic) {
    DiscoveredCharacteristic* discChar = (DiscoveredCharacteristic*)gattcDiscPool_.alloc(sizeof(DiscoveredCharacteristic));
    if (discChar) {
        discChar->characteristic = characteristic;
        discCharacteristicsList_.pushFront(discChar);
        discCharacteristicCount_++;
        return SYSTEM_ERROR_NONE;
    }
    return SYSTEM_ERROR_NO_MEMORY;
}

bool BleObject::GattClient::readServiceUUID128IfNeeded() const {
    auto discService = discServicesList_.front();
    while (discService) {
        if (discService->service.uuid.type == BLE_UUID_TYPE_128BIT_SHORTED) {
            return (sd_ble_gattc_read(currDiscConnHandle_, discService->service.start_handle, 0) == NRF_SUCCESS);
        }
        discService = discService->next;
    }
    return false;
}

bool BleObject::GattClient::readCharacteristicUUID128IfNeeded() const {
    auto discChar = discCharacteristicsList_.front();
    while (discChar) {
        if (discChar->characteristic.uuid.type == BLE_UUID_TYPE_128BIT_SHORTED) {
            return (sd_ble_gattc_read(currDiscConnHandle_, discChar->characteristic.charHandles.decl_handle, 0) == NRF_SUCCESS);
        }
        discChar = discChar->next;
    }
    return false;
}

hal_ble_svc_t* BleObject::GattClient::findDiscoveredService(hal_ble_attr_handle_t attrHandle) {
    auto discService = discServicesList_.front();
    while (discService) {
        if (discService->service.start_handle <= attrHandle && attrHandle <= discService->service.end_handle) {
            return &discService->service;
        }
        discService = discService->next;
    }
    return nullptr;
}

hal_ble_char_t* BleObject::GattClient::findDiscoveredCharacteristic(hal_ble_attr_handle_t attrHandle) {
    hal_ble_char_t* foundChar = nullptr;
    hal_ble_attr_handle_t foundCharDeclHandle = BLE_INVALID_ATTR_HANDLE;
    auto discChar = discCharacteristicsList_.front();
    while (discChar) {
        // The attribute handles increase by sequence.
        if (attrHandle >= discChar->characteristic.charHandles.decl_handle) {
            if (discChar->characteristic.charHandles.decl_handle > foundCharDeclHandle) {
                foundChar = &discChar->characteristic;
                foundCharDeclHandle = discChar->characteristic.charHandles.decl_handle;
            }
        }
        discChar = discChar->next;
    }
    return foundChar;
}

void BleObject::GattClient::onAttMtuExchangeTimerExpired(os_timer_t timer) {
    GattClient* gattc;
    os_timer_get_id(timer, (void**)&gattc);
    if (!gattc->attMtuExchanged(gattc->attMtuExchangeConnHandle_)) {
        LOG_DEBUG(TRACE, "Request to change ATT_MTU from %d to %d", BLE_DEFAULT_ATT_MTU_SIZE, gattc->desiredAttMtu_);
        int ret = sd_ble_gattc_exchange_mtu_request(gattc->attMtuExchangeConnHandle_, gattc->desiredAttMtu_);
        if (ret != NRF_SUCCESS) {
            LOG_DEBUG(TRACE, "sd_ble_gattc_exchange_mtu_request() failed: %d", ret);
        }
    }
    else {
        LOG_DEBUG(TRACE, "ATT_MTU has been exchanged.");
    }
}

void BleObject::GattClient::gattcEventProcessedHook(const hal_ble_evts_t *event, void* context) {
    GattClient* gattc = static_cast<GattClient*>(context);
    if (gattc && event) {
        if (event->type == BLE_EVT_SVC_DISCOVERED) {
            if (event->params.svc_disc.services != nullptr) {
                free(event->params.svc_disc.services);
                LOG_DEBUG(TRACE, "Free discovered services memory: %u", (unsigned)event->params.svc_disc.services);
            }
        } else if (event->type == BLE_EVT_CHAR_DISCOVERED) {
            if (event->params.char_disc.characteristics != nullptr) {
                free(event->params.char_disc.characteristics);
                LOG_DEBUG(TRACE, "Free discovered characteristics memory: %u", (unsigned)event->params.char_disc.characteristics);
            }
        } else if (event->type == BLE_EVT_DATA_NOTIFIED) {
            if (event->params.data_rec.data != nullptr) {
                gattc->gattcDataRecPool_.free(event->params.data_rec.data);
                LOG_DEBUG(TRACE, "Free received data memory: %u", (unsigned)event->params.data_rec.data);
            }
        }
    }
}

void BleObject::GattClient::processGattClientEvents(const ble_evt_t* event, void* context) {
    GattClient* gattc = static_cast<GattClientImpl*>(context)->instance;
    switch (event->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED: {
            // Use the default ATT_MTU on connected.
            gattc->configureAttMtu(event->evt.gap_evt.conn_handle, BLE_DEFAULT_ATT_MTU_SIZE);
            if (gattc->desiredAttMtu_ > BLE_DEFAULT_ATT_MTU_SIZE) {
                // FIXME: What if there is another new connection established before the timer expired?
                if (!os_timer_change(gattc->attMtuExchangeTimer_, OS_TIMER_CHANGE_START, true, 0, 0, nullptr)) {
                    LOG_DEBUG(TRACE, "Attempts to exchange ATT_MTU if needed.");
                    gattc->attMtuExchangeConnHandle_ = event->evt.gap_evt.conn_handle;
                }
            }
            break;
        }
        case BLE_GAP_EVT_DISCONNECTED: {
            if (!os_timer_is_active(gattc->attMtuExchangeTimer_, nullptr)) {
                os_timer_change(gattc->attMtuExchangeTimer_, OS_TIMER_CHANGE_STOP, true, 0, 0, nullptr);
            }
            gattc->clearAttMtu(event->evt.gap_evt.conn_handle);
            if (gattc->isDiscovering_ && event->evt.gap_evt.conn_handle == gattc->currDiscConnHandle_) {
                if (gattc->discoverySemaphore_) {
                    os_semaphore_give(gattc->discoverySemaphore_, false);
                }
            }
            break;
        }
        case BLE_GATTC_EVT_EXCHANGE_MTU_RSP: {
            const ble_gattc_evt_exchange_mtu_rsp_t& attMtuRsp = event->evt.gattc_evt.params.exchange_mtu_rsp;
            size_t effectAttMtu = std::min((size_t)attMtuRsp.server_rx_mtu, gattc->desiredAttMtu_);
            effectAttMtu = std::max(effectAttMtu, (size_t)BLE_MIN_ATT_MTU_SIZE);
            gattc->configureAttMtu(event->evt.gatts_evt.conn_handle, effectAttMtu, true);
            LOG_DEBUG(TRACE, "BLE GATT Client event: exchange ATT MTU response: %d, effective: %d", attMtuRsp.server_rx_mtu, effectAttMtu);
            BleObject::BleEventDispatcher::EventMessage msg;
            msg.evt.type = BLE_EVT_GATT_PARAMS_UPDATED;
            msg.evt.version = BLE_API_VERSION;
            msg.evt.size = sizeof(hal_ble_evts_t);
            msg.evt.params.gatt_params_updated.conn_handle = event->evt.gatts_evt.conn_handle;
            msg.evt.params.gatt_params_updated.att_mtu_size = effectAttMtu;
            BleObject::getInstance().dispatcher()->enqueue(msg);
            break;
        }
        case BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP: {
            const ble_gattc_evt_prim_srvc_disc_rsp_t& primSvcDiscRsp = event->evt.gattc_evt.params.prim_srvc_disc_rsp;
            LOG_DEBUG(TRACE, "BLE GATT Client event: %d primary service discovered.", primSvcDiscRsp.count);
            if (gattc->isDiscovering_ && gattc->currDiscProcedure_ == DiscoveryProcedure::BLE_DISCOVERY_PROCEDURE_SERVICES &&
                    event->evt.gattc_evt.conn_handle == gattc->currDiscConnHandle_) {
                if (event->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_SUCCESS) {
                    for (uint8_t i = 0; i < primSvcDiscRsp.count; i++) {
                        hal_ble_svc_t service = {};
                        service.version = BLE_API_VERSION;
                        service.size = sizeof(hal_ble_svc_t);
                        service.start_handle = primSvcDiscRsp.services[i].handle_range.start_handle;
                        service.end_handle = primSvcDiscRsp.services[i].handle_range.end_handle;
                        BleObject::toHalUUID(&primSvcDiscRsp.services[i].uuid, &service.uuid);
                        if (gattc->addDiscoveredService(service) != SYSTEM_ERROR_NONE) {
                            LOG(ERROR, "There will have no memory to store new discovered services.");
                        }
                    }
                    hal_ble_attr_handle_t currEndHandle = primSvcDiscRsp.services[primSvcDiscRsp.count - 1].handle_range.end_handle;
                    if ((gattc->discoverAll_) && (currEndHandle < SERVICES_TOP_END_HANDLE)) {
                        int ret = sd_ble_gattc_primary_services_discover(gattc->currDiscConnHandle_, currEndHandle + 1, nullptr);
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
                if (gattc->discoverySemaphore_) {
                    os_semaphore_give(gattc->discoverySemaphore_, false);
                }
            }
            break;
        }
        case BLE_GATTC_EVT_CHAR_DISC_RSP: {
            const ble_gattc_evt_char_disc_rsp_t& charDiscRsp = event->evt.gattc_evt.params.char_disc_rsp;
            LOG_DEBUG(TRACE, "BLE GATT Client event: %d characteristic discovered.", charDiscRsp.count);
            if (gattc->isDiscovering_ && gattc->currDiscProcedure_ == DiscoveryProcedure::BLE_DISCOVERY_PROCEDURE_CHARACTERISTICS &&
                    event->evt.gattc_evt.conn_handle == gattc->currDiscConnHandle_) {
                if (event->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_SUCCESS) {
                    for (uint8_t i = 0; i < charDiscRsp.count; i++) {
                        hal_ble_char_t characteristic = {};
                        characteristic.version = BLE_API_VERSION;
                        characteristic.size = sizeof(hal_ble_char_t);
                        characteristic.char_ext_props = charDiscRsp.chars[i].char_ext_props;
                        characteristic.properties = toHalCharProps(charDiscRsp.chars[i].char_props);
                        characteristic.charHandles.version = BLE_API_VERSION;
                        characteristic.charHandles.size = sizeof(hal_ble_char_handles_t);
                        characteristic.charHandles.decl_handle = charDiscRsp.chars[i].handle_decl;
                        characteristic.charHandles.value_handle = charDiscRsp.chars[i].handle_value;
                        BleObject::toHalUUID(&charDiscRsp.chars[i].uuid, &characteristic.uuid);
                        if (gattc->addDiscoveredCharacteristic(characteristic) != SYSTEM_ERROR_NONE) {
                            LOG(ERROR, "There will have no memory to store discovered characteristics.");
                        }
                    }
                    hal_ble_attr_handle_t currEndHandle = charDiscRsp.chars[charDiscRsp.count - 1].handle_value;
                    if (currEndHandle < gattc->currDiscSvc_.end_handle) {
                        ble_gattc_handle_range_t handleRange = {};
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
                if (gattc->discoverySemaphore_) {
                    os_semaphore_give(gattc->discoverySemaphore_, false);
                }
            }
            break;
        }
        case BLE_GATTC_EVT_DESC_DISC_RSP: {
            const ble_gattc_evt_desc_disc_rsp_t& descDiscRsp = event->evt.gattc_evt.params.desc_disc_rsp;
            LOG_DEBUG(TRACE, "BLE GATT Client event: %d descriptors discovered.", descDiscRsp.count);
            if (gattc->isDiscovering_ && gattc->currDiscProcedure_ == DiscoveryProcedure::BLE_DISCOVERY_PROCEDURE_DESCRIPTORS &&
                    event->evt.gattc_evt.conn_handle == gattc->currDiscConnHandle_) {
                if (event->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_SUCCESS) {
                    for (uint8_t i = 0; i < descDiscRsp.count; i++) {
                        // It will report all attributes with 16-bits UUID, filter descriptors.
                        hal_ble_char_t* characteristic = gattc->findDiscoveredCharacteristic(descDiscRsp.descs[i].handle);
                        if (characteristic) {
                            switch (descDiscRsp.descs[i].uuid.uuid) {
                                case BLE_SIG_UUID_CHAR_USER_DESCRIPTION_DESC: {
                                    characteristic->charHandles.user_desc_handle = descDiscRsp.descs[i].handle;
                                } break;
                                case BLE_SIG_UUID_CLIENT_CHAR_CONFIG_DESC: {
                                    characteristic->charHandles.cccd_handle = descDiscRsp.descs[i].handle;
                                } break;
                                case BLE_SIG_UUID_SERVER_CHAR_CONFIG_DESC: {
                                    characteristic->charHandles.sccd_handle = descDiscRsp.descs[i].handle;
                                } break;
                                case BLE_SIG_UUID_CHAR_EXTENDED_PROPERTIES_DESC:
                                case BLE_SIG_UUID_CHAR_PRESENT_FORMAT_DESC:
                                case BLE_SIG_UUID_CHAR_AGGREGATE_FORMAT:
                                default: break;
                            }
                        }
                    }
                    hal_ble_attr_handle_t currEndHandle = descDiscRsp.descs[descDiscRsp.count - 1].handle;
                    if (currEndHandle < gattc->currDiscSvc_.end_handle) {
                        ble_gattc_handle_range_t handleRange = {};
                        handleRange.start_handle = currEndHandle + 1;
                        handleRange.end_handle = gattc->currDiscSvc_.end_handle;
                        int ret = sd_ble_gattc_descriptors_discover(gattc->currDiscConnHandle_, &handleRange);
                        if (ret == NRF_SUCCESS) {
                            return;
                        }
                        LOG(ERROR, "sd_ble_gattc_descriptors_discover() failed: %u", (unsigned)ret);
                    }
                }
                if (gattc->discoverySemaphore_) {
                    os_semaphore_give(gattc->discoverySemaphore_, false);
                }
            }
            break;
        }
        case BLE_GATTC_EVT_READ_RSP: {
            const ble_gattc_evt_read_rsp_t& readRsp = event->evt.gattc_evt.params.read_rsp;
            LOG_DEBUG(TRACE, "BLE GATT Client event: read response.");
            if (gattc->isDiscovering_ && event->evt.gattc_evt.conn_handle == gattc->currDiscConnHandle_) {
                if (event->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_SUCCESS) {
                    if (gattc->currDiscProcedure_ == DiscoveryProcedure::BLE_DISCOVERY_PROCEDURE_SERVICES) {
                        hal_ble_svc_t* service = gattc->findDiscoveredService(readRsp.handle);
                        if (service) {
                            service->uuid.type = BLE_UUID_TYPE_128BIT;
                            memcpy(service->uuid.uuid128, readRsp.data, BLE_SIG_UUID_128BIT_LEN);
                            if (gattc->readServiceUUID128IfNeeded()) {
                                return;
                            }
                        }
                    } else if (gattc->currDiscProcedure_ == DiscoveryProcedure::BLE_DISCOVERY_PROCEDURE_CHARACTERISTICS) {
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
                if (gattc->discoverySemaphore_) {
                    os_semaphore_give(gattc->discoverySemaphore_, false);
                }
            } else if (event->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_SUCCESS) {
                if (gattc->readAttrHandle_ == readRsp.handle && gattc->readBuf_ != nullptr) {
                    gattc->readLen_ = std::min(gattc->readLen_, (size_t)readRsp.len);
                    memcpy(gattc->readBuf_, readRsp.data, gattc->readLen_);
                }
                if (gattc->readWriteSemaphore_) {
                    os_semaphore_give(gattc->readWriteSemaphore_, false);
                }
            } else {
                LOG(ERROR, "BLE read characteristic failed: %d, handle: %d.", event->evt.gattc_evt.gatt_status, event->evt.gattc_evt.error_handle);
            }
            break;
        }
        case BLE_GATTC_EVT_WRITE_RSP: {
            LOG_DEBUG(TRACE, "BLE GATT Client event: write with response completed.");
            if (gattc->readWriteSemaphore_) {
                os_semaphore_give(gattc->readWriteSemaphore_, false);
            }
            break;
        }
        case BLE_GATTC_EVT_WRITE_CMD_TX_COMPLETE: {
            LOG_DEBUG(TRACE, "BLE GATT Client event: write without response completed.");
            if (gattc->readWriteSemaphore_) {
                os_semaphore_give(gattc->readWriteSemaphore_, false);
            }
            break;
        }
        case BLE_GATTC_EVT_HVX: {
            const ble_gattc_evt_hvx_t& hvx = event->evt.gattc_evt.params.hvx;
            LOG_DEBUG(TRACE, "BLE GATT Client event: data received. conn_handle: %d, attr_handle: %d, type: %d",
                    event->evt.gattc_evt.conn_handle, hvx.handle, hvx.type);
            if (hvx.type == BLE_GATT_HVX_INDICATION) {
                int ret = sd_ble_gattc_hv_confirm(event->evt.gattc_evt.conn_handle, hvx.handle);
                if (ret != NRF_SUCCESS) {
                    LOG(ERROR, "sd_ble_gattc_hv_confirm() failed: %u", (unsigned)ret);
                }
            }
            uint8_t* data = (uint8_t*)gattc->gattcDataRecPool_.alloc(hvx.len);
            if (data) {
                LOG_DEBUG(TRACE, "Received data address: %u, len: %u", (unsigned)data, hvx.len);
                BleObject::BleEventDispatcher::EventMessage msg;
                msg.evt.type = BLE_EVT_DATA_NOTIFIED;
                msg.evt.version = BLE_API_VERSION;
                msg.evt.size = sizeof(hal_ble_evts_t);
                msg.evt.params.data_rec.conn_handle = event->evt.gattc_evt.conn_handle;
                msg.evt.params.data_rec.attr_handle = hvx.handle;
                msg.evt.params.data_rec.offset = 0;
                msg.evt.params.data_rec.data_len = hvx.len;
                memcpy(data, hvx.data, hvx.len);
                msg.evt.params.data_rec.data = data;
                msg.hook = gattc->gattcEventProcessedHook;
                msg.hookContext = gattc;
                BleObject::getInstance().dispatcher()->enqueue(msg);
            }
            break;
        }
        case BLE_GATTC_EVT_TIMEOUT: {
            LOG_DEBUG(TRACE, "BLE GATT Client event: timeout, conn handle: %d, source: %d",
                    event->evt.gattc_evt.conn_handle, event->evt.gattc_evt.params.timeout.src);
            // Disconnect on GATT Client timeout event.
            int ret = sd_ble_gap_disconnect(event->evt.gattc_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (ret != NRF_SUCCESS) {
                LOG(ERROR, "sd_ble_gap_disconnect() failed: %u", (unsigned)ret);
            }
            break;
        }
        default: {
            break;
        }
    }
}

bool BleObject::initialized_ = false;

BleObject& BleObject::getInstance() {
    static BleObject instance;
    return instance;
}

int BleObject::init() {
    if (!initialized_) {
        // Configure the BLE stack using the default settings.
        // Fetch the start address of the application RAM.
        uint32_t appRamStart = 0;
        int ret = nrf_sdh_ble_default_cfg_set(BLE_CONN_CFG_TAG, &appRamStart);
        CHECK_NRF_RETURN(ret, sysError(ret));
        LOG_DEBUG(TRACE, "APP RAM start: 0x%08x", (unsigned)appRamStart);
        // Enable the stack
        uint32_t sdRamEnd = appRamStart;
        ret = nrf_sdh_ble_enable(&sdRamEnd);
        LOG_DEBUG(TRACE, "SoftDevice RAM end: 0x%08x", (unsigned)sdRamEnd);
        if (sdRamEnd >= appRamStart) {
            LOG(ERROR, "Need to change APP_RAM_BASE in linker script to be large than: 0x%08x", (unsigned)sdRamEnd - 0x20000000);
        }
        SPARK_ASSERT(sdRamEnd < appRamStart);
        CHECK_NRF_RETURN(ret, sysError(ret));
        CHECK(dispatcher_->init());
        CHECK(gap_->init());
        CHECK(broadcaster_->init());
        CHECK(observer_->init());
        CHECK(connectionsMgr_->init());
        CHECK(gatts_->init());
        CHECK(gattc_->init());
        initialized_ = true;
    }
    return SYSTEM_ERROR_NONE;
}

int BleObject::selectAntenna(hal_ble_ant_type_t antenna) const {
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
    } else {
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

int BleObject::toPlatformUUID(const hal_ble_uuid_t* halUuid, ble_uuid_t* uuid) {
    if (uuid == nullptr || halUuid == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (halUuid->type == BLE_UUID_TYPE_128BIT) {
        ble_uuid128_t uuid128;
        memcpy(uuid128.uuid128, halUuid->uuid128, BLE_SIG_UUID_128BIT_LEN);
        ret_code_t ret = sd_ble_uuid_vs_add(&uuid128, &uuid->type);
        CHECK_NRF_RETURN(ret, sysError(ret));
        ret = sd_ble_uuid_decode(BLE_SIG_UUID_128BIT_LEN, halUuid->uuid128, uuid);
        CHECK_NRF_RETURN(ret, sysError(ret));
    } else if (halUuid->type == BLE_UUID_TYPE_16BIT) {
        uuid->type = BLE_UUID_TYPE_BLE;
        uuid->uuid = halUuid->uuid16;
    } else {
        uuid->type = BLE_UUID_TYPE_UNKNOWN;
        uuid->uuid = halUuid->uuid16;
    }
    return SYSTEM_ERROR_NONE;
}

int BleObject::toHalUUID(const ble_uuid_t* uuid, hal_ble_uuid_t* halUuid) {
    if (uuid == nullptr || halUuid == nullptr) {
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


/**********************************************
 * Particle BLE APIs
 */
int hal_ble_lock(void* reserved) {
    return !s_bleMutex.lock();
}

int hal_ble_unlock(void* reserved) {
    return !s_bleMutex.unlock();
}

int hal_ble_stack_init(void* reserved) {
    BleLock lk;
    /* The SoftDevice has been enabled in core_hal.c */
    LOG_DEBUG(TRACE, "hal_ble_stack_init().");
    return BleObject::getInstance().init();
}

int hal_ble_select_antenna(hal_ble_ant_type_t antenna) {
    return BleObject::getInstance().selectAntenna(antenna);
}

int hal_ble_set_callback_on_events(on_ble_evt_cb_t callback, void* context) {
    BleLock lk;
    BleObject::getInstance().dispatcher()->onGenericEventCallback(callback, context);
    return SYSTEM_ERROR_NONE;
}

/**********************************************
 * BLE GAP APIs
 */
int hal_ble_gap_set_device_address(const hal_ble_addr_t* address) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_device_address().");
    return BleObject::getInstance().gap()->setDeviceAddress(address);
}

int hal_ble_gap_get_device_address(hal_ble_addr_t* address) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_device_address().");
    return BleObject::getInstance().gap()->getDeviceAddress(address);
}

int hal_ble_gap_set_device_name(const char* device_name, size_t len) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_device_name().");
    return BleObject::getInstance().gap()->setDeviceName(device_name, len);
}

int hal_ble_gap_get_device_name(char* device_name, size_t len) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_device_name().");
    return BleObject::getInstance().gap()->getDeviceName(device_name, len);
}

int hal_ble_gap_set_appearance(ble_sig_appearance_t appearance) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_appearance().");
    return BleObject::getInstance().gap()->setAppearance(appearance);
}

int hal_ble_gap_get_appearance(ble_sig_appearance_t* appearance) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_appearance().");
    return BleObject::getInstance().gap()->getAppearance(appearance);
}

int hal_ble_gap_set_ppcp(const hal_ble_conn_params_t* ppcp, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_ppcp().");
    return BleObject::getInstance().connMgr()->setPpcp(ppcp);
}

int hal_ble_gap_get_ppcp(hal_ble_conn_params_t* ppcp, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_ppcp().");
    return BleObject::getInstance().connMgr()->getPpcp(ppcp);
}

int hal_ble_gap_add_whitelist(const hal_ble_addr_t* addr_list, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_add_whitelist().");
    return BleObject::getInstance().gap()->addWhitelist(addr_list, len);
}

int hal_ble_gap_delete_whitelist(void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_delete_whitelist().");
    return BleObject::getInstance().gap()->deleteWhitelist();
}

int hal_ble_gap_set_tx_power(int8_t tx_power) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_tx_power().");
    return BleObject::getInstance().broadcaster()->setTxPower(tx_power);
}

int hal_ble_gap_get_tx_power(int8_t* tx_power, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_tx_power().");
    return BleObject::getInstance().broadcaster()->getTxPower(tx_power);
}

int hal_ble_gap_set_advertising_parameters(const hal_ble_adv_params_t* adv_params, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_advertising_parameters().");
    return BleObject::getInstance().broadcaster()->setAdvertisingParams(adv_params);
}

int hal_ble_gap_get_advertising_parameters(hal_ble_adv_params_t* adv_params, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_advertising_parameters().");
    return BleObject::getInstance().broadcaster()->getAdvertisingParams(adv_params);
}

int hal_ble_gap_set_advertising_data(const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_advertising_data().");
    return BleObject::getInstance().broadcaster()->setAdvertisingData(buf, len);
}

ssize_t hal_ble_gap_get_advertising_data(uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_advertising_data().");
    return BleObject::getInstance().broadcaster()->getAdvertisingData(buf, len);
}

int hal_ble_gap_set_scan_response_data(const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_scan_response_data().");
    return BleObject::getInstance().broadcaster()->setScanResponseData(buf, len);
}

ssize_t hal_ble_gap_get_scan_response_data(uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_scan_response_data().");
    return BleObject::getInstance().broadcaster()->getScanResponseData(buf, len);
}

int hal_ble_gap_start_advertising(void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_start_advertising().");
    return BleObject::getInstance().broadcaster()->startAdvertising();
}

int hal_ble_gap_set_auto_advertise(hal_ble_auto_adv_cfg_t config, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_auto_advertise().");
    return BleObject::getInstance().broadcaster()->setAutoAdvertiseScheme(config);
}

hal_ble_auto_adv_cfg_t hal_ble_gap_get_auto_advertise(void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_auto_advertise().");
    return BleObject::getInstance().broadcaster()->getAutoAdvertiseScheme();
}

int hal_ble_gap_stop_advertising(void) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_stop_advertising().");
    return BleObject::getInstance().broadcaster()->stopAdvertising();
}

bool hal_ble_gap_is_advertising(void) {
    BleLock lk;
    return BleObject::getInstance().broadcaster()->advertising();
}

int hal_ble_gap_set_scan_parameters(const hal_ble_scan_params_t* scan_params, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_scan_parameters().");
    return BleObject::getInstance().observer()->setScanParams(scan_params);
}

int hal_ble_gap_get_scan_parameters(hal_ble_scan_params_t* scan_params, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_scan_parameters().");
    return BleObject::getInstance().observer()->getScanParams(scan_params);
}

int hal_ble_gap_start_scan(on_ble_scan_result_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_start_scan().");
    return BleObject::getInstance().observer()->startScanning(callback, context);
}

bool hal_ble_gap_is_scanning(void) {
    BleLock lk;
    return BleObject::getInstance().observer()->scanning();
}

int hal_ble_gap_stop_scan(void) {
    // Do not acquire the lock here, otherwise another thread cannot cancel the scanning.
    LOG_DEBUG(TRACE, "hal_ble_gap_stop_scan().");
    return BleObject::getInstance().observer()->stopScanning();
}

int hal_ble_gap_connect(const hal_ble_addr_t* address, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_connect().");
    return BleObject::getInstance().connMgr()->connect(address);
}

bool hal_ble_gap_is_connecting(const hal_ble_addr_t* address) {
    BleLock lk;
    return BleObject::getInstance().connMgr()->connecting(address);
}

bool hal_ble_gap_is_connected(const hal_ble_addr_t* address) {
    return BleObject::getInstance().connMgr()->connected(address);
}

int hal_ble_gap_connect_cancel(const hal_ble_addr_t* address) {
    // Do not acquire the lock here, otherwise another thread cannot cancel the connection attempt.
    LOG_DEBUG(TRACE, "hal_ble_gap_connect_cancel().");
    return BleObject::getInstance().connMgr()->connectCancel(address);
}

int hal_ble_gap_disconnect(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_disconnect().");
    return BleObject::getInstance().connMgr()->disconnect(conn_handle);
}

int hal_ble_gap_update_connection_params(hal_ble_conn_handle_t conn_handle, const hal_ble_conn_params_t* conn_params, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_update_connection_params().");
    return BleObject::getInstance().connMgr()->updateConnectionParams(conn_handle, conn_params);
}

int hal_ble_gap_get_connection_params(hal_ble_conn_handle_t conn_handle, hal_ble_conn_params_t* conn_params, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_connection_params().");
    return BleObject::getInstance().connMgr()->getConnectionParams(conn_handle, conn_params);
}

int hal_ble_gap_get_rssi(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_rssi().");
    return 0;
}

/**********************************************
 * BLE GATT Server APIs
 */
int hal_ble_gatt_server_add_service(uint8_t type, const hal_ble_uuid_t* uuid, hal_ble_attr_handle_t* handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_add_service().");
    return BleObject::getInstance().gatts()->addService(type, uuid, handle);
}

int hal_ble_gatt_server_add_characteristic(const hal_ble_char_init_t* char_init, hal_ble_char_handles_t* char_handles, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_add_characteristic().");
    return BleObject::getInstance().gatts()->addCharacteristic(char_init->service_handle, char_init->properties, &char_init->uuid, char_init->description, char_handles);
}

int hal_ble_gatt_server_add_descriptor(const hal_ble_desc_init_t* desc_init, hal_ble_attr_handle_t* handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_add_descriptor().");
    return BleObject::getInstance().gatts()->addDescriptor(desc_init->char_handle, &desc_init->uuid, desc_init->descriptor, desc_init->len, handle);
}

ssize_t hal_ble_gatt_server_set_characteristic_value(hal_ble_attr_handle_t value_handle, const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_set_characteristic_value().");
    return BleObject::getInstance().gatts()->setValue(value_handle, buf, len);
}

ssize_t hal_ble_gatt_server_get_characteristic_value(hal_ble_attr_handle_t value_handle, uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_get_characteristic_value().");
    return BleObject::getInstance().gatts()->getValue(value_handle, buf, len);
}

/**********************************************
 * BLE GATT Client APIs
 */
int hal_ble_gatt_client_discover_all_services(hal_ble_conn_handle_t conn_handle, on_ble_disc_service_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_discover_all_services().");
    return BleObject::getInstance().gattc()->discoverServices(conn_handle, nullptr, callback, context);
}

int hal_ble_gatt_client_discover_service_by_uuid(hal_ble_conn_handle_t conn_handle, const hal_ble_uuid_t* uuid, on_ble_disc_service_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_discover_service_by_uuid().");
    return BleObject::getInstance().gattc()->discoverServices(conn_handle, uuid, callback, context);
}

int hal_ble_gatt_client_discover_characteristics(hal_ble_conn_handle_t conn_handle, const hal_ble_svc_t* service, on_ble_disc_char_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_discover_characteristics().");
    return BleObject::getInstance().gattc()->discoverCharacteristics(conn_handle, service, callback, context);
}

int hal_ble_gatt_client_discover_characteristics_by_uuid(hal_ble_conn_handle_t conn_handle, const hal_ble_svc_t* service, const hal_ble_uuid_t* uuid, on_ble_disc_char_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

bool hal_ble_gatt_client_is_discovering(hal_ble_conn_handle_t conn_handle) {
    BleLock lk;
    return BleObject::getInstance().gattc()->discovering(conn_handle);
}

int hal_ble_gatt_client_set_att_mtu(size_t att_mtu, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "ble_gatt_client_set_att_mtu().");
    return BleObject::getInstance().gattc()->setDesiredAttMtu(att_mtu);
}

int hal_ble_gatt_client_configure_cccd(hal_ble_conn_handle_t conn_handle, hal_ble_attr_handle_t cccd_handle, ble_sig_cccd_value_t cccd_value, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_configure_cccd().");
    if (conn_handle == BLE_CONN_HANDLE_INVALID || cccd_handle == BLE_INVALID_ATTR_HANDLE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (cccd_value > BLE_SIG_CCCD_VAL_INDICATION) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    uint8_t buf[2] = {0x00, 0x00};
    buf[0] = cccd_value;
    if (BleObject::getInstance().gattc()->writeAttribute(conn_handle, cccd_handle, buf, sizeof(buf), true) == 0) {
        return SYSTEM_ERROR_INTERNAL;
    }
    return SYSTEM_ERROR_NONE;
}

ssize_t hal_ble_gatt_client_write_with_response(hal_ble_conn_handle_t conn_handle, hal_ble_attr_handle_t value_handle, const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_write_with_response().");
    return BleObject::getInstance().gattc()->writeAttribute(conn_handle, value_handle, buf, len, true);
}

ssize_t hal_ble_gatt_client_write_without_response(hal_ble_conn_handle_t conn_handle, hal_ble_attr_handle_t value_handle, const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_write_without_response().");
    return BleObject::getInstance().gattc()->writeAttribute(conn_handle, value_handle, buf, len, false);
}

ssize_t hal_ble_gatt_client_read(hal_ble_conn_handle_t conn_handle, hal_ble_attr_handle_t value_handle, uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_read().");
    return BleObject::getInstance().gattc()->readAttribute(conn_handle, value_handle, buf, len);
}

#endif // HAL_PLATFORM_BLE
