/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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

#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_TRACE
#include "logging.h"
LOG_SOURCE_CATEGORY("hal.ble");

#include "ble_hal.h"

#if HAL_PLATFORM_BLE

extern "C" {
#include "rtl8721d.h"
#include "rtl8721d_efuse.h"
#include "rtk_coex.h"
}

#include <platform_opts_bt.h>
#include <os_sched.h>
#include <string.h>
#include <trace_app.h>
#include <gap.h>
#include <gap_adv.h>
#include <gap_scan.h>
#include <gap_bond_le.h>
#include <gap_le.h>
#include <gap_conn_le.h>
#include <profile_server.h>
#include <profile_client.h>
#include <gatt_builtin_services.h>
#include <gap_msg.h>
#include <simple_ble_service.h>
#include <bas.h>
#include <bte.h>
#include <gap_config.h>
#include <bt_flags.h>
#include <stdio.h>
#include "wifi_constants.h"
#include <wifi/wifi_conf.h>
#include "rtk_coex.h"
#include "ftl_int.h"
#include "bt_intf.h"

//FIXME
#include "app_msg.h"
#include "os_msg.h"
#include "os_task.h"

#include "delay_hal.h"
#include "device_code.h"
#include "concurrent_hal.h"
#include "static_recursive_mutex.h"
#include <mutex>
#include "spark_wiring_vector.h"
#include <string.h>
#include <memory>
#include "check.h"
#include "scope_guard.h"
#include "rtl_system_error.h"
#include "radio_common.h"
#include "freertos/wrapper.h"

#include "timer_hal.h"

#include "mbedtls/ecdh.h"
#include "mbedtls_util.h"
#include "spark_wiring_thread.h"

// FIXME
#undef OFF
#undef ON
#include "network/ncp/wifi/ncp.h"
#include "network/ncp/wifi/wifi_network_manager.h"
#include "network/ncp/wifi/wifi_ncp_client.h"
#include "rtl_sdk_support.h"

using spark::Vector;
using namespace particle;
using namespace particle::ble;

struct pcoex_reveng {
    uint32_t state;
    uint32_t unknown;
    _mutex* mutex;
};

extern "C" pcoex_reveng* pcoex[4];

extern "C" Rltk_wlan_t rltk_wlan_info[NET_IF_NUM];
extern "C" int rtw_coex_wifi_enable(void* priv, uint32_t state);
extern "C" int rtw_coex_bt_enable(void* priv, uint32_t state);
extern "C" void __real_bt_coex_handle_specific_evt(uint8_t* p, uint8_t len);
extern "C" void __wrap_bt_coex_handle_specific_evt(uint8_t* p, uint8_t len);

void __wrap_bt_coex_handle_specific_evt(uint8_t* p, uint8_t len) {
    const auto BT_COEX_EVENT_SCAN_MASK = 0xf0;
    const auto BT_COEX_EVENT_SCAN_START = 0x20;
    const auto BT_COEX_EVENT_SCAN_STOP = 0x00;
    // FIXME: This is a hack to prioritize BLE over WiFI while performing a BLE scan
    // This blocks any wifi comms (both RX and TX paths) while the scan is performing,
    // but for now this is the best solution we have for the coexistence issue.
    if (len >= 6) {
        if ((p[5] & BT_COEX_EVENT_SCAN_MASK) == BT_COEX_EVENT_SCAN_START) {
            // Scan start
            rtlk_bt_set_gnt_bt(PTA_BT);
        } else if ((p[5] & BT_COEX_EVENT_SCAN_MASK) == BT_COEX_EVENT_SCAN_STOP) {
            rtlk_bt_set_gnt_bt(PTA_AUTO);
        }
    }
	__real_bt_coex_handle_specific_evt(p, len);
}


namespace {

#define CHECK_RTL(_expr) \
        ({ \
            const auto _ret = _expr; \
            if (_ret != GAP_CAUSE_SUCCESS) { \
                _LOG_CHECKED_ERROR(_expr, rtl_ble_error_to_system(_ret)); \
                return rtl_ble_error_to_system(_ret); \
            } \
            _ret; \
        })

#define WAIT_TIMED(timeout_ms, what) ({ \
    system_tick_t _micros = HAL_Timer_Get_Micro_Seconds();                      \
    bool res = true;                                                            \
    while ((what)) {                                                            \
        system_tick_t dt = (HAL_Timer_Get_Micro_Seconds() - _micros);           \
        bool nok = (((timeout_ms * 1000) < dt) && (what));                      \
        if (nok) {                                                              \
            res = false;                                                        \
            break;                                                              \
        }                                                                       \
    }                                                                           \
    res;                                                                        \
})

constexpr uint32_t BLE_OPERATION_TIMEOUT_MS = 60000;
constexpr system_tick_t BLE_ENQUEUE_TIMEOUT_MS = 5000;

StaticRecursiveMutex s_bleMutex;

bool isUuidEqual(const hal_ble_uuid_t* uuid1, const hal_ble_uuid_t* uuid2) {
    if (uuid1->type != uuid2->type) {
        return false;
    }
    if (uuid1->type == BLE_UUID_TYPE_128BIT) {
        return memcmp(uuid1->uuid128, uuid2->uuid128, BLE_SIG_UUID_128BIT_LEN) == 0;
    } else {
        return uuid1->uuid16 == uuid2->uuid16;
    }
}

bool addressEqual(const hal_ble_addr_t& srcAddr, const hal_ble_addr_t& destAddr) {
    return (srcAddr.addr_type == destAddr.addr_type && !memcmp(srcAddr.addr, destAddr.addr, BLE_SIG_ADDR_LEN));
}

hal_ble_addr_t chipDefaultPublicAddress() {
    hal_ble_addr_t localAddr = {};
    uint8_t mac[BLE_SIG_ADDR_LEN] = {};
    if (hal_get_mac_address(HAL_DEVICE_MAC_BLE, mac, BLE_SIG_ADDR_LEN, nullptr) == BLE_SIG_ADDR_LEN) {
        // As per BLE spec, we store BLE data in little-endian
        for (uint8_t i = 0, j = BLE_SIG_ADDR_LEN - 1; i < BLE_SIG_ADDR_LEN; i++, j--) {
            localAddr.addr[i] = mac[j];
        }
        localAddr.addr_type = BLE_SIG_ADDR_TYPE_PUBLIC;
    }
    return localAddr;
}

T_GAP_IO_CAP toPlatformIoCaps(hal_ble_pairing_io_caps_t index) {
    constexpr T_GAP_IO_CAP iocaps[] = {
        GAP_IO_CAP_NO_INPUT_NO_OUTPUT,
        GAP_IO_CAP_DISPLAY_ONLY,
        GAP_IO_CAP_DISPLAY_YES_NO,
        GAP_IO_CAP_KEYBOARD_ONLY,
        GAP_IO_CAP_KEYBOARD_DISPLAY
    };
    return iocaps[index];
}

} //anonymous namespace


class BleEventDispatcher {
public:
    int start();
    int stop();

    static BleEventDispatcher& getInstance() {
        static BleEventDispatcher instance;
        return instance;
    }

    bool isThreadCurrent() const {
        if (!thread_) {
            return false;
        }

        return os_thread_current(nullptr) == thread_;
    }

private:
    BleEventDispatcher()
            : thread_(nullptr),
              allEvtQueue_(nullptr),
              ioEvtQueue_(nullptr),
              started_(false) {
    }
    ~BleEventDispatcher() = default;

    void handleIoMessage(T_IO_MSG message) const;
    static void bleEventDispatchThread(void *context);

    void cleanup();

    void* thread_;
    void* allEvtQueue_;
    void* ioEvtQueue_;
    bool started_;
    static constexpr uint8_t BLE_EVENT_THREAD_PRIORITY = 3; // Higher than application thread priority, which is 2
    static constexpr uint8_t MAX_NUMBER_OF_GAP_MESSAGE = 0x20;
    static constexpr uint8_t MAX_NUMBER_OF_IO_MESSAGE = 0x20;
    static constexpr uint8_t MAX_NUMBER_OF_EVENT_MESSAGE = (MAX_NUMBER_OF_GAP_MESSAGE + MAX_NUMBER_OF_IO_MESSAGE);
    static constexpr uint8_t BLE_EVENT_STOP = 0xff;
};

union RtlGapDevState {
    T_GAP_DEV_STATE state;
    uint8_t raw;
    static_assert(sizeof(state) == sizeof(raw), "Size of T_GAP_DEV_STATE does not match uint8_t");
};

class BleGapDevState {
public:
    BleGapDevState()
            : state_{},
              initSet_{false},
              advSet_{false},
              advSubSet_{false},
              scanSet_{false},
              connSet_{false} {
    }

    explicit BleGapDevState(RtlGapDevState s)
            : state_(s),
              initSet_{true},
              advSet_{true},
              advSubSet_{true},
              scanSet_{true},
              connSet_{true} {
    }

    BleGapDevState(BleGapDevState&& other) = default;
    BleGapDevState(const BleGapDevState& other) = default;

    BleGapDevState& init(uint8_t state) {
        state_.state.gap_init_state = state;
        initSet_ = true;
        return *this;
    }
    BleGapDevState& adv(uint8_t state) {
        state_.state.gap_adv_state = state;
        advSet_ = true;
        return *this;
    }
    BleGapDevState& advSub(uint8_t state) {
        state_.state.gap_adv_sub_state = state;
        advSubSet_ = true;
        return *this;
    }
    BleGapDevState& scan(uint8_t state) {
        state_.state.gap_scan_state = state;
        scanSet_ = true;
        return *this;
    }
    BleGapDevState& conn(uint8_t state) {
        state_.state.gap_conn_state = state;
        connSet_ = true;
        return *this;
    }

    bool operator==(const BleGapDevState& other) {
        auto a = sanitizedState();
        auto b = other.sanitizedState();
        return !memcmp(&a, &b, sizeof(a));
    }

    BleGapDevState& copySetParamsFrom(const BleGapDevState& other) {
        initSet_ = other.initSet_;
        advSet_ = other.advSet_;
        advSubSet_ = other.advSubSet_;
        scanSet_ = other.scanSet_;
        connSet_ = other.connSet_;
        return *this;
    }

    bool isNotInitialized() const {
        T_GAP_DEV_STATE state = {};
        return initSet_ && advSet_ && advSubSet_ && scanSet_ && connSet_ && !memcmp(&state, &state_, sizeof(state));
    }

    RtlGapDevState state() const {
        return state_;
    }

private:
    RtlGapDevState sanitizedState() const {
        auto state = state_;
        if (!initSet_) {
            state.state.gap_init_state = 0;
        }
        if (!advSet_) {
            state.state.gap_adv_state = 0;
        }
        if (!advSubSet_) {
            state.state.gap_adv_sub_state = 0;
        }
        if (!scanSet_) {
            state.state.gap_scan_state = 0;
        }
        if (!connSet_) {
            state.state.gap_conn_state = 0;
        }
        return state;
    }

    RtlGapDevState state_;
    bool initSet_;
    bool advSet_;
    bool advSubSet_;
    bool scanSet_;
    bool connSet_;
};

class BleGap {
public:
    int start();
    int stop();
    int init();

    bool initialized() const {
        return initialized_;
    }
    int setAppearance(uint16_t appearance) const;
    int setDeviceName(const char* deviceName, size_t len);
    int getDeviceName(char* deviceName, size_t len) const;
    int setDeviceAddress(const hal_ble_addr_t* address);
    int getDeviceAddress(hal_ble_addr_t* address) const;

    int setAdvertisingParameters(const hal_ble_adv_params_t* params);
    int getAdvertisingParameters(hal_ble_adv_params_t* params) const;
    int setAdvertisingData(const uint8_t* buf, size_t len);
    ssize_t getAdvertisingData(uint8_t* buf, size_t len) const;
    int setScanResponseData(const uint8_t* buf, size_t len);
    ssize_t getScanResponseData(uint8_t* buf, size_t len) const;
    int startAdvertising(bool wait = true); // TODO: always wait, now we have command thread to execute asynchronously
    int stopAdvertising(bool wait = true); // TODO: always wait, now we have command thread to execute asynchronously
    int notifyAdvStop();

    bool isAdvertising() const {
        return isAdvertising_;
    }

    bool scanning();
    int setScanParams(const hal_ble_scan_params_t* params);
    int getScanParams(hal_ble_scan_params_t* params) const;
    int startScanning(hal_ble_on_scan_result_cb_t callback, void* context);
    int stopScanning();

    int setPpcp(const hal_ble_conn_params_t* ppcp);
    int getPpcp(hal_ble_conn_params_t* ppcp) const;
    int connect(const hal_ble_conn_cfg_t* config, hal_ble_conn_handle_t* conn_handle);
    int connectCancel(const hal_ble_addr_t* address);
    ssize_t getAttMtu(hal_ble_conn_handle_t connHandle);
    int getConnectionInfo(hal_ble_conn_handle_t connHandle, hal_ble_conn_info_t* info);
    int disconnect(hal_ble_conn_handle_t connHandle);
    int disconnectAll();
    bool valid(hal_ble_conn_handle_t connHandle);

    bool connected(const hal_ble_addr_t* address) {
        std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
        if (address == nullptr) {
            return connections_.size() > 0;
        }
        if (fetchConnection(address)) {
            return true;
        }
        return false;
    }

    bool connecting() const {
        return connecting_;
    }

    int setPairingConfig(const hal_ble_pairing_config_t* config);
    int getPairingConfig(hal_ble_pairing_config_t* config) const;
    int setPairingAuthData(hal_ble_conn_handle_t connHandle, const hal_ble_pairing_auth_data_t* auth);
    int startPairing(hal_ble_conn_handle_t connHandle);
    int rejectPairing(hal_ble_conn_handle_t connHandle);
    bool isPairing(hal_ble_conn_handle_t connHandle);
    bool isPaired(hal_ble_conn_handle_t connHandle);

    int waitState(BleGapDevState state, system_tick_t timeout = BLE_STATE_DEFAULT_TIMEOUT, bool forcePoll = false);

    void handleDevStateChanged(T_GAP_DEV_STATE state, uint16_t cause);
    void handleConnectionStateChanged(uint8_t connHandle, T_GAP_CONN_STATE newState, uint16_t discCause);
    void handleMtuUpdated(uint8_t connHandle, uint16_t mtuSize);
    void handleConnParamsUpdated(uint8_t connHandle, uint8_t status, uint16_t cause);
    int handleAuthenStateChanged(uint8_t connHandle, uint8_t state, uint16_t cause);
    int handlePairJustWork(uint8_t connHandle);
    int handlePairPasskeyDisplay(uint8_t connHandle, bool displayOnly = true);
    int handlePairPasskeyInput(uint8_t connHandle);

    int onPeripheralLinkEventCallback(hal_ble_on_link_evt_cb_t cb, void* context) {
        std::pair<hal_ble_on_link_evt_cb_t, void*> callback(cb, context);
        CHECK_TRUE(periphEvtCallbacks_.append(callback), SYSTEM_ERROR_NO_MEMORY);
        return SYSTEM_ERROR_NONE;
    }

    int enqueue(uint8_t cmd) {
        CHECK_TRUE(cmdQueue_, SYSTEM_ERROR_INVALID_STATE);
        if (os_queue_put(cmdQueue_, &cmd, BLE_ENQUEUE_TIMEOUT_MS, nullptr)) {
            LOG(ERROR, "os_queue_put() failed.");
            SPARK_ASSERT(false);
        }
        return SYSTEM_ERROR_NONE;
    }

    void lockMode(bool lock) {
        bleInLockedMode_ = lock;
    }

    bool lockMode() {
        return bleInLockedMode_;
    }

    static BleGap& getInstance() {
        static BleGap instance;
        return instance;
    }

    int onAdvEventCallback(hal_ble_on_adv_evt_cb_t callback, void* context);
    void cancelAdvEventCallback(hal_ble_on_adv_evt_cb_t callback, void* context);

private:
    enum BlePairingState {
        BLE_PAIRING_STATE_NOT_INITIATED,
        BLE_PAIRING_STATE_INITIATED, // Central role only
        BLE_PAIRING_STATE_STARTED,
        BLE_PAIRING_STATE_USER_REQ_REJECT,
        BLE_PAIRING_STATE_SET_AUTH_DATA,
        BLE_PAIRING_STATE_REJECTED,
        BLE_PAIRING_STATE_PAIRED
    };

    enum BleGapCommand {
        BLE_CMD_EXIT_THREAD,
        BLE_CMD_STOP_ADV,
        BLE_CMD_STOP_ADV_NOTIFY,
        BLE_CMD_START_ADV
    };

    struct BleConnection {
        hal_ble_conn_info_t info;
        std::pair<hal_ble_on_link_evt_cb_t, void*> handler; // It is used for central link only.
        BlePairingState pairState;
    };

    BleGap()
            : cmdThread_(nullptr),
              cmdQueue_(nullptr),
              initialized_(false),
              btStackStarted_(false),
              state_{},
              addr_{},
              advParams_{},
              advTimeoutTimer_(nullptr),
              isScanning_(false),
              scanParams_{},
              scanSemaphore_(nullptr),
              scanResultCallback_(nullptr),
              context_(nullptr),
              periphEvtCallbacks_{},
              connectingAddr_{},
              connectSemaphore_(nullptr),
              disconnectSemaphore_(nullptr),
              disconnectingHandle_(BLE_INVALID_CONN_HANDLE),
              connecting_(false),
              ppcp_{},
              pairingConfig_{},
              devNameLen_(0),
              isAdvertising_(false),
              stateSemaphore_(nullptr),
              pairingLesc_(false),
              bleInLockedMode_(false) {
        advParams_.version = BLE_API_VERSION;
        advParams_.size = sizeof(hal_ble_adv_params_t);
        advParams_.interval = BLE_DEFAULT_ADVERTISING_INTERVAL;
        advParams_.timeout = BLE_DEFAULT_ADVERTISING_TIMEOUT;
        advParams_.type = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT;
        advParams_.filter_policy = BLE_ADV_FP_ANY;
        advParams_.inc_tx_power = false;
        advParams_.primary_phy = 0; // Not used

        scanParams_.version = BLE_API_VERSION;
        scanParams_.size = sizeof(hal_ble_scan_params_t);
        scanParams_.active = true;
        scanParams_.filter_policy = BLE_SCAN_FP_ACCEPT_ALL;
        scanParams_.interval = BLE_DEFAULT_SCANNING_INTERVAL;
        scanParams_.window = BLE_DEFAULT_SCANNING_WINDOW;
        scanParams_.timeout = BLE_DEFAULT_SCANNING_TIMEOUT;
        scanParams_.scan_phys = BLE_PHYS_AUTO;

        ppcp_.version = BLE_API_VERSION;
        ppcp_.size = sizeof(hal_ble_conn_params_t);
        ppcp_.min_conn_interval = BLE_DEFAULT_MIN_CONN_INTERVAL;
        ppcp_.max_conn_interval = BLE_DEFAULT_MAX_CONN_INTERVAL;
        ppcp_.slave_latency = BLE_DEFAULT_SLAVE_LATENCY;
        ppcp_.conn_sup_timeout = BLE_DEFAULT_CONN_SUP_TIMEOUT;

        memset(advData_, 0x00, sizeof(advData_));
        memset(scanRespData_, 0x00, sizeof(scanRespData_));
        advDataLen_ = scanRespDataLen_ = 0;
        /* Default advertising data. */
        // FLags
        advData_[advDataLen_++] = 0x02; // Length field of an AD structure, it is the length of the rest AD structure data.
        advData_[advDataLen_++] = BLE_SIG_AD_TYPE_FLAGS; // Type field of an AD structure.
        advData_[advDataLen_++] = BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE; // Payload field of an AD structure.

        pairingConfig_ = {
            .version = BLE_API_VERSION,
            .size = sizeof(hal_ble_pairing_config_t),
            .io_caps = BLE_IO_CAPS_NONE,
            .algorithm = BLE_PAIRING_ALGORITHM_AUTO,
            .reserved = {}
        };
    }
    ~BleGap() = default;

    T_GAP_ADTYPE toPlatformAdvEvtType(hal_ble_adv_evt_type_t type) const {
        switch (type) {
            case BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT:
            case BLE_ADV_CONNECTABLE_UNDIRECTED_EVT: {
                return GAP_ADTYPE_ADV_IND;
            }
            case BLE_ADV_CONNECTABLE_DIRECTED_EVT: {
                return GAP_ADTYPE_ADV_HDC_DIRECT_IND;
            }
            case BLE_ADV_NON_CONNECTABLE_NON_SCANABLE_UNDIRECTED_EVT:
            case BLE_ADV_NON_CONNECTABLE_NON_SCANABLE_DIRECTED_EVT: {
                return GAP_ADTYPE_ADV_NONCONN_IND;
            }
            case BLE_ADV_SCANABLE_UNDIRECTED_EVT:
            case BLE_ADV_SCANABLE_DIRECTED_EVT: {
                return GAP_ADTYPE_ADV_SCAN_IND;
            }
            default:{
                return GAP_ADTYPE_ADV_IND;
            }
        }
    }

    RtlGapDevState getState() const {
        RtlGapDevState s;
        s.raw = state_.raw;
        return s;
    }

    void notifyScanResult(T_LE_SCAN_INFO* advReport);
    hal_ble_scan_result_evt_t* getPendingResult(const hal_ble_addr_t& address);
    int addPendingResult(const hal_ble_scan_result_evt_t& resultEvt);
    void removePendingResult(const hal_ble_addr_t& address);
    void clearPendingResult();

    bool connectedAsBlePeripheral() const {
        for (auto& connection : connections_) {
            if (connection.info.role == BLE_ROLE_PERIPHERAL) {
                return true;
            }
        }
        return false;
    }

    BleConnection* fetchConnection(hal_ble_conn_handle_t connHandle);
    BleConnection* fetchConnection(const hal_ble_addr_t* address);
    int addConnection(BleConnection&& connection);
    void removeConnection(hal_ble_conn_handle_t connHandle);

    void notifyLinkEvent(const hal_ble_link_evt_t& event);

    static T_APP_RESULT gapEventCallback(uint8_t type, void *data);
    static void onAdvTimeoutTimerExpired(os_timer_t timer);
    static void bleCommandThread(void *context);

    struct BleAdvEventHandler {
        hal_ble_on_adv_evt_cb_t callback;
        void* context;
    };

    void* cmdThread_;
    void* cmdQueue_;
    bool initialized_;
    bool btStackStarted_;
    volatile RtlGapDevState state_;                 /**< This should be atomically r/w as the struct is <= uint32_t */
    hal_ble_addr_t addr_;
    hal_ble_adv_params_t advParams_;
    os_timer_t advTimeoutTimer_;                    /**< Timer for advertising timeout.  */
    volatile bool isScanning_;                      /**< If it is scanning or not. */
    hal_ble_scan_params_t scanParams_;              /**< BLE scanning parameters. */
    os_semaphore_t scanSemaphore_;                  /**< Semaphore to wait until the scan procedure completed. */
    hal_ble_on_scan_result_cb_t scanResultCallback_;    /**< Callback function on scan result. */
    void* context_;                                     /**< Context of the scan result callback function. */
    Vector<hal_ble_scan_result_evt_t> pendingResults_;
    Vector<std::pair<hal_ble_on_link_evt_cb_t, void*>> periphEvtCallbacks_;
    hal_ble_addr_t connectingAddr_;                 /**< Address of peer the Central is connecting to. */
    os_semaphore_t connectSemaphore_;               /**< Semaphore to wait until connection established. */
    std::pair<hal_ble_on_link_evt_cb_t, void*> centralLinkCbCache_; // It is used for central link only.
    os_semaphore_t disconnectSemaphore_;            /**< Semaphore to wait until connection disconnected. */
    volatile hal_ble_conn_handle_t disconnectingHandle_;    /**< Handle of connection to be disconnected. */
    Vector<BleConnection> connections_;
    volatile bool connecting_;
    hal_ble_conn_params_t ppcp_;
    hal_ble_pairing_config_t pairingConfig_;
    uint8_t advData_[BLE_MAX_ADV_DATA_LEN];         /**< Current advertising data. */
    size_t advDataLen_;                             /**< Current advertising data length. */
    uint8_t scanRespData_[BLE_MAX_ADV_DATA_LEN];    /**< Current scan response data. */
    size_t scanRespDataLen_;                        /**< Current scan response data length. */
    char devName_[BLE_MAX_DEV_NAME_LEN + 1];        // null-terminated
    size_t devNameLen_;
    static constexpr uint16_t BLE_ADV_TIMEOUT_EXT_MS = 50;
    volatile bool isAdvertising_;
    os_semaphore_t stateSemaphore_;
    uint8_t pairingLesc_;
    bool bleInLockedMode_;
    Vector<BleAdvEventHandler> advEventHandlers_;
    Mutex advEventMutex_;
    RecursiveMutex connectionsMutex_;

    static constexpr system_tick_t BLE_WAIT_STATE_POLL_PERIOD_MS = 10;
    static constexpr system_tick_t BLE_STATE_DEFAULT_TIMEOUT = 5000;

    static constexpr uint8_t BLE_CMD_THREAD_PRIORITY = 2;
    static constexpr uint16_t BLE_CMD_THREAD_STACK_SIZE = 2048;
    static constexpr uint8_t BLE_CMD_QUEUE_SIZE = 0x20;
};


class BleGatt {
public:
    struct Subscriber {
        hal_ble_conn_handle_t connHandle;
        ble_sig_cccd_value_t config;
    };

    struct CccdConfig {
        uint16_t index;
        Subscriber subscriber; // FIXME: now we assume that only one client is connected
    };

    struct BleCharacteristic {
        hal_ble_attr_handle_t handle;
        uint16_t index;
        hal_ble_on_char_evt_cb_t callback;
        void* context;
    };

    struct BleService {
        T_SERVER_ID id;
        Vector<T_ATTRIB_APPL> attrTable;
        hal_ble_uuid_t uuid;
        hal_ble_attr_handle_t startHandle;
        hal_ble_attr_handle_t endHandle; 
        Vector<BleCharacteristic> characteristics;
        Vector<CccdConfig> cccdConfigs;
    };

    int init();
    int deinit();
    int addService(uint8_t type, const hal_ble_uuid_t* uuid, hal_ble_attr_handle_t* svcHandle);
    int addCharacteristic(const hal_ble_char_init_t* charInit, hal_ble_char_handles_t* charHandles);
    int registerAttributeTable();
    ssize_t setValue(hal_ble_attr_handle_t attrHandle, const uint8_t* buf, size_t len);
    ssize_t getValue(hal_ble_attr_handle_t attrHandle, uint8_t* buf, size_t len);
    ssize_t notifyValue(hal_ble_attr_handle_t attrHandle, const uint8_t* buf, size_t len, bool ack);
    int removeSubscriber(hal_ble_conn_handle_t connHandle);

    bool discovering(hal_ble_conn_handle_t connHandle) const;
    int discoverServices(hal_ble_conn_handle_t connHandle, const hal_ble_uuid_t* uuid, hal_ble_on_disc_service_cb_t callback, void* context);
    int discoverCharacteristics(hal_ble_conn_handle_t connHandle, const hal_ble_svc_t* service, const hal_ble_uuid_t* uuid, hal_ble_on_disc_char_cb_t callback, void* context);
    int configureRemoteCCCD(const hal_ble_cccd_config_t* config);
    ssize_t writeAttribute(hal_ble_conn_handle_t connHandle, hal_ble_attr_handle_t attrHandle, const uint8_t* buf, size_t len, bool response);
    ssize_t readAttribute(hal_ble_conn_handle_t connHandle, hal_ble_attr_handle_t attrHandle, uint8_t* buf, size_t len);

    int setDesiredAttMtu(size_t attMtu);

    bool registered() const {
        return serviceRegistered_;
    }

    Vector<BleService>& services() {
        return services_;
    }

    static BleGatt& getInstance() {
        static BleGatt instance;
        return instance;
    }

private:
    BleGatt()
            : serviceRegistered_(false),
              attrConfigured_(false),
              isNotifying_(false),
              notifySemaphore_(nullptr),
              isDiscovering_(false),
              currDiscConnHandle_(BLE_INVALID_CONN_HANDLE),
              currDiscService_(nullptr),
              discoverySemaphore_(nullptr),
              discSvcUuid_(nullptr),
              discSvcCallback_(nullptr),
              discSvcContext_(nullptr),
              discCharCallback_(nullptr),
              discCharContext_(nullptr),
              clientId_(CLIENT_PROFILE_GENERAL_ID),
              clientCallbacks_{},
              serverCallbacks_(),
              currReadConnHandle_(BLE_INVALID_CONN_HANDLE),
              readAttrHandle_(BLE_INVALID_ATTR_HANDLE),
              readBuf_(nullptr),
              readLen_(0),
              readSemaphore_(nullptr),
              currWriteConnHandle_(BLE_INVALID_CONN_HANDLE),
              writeAttrHandle_(BLE_INVALID_ATTR_HANDLE),
              writeSemaphore_(nullptr),
              desiredAttMtu_(BLE_MAX_ATT_MTU_SIZE) {
        clientCallbacks_.discover_state_cb = onDiscoverStateCallback;
        clientCallbacks_.discover_result_cb = onDiscoverResultCallback;
        clientCallbacks_.read_result_cb = onReadResultCallback;
        clientCallbacks_.write_result_cb = onWriteResultCallback;
        clientCallbacks_.notify_ind_result_cb = onNotifyIndResultCallback;
        clientCallbacks_.disconnect_cb = onGattClientDisconnectCallback;

        serverCallbacks_.read_attr_cb = gattReadAttrCallback;
        serverCallbacks_.write_attr_cb = gattWriteAttrCallback;
        serverCallbacks_.cccd_update_cb = gattCccdUpdatedCallback;

        resetDiscoveryState();
    }
    ~BleGatt() = default;

    struct Publisher {
        hal_ble_on_char_evt_cb_t callback;
        void* context;
        hal_ble_conn_handle_t connHandle;
        hal_ble_attr_handle_t valueHandle;
    };

    T_ATTRIB_APPL* findAttribute(hal_ble_attr_handle_t attrHandle) {
        for (auto& svc : services_) {
            if (attrHandle < svc.startHandle && attrHandle > svc.endHandle) {
                continue;
            }
            for (const auto& charact : svc.characteristics) {
                if (charact.handle != attrHandle) {
                    continue;
                }
                LOG_DEBUG(TRACE, "Found attr, id:%d, index:%d", svc.id, charact.index);
                return &svc.attrTable[charact.index];
            }
        }
        return nullptr;
    }

    void resetDiscoveryState();
    hal_ble_char_t* findDiscoveredCharacteristic(hal_ble_attr_handle_t attrHandle);
    int addPublisher(hal_ble_conn_handle_t connHandle, hal_ble_attr_handle_t valueHandle, hal_ble_on_char_evt_cb_t callback, void* context);
    int removePublisher(hal_ble_conn_handle_t connHandle, hal_ble_attr_handle_t valueHandle);
    int removeAllPublishersOfConnection(hal_ble_conn_handle_t connHandle);

    static T_APP_RESULT gattServerEventCallback(T_SERVER_ID serviceId, void *pData);
    static T_APP_RESULT gattReadAttrCallback(uint8_t connHandle, T_SERVER_ID serviceId, uint16_t index, uint16_t offset, uint16_t *length, uint8_t **value);
    static T_APP_RESULT gattWriteAttrCallback(uint8_t connHandle, T_SERVER_ID serviceId, uint16_t index, T_WRITE_TYPE type, uint16_t length, uint8_t *value, P_FUN_WRITE_IND_POST_PROC *postProc);
    static void gattCccdUpdatedCallback(uint8_t connHandle, T_SERVER_ID serviceId, uint16_t index, uint16_t bits);

    static void onDiscoverStateCallback(uint8_t connHandle, T_DISCOVERY_STATE discoveryState);
    static void onDiscoverResultCallback(uint8_t connHandle, T_DISCOVERY_RESULT_TYPE type, T_DISCOVERY_RESULT_DATA data);
    static void onReadResultCallback(uint8_t connHandle, uint16_t cause, uint16_t handle, uint16_t size, uint8_t *value);
    static void onWriteResultCallback(uint8_t connHandle, T_GATT_WRITE_TYPE type, uint16_t handle, uint16_t cause, uint8_t credits);
    static T_APP_RESULT onNotifyIndResultCallback(uint8_t connHandle, bool notify, uint16_t handle, uint16_t size, uint8_t *value);
    static void onGattClientDisconnectCallback(uint8_t connHandle);

    bool serviceRegistered_;
    bool attrConfigured_;
    volatile bool isNotifying_;
    os_semaphore_t notifySemaphore_;                                /**< Semaphore to sync notify/indicate operation. */
    volatile bool isDiscovering_;                                   /**< If there is on-going discovery procedure. */
    hal_ble_conn_handle_t currDiscConnHandle_;                      /**< Current connection handle under which the service and characteristics to be discovered. */
    const hal_ble_svc_t* currDiscService_;                          /**< Used for discovering descriptors */
    os_semaphore_t discoverySemaphore_;                             /**< Semaphore to wait until the discovery procedure completed. */
    const hal_ble_uuid_t* discSvcUuid_;                             /**< Used only when discover service by UUID. */
    hal_ble_on_disc_service_cb_t discSvcCallback_;                  /**< Callback function on services discovered. */
    void* discSvcContext_;                                          /**< Context of services discovered callback function. */
    hal_ble_on_disc_char_cb_t discCharCallback_;                    /**< Callback function on characteristics discovered. */
    void* discCharContext_;                                         /**< Context of characteristics discovered callback function. */
    Vector<hal_ble_svc_t> discServices_;                            /**< Discover services. */
    Vector<hal_ble_char_t> discCharacteristics_;                    /**< Discovered characteristics. */
    T_CLIENT_ID clientId_;
    T_FUN_CLIENT_CBS clientCallbacks_;
    T_FUN_GATT_SERVICE_CBS serverCallbacks_;
    hal_ble_conn_handle_t currReadConnHandle_;
    hal_ble_attr_handle_t readAttrHandle_;                          /**< Current handle of which attribute to be read. */
    uint8_t* readBuf_;                                              /**< Current buffer to be filled for the read data. */
    size_t readLen_;                                                /**< Length of read data. */
    os_semaphore_t readSemaphore_;                                  /**< Semaphore to wait until the read operation completed. */
    hal_ble_conn_handle_t currWriteConnHandle_;
    hal_ble_attr_handle_t writeAttrHandle_;                          /**< Current handle of which attribute to be read. */
    os_semaphore_t writeSemaphore_;                                 /**< Semaphore to wait until the write operation completed. */
    size_t desiredAttMtu_;
    Vector<BleService> services_;
    Vector<Publisher> publishers_;
    static constexpr uint8_t MAX_ALLOWED_BLE_SERVICES = 5;
    static constexpr uint16_t CUSTOMER_SERVICE_START_HANDLE = 30;
    static constexpr uint8_t SERVICE_HANDLE_RANGE_RESERVED = 20;
};

int BleEventDispatcher::start() {
    if (started_) {
        // Already running
        return SYSTEM_ERROR_NONE;
    }
    SCOPE_GUARD({
        if (!started_) {
            cleanup();
        }
    });
    if (!ioEvtQueue_) {
        CHECK_TRUE(os_msg_queue_create(&ioEvtQueue_, MAX_NUMBER_OF_IO_MESSAGE, sizeof(T_IO_MSG)), SYSTEM_ERROR_INTERNAL);
    }
    if (!allEvtQueue_) {
        CHECK_TRUE(os_msg_queue_create(&allEvtQueue_, MAX_NUMBER_OF_EVENT_MESSAGE, sizeof(uint8_t)), SYSTEM_ERROR_INTERNAL);
    }
    if (!thread_) {
        CHECK_TRUE(os_thread_create(&thread_, "bleEvtThread", BLE_EVENT_THREAD_PRIORITY, bleEventDispatchThread, this, BLE_EVENT_THREAD_STACK_SIZE) == 0, SYSTEM_ERROR_INTERNAL);
    }
    CHECK_TRUE(gap_start_bt_stack(allEvtQueue_, ioEvtQueue_, MAX_NUMBER_OF_GAP_MESSAGE), SYSTEM_ERROR_INTERNAL);
    started_ = true;
    return SYSTEM_ERROR_NONE;
}

int BleEventDispatcher::stop() {
    gap_start_bt_stack(nullptr, nullptr, 0);
    cleanup();
    started_ = false;
    return SYSTEM_ERROR_NONE;
}

void BleEventDispatcher::cleanup() {
    if (thread_ && allEvtQueue_ && !isThreadCurrent()) {
        uint8_t ev = BLE_EVENT_STOP;
        SPARK_ASSERT(os_msg_send(allEvtQueue_, &ev, BLE_ENQUEUE_TIMEOUT_MS));
        os_thread_join(thread_);
        thread_ = nullptr;
    }
    if (allEvtQueue_) {
        if (!thread_) {
            os_msg_queue_delete(allEvtQueue_);
            allEvtQueue_ = nullptr;
        } else {
            uint8_t event;
            while (os_msg_recv(allEvtQueue_, &event, 0) == true) {}
        }
    }
    if (ioEvtQueue_) {
        if (!thread_) {
            os_msg_queue_delete(ioEvtQueue_);
            ioEvtQueue_ = nullptr;
        } else {
            T_IO_MSG message;
            while (os_msg_recv(ioEvtQueue_, &message, 0) == true) {}
        }
    }
}

void BleEventDispatcher::handleIoMessage(T_IO_MSG message) const {
    switch (message.type) {
        case IO_MSG_TYPE_BT_STATUS: {
            T_LE_GAP_MSG gap_msg;
            memcpy(&gap_msg, &message.u.param, sizeof(message.u.param));
            switch (message.subtype) {
                case GAP_MSG_LE_DEV_STATE_CHANGE: {
                    LOG_DEBUG(TRACE, "handleIoMessage, GAP_MSG_LE_DEV_STATE_CHANGE");
                    const auto& devStateChange = gap_msg.msg_data.gap_dev_state_change;
                    BleGap::getInstance().handleDevStateChanged(devStateChange.new_state, devStateChange.cause);
                    break;
                }
                case GAP_MSG_LE_CONN_STATE_CHANGE: {
                    LOG_DEBUG(TRACE, "handleIoMessage, GAP_MSG_LE_CONN_STATE_CHANGE");
                    const auto& connStateChange = gap_msg.msg_data.gap_conn_state_change;
                    BleGap::getInstance().handleConnectionStateChanged(connStateChange.conn_id, (T_GAP_CONN_STATE)connStateChange.new_state, connStateChange.disc_cause);
                    break;
                }
                case GAP_MSG_LE_CONN_MTU_INFO: {
                    LOG_DEBUG(TRACE, "handleIoMessage, GAP_MSG_LE_CONN_MTU_INFO");
                    const auto& mtuUpdated = gap_msg.msg_data.gap_conn_mtu_info;
                    BleGap::getInstance().handleMtuUpdated(mtuUpdated.conn_id, mtuUpdated.mtu_size);
                    break;
                }
                case GAP_MSG_LE_CONN_PARAM_UPDATE: {
                    LOG_DEBUG(TRACE, "handleIoMessage, GAP_MSG_LE_CONN_PARAM_UPDATE");
                    const auto& connParamsUpdated = gap_msg.msg_data.gap_conn_param_update;
                    BleGap::getInstance().handleConnParamsUpdated(connParamsUpdated.conn_id, connParamsUpdated.status, connParamsUpdated.cause);
                    break;
                }
                case GAP_MSG_LE_AUTHEN_STATE_CHANGE: {
                    LOG_DEBUG(TRACE, "handleIoMessage, GAP_MSG_LE_AUTHEN_STATE_CHANGE");
                    const auto& authenStateChanged = gap_msg.msg_data.gap_authen_state;
                    BleGap::getInstance().handleAuthenStateChanged(authenStateChanged.conn_id, authenStateChanged.new_state, authenStateChanged.status);
                    break;
                }
                case GAP_MSG_LE_BOND_JUST_WORK: {
                    LOG_DEBUG(TRACE, "handleIoMessage, GAP_MSG_LE_BOND_JUST_WORK");
                    const auto& pairJustWork = gap_msg.msg_data.gap_bond_just_work_conf;
                    BleGap::getInstance().handlePairJustWork(pairJustWork.conn_id);
                    break;
                }
                case GAP_MSG_LE_BOND_PASSKEY_DISPLAY: {
                    LOG_DEBUG(TRACE, "handleIoMessage, GAP_MSG_LE_BOND_PASSKEY_DISPLAY");
                    auto& passkeyDisplay = gap_msg.msg_data.gap_bond_passkey_display;
                    BleGap::getInstance().handlePairPasskeyDisplay(passkeyDisplay.conn_id);
                    break;
                }
                case GAP_MSG_LE_BOND_USER_CONFIRMATION: {
                    // Numeric comparison
                    LOG_DEBUG(TRACE, "handleIoMessage, GAP_MSG_LE_BOND_USER_CONFIRMATION");
                    auto& numericComparison = gap_msg.msg_data.gap_bond_user_conf;
                    BleGap::getInstance().handlePairPasskeyDisplay(numericComparison.conn_id, false);
                    break;
                }
                case GAP_MSG_LE_BOND_PASSKEY_INPUT: {
                    LOG_DEBUG(TRACE, "handleIoMessage: GAP_MSG_LE_BOND_PASSKEY_INPUT");
                    auto& passkeyInput = gap_msg.msg_data.gap_bond_passkey_input;
                    BleGap::getInstance().handlePairPasskeyInput(passkeyInput.conn_id);
                    break;
                }
                default: {
                    LOG_DEBUG(TRACE, "handleIoMessage: unknown subtype %d", message.subtype);
                    break;
                }
            }
            break;
        }
        case IO_MSG_TYPE_AT_CMD: {
            LOG_DEBUG(TRACE, "handleIoMessage: IO_MSG_TYPE_AT_CMD");
            break;
        }
        case IO_MSG_TYPE_QDECODE: {
            LOG_DEBUG(TRACE, "handleIoMessage: IO_MSG_TYPE_QDECODE");
            break;
        }
        default: break;
    }
}

void BleEventDispatcher::bleEventDispatchThread(void *context) {
    BleEventDispatcher* dispatcher = (BleEventDispatcher*)context;
    while (true) {
        uint8_t event;
        if (os_msg_recv(dispatcher->allEvtQueue_, &event, 0xFFFFFFFF) == true) {
            if (event == EVENT_IO_TO_APP) {
                T_IO_MSG message;
                if (os_msg_recv(dispatcher->ioEvtQueue_, &message, 0) == true) {
                    // Handled by application directly
                    dispatcher->handleIoMessage(message);
                }
            } else if (event == BLE_EVENT_STOP) {
                break;
            } else {
                // Handled by BT lib, which will finally invoke the registered callback.
                // Do not do time consuming stuff in the callbacks.
                gap_handle_msg(event);
            }
        }
    }
    os_thread_exit(nullptr);
}

void BleGap::bleCommandThread(void *context) {
    BleGap* gap = (BleGap*)context;
    while (true) {
        uint8_t command;
        if (!os_queue_take(gap->cmdQueue_, &command, CONCURRENT_WAIT_FOREVER, nullptr)) {
            if (command == BLE_CMD_STOP_ADV || command == BLE_CMD_STOP_ADV_NOTIFY) {
                if (gap->isAdvertising()) {
                    gap->stopAdvertising();
                    if (command == BLE_CMD_STOP_ADV_NOTIFY) {
                        gap->notifyAdvStop();
                    }
                }
            } else if (command == BLE_CMD_START_ADV) {
                gap->startAdvertising();
            } else if (command == BLE_CMD_EXIT_THREAD) {
                break;
            }
        }
    }
    os_thread_exit(nullptr);
}

int BleGap::init() {
    if (initialized_) {
        return SYSTEM_ERROR_NONE;
    }
    rtwRadioAcquire(RTW_RADIO_BLE);
    state_.raw = 0;

    SCOPE_GUARD({
        if (!initialized_) {
            stop();
        }
    });

    RCC_PeriphClockCmd(APBPeriph_UART1, APBPeriph_UART1_CLOCK, ENABLE);

    if (!cmdQueue_) {
        CHECK_TRUE(os_msg_queue_create(&cmdQueue_, BLE_CMD_QUEUE_SIZE, sizeof(uint8_t)), SYSTEM_ERROR_INTERNAL);
    }
    if (!cmdThread_) {
        CHECK_TRUE(os_thread_create(&cmdThread_, "bleCommandThread", BLE_CMD_THREAD_PRIORITY, bleCommandThread, this, BLE_CMD_THREAD_STACK_SIZE) == 0, SYSTEM_ERROR_INTERNAL);
    }
    CHECK_TRUE(os_semaphore_create(&scanSemaphore_, 1, 0) == 0, SYSTEM_ERROR_INTERNAL);
    CHECK_TRUE(os_semaphore_create(&stateSemaphore_, 1, 0) == 0, SYSTEM_ERROR_INTERNAL);
    CHECK_TRUE(os_semaphore_create(&connectSemaphore_, 1, 0) == 0, SYSTEM_ERROR_INTERNAL);
    CHECK_TRUE(os_semaphore_create(&disconnectSemaphore_, 1, 0) == 0, SYSTEM_ERROR_INTERNAL);
    // FreeRTOS timers are not supposed to be created with 0 timeout, enabling configASSERT will trigger it
    // We're just creating the timer with fixed period and we're going to change it anyway before starting advertising.
    CHECK_TRUE(os_timer_create(&advTimeoutTimer_, 1000, onAdvTimeoutTimerExpired, this, true, nullptr) == 0, SYSTEM_ERROR_INTERNAL);

    gap_config_max_le_link_num(BLE_MAX_LINK_COUNT);
    gap_config_max_le_paired_device(BLE_MAX_LINK_COUNT);

    rtwCoexRunDisable(0);

    CHECK_TRUE(bte_init(), SYSTEM_ERROR_INTERNAL);
    bt_coex_init();

    CHECK_TRUE(le_gap_init(BLE_MAX_LINK_COUNT), SYSTEM_ERROR_INTERNAL);
    uint8_t mtuReq = true;
    CHECK_RTL(le_set_gap_param(GAP_PARAM_SLAVE_INIT_GATT_MTU_REQ, sizeof(mtuReq), &mtuReq));
    CHECK(setAppearance(GAP_GATT_APPEARANCE_UNKNOWN));
    CHECK(setDeviceName(devName_, devNameLen_));
    CHECK(setAdvertisingParameters(&advParams_));
    CHECK(setScanParams(&scanParams_));

    constexpr uint8_t zeros[BLE_SIG_ADDR_LEN] = {0,0,0,0,0,0};
    if (!memcmp(addr_.addr, zeros, BLE_SIG_ADDR_LEN)) {
        CHECK(setDeviceAddress(nullptr));
    } else {
        CHECK(setDeviceAddress(&addr_));
    }

    uint8_t pairable = GAP_PAIRING_MODE_PAIRABLE;
    CHECK_RTL(gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(pairable), &pairable));
    // Bit mask: GAP_AUTHEN_BIT_BONDING_FLAG, GAP_AUTHEN_BIT_MITM_FLAG, GAP_AUTHEN_BIT_SC_FLAG
    uint16_t authFlags = 0;
    if (pairingConfig_.algorithm == BLE_PAIRING_ALGORITHM_AUTO) {
        // Prefer LESC
        authFlags = GAP_AUTHEN_BIT_SC_FLAG;
    } else if (pairingConfig_.algorithm == BLE_PAIRING_ALGORITHM_LESC_ONLY) {
        authFlags = GAP_AUTHEN_BIT_SC_ONLY_FLAG | GAP_AUTHEN_BIT_SC_FLAG;
    }
    CHECK_RTL(gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(authFlags), &authFlags));
    // IO Capabilities
    uint8_t ioCaps = toPlatformIoCaps(pairingConfig_.io_caps);
    CHECK_RTL(gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(ioCaps), &ioCaps));
    // Send smp security request automatically when connected
    uint8_t secReq = false;
    CHECK_RTL(le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(secReq), &secReq));
    uint16_t secReqFlags = authFlags;
    if (pairingConfig_.io_caps != BLE_IO_CAPS_NONE) {
        secReqFlags |= GAP_AUTHEN_BIT_MITM_FLAG;
    }
    CHECK_RTL(le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(secReqFlags), &secReqFlags));
    CHECK_RTL(gap_set_pairable_mode());

    /* register gap message callback */
    le_register_app_cb(gapEventCallback);

    CHECK(BleGatt::getInstance().init());

    initialized_ = true;
    return SYSTEM_ERROR_NONE;
}

int BleGap::start() {
    if (btStackStarted_) {
        return SYSTEM_ERROR_NONE;
    }

    CHECK(init());

    SCOPE_GUARD({
        if (!btStackStarted_) {
            stop();
        }
    });

    // NOTE: we will miss some of the initial events between bte_init() and when we start the event dispatcher
    CHECK(BleEventDispatcher::getInstance().start());
    // NOTE: we have to wait for the BLE stack to get initialized otherwise other operations
    // with it may cause race conditions, memory leaks and other problems
    CHECK(waitState(BleGapDevState().init(GAP_INIT_STATE_STACK_READY), BLE_STATE_DEFAULT_TIMEOUT, true /* force poll */));
    btStackStarted_ = true;
    return SYSTEM_ERROR_NONE;
}

int BleGap::stop() {
    // NOTE: ignoring errors
    if (btStackStarted_) {
        // Abort any commands, e.g. the re-adv command after disconnection.
        if (cmdThread_ && !os_thread_is_current(cmdThread_)) {
            if (enqueue(BLE_CMD_EXIT_THREAD) == SYSTEM_ERROR_NONE) {
                os_thread_join(cmdThread_);
                cmdThread_ = nullptr;
            }
        }
        if (cmdQueue_) {
            if (!cmdThread_) {
                os_queue_destroy(cmdQueue_, nullptr);
                cmdQueue_ = nullptr;
            } else {
                uint8_t command;
                while (!os_queue_take(cmdQueue_, &command, 0, nullptr)) {}
            }
        }

        // NOTE: we have to wait for the BLE stack to get initialized otherwise other operations
        // with it may cause race conditions, memory leaks and other problems
        waitState(BleGapDevState().init(GAP_INIT_STATE_STACK_READY), BLE_STATE_DEFAULT_TIMEOUT, true /* force poll */);
        disconnectAll();
        if (isAdvertising_) {
            // This will also wait for advertisements to stop
            stopAdvertising();
        }

        if (isScanning_) {
            stopScanning();
            le_scan_stop(); // Just in case
        }

        // Prevent BLE stack from generating coexistence events, otherwise we may leak memory
        gap_register_vendor_cb([](uint8_t cb_type, void *p_cb_data) -> void {
        });
        {
            // This call makes rtw_coex_run_enable(123, false) not cleanup the mutex
            // which might be still accessed by other coexistence tasks
            // but still allows rtw_coex_bt_enable to deinitialize BT coexistence.
            // Subsequently we restore the state with rtwCoexRunEnable() and rtw_coex_wifi_enable
            // will call into rtw_coex_run_enable(123, false) which will finally cleanup the mutex
            rtwCoexRunDisable(0);
            rtw_coex_bt_enable(*(void**)rltk_wlan_info[0].dev->priv, 0);
            HAL_Delay_Milliseconds(100);
            rtwCoexRunEnable(0);
            rtw_coex_wifi_enable(*(void**)rltk_wlan_info[0].dev->priv, 0);
            rtwCoexRunDisable(0);
            rtw_coex_wifi_enable(*(void**)rltk_wlan_info[0].dev->priv, 1);
            rtwCoexCleanupMutex(0);
        }
    }
    bte_deinit();
    // This shoulld be called after BT stack is stopped so that BLE events in queue can be safely cleared.
    BleEventDispatcher::getInstance().stop();

    isAdvertising_ = false;
    isScanning_ = false;
    initialized_ = false;
    btStackStarted_ = false;

    if (scanSemaphore_) {
        os_semaphore_destroy(scanSemaphore_);
        scanSemaphore_ = nullptr;
    }
    if (stateSemaphore_) {
        os_semaphore_destroy(stateSemaphore_);
        stateSemaphore_ = nullptr;
    }
    if (connectSemaphore_) {
        os_semaphore_destroy(connectSemaphore_);
        connectSemaphore_ = nullptr;
    }
    if (disconnectSemaphore_) {
        os_semaphore_destroy(disconnectSemaphore_);
        disconnectSemaphore_ = nullptr;
    }
    if (advTimeoutTimer_) {
        os_timer_destroy(advTimeoutTimer_, nullptr);
        advTimeoutTimer_ = nullptr;
    }

    BleGatt::getInstance().deinit();

    RCC_PeriphClockCmd(APBPeriph_UART1, APBPeriph_UART1_CLOCK, DISABLE);
    rtwRadioRelease(RTW_RADIO_BLE);
    return SYSTEM_ERROR_NONE;
}

int BleGap::setAppearance(uint16_t appearance) const {
    CHECK_RTL(le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance));
    return SYSTEM_ERROR_NONE;
}

int BleGap::setDeviceName(const char* deviceName, size_t len) {
    if (deviceName == nullptr || len == 0) {
        size_t len = CHECK(get_device_name(devName_, sizeof(devName_))); // null-terminated
        devNameLen_ = std::min(len, sizeof(devName_) - 1);
    } else {
        devNameLen_ = std::min(len, sizeof(devName_) - 1);
        memcpy(devName_, deviceName, devNameLen_);
        devName_[devNameLen_] = '\0';
    }
    CHECK_RTL(le_set_gap_param(GAP_PARAM_DEVICE_NAME, std::min(BLE_MAX_DEV_NAME_LEN, (int)devNameLen_), (void*)devName_));
    return SYSTEM_ERROR_NONE;
}

int BleGap::getDeviceName(char* deviceName, size_t len) const {
    CHECK_TRUE(deviceName, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(len, SYSTEM_ERROR_INVALID_ARGUMENT);
    uint16_t nameLen = std::min(len - 1, devNameLen_); // Reserve 1 byte for the NULL-terminated character.
    memcpy(deviceName, devName_, nameLen);
    deviceName[nameLen] = '\0';
    return SYSTEM_ERROR_NONE;
}

int BleGap::setDeviceAddress(const hal_ble_addr_t* address) {
    CHECK_FALSE(isAdvertising(), SYSTEM_ERROR_INVALID_STATE);
    CHECK_FALSE(scanning(), SYSTEM_ERROR_INVALID_STATE);
    CHECK_FALSE(address && addressEqual(*address, addr_), SYSTEM_ERROR_NONE);
    // RTL872x doesn't support changing the the public address.
    // But to be compatible with existing BLE platforms, it should accept nulllptr.
    hal_ble_addr_t defaultAddr = chipDefaultPublicAddress();
    if (!address || addressEqual(*address, defaultAddr)) {
        addr_ = defaultAddr;
        return SYSTEM_ERROR_NONE;
    }
    if (address->addr_type != BLE_SIG_ADDR_TYPE_RANDOM_STATIC) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if ((address->addr[5] & 0xC0) != 0xC0) {
        // For random static address, the two most significant bits of the address shall be equal to 1.
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    uint8_t addr[BLE_SIG_ADDR_LEN] = {};
    memcpy(addr, address->addr, BLE_SIG_ADDR_LEN);
    CHECK_RTL(le_cfg_local_identity_address(addr, GAP_IDENT_ADDR_RAND));
    CHECK_RTL(le_set_gap_param(GAP_PARAM_RANDOM_ADDR, BLE_SIG_ADDR_LEN, addr));
    addr_ = *address;

    uint8_t advLocalAddrType = GAP_LOCAL_ADDR_LE_PUBLIC;
    if (address->addr_type == BLE_SIG_ADDR_TYPE_RANDOM_STATIC) {
        advLocalAddrType = GAP_LOCAL_ADDR_LE_RANDOM;
    }
    CHECK_RTL(le_adv_set_param(GAP_PARAM_ADV_LOCAL_ADDR_TYPE, sizeof(advLocalAddrType), &advLocalAddrType));
    uint8_t scanLocalAddrType = GAP_LOCAL_ADDR_LE_PUBLIC;
    if (address->addr_type == BLE_SIG_ADDR_TYPE_RANDOM_STATIC) {
        scanLocalAddrType = GAP_LOCAL_ADDR_LE_RANDOM;
    }
    CHECK_RTL(le_scan_set_param(GAP_PARAM_SCAN_LOCAL_ADDR_TYPE, sizeof(uint8_t), &scanLocalAddrType));

    return SYSTEM_ERROR_NONE;
}

int BleGap::getDeviceAddress(hal_ble_addr_t* address) const {
    CHECK_TRUE(address, SYSTEM_ERROR_INVALID_ARGUMENT);
    *address = addr_;
    return SYSTEM_ERROR_NONE;
}

int BleGap::setAdvertisingParameters(const hal_ble_adv_params_t* params) {
    hal_ble_adv_params_t tempParams = {};
    tempParams.version = BLE_API_VERSION;
    tempParams.size = sizeof(hal_ble_adv_params_t);
    if (params == nullptr) {
        tempParams.type = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT;
        tempParams.filter_policy = BLE_ADV_FP_ANY;
        tempParams.interval = BLE_DEFAULT_ADVERTISING_INTERVAL;
        tempParams.timeout = BLE_DEFAULT_ADVERTISING_TIMEOUT;
        tempParams.inc_tx_power = false;
        tempParams.primary_phy = BLE_PHYS_AUTO;
    } else {
        memcpy(&tempParams, params, std::min(tempParams.size, params->size));
        if (tempParams.primary_phy != BLE_PHYS_AUTO && tempParams.primary_phy != BLE_PHYS_1MBPS && tempParams.primary_phy != BLE_PHYS_CODED) {
            LOG(ERROR, "primary_phy value not supported");
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
    }

    bool advertising = isAdvertising();
    SCOPE_GUARD ({
        if (advertising) {
            startAdvertising();
        }
    });
    CHECK(stopAdvertising());
    memcpy(&advParams_, &tempParams, std::min(advParams_.size, tempParams.size));
    uint8_t  advEvtType = toPlatformAdvEvtType(advParams_.type);
    uint8_t  advDirectType = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  advDirectAddr[GAP_BD_ADDR_LEN] = { 0 };
    uint8_t  advChannMap = GAP_ADVCHAN_ALL;
    uint8_t  advFilterPolicy = advParams_.filter_policy;
    uint16_t advInterval = advParams_.interval;
    CHECK_RTL(le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(advEvtType), &advEvtType));
    CHECK_RTL(le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR_TYPE, sizeof(advDirectType), &advDirectType));
    CHECK_RTL(le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR, sizeof(advDirectAddr), advDirectAddr));
    CHECK_RTL(le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(advChannMap), &advChannMap));
    CHECK_RTL(le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(advFilterPolicy), &advFilterPolicy));
    CHECK_RTL(le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(advInterval), &advInterval));
    CHECK_RTL(le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(advInterval), &advInterval));
    return SYSTEM_ERROR_NONE;
}

int BleGap::getAdvertisingParameters(hal_ble_adv_params_t* params) const {
    CHECK_TRUE(params, SYSTEM_ERROR_INVALID_ARGUMENT);
    memcpy(params, &advParams_, std::min(advParams_.size, params->size));
    return SYSTEM_ERROR_NONE;
}

int BleGap::setAdvertisingData(const uint8_t* buf, size_t len) {
    bool advertising = isAdvertising();
    SCOPE_GUARD ({
        if (advertising) {
            startAdvertising();
        }
    });
    CHECK(stopAdvertising());
    if (buf != nullptr) {
        len = std::min(len, (size_t)BLE_MAX_ADV_DATA_LEN);
        memcpy(advData_, buf, len);
    } else {
        len = 0;
    }
    advDataLen_ = len;
    CHECK_RTL(le_adv_set_param(GAP_PARAM_ADV_DATA, len, (void*)buf));
    return SYSTEM_ERROR_NONE;
}

ssize_t BleGap::getAdvertisingData(uint8_t* buf, size_t len) const {
    if (!buf) {
        return advDataLen_;
    }
    len = std::min(len, advDataLen_);
    memcpy(buf, advData_, len);
    return len;
}

int BleGap::setScanResponseData(const uint8_t* buf, size_t len) {
    bool advertising = isAdvertising();
    SCOPE_GUARD ({
        if (advertising) {
            startAdvertising();
        }
    });
    CHECK(stopAdvertising());
    if (buf != nullptr) {
        len = std::min(len, (size_t)BLE_MAX_ADV_DATA_LEN);
        memcpy(scanRespData_, buf, len);
    } else {
        len = 0;
    }
    scanRespDataLen_ = len;
    CHECK_RTL(le_adv_set_param(GAP_PARAM_SCAN_RSP_DATA, len, (void*)buf));
    return SYSTEM_ERROR_NONE;
}

ssize_t BleGap::getScanResponseData(uint8_t* buf, size_t len) const {
    if (!buf) {
        return scanRespDataLen_;
    }
    len = std::min(len, scanRespDataLen_);
    memcpy(buf, scanRespData_, len);
    return len;
}

int BleGap::onAdvEventCallback(hal_ble_on_adv_evt_cb_t callback, void* context) {
    std::lock_guard<Mutex> lk(advEventMutex_);
    BleAdvEventHandler handler = {};
    handler.callback = callback;
    handler.context = context;
    CHECK_TRUE(advEventHandlers_.append(handler), SYSTEM_ERROR_NO_MEMORY);
    return SYSTEM_ERROR_NONE;
}

void BleGap::cancelAdvEventCallback(hal_ble_on_adv_evt_cb_t callback, void* context) {
    std::lock_guard<Mutex> lk(advEventMutex_);
    for (int i = 0; i < advEventHandlers_.size(); i = i) {
        const auto& handler = advEventHandlers_[i];
        if (handler.callback == callback && handler.context == context) {
            advEventHandlers_.removeAt(i);
            continue;
        }
        i++;
    }
}

int BleGap::startAdvertising(bool wait) {
    if (isAdvertising_) {
        return SYSTEM_ERROR_NONE;
    }

    if (wait) {
        if (BleEventDispatcher::getInstance().isThreadCurrent()) {
            // Can't block event processing thread
            wait = false;
        }
    }
    RtlGapDevState s;
    CHECK_RTL(le_get_gap_param(GAP_PARAM_DEV_STATE, &s.state));
    if (s.state.gap_adv_state == GAP_ADV_STATE_STOP) {
        // Previous stopAdvertising() caused a race condition in the BT stack, try to recover
        // Ignore error here
        auto r = le_adv_stop();
        if (wait) {
            if (r != GAP_CAUSE_SUCCESS || waitState(BleGapDevState().adv(GAP_ADV_STATE_IDLE))) {
                LOG(ERROR, "Failed to get notified that advertising has stopped, resetting stack");
                CHECK(stop());
                CHECK(init());
            }
        }
    }

    bool ok = false;
    SCOPE_GUARD({
        if (!ok) {
            isAdvertising_ = false;
        }
    });
    if (!BleGatt::getInstance().registered()) {
        if (btStackStarted_) {
            if (!wait) {
                // Prevent from blocking
                return SYSTEM_ERROR_INVALID_STATE;
            }
            CHECK(stop());
            // The attribute table needs to be registered before BT stack starts.
            // Now we assume that the attribute table is finalized.
            // Register it to BT stack
            CHECK(init());
        }
        CHECK(BleGatt::getInstance().registerAttributeTable());
    }
    isAdvertising_ = true; // Set it to true here, because stop() will be called if attribute table is not registered yet.

    CHECK(start());

    SCOPE_GUARD ({
        // NOTE: this will run first before the other SCOPE_GUARD, make sure to look at 'ok' state as well
        if ((!isAdvertising() || !ok) && os_timer_is_active(advTimeoutTimer_, nullptr)) {
            os_timer_change(advTimeoutTimer_, OS_TIMER_CHANGE_STOP, hal_interrupt_is_isr() ? true : false, 0, 0, nullptr);
        }
    });
    if (advParams_.timeout != 0) { // 0 for advertising infinitely
        if (os_timer_change(advTimeoutTimer_, OS_TIMER_CHANGE_PERIOD, hal_interrupt_is_isr() ? true : false, advParams_.timeout * 10 + BLE_ADV_TIMEOUT_EXT_MS, 0, nullptr)) {
            LOG(ERROR, "Failed to start timer.");
            return SYSTEM_ERROR_INTERNAL;
        }
    }

    uint8_t advEvtType = toPlatformAdvEvtType(advParams_.type);
    if (connectedAsBlePeripheral()) {
        advEvtType = GAP_ADTYPE_ADV_SCAN_IND;
    }
    CHECK_RTL(le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(advEvtType), &advEvtType));

    CHECK_RTL(le_adv_start());

    if (wait) {
        LOG_DEBUG(TRACE, "Starting advertising...");
        if (waitState(BleGapDevState().adv(GAP_ADV_STATE_ADVERTISING))) {
            LOG(ERROR, "Failed to get notified that advertising has started");
        }
    }
    ok = true;
    LOG_DEBUG(TRACE, "Advertising started");
    return SYSTEM_ERROR_NONE;
}

int BleGap::stopAdvertising(bool wait) {
    if (wait) {
        if (BleEventDispatcher::getInstance().isThreadCurrent()) {
            // Can't block event processing thread
            wait = false;
        }
    }
    if (!isAdvertising_) {
        return SYSTEM_ERROR_NONE;
    }
    // In case of invoking startAdvertising(false) previously
    // Update: now we use BLE command queue to make sure adv state is correctly set
    // if (wait && state_.state.gap_adv_state != GAP_ADV_STATE_ADVERTISING) {
    //     if (waitState(BleGapDevState().adv(GAP_ADV_STATE_ADVERTISING), BLE_STATE_DEFAULT_TIMEOUT, true)) {
    //         LOG(ERROR, "Failed to get notified that advertising has started");
    //     }
    // }
    isAdvertising_ = false;
    CHECK_RTL(le_adv_stop());
    if (os_timer_is_active(advTimeoutTimer_, nullptr)) {
        os_timer_change(advTimeoutTimer_, OS_TIMER_CHANGE_STOP, hal_interrupt_is_isr() ? true : false, 0, 0, nullptr);
    }
    if (wait) {
        LOG_DEBUG(TRACE, "Stopping advertising...");
        if (waitState(BleGapDevState().adv(GAP_ADV_STATE_IDLE))) {
            LOG(ERROR, "Failed to get notified that advertising has stopped");
        }
    }
    LOG_DEBUG(TRACE, "Advertising stopped");
    return SYSTEM_ERROR_NONE;
}

int BleGap::notifyAdvStop() {
    hal_ble_adv_evt_t advEvent = {};
    advEvent.type = BLE_EVT_ADV_STOPPED;
    advEvent.params.reason = BLE_ADV_STOPPED_REASON_TIMEOUT;
    std::lock_guard<Mutex> lk(advEventMutex_);
    for (const auto& handler : advEventHandlers_) {
        if (handler.callback) {
            handler.callback(&advEvent, handler.context);
        }
    }
    return SYSTEM_ERROR_NONE;
}

void BleGap::onAdvTimeoutTimerExpired(os_timer_t timer) {
    BleGap* gap;
    os_timer_get_id(timer, (void**)&gap);
    gap->enqueue(BLE_CMD_STOP_ADV_NOTIFY);
}

bool BleGap::scanning() {
    return isScanning_;
}

int BleGap::setScanParams(const hal_ble_scan_params_t* params) {
    CHECK_TRUE(params, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(params->interval >= BLE_SCAN_INTERVAL_MIN, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(params->interval <= BLE_SCAN_INTERVAL_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(params->window >= BLE_SCAN_WINDOW_MIN, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(params->window <= BLE_SCAN_WINDOW_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(params->window <= params->interval, SYSTEM_ERROR_INVALID_ARGUMENT);
    memcpy(&scanParams_, params, std::min(scanParams_.size, params->size));
    scanParams_.size = sizeof(hal_ble_scan_params_t);
    scanParams_.version = BLE_API_VERSION;
    uint8_t filterDuplicate = GAP_SCAN_FILTER_DUPLICATE_ENABLE;
    CHECK_RTL(le_scan_set_param(GAP_PARAM_SCAN_MODE, sizeof(uint8_t), &scanParams_.active));
    CHECK_RTL(le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(uint16_t), &scanParams_.interval));
    CHECK_RTL(le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(uint16_t), &scanParams_.window));
    CHECK_RTL(le_scan_set_param(GAP_PARAM_SCAN_FILTER_POLICY, sizeof(uint8_t), &scanParams_.filter_policy));
    CHECK_RTL(le_scan_set_param(GAP_PARAM_SCAN_FILTER_DUPLICATES, sizeof(uint8_t), &filterDuplicate));
    return SYSTEM_ERROR_NONE;
}

int BleGap::getScanParams(hal_ble_scan_params_t* params) const {
    CHECK_TRUE(params, SYSTEM_ERROR_INVALID_ARGUMENT);
    memcpy(params, &scanParams_, std::min(scanParams_.size, params->size));
    return SYSTEM_ERROR_NONE;
}

int BleGap::startScanning(hal_ble_on_scan_result_cb_t callback, void* context) {
    CHECK(start());
    CHECK_FALSE(isScanning_, SYSTEM_ERROR_INVALID_STATE);

    SCOPE_GUARD ({
        if (isScanning_) {
            const int LE_SCAN_STOP_RETRIES = 10;
            for (int i = 0; i < LE_SCAN_STOP_RETRIES; i++) {
                // This has seen failing a number of times at least
                // with btgap logging enabled. Retry a few times just in case,
                // otherwise the next scan operation fails with invalid state.
                auto r = le_scan_stop();
                if (r == GAP_CAUSE_SUCCESS) {
                    if (!waitState(BleGapDevState().scan(GAP_SCAN_STATE_IDLE))) {
                        isScanning_ = false;
                    }
                    break;
                }
                HAL_Delay_Milliseconds(10);
            }
            if (isScanning_) {
                int ret = stop();
                SPARK_ASSERT(ret == SYSTEM_ERROR_NONE);
                ret = init();
                SPARK_ASSERT(ret == SYSTEM_ERROR_NONE);
                ret = start();
                SPARK_ASSERT(ret == SYSTEM_ERROR_NONE);
            }
            isScanning_ = false;
            clearPendingResult();
        }
    });
    scanResultCallback_ = callback;
    context_ = context;
    CHECK_RTL(le_scan_start());
    isScanning_ = true;
    // GAP_SCAN_STATE_SCANNING may be propagated immediately following the GAP_SCAN_STATE_START
    if (waitState(BleGapDevState().scan(GAP_SCAN_STATE_SCANNING))) {
        return SYSTEM_ERROR_TIMEOUT;
    }
    // To be consistent with Gen3, the scan proceedure is blocked for now,
    // so we can simply wait for the semaphore to be given without introducing a dedicated timer.
    os_semaphore_take(scanSemaphore_, (scanParams_.timeout == BLE_SCAN_TIMEOUT_UNLIMITED) ? CONCURRENT_WAIT_FOREVER : (scanParams_.timeout * 10), false);
    return SYSTEM_ERROR_NONE;
}

int BleGap::stopScanning() {
    if (!isScanning_) {
        return SYSTEM_ERROR_NONE;
    }
    os_semaphore_give(scanSemaphore_, false);
    return SYSTEM_ERROR_NONE;
}

// This function is called from thread
void BleGap::notifyScanResult(T_LE_SCAN_INFO* advReport) {
    if (!isScanning_ || !scanResultCallback_) {
        return;
    }

    hal_ble_addr_t newAddr = {};
    newAddr.addr_type = (ble_sig_addr_type_t)advReport->remote_addr_type;
    memcpy(newAddr.addr, advReport->bd_addr, BLE_SIG_ADDR_LEN);

    hal_ble_scan_result_evt_t* pendingResult = getPendingResult(newAddr);
    if (advReport->adv_type != GAP_ADV_EVT_TYPE_SCAN_RSP) {
        if (pendingResult || advReport->data_len < 3) { // The advertising data should at least contains AD flag (len + type + AD flag)
            // Repeated adv packet
            return;
        }
        hal_ble_scan_result_evt_t result = {};
        result.rssi = advReport->rssi;
        if ((advReport->adv_type == GAP_ADV_EVT_TYPE_UNDIRECTED) || (advReport->adv_type == GAP_ADV_EVT_TYPE_DIRECTED)) {
            result.type.connectable = true;
            result.type.scannable = true;
        } else {
            result.type.connectable = false;
            result.type.scannable = false;
        }
        if (advReport->adv_type == GAP_ADV_EVT_TYPE_SCANNABLE) {
            result.type.scannable = true;
        }
        if (advReport->adv_type == GAP_ADV_EVT_TYPE_DIRECTED) {
            result.type.directed = true;
        } else {
            result.type.directed = false;
        }
        result.type.extended_pdu = false;
        result.peer_addr = newAddr;
        result.adv_data_len = advReport->data_len;
        result.adv_data = advReport->data;
        if (!scanParams_.active || !result.type.scannable) {
            // No scan response data is expected.
            scanResultCallback_(&result, context_);
            return;
        }
        uint8_t* advData = (uint8_t*)malloc(result.adv_data_len);
        if (!advData) {
            return;
        }
        memcpy(advData, result.adv_data, result.adv_data_len);
        result.adv_data = advData;
        if (addPendingResult(result) != SYSTEM_ERROR_NONE) {
            free(advData);
        }
    } else {
        if (!pendingResult) {
            // We received the scan response data, but the advertising data should come first. Abort it.
            return;
        }
        pendingResult->sr_data_len = advReport->data_len;
        pendingResult->sr_data = advReport->data;
        scanResultCallback_(pendingResult, context_);
        removePendingResult(newAddr);
    }
}

hal_ble_scan_result_evt_t* BleGap::getPendingResult(const hal_ble_addr_t& address) {
    for (auto& result : pendingResults_) {
        if (addressEqual(result.peer_addr, address)) {
            return &result;
        }
    }
    return nullptr;
}

int BleGap::addPendingResult(const hal_ble_scan_result_evt_t& result) {
    if (getPendingResult(result.peer_addr) != nullptr) {
        return SYSTEM_ERROR_INTERNAL;
    }
    CHECK_TRUE(pendingResults_.append(result), SYSTEM_ERROR_NO_MEMORY);
    return SYSTEM_ERROR_NONE;
}

void BleGap::removePendingResult(const hal_ble_addr_t& address) {
    for (int i = 0; i < pendingResults_.size();) {
        if (addressEqual(pendingResults_[i].peer_addr, address)) {
            if (pendingResults_[i].adv_data) {
                free(pendingResults_[i].adv_data);
            }
            pendingResults_.removeAt(i);
            continue;
        }
        i++;
    }
}

void BleGap::clearPendingResult() {
    for (const auto& result : pendingResults_) {
        // Notify the pending results, in case that they are not expecting scan response data.
        scanResultCallback_(&result, context_);
        if (result.adv_data) {
            free(result.adv_data);
        }
    }
    pendingResults_.clear();
}

ssize_t BleGap::getAttMtu(hal_ble_conn_handle_t connHandle) {
    std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
    const auto connection = fetchConnection(connHandle);
    CHECK_TRUE(connection, SYSTEM_ERROR_NOT_FOUND);
    return connection->info.att_mtu;
}

int BleGap::getConnectionInfo(hal_ble_conn_handle_t connHandle, hal_ble_conn_info_t* info) {
    std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
    const BleConnection* connection = fetchConnection(connHandle);
    CHECK_TRUE(connection, SYSTEM_ERROR_NOT_FOUND);
    uint16_t size = std::min(connection->info.size, info->size);
    memcpy(info, &connection->info, size);
    return SYSTEM_ERROR_NONE;
}

int BleGap::setPpcp(const hal_ble_conn_params_t* ppcp) {
    CHECK_TRUE(ppcp, SYSTEM_ERROR_INVALID_ARGUMENT);
    if (ppcp->min_conn_interval != BLE_SIG_CP_MIN_CONN_INTERVAL_NONE) {
        CHECK_TRUE(ppcp->min_conn_interval >= BLE_SIG_CP_MIN_CONN_INTERVAL_MIN, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK_TRUE(ppcp->min_conn_interval <= BLE_SIG_CP_MIN_CONN_INTERVAL_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    }
    if (ppcp->max_conn_interval != BLE_SIG_CP_MAX_CONN_INTERVAL_NONE) {
        CHECK_TRUE(ppcp->max_conn_interval >= BLE_SIG_CP_MAX_CONN_INTERVAL_MIN, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK_TRUE(ppcp->max_conn_interval <= BLE_SIG_CP_MAX_CONN_INTERVAL_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    }
    CHECK_TRUE(ppcp->slave_latency < BLE_SIG_CP_SLAVE_LATENCY_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    if (ppcp->conn_sup_timeout != BLE_SIG_CP_CONN_SUP_TIMEOUT_NONE) {
        CHECK_TRUE(ppcp->conn_sup_timeout >= BLE_SIG_CP_CONN_SUP_TIMEOUT_MIN, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK_TRUE(ppcp->conn_sup_timeout <= BLE_SIG_CP_CONN_SUP_TIMEOUT_MAX, SYSTEM_ERROR_INVALID_ARGUMENT);
    }
    memcpy(&ppcp_, ppcp, std::min(ppcp_.size, ppcp->size));
    return SYSTEM_ERROR_NONE;
}

int BleGap::getPpcp(hal_ble_conn_params_t* ppcp) const {
    CHECK_TRUE(ppcp, SYSTEM_ERROR_INVALID_ARGUMENT);
    memcpy(ppcp, &ppcp_, std::min(ppcp_.size, ppcp->size));
    return SYSTEM_ERROR_NONE;
}

int BleGap::connect(const hal_ble_conn_cfg_t* config, hal_ble_conn_handle_t* connHandle) {
    CHECK_FALSE(connecting_, SYSTEM_ERROR_INVALID_STATE); // Device is connecting by peer device.
    CHECK_TRUE(connections_.size() < BLE_MAX_LINK_COUNT, SYSTEM_ERROR_LIMIT_EXCEEDED);
    CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(connHandle, SYSTEM_ERROR_INVALID_ARGUMENT);

    // Make sure that event dispatching is started
    CHECK(start());

    // Stop scanning first to give the scanning semaphore if possible.
    CHECK(stopScanning());
    SCOPE_GUARD ({
        connectingAddr_ = {};
        connecting_ = false;
    });
    const hal_ble_conn_params_t* connParams = nullptr;
    if (config->conn_params == nullptr) {
        connParams = &ppcp_;
    } else {
        connParams = config->conn_params;
    }
    T_GAP_LE_CONN_REQ_PARAM platformConnParams;
    platformConnParams.scan_interval = scanParams_.interval;
    platformConnParams.scan_window = scanParams_.window;
    platformConnParams.conn_interval_min = connParams->min_conn_interval;
    platformConnParams.conn_interval_max = connParams->max_conn_interval;
    platformConnParams.conn_latency = connParams->slave_latency;
    platformConnParams.supv_tout = connParams->conn_sup_timeout;
    platformConnParams.ce_len_min = 2 * (platformConnParams.conn_interval_min - 1);
    platformConnParams.ce_len_max = 2 * (platformConnParams.conn_interval_max - 1);
    // NOTE: 2M and CODED PHYs are not supported. Refer to where GAP_CONN_PARAM_1M is defined.
    // TODO: As per the description of the API, it requires an existing connection?
    CHECK_RTL(le_set_conn_param(GAP_CONN_PARAM_1M, &platformConnParams));

    memcpy(&connectingAddr_, &config->address, sizeof(hal_ble_addr_t));
    uint8_t connLocalAddrType = GAP_LOCAL_ADDR_LE_PUBLIC;
    if (addr_.addr_type == BLE_SIG_ADDR_TYPE_RANDOM_STATIC) {
        connLocalAddrType = GAP_LOCAL_ADDR_LE_RANDOM;
    }
    centralLinkCbCache_.first = config->callback;
    centralLinkCbCache_.second = config->context;
    connecting_ = true;
    CHECK_RTL(le_connect(GAP_CONN_PARAM_1M, connectingAddr_.addr, (T_GAP_REMOTE_ADDR_TYPE)connectingAddr_.addr_type,
        (T_GAP_LOCAL_ADDR_TYPE)connLocalAddrType, BLE_DEFAULT_SCANNING_TIMEOUT));
    if (os_semaphore_take(connectSemaphore_, BLE_OPERATION_TIMEOUT_MS, false)) {
        SPARK_ASSERT(false);
        return SYSTEM_ERROR_TIMEOUT;
    }
    std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
    BleConnection* connection = fetchConnection(&config->address);
    CHECK_TRUE(connection, SYSTEM_ERROR_INTERNAL);
    *connHandle = connection->info.conn_handle;
    return SYSTEM_ERROR_NONE;
}

int BleGap::connectCancel(const hal_ble_addr_t* address) {
    CHECK_TRUE(connecting_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(address, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(addressEqual(connectingAddr_, *address), SYSTEM_ERROR_INVALID_ARGUMENT);
    // NOTE: there is no API to cancel on-going connection attempt
    // We have to wait until it is connected followed by calling the disconnect API
    if (!WAIT_TIMED(BLE_OPERATION_TIMEOUT_MS, connecting_)) {
        return SYSTEM_ERROR_TIMEOUT;
    }
    BleConnection* connection = nullptr;
    {
        std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
        connection = fetchConnection(address);
    }
    CHECK_TRUE(connection, SYSTEM_ERROR_NONE); // Connection is not established
    CHECK(disconnect(connection->info.conn_handle));
    return SYSTEM_ERROR_NONE;
}

int BleGap::disconnect(hal_ble_conn_handle_t connHandle) {
    {
        std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
        CHECK_TRUE(fetchConnection(connHandle), SYSTEM_ERROR_NOT_FOUND);
    }
    SCOPE_GUARD ({
        disconnectingHandle_ = BLE_INVALID_CONN_HANDLE;
    });
    disconnectingHandle_ = connHandle;
    CHECK_RTL(le_disconnect(connHandle));
    if (os_semaphore_take(disconnectSemaphore_, BLE_OPERATION_TIMEOUT_MS, false)) {
        SPARK_ASSERT(false);
        return SYSTEM_ERROR_TIMEOUT;
    }
    return SYSTEM_ERROR_NONE;
}

int BleGap::disconnectAll() {
    while (1) {
        BleConnection* connection = nullptr;
        {
            std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
            if (connections_.size() > 0) {
                connection = &connections_[0];
            }
        }
        if (!connection) {
            break;
        }
        disconnect(connection->info.conn_handle);
    }
    return SYSTEM_ERROR_NONE;
}

void BleGap::notifyLinkEvent(const hal_ble_link_evt_t& event) {
    std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
    BleConnection* connection = fetchConnection(event.conn_handle);
    if (connection) {
        if (connection->info.role == BLE_ROLE_CENTRAL) {
            if (connection->handler.first) {
                connection->handler.first(&event, connection->handler.second);
            }
        } else {
            for (const auto& handler : periphEvtCallbacks_) {
                if (handler.first) {
                    handler.first(&event, handler.second);
                }
            }
        }
    }
}

bool BleGap::valid(hal_ble_conn_handle_t connHandle) {
    std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
    const BleConnection* connection = fetchConnection(connHandle);
    return connection != nullptr;
}

BleGap::BleConnection* BleGap::fetchConnection(hal_ble_conn_handle_t connHandle) {
    CHECK_TRUE(connHandle != BLE_INVALID_CONN_HANDLE, nullptr);
    for (auto& connection : connections_) {
        if (connection.info.conn_handle == connHandle) {
            return &connection;
        }
    }
    return nullptr;
}

BleGap::BleConnection* BleGap::fetchConnection(const hal_ble_addr_t* address) {
    CHECK_TRUE(address, nullptr);
    for (auto& connection : connections_) {
        if (addressEqual(connection.info.address, *address)) {
            return &connection;
        }
    }
    return nullptr;
}

int BleGap::addConnection(BleConnection&& connection) {
    std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
    CHECK_TRUE(fetchConnection(connection.info.conn_handle) == nullptr, SYSTEM_ERROR_INTERNAL);
    CHECK_TRUE(connections_.append(std::move(connection)), SYSTEM_ERROR_NO_MEMORY);
    return SYSTEM_ERROR_NONE;
}

void BleGap::removeConnection(hal_ble_conn_handle_t connHandle) {
    std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
    size_t i = 0;
    for (const auto& connection : connections_) {
        if (connection.info.conn_handle == connHandle) {
            connections_.removeAt(i);
            return;
        }
        i++;
    }
}

int BleGap::setPairingConfig(const hal_ble_pairing_config_t* config) {
    CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_FALSE(connected(nullptr), SYSTEM_ERROR_INVALID_STATE);
    CHECK_FALSE(isScanning_, SYSTEM_ERROR_INVALID_STATE);
    bool adv = isAdvertising();
    bool ok = false;
    SCOPE_GUARD({
        if (ok && adv) {
            startAdvertising();
        }
    });
    if (btStackStarted_) {
        CHECK(stop());
        CHECK(init());
    }
    // Bit mask: GAP_AUTHEN_BIT_BONDING_FLAG, GAP_AUTHEN_BIT_MITM_FLAG, GAP_AUTHEN_BIT_SC_FLAG
    uint16_t authFlags = 0;
    if (config->algorithm == BLE_PAIRING_ALGORITHM_AUTO) {
        // Prefer LESC
        authFlags = GAP_AUTHEN_BIT_SC_FLAG;
    } else if (config->algorithm == BLE_PAIRING_ALGORITHM_LESC_ONLY) {
        authFlags = GAP_AUTHEN_BIT_SC_ONLY_FLAG | GAP_AUTHEN_BIT_SC_FLAG;
    }
    CHECK_RTL(gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(authFlags), &authFlags));
    // IO Capabilities
    uint8_t ioCaps = toPlatformIoCaps(config->io_caps);
    CHECK_RTL(gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(ioCaps), &ioCaps));
    uint16_t secReqFlags = authFlags;
    if (config->io_caps != BLE_IO_CAPS_NONE) {
        secReqFlags |= GAP_AUTHEN_BIT_MITM_FLAG;
    }
    CHECK_RTL(le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(secReqFlags), &secReqFlags));
    pairingConfig_ = {};
    pairingConfig_.size = sizeof(hal_ble_pairing_config_t);
    memcpy(&pairingConfig_, config, std::min(pairingConfig_.size, config->size));

    CHECK(start());

    ok = true;
    return SYSTEM_ERROR_NONE;
}

int BleGap::getPairingConfig(hal_ble_pairing_config_t* config) const {
    CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);
    memcpy(config, &pairingConfig_, std::min(pairingConfig_.size, config->size));
    return SYSTEM_ERROR_NONE;
}

int BleGap::setPairingAuthData(hal_ble_conn_handle_t connHandle, const hal_ble_pairing_auth_data_t* auth) {
    CHECK_TRUE(auth, SYSTEM_ERROR_INVALID_ARGUMENT);
    std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
    BleConnection* connection = fetchConnection(connHandle);
    CHECK_TRUE(connection, SYSTEM_ERROR_NOT_FOUND);
    if (auth->type == BLE_PAIRING_AUTH_DATA_NUMERIC_COMPARISON) {
        CHECK_TRUE(connection->pairState == BLE_PAIRING_STATE_STARTED, SYSTEM_ERROR_INVALID_STATE);
        connection->pairState = BLE_PAIRING_STATE_SET_AUTH_DATA;
        if (auth->params.equal) {
            CHECK_RTL(le_bond_user_confirm(connHandle, GAP_CFM_CAUSE_ACCEPT));
        } else {
            CHECK_RTL(le_bond_user_confirm(connHandle, GAP_CFM_CAUSE_REJECT));
        }
    }
    else if (auth->type == BLE_PAIRING_AUTH_DATA_PASSKEY) {
        CHECK_TRUE(connection->pairState == BLE_PAIRING_STATE_STARTED, SYSTEM_ERROR_INVALID_STATE);
        connection->pairState = BLE_PAIRING_STATE_SET_AUTH_DATA;
        for (uint8_t i = 0; i < BLE_PAIRING_PASSKEY_LEN; i++) {
            if (!std::isdigit(auth->params.passkey[i])) {
                LOG(ERROR, "Invalid digits.");
                CHECK_RTL(le_bond_passkey_input_confirm(connHandle, 0, GAP_CFM_CAUSE_REJECT));
                return SYSTEM_ERROR_INVALID_ARGUMENT;
            }
        }
        uint32_t passkey = std::atoi((const char*)auth->params.passkey);
        CHECK_RTL(le_bond_passkey_input_confirm(connHandle, passkey, GAP_CFM_CAUSE_ACCEPT));
    }
    else {
        LOG(ERROR, "SYSTEM_ERROR_NOT_SUPPORTED");
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    return SYSTEM_ERROR_NONE;
}

int BleGap::startPairing(hal_ble_conn_handle_t connHandle) {
    std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
    BleConnection* connection = fetchConnection(connHandle);
    CHECK_TRUE(connection, SYSTEM_ERROR_NOT_FOUND);
    CHECK_TRUE(connection->pairState == BLE_PAIRING_STATE_NOT_INITIATED ||
               connection->pairState == BLE_PAIRING_STATE_REJECTED, SYSTEM_ERROR_INVALID_STATE);
    connection->pairState = BLE_PAIRING_STATE_INITIATED;
    auto ret = le_bond_pair(connHandle);
    if (ret != GAP_CAUSE_SUCCESS) {
        hal_ble_link_evt_t linkEvent = {};
        linkEvent.type = BLE_EVT_PAIRING_STATUS_UPDATED;
        linkEvent.conn_handle = connection->info.conn_handle;
        linkEvent.params.pairing_status.status = rtl_ble_error_to_system(ret);
        notifyLinkEvent(linkEvent);
        connection->pairState = BLE_PAIRING_STATE_NOT_INITIATED;
        return rtl_ble_error_to_system(ret);
    }
    return SYSTEM_ERROR_NONE;
}

/*
 * User may reject pairing under the following circumstances

 * If just work is used:
 *   1. calling rejectPairing() on BLE_EVT_PAIRING_REQUEST_RECEIVED received
 * 
 * If passkey display is used:
 *   1. calling rejectPairing() on BLE_EVT_PAIRING_REQUEST_RECEIVED received
 *   2. calling rejectPairing() on BLE_EVT_PAIRING_PASSKEY_DISPLAY received
 * 
 * If numeric comparison is used:
 *   1. calling rejectPairing() on BLE_EVT_PAIRING_REQUEST_RECEIVED received
 *   2. calling rejectPairing() on BLE_EVT_PAIRING_NUMERIC_COMPARISON received
 *   3. choosing "NO" on BLE_EVT_PAIRING_NUMERIC_COMPARISON received
 *   4. not setting authentication data on BLE_EVT_PAIRING_NUMERIC_COMPARISON received
 * 
 * If passkey input is used:
 *   1. calling rejectPairing() on BLE_EVT_PAIRING_REQUEST_RECEIVED received
 *   2. calling rejectPairing() on BLE_EVT_PAIRING_PASSKEY_INPUT received
 *   3. entering wrong passkey on BLE_EVT_PAIRING_PASSKEY_INPUT received
 *   4. not setting authentication data on BLE_EVT_PAIRING_PASSKEY_INPUT received
 * 
 * Other circumstances are considered to continue the pairing process。
 */
int BleGap::rejectPairing(hal_ble_conn_handle_t connHandle) {
    std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
    BleConnection* connection = fetchConnection(connHandle);
    CHECK_TRUE(connection, SYSTEM_ERROR_NOT_FOUND);
    // Note: If user called rejectPairing() after setPairingAuthData(), it won't take any effect
    if (connection->pairState == BLE_PAIRING_STATE_INITIATED || connection->pairState == BLE_PAIRING_STATE_STARTED) {
        connection->pairState = BLE_PAIRING_STATE_USER_REQ_REJECT;
    }
    return SYSTEM_ERROR_NONE;
}

bool BleGap::isPairing(hal_ble_conn_handle_t connHandle) {
    std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
    BleConnection* connection = fetchConnection(connHandle);
    CHECK_TRUE(connection, false);
    return connection->pairState == BLE_PAIRING_STATE_INITIATED ||
           connection->pairState == BLE_PAIRING_STATE_STARTED ||
           connection->pairState == BLE_PAIRING_STATE_SET_AUTH_DATA ||
           connection->pairState == BLE_PAIRING_STATE_USER_REQ_REJECT;
}

bool BleGap::isPaired(hal_ble_conn_handle_t connHandle) {
    std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
    BleConnection* connection = fetchConnection(connHandle);
    CHECK_TRUE(connection, false);
    return connection->pairState == BLE_PAIRING_STATE_PAIRED;
}

int BleGap::waitState(BleGapDevState state, system_tick_t timeout, bool forcePoll) {
    auto current = BleGapDevState(getState());
    if (forcePoll || current.isNotInitialized() || !btStackStarted_) {
        // Poll, as some of the initial events are missing
        RtlGapDevState s;
        auto start = hal_timer_millis(nullptr);
        while (hal_timer_millis(nullptr) - start < timeout) {
            CHECK_RTL(le_get_gap_param(GAP_PARAM_DEV_STATE, &s.state));
            if (BleGapDevState(s).copySetParamsFrom(state) == state) {
                return SYSTEM_ERROR_NONE;
            }
            HAL_Delay_Milliseconds(BLE_WAIT_STATE_POLL_PERIOD_MS);
        }
    } else {
        auto end = hal_timer_millis(nullptr) + timeout;
        for (auto now = hal_timer_millis(nullptr); now < end; now = hal_timer_millis(nullptr)) {
            os_semaphore_take(stateSemaphore_, end - now, false);
            if (BleGapDevState(getState()).copySetParamsFrom(state) == state) {
                return SYSTEM_ERROR_NONE;
            }
        }
    }
    LOG(ERROR, "Timeout waiting state");
    return SYSTEM_ERROR_TIMEOUT;
}

void BleGap::handleDevStateChanged(T_GAP_DEV_STATE newState, uint16_t cause) {
    LOG_DEBUG(TRACE, "GAP state updated, init:%d, adv:%d, adv-sub:%d, scan:%d, conn:%d, cause:%d",
                        newState.gap_init_state, newState.gap_adv_state, newState.gap_adv_sub_state, newState.gap_scan_state, newState.gap_conn_state, cause);
    RtlGapDevState nState;
    nState.state = newState;
    // NOTE: this event is generated before the connection is established
    if (newState.gap_adv_state != state_.state.gap_adv_state && newState.gap_adv_state == GAP_ADV_STATE_IDLE) {
        if (newState.gap_adv_sub_state == GAP_ADV_TO_IDLE_CAUSE_CONN) {
            isAdvertising_ = false; // adv timer has higher priority, just in case that the BLE_CMD_STOP_ADV_NOTIFY
                                    // has been enqueued, followed by nontifying the ADV stopped event. Under
                                    // certain circumstance notifying the ADV stopped event will cause BLE re-adv,
                                    // e.g. in BLE listening mode.
            if (os_timer_is_active(advTimeoutTimer_, nullptr)) {
                os_timer_change(advTimeoutTimer_, OS_TIMER_CHANGE_STOP, hal_interrupt_is_isr() ? true : false, 0, 0, nullptr);
            }
        }
    }
    state_.raw = nState.raw;
    os_semaphore_give(stateSemaphore_, false);
}

void BleGap::handleConnectionStateChanged(uint8_t connHandle, T_GAP_CONN_STATE newState, uint16_t discCause) {
    std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
    switch (newState) {
        case GAP_CONN_STATE_CONNECTING: {
            LOG_DEBUG(TRACE, "Connecting...");
            connecting_ = true;
            break;
        }
        case GAP_CONN_STATE_DISCONNECTING: {
            LOG_DEBUG(TRACE, "Disconnecting...");
            break;
        }
        case GAP_CONN_STATE_DISCONNECTED: {
            LOG_DEBUG(TRACE, "GAP_CONN_STATE_DISCONNECTED");
            BleConnection* connection = fetchConnection(connHandle);
            if (!connection || (connection && addressEqual(connectingAddr_, connection->info.address))) {
                if (connecting_) {
                    connecting_ = false;
                    // Central failed to connect
                    os_semaphore_give(connectSemaphore_, false);
                }
                if (!connection) {
                    return;
                }
            }
            if ((discCause != (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE)) && (discCause != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE))) {
                LOG(TRACE, "handleConnectionStateChanged: connection lost cause 0x%x", discCause);
            }
            LOG_DEBUG(TRACE, "Disconnected, handle:%d, cause:0x%x", connHandle, discCause);
            // If the disconnection is initiated by application.
            if (disconnectingHandle_ == connection->info.conn_handle) {
                os_semaphore_give(disconnectSemaphore_, false);
            }
#if HAL_PLATFORM_BLE_ACTIVE_EVENT
            {
#else
            else {
#endif
                // Notify the disconnected event.
                hal_ble_link_evt_t evt = {};
                evt.type = BLE_EVT_DISCONNECTED;
                evt.conn_handle = connHandle;
                evt.params.disconnected.reason = (uint8_t)discCause;
                notifyLinkEvent(evt);
            }
            hal_ble_role_t role = connection->info.role;
            removeConnection(connHandle);
            BleGatt::getInstance().removeSubscriber(connHandle);
            if (role == BLE_ROLE_PERIPHERAL) {
                // FIXME: check whether it's enabled?
                if (isAdvertising()) {
                    enqueue(BLE_CMD_STOP_ADV);
                }
                enqueue(BLE_CMD_START_ADV);
            }
            break;
        }
        case GAP_CONN_STATE_CONNECTED: {
            LOG_DEBUG(TRACE, "GAP_CONN_STATE_CONNECTED");
            hal_ble_conn_params_t connParams = {};
            connParams.version = BLE_API_VERSION;
            connParams.size = sizeof(hal_ble_conn_params_t);
            hal_ble_addr_t peerAddr = {};
            peerAddr.addr_type = BLE_SIG_ADDR_TYPE_PUBLIC;
            le_get_conn_addr(connHandle, peerAddr.addr, (uint8_t *)&peerAddr.addr_type);
            auto existingConnection = fetchConnection(connHandle);
            if (existingConnection) {
                // Some other event might have already initialized the connection object
                if (!addressEqual(peerAddr, existingConnection->info.address)) {
                    LOG(ERROR, "Peer addresses do not match");
                    disconnect(connHandle);
                }
                return;
            }
            le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &connParams.min_conn_interval, connHandle);
            le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &connParams.max_conn_interval, connHandle);
            le_get_conn_param(GAP_PARAM_CONN_LATENCY, &connParams.slave_latency, connHandle);
            le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &connParams.conn_sup_timeout, connHandle);
            LOG_DEBUG(TRACE, "Connected, interval:0x%x, latency:0x%x, timeout:0x%x peer: %02x:%02x:%02x:%02x:%02x:%02x",
                            connParams.max_conn_interval, connParams.slave_latency, connParams.conn_sup_timeout,
                            peerAddr.addr[0], peerAddr.addr[1], peerAddr.addr[2], peerAddr.addr[3], peerAddr.addr[4], peerAddr.addr[5]);

            BleConnection connection = {};
            connection.info.version = BLE_API_VERSION;
            connection.info.size = sizeof(hal_ble_conn_info_t);
            if (addressEqual(connectingAddr_, peerAddr)) {
                connection.info.role = BLE_ROLE_CENTRAL;
                connection.handler = centralLinkCbCache_;
                LOG_DEBUG(TRACE, "Connected as Central");
            } else {
                connection.info.role = BLE_ROLE_PERIPHERAL;
                LOG_DEBUG(TRACE, "Connected as Peripheral");
            }
            connection.info.conn_handle = connHandle;
            connection.info.conn_params = connParams;
            connection.info.address = peerAddr;
            connection.info.att_mtu = BLE_DEFAULT_ATT_MTU_SIZE; // Use the default ATT_MTU on connected.
            connection.pairState = BLE_PAIRING_STATE_NOT_INITIATED;
            int ret = addConnection(std::move(connection));
            if (ret != SYSTEM_ERROR_NONE) {
                LOG(ERROR, "Add new connection failed. Disconnects from peer.");
                disconnect(connHandle);
                return;
            }

            if (connection.info.role == BLE_ROLE_PERIPHERAL
#if HAL_PLATFORM_BLE_ACTIVE_EVENT
                || (connecting_ && connection.info.role == BLE_ROLE_CENTRAL)) {
#else
            ) {
#endif
                connecting_ = false;
                hal_ble_link_evt_t evt = {};
                evt.type = BLE_EVT_CONNECTED;
                evt.conn_handle = connHandle;
                evt.params.connected.info = &connection.info;
                notifyLinkEvent(evt);
            } else {
                // See: handleMtuUpdated()
                // os_semaphore_give(connectSemaphore_, false);
            }
            break;
        }
        default: break;
    }
}

void BleGap::handleMtuUpdated(uint8_t connHandle, uint16_t mtuSize) {
    std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
    LOG_DEBUG(TRACE, "handleMtuUpdated: handle:%d, mtu_size:%d", connHandle, mtuSize);
    BleConnection* connection = fetchConnection(connHandle);
    if (!connection && connecting_) {
        // Race condition in the stack
        LOG_DEBUG(WARN, "handleMtuUpdated force adding connection");
        handleConnectionStateChanged(connHandle, GAP_CONN_STATE_CONNECTED, 0);
        connection = fetchConnection(connHandle);
    }
    if (!connection) {
        return;
    }
    // FIXME: when device initiates the connection establishment,
    // it will perform the ATT MTU exchange automatically on connected. This may result in
    // service discovery failure when the peer device is of other Gen3 platform.
    if (connection->info.role == BLE_ROLE_CENTRAL) {
        connecting_ = false;
        os_semaphore_give(connectSemaphore_, false);
    }
    connection->info.att_mtu = mtuSize;
    hal_ble_link_evt_t evt = {};
    evt.type = BLE_EVT_ATT_MTU_UPDATED;
    evt.conn_handle = connHandle;
    evt.params.att_mtu_updated.att_mtu_size = mtuSize;
    notifyLinkEvent(evt);
}

void BleGap::handleConnParamsUpdated(uint8_t connHandle, uint8_t status, uint16_t cause) {
    std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
    BleConnection* connection = fetchConnection(connHandle);
    if (!connection && connecting_) {
        // Race condition in the stack
        LOG_DEBUG(WARN, "handleConnParamsUpdated force adding connection");
        handleConnectionStateChanged(connHandle, GAP_CONN_STATE_CONNECTED, 0);
        connection = fetchConnection(connHandle);
    }
    if (!connection) {
        return;
    }
    switch (status) {
        case GAP_CONN_PARAM_UPDATE_STATUS_SUCCESS: {
            le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &connection->info.conn_params.min_conn_interval, connHandle);
            le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &connection->info.conn_params.max_conn_interval, connHandle);
            le_get_conn_param(GAP_PARAM_CONN_LATENCY, &connection->info.conn_params.slave_latency, connHandle);
            le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &connection->info.conn_params.conn_sup_timeout, connHandle);
            LOG_DEBUG(TRACE, "handleConnParamsUpdated: update success: interval:0x%x, latency:0x%x, timeout:0x%x",
                            connection->info.conn_params.max_conn_interval, connection->info.conn_params.slave_latency, connection->info.conn_params.conn_sup_timeout);
            break;
        }
        case GAP_CONN_PARAM_UPDATE_STATUS_FAIL: {
            LOG_DEBUG(TRACE, "handleConnParamsUpdated, update failed: cause 0x%x", cause);
            break;
        }
        case GAP_CONN_PARAM_UPDATE_STATUS_PENDING: {
            LOG_DEBUG(TRACE, "handleConnParamsUpdated, update pending: conn_id %d", connHandle);
            break;
        }
        default: break;
    }
}

int BleGap::handleAuthenStateChanged(uint8_t connHandle, uint8_t state, uint16_t cause) {
    std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
    LOG_DEBUG(TRACE, "handleAuthenStateChanged: handle: %d, state: 0x%02x cause: 0x%x", state, connHandle, cause);
    BleConnection* connection = fetchConnection(connHandle);
    LOG_DEBUG(TRACE, "handleAuthenStateChanged connection=%x", connection);
    if (!connection && connecting_) {
        // Race condition in the stack
        LOG_DEBUG(WARN, "handleAuthenStateChanged force adding connection");
        handleConnectionStateChanged(connHandle, GAP_CONN_STATE_CONNECTED, 0);
        connection = fetchConnection(connHandle);
    }
    CHECK_TRUE(connection, SYSTEM_ERROR_NOT_FOUND);
    switch (state) {
        case GAP_AUTHEN_STATE_STARTED: {
            LOG_DEBUG(TRACE, "GAP_AUTHEN_STATE_STARTED");
            auto state = connection->pairState;
            connection->pairState = BLE_PAIRING_STATE_STARTED;
            // Notify the event only if the other side initiates the pairing procedure
            if (state == BLE_PAIRING_STATE_NOT_INITIATED || state == BLE_PAIRING_STATE_REJECTED) {
                hal_ble_link_evt_t linkEvent = {};
                linkEvent.type = BLE_EVT_PAIRING_REQUEST_RECEIVED;
                linkEvent.conn_handle = connHandle;
                // User may call rejectPairing() in the event handler
                notifyLinkEvent(linkEvent);
            }
            break;
        }
        case GAP_AUTHEN_STATE_COMPLETE: {
            hal_ble_link_evt_t linkEvent = {};
            linkEvent.type = BLE_EVT_PAIRING_STATUS_UPDATED;
            linkEvent.conn_handle = connection->info.conn_handle;
            linkEvent.params.pairing_status.bonded = 0;
            linkEvent.params.pairing_status.lesc = 0;
            BlePairingState state = BLE_PAIRING_STATE_REJECTED;
            if (cause == GAP_SUCCESS) {
                LOG_DEBUG(TRACE, "GAP_AUTHEN_STATE_COMPLETE pair success");
                state = BLE_PAIRING_STATE_PAIRED;
                linkEvent.params.pairing_status.lesc = pairingLesc_;
                linkEvent.params.pairing_status.status = SYSTEM_ERROR_NONE;
            } else {
                LOG_DEBUG(TRACE, "GAP_AUTHEN_STATE_COMPLETE pair failed");
                linkEvent.params.pairing_status.status = SYSTEM_ERROR_INTERNAL;
            }
            notifyLinkEvent(linkEvent);
            connection->pairState = state;
            break;
        }
        default: {
            LOG_DEBUG(TRACE, "unknown newstate %d", state);
            break;
        }
    }
    return SYSTEM_ERROR_NONE;
}

int BleGap::handlePairJustWork(uint8_t connHandle) {
    std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
    BleConnection* connection = fetchConnection(connHandle);
    T_GAP_CAUSE ret = le_bond_get_pair_procedure_type(connHandle, &pairingLesc_);
    if (ret == GAP_CAUSE_SUCCESS && connection && connection->pairState == BLE_PAIRING_STATE_STARTED) {
        CHECK_RTL(le_bond_just_work_confirm(connHandle, GAP_CFM_CAUSE_ACCEPT));
    } else {
        CHECK_RTL(le_bond_just_work_confirm(connHandle, GAP_CFM_CAUSE_REJECT));
    }
    return SYSTEM_ERROR_NONE;
}

int BleGap::handlePairPasskeyDisplay(uint8_t connHandle, bool displayOnly) {
    std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
    BleConnection* connection = fetchConnection(connHandle);
    T_GAP_CAUSE ret = le_bond_get_pair_procedure_type(connHandle, &pairingLesc_);
    if (ret == GAP_CAUSE_SUCCESS && connection && connection->pairState == BLE_PAIRING_STATE_STARTED) {
        uint32_t displayValue = 0;
        le_bond_get_display_key(connHandle, &displayValue);
        char passkey[BLE_PAIRING_PASSKEY_LEN + 1];
        sprintf(passkey, "%06ld", displayValue);
        hal_ble_link_evt_t linkEvent = {};
        if (displayOnly) {
            linkEvent.type = BLE_EVT_PAIRING_PASSKEY_DISPLAY;
        } else {
            linkEvent.type = BLE_EVT_PAIRING_NUMERIC_COMPARISON;
        }
        linkEvent.conn_handle = connHandle;
        linkEvent.params.passkey_display.passkey = (const uint8_t*)passkey;
        // User may call rejectPairing() in the event handler
        notifyLinkEvent(linkEvent);
        if (connection->pairState == BLE_PAIRING_STATE_USER_REQ_REJECT) {
            goto reject;
        }
        if (displayOnly) {
            le_bond_passkey_display_confirm(connHandle, GAP_CFM_CAUSE_ACCEPT);
        } else if (connection->pairState != BLE_PAIRING_STATE_SET_AUTH_DATA) {
            // User didn't set authentication data in event callback, reject pairing automatically
            goto reject;
        }
        return SYSTEM_ERROR_NONE;
    }

reject:
    if (displayOnly) {
        CHECK_RTL(le_bond_passkey_display_confirm(connHandle, GAP_CFM_CAUSE_REJECT));
    } else {
        CHECK_RTL(le_bond_user_confirm(connHandle, GAP_CFM_CAUSE_REJECT));
    }
    return SYSTEM_ERROR_NONE;
}

int BleGap::handlePairPasskeyInput(uint8_t connHandle) {
    std::lock_guard<RecursiveMutex> lk(connectionsMutex_);
    BleConnection* connection = fetchConnection(connHandle);
    T_GAP_CAUSE ret = le_bond_get_pair_procedure_type(connHandle, &pairingLesc_);
    if (ret == GAP_CAUSE_SUCCESS && connection && connection->pairState == BLE_PAIRING_STATE_STARTED) {
        hal_ble_link_evt_t linkEvent = {};
        linkEvent.type = BLE_EVT_PAIRING_PASSKEY_INPUT;
        linkEvent.conn_handle = connHandle;
        notifyLinkEvent(linkEvent);
        if (connection->pairState == BLE_PAIRING_STATE_SET_AUTH_DATA) {
            return SYSTEM_ERROR_NONE;
        }
    }
    CHECK_RTL(le_bond_passkey_input_confirm(connHandle, 0, GAP_CFM_CAUSE_REJECT));
    return SYSTEM_ERROR_NONE;
}

T_APP_RESULT BleGap::gapEventCallback(uint8_t type, void *data) {
    T_APP_RESULT result = APP_RESULT_SUCCESS;
#ifdef DEBUG_BUILD
    T_LE_CB_DATA *p_data = (T_LE_CB_DATA *)data;
#endif
    switch (type) {
#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
        case GAP_MSG_LE_DATA_LEN_CHANGE_INFO: {
            LOG_DEBUG(TRACE, "gapEventCallback: GAP_MSG_LE_DATA_LEN_CHANGE_INFO, handle: %d, tx_octets: %d, max_tx_time: %d",
                            p_data->p_le_data_len_change_info->conn_id,
                            p_data->p_le_data_len_change_info->max_tx_octets,
                            p_data->p_le_data_len_change_info->max_tx_time);
            break;
        }
#endif
        case GAP_MSG_LE_MODIFY_WHITE_LIST: {
            LOG_DEBUG(TRACE, "gapEventCallback: GAP_MSG_LE_MODIFY_WHITE_LIST, operation %d, cause 0x%x",
                            p_data->p_le_modify_white_list_rsp->operation, p_data->p_le_modify_white_list_rsp->cause);
            break;
        }
        case GAP_MSG_LE_SCAN_INFO: {
            LOG_DEBUG(TRACE, "gapEventCallback, GAP_MSG_LE_SCAN_INFO");
            BleGap::getInstance().notifyScanResult(((T_LE_CB_DATA *)data)->p_le_scan_info);
            break;
        }
        default: {
            LOG_DEBUG(TRACE, "gapEventCallback: unhandled type 0x%x", type);
            break;
        }
    }
    return result;
}

int BleGatt::init() {
    CHECK_TRUE(os_semaphore_create(&notifySemaphore_, 1, 0) == 0, SYSTEM_ERROR_INTERNAL);
    CHECK_TRUE(os_semaphore_create(&discoverySemaphore_, 1, 0) == 0, SYSTEM_ERROR_INTERNAL);
    CHECK_TRUE(os_semaphore_create(&writeSemaphore_, 1, 0) == 0, SYSTEM_ERROR_INTERNAL);
    CHECK_TRUE(os_semaphore_create(&readSemaphore_, 1, 0) == 0, SYSTEM_ERROR_INTERNAL);

    client_init(1);
    CHECK_TRUE(client_register_spec_client_cb(&clientId_, &clientCallbacks_), SYSTEM_ERROR_INTERNAL);

    server_init(MAX_ALLOWED_BLE_SERVICES);
    server_register_app_cb(gattServerEventCallback);
    serviceRegistered_ = false;
    if (attrConfigured_) {
        CHECK(BleGatt::getInstance().registerAttributeTable());
    }
    return SYSTEM_ERROR_NONE;
}

int BleGatt::deinit() {
    if (notifySemaphore_) {
        os_semaphore_destroy(notifySemaphore_);
        notifySemaphore_ = nullptr;
    }
    if (discoverySemaphore_) {
        os_semaphore_destroy(discoverySemaphore_);
        discoverySemaphore_ = nullptr;
    }
    if (writeSemaphore_) {
        os_semaphore_destroy(writeSemaphore_);
        writeSemaphore_ = nullptr;
    }
    if (readSemaphore_) {
        os_semaphore_destroy(readSemaphore_);
        readSemaphore_ = nullptr;
    }
    return SYSTEM_ERROR_NONE;
}

int BleGatt::setDesiredAttMtu(size_t attMtu) {
    CHECK_TRUE(attMtu >= BLE_MIN_ATT_MTU_SIZE && attMtu <= BLE_MAX_ATT_MTU_SIZE, SYSTEM_ERROR_INVALID_ARGUMENT);
    desiredAttMtu_ = attMtu;
    gap_config_max_mtu_size(attMtu);
    return SYSTEM_ERROR_NONE;
}

T_APP_RESULT BleGatt::gattServerEventCallback(T_SERVER_ID serviceId, void *pData) {
    LOG_DEBUG(TRACE, "gattServerEventCallback()");
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    if (serviceId == SERVICE_PROFILE_GENERAL_ID) {
        T_SERVER_APP_CB_DATA *pParam = (T_SERVER_APP_CB_DATA *)pData;
        switch (pParam->eventId) {
            case PROFILE_EVT_SRV_REG_COMPLETE: {// srv register result event.
                LOG_DEBUG(TRACE, "PROFILE_EVT_SRV_REG_COMPLETE: result %d", pParam->event_data.service_reg_result);
                break;
            }
            case PROFILE_EVT_SEND_DATA_COMPLETE: {
                auto& gatt = BleGatt::getInstance();
                if (!gatt.isNotifying_) {
                    return result;
                }
                gatt.isNotifying_ = false;
                LOG_DEBUG(TRACE, "PROFILE_EVT_SEND_DATA_COMPLETE: conn_id %d, cause: 0x%x, serviceId: %d, index: 0x%x, credits: %d",
                            pParam->event_data.send_data_result.conn_id,
                            pParam->event_data.send_data_result.cause,
                            pParam->event_data.send_data_result.service_id,
                            pParam->event_data.send_data_result.attrib_idx,
                            pParam->event_data.send_data_result.credits);
                if (pParam->event_data.send_data_result.cause == GAP_SUCCESS) {
                    os_semaphore_give(gatt.notifySemaphore_, false);
                }
                break;
            }
            default: break;
        }
    }
    return result;
}

T_APP_RESULT BleGatt::gattReadAttrCallback(uint8_t connHandle, T_SERVER_ID serviceId, uint16_t index, uint16_t offset, uint16_t* length, uint8_t** value) {
    LOG_DEBUG(TRACE, "gattReadAttrCallback(), handle: %d, serviceId: %d, index: %d, offset: %d", connHandle, serviceId, index, offset);
    T_APP_RESULT cause = APP_RESULT_SUCCESS;
    if (!BleGap::getInstance().valid(connHandle)) {
        return cause;
    }
    *length = 0;
    auto& services = BleGatt::getInstance().services();
    for (auto& svc : services) {
        if (svc.id != serviceId) {
            continue;
        }
        *length = svc.attrTable[index].value_len;
        *value = (uint8_t*)svc.attrTable[index].p_value_context;
    }
    return (cause);
}

T_APP_RESULT BleGatt::gattWriteAttrCallback(uint8_t connHandle, T_SERVER_ID serviceId, uint16_t index, T_WRITE_TYPE type, uint16_t length, uint8_t *value, P_FUN_WRITE_IND_POST_PROC *postProc) {
    T_APP_RESULT cause = APP_RESULT_SUCCESS;
    if (!BleGap::getInstance().valid(connHandle)) {
        return cause;
    }
    *postProc = nullptr;
    LOG_DEBUG(TRACE, "gattWriteAttrCallback, type:%d, id:%d, index:%d, len:%d", type, serviceId, index, length);
    auto& services = BleGatt::getInstance().services();
    for (auto& svc : services) {
        if (svc.id != serviceId) {
            continue;
        }
        switch (type) {
            case WRITE_REQUEST:
            case WRITE_WITHOUT_RESPONSE: {
                for (auto& charact : svc.characteristics) {
                    if (charact.index != index) {
                        continue;
                    }
                    svc.attrTable[index].value_len = length;
                    memcpy(svc.attrTable[index].p_value_context, value, svc.attrTable[index].value_len);
                    if (charact.callback) {
                        hal_ble_char_evt_t evt = {};
                        evt.type = BLE_EVT_DATA_WRITTEN;
                        evt.conn_handle = connHandle;
                        evt.attr_handle = charact.handle;
                        evt.params.data_written.offset = 0;
                        evt.params.data_written.len = svc.attrTable[index].value_len;
                        evt.params.data_written.data = (uint8_t*)svc.attrTable[index].p_value_context;
                        charact.callback(&evt, charact.context);
                    }
                    return cause;
                }
                LOG_DEBUG(TRACE, "Characteristic is not found");
                break;
            }
            case WRITE_SIGNED_WITHOUT_RESPONSE: {
                LOG_DEBUG(TRACE, "WRITE_SIGNED_WITHOUT_RESPONSE");
                break;
            }
            default: {
                LOG_DEBUG(TRACE, "WRITE_LONG");
                break;
            }
        }
    }
    return (cause);
}

void BleGatt::gattCccdUpdatedCallback(uint8_t connHandle, T_SERVER_ID serviceId, uint16_t index, uint16_t bits) {
    LOG_DEBUG(TRACE, "gattCccdUpdatedCallback, id:%d, index:%d, cccd:%d", serviceId, index, bits);
    if (!BleGap::getInstance().valid(connHandle)) {
        return;
    }
    // Note: the index passed in is the CCCD attrribute index
    uint16_t valudIndex = index - 1;
    auto& services = BleGatt::getInstance().services();
    for (auto& svc : services) {
        if (svc.id != serviceId) {
            continue;
        }
        for (auto& config : svc.cccdConfigs) {
            if (config.index != valudIndex) {
                continue;
            }
            config.subscriber.connHandle = connHandle;
            config.subscriber.config = (ble_sig_cccd_value_t)bits;
            for (const auto& charact : svc.characteristics) {
                if (charact.index != config.index) {
                    continue;
                }
                if (charact.callback) {
                    hal_ble_char_evt_t evt = {};
                    evt.conn_handle = connHandle;
                    evt.attr_handle = charact.handle + 1; // CCCD attribute handle
                    evt.type = BLE_EVT_CHAR_CCCD_UPDATED;
                    evt.params.cccd_config.value = config.subscriber.config;
                    charact.callback(&evt, charact.context);
                }
            }
            return;
        }
    }
    LOG_DEBUG(TRACE, "Failed to config CCCD, not found.");
}

int BleGatt::addService(uint8_t type, const hal_ble_uuid_t* uuid, hal_ble_attr_handle_t* svcHandle) {
    CHECK_TRUE(uuid, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(svcHandle, SYSTEM_ERROR_INVALID_ARGUMENT);

    for (auto& svc : services_) {
        if (isUuidEqual(&svc.uuid, uuid)) {
            LOG_DEBUG(TRACE, "Service is already exist.");
            return SYSTEM_ERROR_ALREADY_EXISTS;
        }
    }

    uint8_t* uuid128 = nullptr;
    int ret = SYSTEM_ERROR_INTERNAL;
    SCOPE_GUARD ({
        if (ret != SYSTEM_ERROR_NONE) {
            if (uuid128) {
                free(uuid128);
            }
        }
    });
    
    LOG_DEBUG(TRACE, "Create new service.");
    BleService service = {};
    memcpy(&service.uuid, uuid, sizeof(hal_ble_uuid_t));
    uint8_t sevCount = services_.size();
    service.startHandle = CUSTOMER_SERVICE_START_HANDLE + sevCount * SERVICE_HANDLE_RANGE_RESERVED;
    service.endHandle = service.startHandle;

    // Service declaration attribute
    T_ATTRIB_APPL attribute = {};
    attribute.type_value[0] = LO_WORD(BLE_SIG_UUID_PRIMARY_SVC_DECL);
    attribute.type_value[1] = HI_WORD(BLE_SIG_UUID_PRIMARY_SVC_DECL);
    if (uuid->type == BLE_UUID_TYPE_128BIT) {
        attribute.flags = ATTRIB_FLAG_VOID | ATTRIB_FLAG_LE; // p_value_context -> attribute value
        attribute.value_len = BLE_SIG_UUID_128BIT_LEN;
        uuid128 = (uint8_t*)malloc(BLE_SIG_UUID_128BIT_LEN);
        CHECK_TRUE(uuid128, SYSTEM_ERROR_NO_MEMORY);
        memcpy(uuid128, uuid->uuid128, BLE_SIG_UUID_128BIT_LEN);
        attribute.p_value_context = (void*)uuid128;
    } else {
        attribute.flags = ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE; // type_value -> attribute value
        attribute.type_value[2] = LO_WORD(uuid->uuid16);
        attribute.type_value[3] = HI_WORD(uuid->uuid16);
        attribute.value_len = BLE_SIG_UUID_16BIT_LEN;
        attribute.p_value_context = nullptr;
    }
    attribute.permissions = GATT_PERM_READ;
    CHECK_TRUE(service.attrTable.append(attribute), SYSTEM_ERROR_NO_MEMORY);

    bool adv = BleGap::getInstance().isAdvertising();
    bool registered = serviceRegistered_;
    SCOPE_GUARD({
        if (adv) {
            BleGap::getInstance().startAdvertising();
        }
    });
    if (registered) {
        CHECK(BleGap::getInstance().stop());
        CHECK(BleGap::getInstance().init());
    }
    CHECK_TRUE(services_.append(service), SYSTEM_ERROR_NO_MEMORY);
    *svcHandle = service.startHandle;
    return ret = SYSTEM_ERROR_NONE;
}

int BleGatt::addCharacteristic(const hal_ble_char_init_t* charInit, hal_ble_char_handles_t* charHandles) {
    CHECK_TRUE(charHandles, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(charInit, SYSTEM_ERROR_INVALID_ARGUMENT);

    uint8_t charUuid[BLE_SIG_UUID_128BIT_LEN] = {};
    if (charInit->uuid.type == BLE_UUID_TYPE_128BIT) {
        memcpy(charUuid, charInit->uuid.uuid128, BLE_SIG_UUID_128BIT_LEN);
    } else {
        charUuid[0] = LO_WORD(charInit->uuid.uuid16);
        charUuid[1] = HI_WORD(charInit->uuid.uuid16);
    }

    BleService* targetService = nullptr;
    for (auto& svc : services_) {
        if (svc.startHandle == charInit->service_handle) {
            for (const auto& charact : svc.characteristics) {
                if (!memcmp(svc.attrTable[charact.index].type_value, charUuid, sizeof(charUuid))) {
                    LOG_DEBUG(TRACE, "Characteristic is already exist.");
                    return SYSTEM_ERROR_ALREADY_EXISTS;
                }
            }
            targetService = &svc;
            break;
        }
    }
    if (!targetService) {
        LOG_DEBUG(TRACE, "Service not found.");
        return SYSTEM_ERROR_NOT_FOUND;
    }

    // Copy
    auto serviceCopy = *targetService;
    auto service = &serviceCopy;

    uint8_t* value = nullptr;
    int ret = SYSTEM_ERROR_INTERNAL;
    SCOPE_GUARD ({
        if (ret != SYSTEM_ERROR_NONE) {
            if (value) {
                free(value);
            }
        }
    });

    LOG_DEBUG(TRACE, "Create new characteristic.");

    // Characteristic declaration attribute
    T_ATTRIB_APPL declAttribute = {};
    declAttribute.flags = ATTRIB_FLAG_VALUE_INCL; // type_value -> declAttribute value
    declAttribute.type_value[0] = LO_WORD(BLE_SIG_UUID_CHAR_DECL);
    declAttribute.type_value[1] = HI_WORD(BLE_SIG_UUID_CHAR_DECL);
    declAttribute.type_value[2] = charInit->properties;
    declAttribute.value_len = sizeof(uint8_t);
    declAttribute.p_value_context = nullptr;
    declAttribute.permissions = GATT_PERM_READ;
    CHECK_TRUE(service->attrTable.append(declAttribute), SYSTEM_ERROR_NO_MEMORY);
    service->endHandle++;
    charHandles->decl_handle = service->endHandle;

    // Characteristic value attribute
    T_ATTRIB_APPL valueAttribute = {};
    valueAttribute.flags = ATTRIB_FLAG_VALUE_APPL; // gattReadAttrCallback -> attribute value
    if (charInit->uuid.type == BLE_UUID_TYPE_128BIT) {
        valueAttribute.flags |= ATTRIB_FLAG_UUID_128BIT;
    }
    memcpy(valueAttribute.type_value, charUuid, BLE_SIG_UUID_128BIT_LEN);
    value = (uint8_t*)malloc(BLE_MAX_ATTR_VALUE_PACKET_SIZE);
    CHECK_TRUE(value, SYSTEM_ERROR_NO_MEMORY);
    valueAttribute.value_len = 0; // variable length
    valueAttribute.p_value_context = value; // gattReadAttrCallback will use the content pointed by this pointer to provide value for BT stack.
    valueAttribute.permissions = GATT_PERM_READ | GATT_PERM_WRITE;
    CHECK_TRUE(service->attrTable.append(valueAttribute), SYSTEM_ERROR_NO_MEMORY);
    service->endHandle++;
    charHandles->value_handle = service->endHandle;
    BleCharacteristic characteristic = {};
    characteristic.handle = charHandles->value_handle;
    characteristic.index = service->attrTable.size() - 1;
    characteristic.callback = charInit->callback;
    characteristic.context = charInit->context;
    CHECK_TRUE(service->characteristics.append(characteristic), SYSTEM_ERROR_NO_MEMORY);

    // Characteristic CCCD descriptor attribute
    T_ATTRIB_APPL cccdAttribute = {};
    CccdConfig config = {};
    if ((charInit->properties & BLE_SIG_CHAR_PROP_NOTIFY) || (charInit->properties & BLE_SIG_CHAR_PROP_INDICATE)) {
        cccdAttribute.flags = ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_CCCD_APPL; // type_value -> attribute value
        cccdAttribute.type_value[0] = LO_WORD(BLE_SIG_UUID_CLIENT_CHAR_CONFIG_DESC);
        cccdAttribute.type_value[1] = HI_WORD(BLE_SIG_UUID_CLIENT_CHAR_CONFIG_DESC);
        cccdAttribute.type_value[2] = LO_WORD(BLE_SIG_CCCD_VAL_DISABLED);
        cccdAttribute.type_value[3] = HI_WORD(BLE_SIG_CCCD_VAL_DISABLED);
        cccdAttribute.value_len = sizeof(ble_sig_cccd_value_t);
        cccdAttribute.p_value_context = nullptr;
        cccdAttribute.permissions = GATT_PERM_READ | GATT_PERM_WRITE;
        CHECK_TRUE(service->attrTable.append(cccdAttribute), SYSTEM_ERROR_NO_MEMORY);
        service->endHandle++;
        charHandles->cccd_handle = service->endHandle;
        config.index = characteristic.index;
        config.subscriber.connHandle = BLE_INVALID_CONN_HANDLE;
        config.subscriber.config = BLE_SIG_CCCD_VAL_DISABLED;
        CHECK_TRUE(service->cccdConfigs.append(config), SYSTEM_ERROR_NO_MEMORY);
    } else {
        charHandles->cccd_handle = BLE_INVALID_ATTR_HANDLE;
    }
    charHandles->sccd_handle = BLE_INVALID_ATTR_HANDLE; // FIXME: not supported for now
    
    // User description descriptor attribute
    T_ATTRIB_APPL descrAttribute = {};
    if (charInit->description) {
        descrAttribute.flags = ATTRIB_FLAG_VOID | ATTRIB_FLAG_ASCII_Z; // p_value_context -> attribute value
        descrAttribute.type_value[0] = LO_WORD(BLE_SIG_UUID_CHAR_USER_DESCRIPTION_DESC);
        descrAttribute.type_value[1] = HI_WORD(BLE_SIG_UUID_CHAR_USER_DESCRIPTION_DESC);
        descrAttribute.value_len = std::min((size_t)BLE_MAX_DESC_LEN, strlen(charInit->description));
        descrAttribute.p_value_context = (void*)charInit->description;
        descrAttribute.permissions = GATT_PERM_READ;
        CHECK_TRUE(service->attrTable.append(descrAttribute), SYSTEM_ERROR_NO_MEMORY);
        service->endHandle++;
        charHandles->user_desc_handle = service->endHandle;
    } else {
        charHandles->user_desc_handle = BLE_INVALID_ATTR_HANDLE;
    }

    bool adv = BleGap::getInstance().isAdvertising();
    bool registered = serviceRegistered_;
    SCOPE_GUARD({
        if (adv) {
            BleGap::getInstance().startAdvertising();
        }
    });
    if (registered) {
        CHECK(BleGap::getInstance().stop());
        CHECK(BleGap::getInstance().init());
    }

    std::swap(*targetService, *service);

    return ret = SYSTEM_ERROR_NONE;
}

int BleGatt::registerAttributeTable() {
    attrConfigured_ = true;
    if (!serviceRegistered_) {
        for (auto& svc : services_) {
            if (!server_add_service_by_start_handle(&svc.id, (uint8_t *)svc.attrTable.data(), svc.attrTable.size() * sizeof(T_ATTRIB_APPL), serverCallbacks_, svc.startHandle)) {
                LOG_DEBUG(TRACE, "Failed to add service.");
                svc.id = 0xFF;
                return SYSTEM_ERROR_INTERNAL;
            }
        }
        serviceRegistered_ = true;
    }
    return SYSTEM_ERROR_NONE;
}

ssize_t BleGatt::setValue(hal_ble_attr_handle_t attrHandle, const uint8_t* buf, size_t len) {
    CHECK_TRUE(attrHandle != BLE_INVALID_ATTR_HANDLE, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(buf, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(len, SYSTEM_ERROR_INVALID_ARGUMENT);
    T_ATTRIB_APPL* attribute = findAttribute(attrHandle);
    if (!attribute) {
        return 0;
    }
    attribute->value_len = std::min((int)len, BLE_MAX_ATTR_VALUE_PACKET_SIZE);
    memcpy(attribute->p_value_context, buf, attribute->value_len);
    return attribute->value_len;
}

ssize_t BleGatt::getValue(hal_ble_attr_handle_t attrHandle, uint8_t* buf, size_t len) {
    CHECK_TRUE(attrHandle != BLE_INVALID_ATTR_HANDLE, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(buf, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(len, SYSTEM_ERROR_INVALID_ARGUMENT);
    T_ATTRIB_APPL* attribute = findAttribute(attrHandle);
    if (!attribute) {
        return 0;
    }
    len = std::min(len, (size_t)attribute->value_len);
    memcpy(buf, attribute->p_value_context, len);
    return len;
}

ssize_t BleGatt::notifyValue(hal_ble_attr_handle_t attrHandle, const uint8_t* buf, size_t len, bool ack) {
    CHECK_TRUE(attrHandle != BLE_INVALID_ATTR_HANDLE, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(buf, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(len, SYSTEM_ERROR_INVALID_ARGUMENT);
    // Set value first, so that user can read back the data that is sent to peer.
    int ret = setValue(attrHandle, buf, len);
    CHECK_TRUE(ret > 0, ret);
    for (auto& svc : services_) {
        if (attrHandle < svc.startHandle && attrHandle > svc.endHandle) {
            continue;
        }
        // Found service
        for (const auto& charact : svc.characteristics) {
            if (charact.handle != attrHandle) {
                continue;
            }
            // Found characteristic
            for (auto& config : svc.cccdConfigs) {
                if (config.index != charact.index) {
                    continue;
                }
                // Found CCCD config
                if (config.subscriber.connHandle == BLE_INVALID_CONN_HANDLE) {
                    return 0;
                }
                T_ATTRIB_APPL& attribute = svc.attrTable[config.index];
                T_GATT_PDU_TYPE type = GATT_PDU_TYPE_ANY;
                if (ack && (config.subscriber.config & BLE_SIG_CCCD_VAL_INDICATION)) {
                    type = GATT_PDU_TYPE_INDICATION;
                } else if(!ack && (config.subscriber.config & BLE_SIG_CCCD_VAL_NOTIFICATION)) {
                    type = GATT_PDU_TYPE_NOTIFICATION;
                } else {
                    return 0;
                }
                LOG_DEBUG(TRACE, "notify %x %x %x %u %x", config.subscriber.connHandle, svc.id, config.index, attribute.value_len, type);
                if (!BleGap::getInstance().valid(config.subscriber.connHandle)) {
                    return 0;
                }
                
                uint32_t timeoutMs = 100;
                hal_ble_conn_info_t info = {};
                info.version = BLE_API_VERSION;
                info.size = sizeof(hal_ble_conn_info_t);
                int ret = BleGap::getInstance().getConnectionInfo(config.subscriber.connHandle, &info);
                if (ret == SYSTEM_ERROR_NONE) {
                    timeoutMs += info.conn_params.max_conn_interval * 1250/*us*/ / 1000;
                }
                uint8_t retry = 3;
                bool success;
                do {
                    success = true;
                    isNotifying_ = true;
                    CHECK_TRUE(server_send_data(config.subscriber.connHandle, svc.id, config.index, (uint8_t*)attribute.p_value_context, attribute.value_len, type), SYSTEM_ERROR_INTERNAL);
                    if (BleEventDispatcher::getInstance().isThreadCurrent()) {
                        isNotifying_ = false;
                        break;
                    }
                    if (os_semaphore_take(notifySemaphore_, timeoutMs, false)) {
                        success = false;
                    }
                } while (--retry > 0 && !success);
                if (!success) {
                    return SYSTEM_ERROR_TIMEOUT;
                }
                return attribute.value_len;
            }
        }
    }
    return 0;
}

int BleGatt::removeSubscriber(hal_ble_conn_handle_t connHandle) {
    for (auto& svc : services_) {
        for (auto& config : svc.cccdConfigs) {
            config.subscriber.connHandle = BLE_INVALID_CONN_HANDLE;
            config.subscriber.config = BLE_SIG_CCCD_VAL_DISABLED;
        }
    }
    return SYSTEM_ERROR_NONE;
}

void BleGatt::onDiscoverStateCallback(uint8_t connHandle, T_DISCOVERY_STATE discoveryState) {
    if (!BleGap::getInstance().valid(connHandle)) {
        return;
    }
    auto& gatt = BleGatt::getInstance();
    if (!gatt.isDiscovering_ || gatt.currDiscConnHandle_ != connHandle) {
        return;
    }
    switch (discoveryState) {
        case DISC_STATE_SRV_DONE: {
            gatt.isDiscovering_ = false;
            if (gatt.discSvcCallback_) {
                hal_ble_svc_discovered_evt_t svcDiscEvent = {};
                svcDiscEvent.conn_handle = gatt.currDiscConnHandle_;
                svcDiscEvent.count = gatt.discServices_.size();
                svcDiscEvent.services = gatt.discServices_.data();
                gatt.discSvcCallback_(&svcDiscEvent, gatt.discSvcContext_);
            }
            os_semaphore_give(gatt.discoverySemaphore_, false);
            break;
        }
        case DISC_STATE_CHAR_DONE:
        case DISC_STATE_CHAR_UUID16_DONE:
        case DISC_STATE_CHAR_UUID128_DONE: {
            // Start discovering descriptors
            if (client_all_char_descriptor_discovery(connHandle, gatt.clientId_, gatt.currDiscService_->start_handle, gatt.currDiscService_->end_handle) != GAP_CAUSE_SUCCESS) {
                LOG(ERROR, "Imcompleted characteristic discovery procedure!");
                gatt.isDiscovering_ = false;
                os_semaphore_give(gatt.discoverySemaphore_, false);
            }
            break;
        }
        case DISC_STATE_CHAR_DESCRIPTOR_DONE: {
            // Characteristic discovery procedure completed
            gatt.isDiscovering_ = false;
            if (gatt.discCharCallback_) {
                hal_ble_char_discovered_evt_t charDiscEvent = {};
                charDiscEvent.conn_handle = gatt.currDiscConnHandle_;
                charDiscEvent.count = gatt.discCharacteristics_.size();
                charDiscEvent.characteristics = gatt.discCharacteristics_.data();
                gatt.discCharCallback_(&charDiscEvent, gatt.discCharContext_);
            }
            os_semaphore_give(gatt.discoverySemaphore_, false);
            break;
        }
        case DISC_STATE_FAILED: {
            if (gatt.isDiscovering_) {
                gatt.isDiscovering_ = false;
                os_semaphore_give(gatt.discoverySemaphore_, false);
            }
            break;
        }
        default: {
            LOG_DEBUG(TRACE, "Unhandled discover state");
            break;
        }
    }
}

void BleGatt::onDiscoverResultCallback(uint8_t connHandle, T_DISCOVERY_RESULT_TYPE type, T_DISCOVERY_RESULT_DATA data) {
    if (!BleGap::getInstance().valid(connHandle)) {
        return;
    }
    auto& gatt = BleGatt::getInstance();
    if (!gatt.isDiscovering_ || gatt.currDiscConnHandle_ != connHandle) {
        return;
    }
    switch (type) {
        case DISC_RESULT_ALL_SRV_UUID16:
        case DISC_RESULT_ALL_SRV_UUID128:
        case DISC_RESULT_SRV_DATA: {
            hal_ble_svc_t service = {};
            service.version = BLE_API_VERSION;
            service.size = sizeof(hal_ble_svc_t);
            service.start_handle = data.p_srv_uuid16_disc_data->att_handle; // memory layout is compatible with other type of calback data
            service.end_handle = data.p_srv_uuid16_disc_data->end_group_handle; // memory layout is compatible with other type of calback data
            if (type == DISC_RESULT_ALL_SRV_UUID16) {
                service.uuid.type = BLE_UUID_TYPE_16BIT;
                service.uuid.uuid16 = data.p_srv_uuid16_disc_data->uuid16;
            } else if (type == DISC_RESULT_ALL_SRV_UUID128) {
                service.uuid.type = BLE_UUID_TYPE_128BIT;
                memcpy(service.uuid.uuid128, data.p_srv_uuid128_disc_data->uuid128, BLE_SIG_UUID_128BIT_LEN);
            } else {
                service.uuid = *(gatt.discSvcUuid_);
            }
            if (!gatt.discServices_.append(service)) {
                LOG(ERROR, "Failed to append discovered service.");
                // Do nothing. We'll give the semaphore in the discover state callback
            }
            break;
        }
        case DISC_RESULT_CHAR_UUID16:
        case DISC_RESULT_CHAR_UUID128:
        case DISC_RESULT_BY_UUID16_CHAR:
        case DISC_RESULT_BY_UUID128_CHAR: {
            hal_ble_char_t characteristic = {};
            characteristic.version = BLE_API_VERSION;
            characteristic.size = sizeof(hal_ble_char_t);
            characteristic.charHandles.version = BLE_API_VERSION;
            characteristic.charHandles.size = sizeof(hal_ble_char_handles_t);
            characteristic.char_ext_props = data.p_char_uuid16_disc_data->properties & 0x80 ? 0x01 : 0x00; // memory layout is compatible with other type of calback data
            characteristic.properties = data.p_char_uuid16_disc_data->properties & 0x7F; // memory layout is compatible with other type of calback data
            characteristic.charHandles.decl_handle = data.p_char_uuid16_disc_data->decl_handle; // memory layout is compatible with other type of calback data
            characteristic.charHandles.value_handle = data.p_char_uuid16_disc_data->value_handle; // memory layout is compatible with other type of calback data
            if (type == DISC_RESULT_CHAR_UUID16 || type == DISC_RESULT_BY_UUID16_CHAR) {
                characteristic.uuid.type = BLE_UUID_TYPE_16BIT;
                characteristic.uuid.uuid16 = data.p_char_uuid16_disc_data->uuid16;
            } else {
                characteristic.uuid.type = BLE_UUID_TYPE_128BIT;
                memcpy(characteristic.uuid.uuid128, data.p_char_uuid128_disc_data->uuid128, BLE_SIG_UUID_128BIT_LEN);
            }
            if (!gatt.discCharacteristics_.append(characteristic)) {
                LOG(ERROR, "Failed to append discovered characteristic.");
                // Do nothing. We'll give the semaphore in the discover state callback
            }
            break;
        }
        case DISC_RESULT_CHAR_DESC_UUID16: {
            auto descriptor = (T_GATT_CHARACT_DESC_ELEM16*)data.p_char_desc_uuid16_disc_data;
            hal_ble_char_t* characteristic = gatt.findDiscoveredCharacteristic(descriptor->handle);
            if (!characteristic) {
                return;
            }
            if (descriptor->uuid16 == BLE_SIG_UUID_CHAR_USER_DESCRIPTION_DESC) {
                characteristic->charHandles.user_desc_handle = descriptor->handle;
            } else if (descriptor->uuid16 == BLE_SIG_UUID_CLIENT_CHAR_CONFIG_DESC) {
                characteristic->charHandles.cccd_handle = descriptor->handle;
            } else if (descriptor->uuid16 == BLE_SIG_UUID_SERVER_CHAR_CONFIG_DESC) {
                characteristic->charHandles.sccd_handle = descriptor->handle;
            }
            // Disregard any other descriptors
            break;
        }
        default: {
            LOG_DEBUG(TRACE, "Unhandled discover result");
            break;
        }
    }
}

void BleGatt::onReadResultCallback(uint8_t connHandle, uint16_t cause, uint16_t handle, uint16_t size, uint8_t *value) {
    if (!BleGap::getInstance().valid(connHandle)) {
        return;
    }
    auto& gatt = BleGatt::getInstance();
    if (gatt.currReadConnHandle_ != connHandle || gatt.readAttrHandle_ != handle) {
        return;
    }
    gatt.readLen_ = std::min(gatt.readLen_, (size_t)size);
    memcpy(gatt.readBuf_, value, gatt.readLen_);
    os_semaphore_give(gatt.readSemaphore_, false);
}

void BleGatt::onWriteResultCallback(uint8_t connHandle, T_GATT_WRITE_TYPE type, uint16_t handle, uint16_t cause, uint8_t credits) {
    if (!BleGap::getInstance().valid(connHandle)) {
        return;
    }
    auto& gatt = BleGatt::getInstance();
    if (gatt.currWriteConnHandle_ != connHandle || gatt.writeAttrHandle_ != handle) {
        return;
    }
    os_semaphore_give(gatt.writeSemaphore_, false);
}

T_APP_RESULT BleGatt::onNotifyIndResultCallback(uint8_t connHandle, bool notify, uint16_t handle, uint16_t size, uint8_t *value) {
    if (!BleGap::getInstance().valid(connHandle)) {
        return APP_RESULT_REJECT;
    }
    auto& gatt = BleGatt::getInstance();
    if (!notify) {
        if (client_attr_ind_confirm(connHandle) != GAP_CAUSE_SUCCESS) {
            LOG(ERROR, "Failed to confirm indication!");
        }
    }
    hal_ble_char_evt_t charEvent = {};
    charEvent.conn_handle = connHandle;
    charEvent.attr_handle = handle;
    charEvent.type = BLE_EVT_DATA_NOTIFIED;
    charEvent.params.data_written.offset = 0;
    charEvent.params.data_written.len = size;
    charEvent.params.data_written.data = value;
    for (const auto& publisher : gatt.publishers_) {
        if (publisher.connHandle == connHandle && publisher.valueHandle == handle) {
            if (publisher.callback) {
                publisher.callback(&charEvent, publisher.context);
            }
            break;
        }
    }
    return APP_RESULT_SUCCESS;
}

void BleGatt::onGattClientDisconnectCallback(uint8_t connHandle) {
    if (!BleGap::getInstance().valid(connHandle)) {
        return;
    }
    auto& gatt = BleGatt::getInstance();
    if (gatt.isDiscovering_) {
        os_semaphore_give(gatt.discoverySemaphore_, false);
    }
    if (gatt.currWriteConnHandle_ != BLE_INVALID_CONN_HANDLE) {
        os_semaphore_give(gatt.writeSemaphore_, false);
    }
    if (gatt.currReadConnHandle_ != BLE_INVALID_CONN_HANDLE) {
        os_semaphore_give(gatt.readSemaphore_, false);
    }
}

bool BleGatt::discovering(hal_ble_conn_handle_t connHandle) const {
    // TODO: discovering service and characteristic on multi-links concurrently.
    return isDiscovering_;
}

void BleGatt::resetDiscoveryState() {
    isDiscovering_ = false;
    currDiscConnHandle_ = BLE_INVALID_CONN_HANDLE;
    currDiscService_ = nullptr;
    discSvcUuid_ = nullptr;
    discServices_.clear();
    discCharacteristics_.clear();
}

hal_ble_char_t* BleGatt::findDiscoveredCharacteristic(hal_ble_attr_handle_t attrHandle) {
    hal_ble_char_t* foundChar = nullptr;
    hal_ble_attr_handle_t foundCharDeclHandle = BLE_INVALID_ATTR_HANDLE;
    for (auto& characteristic : discCharacteristics_) {
        // The attribute handles increase by sequence.
        if (attrHandle >= characteristic.charHandles.decl_handle) {
            if (characteristic.charHandles.decl_handle > foundCharDeclHandle) {
                foundChar = &characteristic;
                foundCharDeclHandle = characteristic.charHandles.decl_handle;
            }
        }
    }
    return foundChar;
}

int BleGatt::discoverServices(hal_ble_conn_handle_t connHandle, const hal_ble_uuid_t* uuid, hal_ble_on_disc_service_cb_t callback, void* context) {
    CHECK_TRUE(BleGap::getInstance().valid(connHandle), SYSTEM_ERROR_NOT_FOUND);
    CHECK_FALSE(isDiscovering_, SYSTEM_ERROR_INVALID_STATE);
    SCOPE_GUARD ({
        resetDiscoveryState();
    });
    currDiscConnHandle_ = connHandle;
    discSvcCallback_ = callback;
    discSvcContext_ = context;
    if (uuid == nullptr) {
        CHECK_RTL(client_all_primary_srv_discovery(connHandle, clientId_));
    } else {
        discSvcUuid_ = uuid;
        if (uuid->type == BLE_UUID_TYPE_16BIT) {
            CHECK_RTL(client_by_uuid_srv_discovery(connHandle, clientId_, uuid->uuid16));
        } else if (uuid->type == BLE_UUID_TYPE_128BIT) {
            CHECK_RTL(client_by_uuid128_srv_discovery(connHandle, clientId_, (uint8_t*)uuid->uuid128));
        } else {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
    }
    isDiscovering_ = true;
    if (os_semaphore_take(discoverySemaphore_, BLE_OPERATION_TIMEOUT_MS, false)) {
        SPARK_ASSERT(false);
        return SYSTEM_ERROR_TIMEOUT;
    }
    return SYSTEM_ERROR_NONE;
}

int BleGatt::discoverCharacteristics(hal_ble_conn_handle_t connHandle, const hal_ble_svc_t* service,
        const hal_ble_uuid_t* uuid, hal_ble_on_disc_char_cb_t callback, void* context) {
    CHECK_TRUE(BleGap::getInstance().valid(connHandle), SYSTEM_ERROR_NOT_FOUND);
    CHECK_TRUE(service, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_FALSE(isDiscovering_, SYSTEM_ERROR_INVALID_STATE);
    SCOPE_GUARD ({
        resetDiscoveryState();
    });
    currDiscConnHandle_ = connHandle;
    discCharCallback_ = callback;
    discCharContext_ = context;
    currDiscService_ = service;
    if (uuid == nullptr) {
        CHECK_RTL(client_all_char_discovery(connHandle, clientId_, service->start_handle, service->end_handle));
    } else {
        if (uuid->type == BLE_UUID_TYPE_16BIT) {
            CHECK_RTL(client_by_uuid_char_discovery(connHandle, clientId_, service->start_handle, service->end_handle, uuid->uuid16));
        } else if (uuid->type == BLE_UUID_TYPE_128BIT) {
            CHECK_RTL(client_by_uuid128_char_discovery(connHandle, clientId_, service->start_handle, service->end_handle, (uint8_t*)uuid->uuid128));
        } else {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
    }
    isDiscovering_ = true;
    if (os_semaphore_take(discoverySemaphore_, BLE_OPERATION_TIMEOUT_MS, false)) {
        SPARK_ASSERT(false);
        return SYSTEM_ERROR_TIMEOUT;
    }
    return SYSTEM_ERROR_NONE;
}

int BleGatt::addPublisher(hal_ble_conn_handle_t connHandle, hal_ble_attr_handle_t valueHandle, hal_ble_on_char_evt_cb_t callback, void* context) {
    for (auto& publisher : publishers_) {
        if (publisher.connHandle == connHandle && publisher.valueHandle == valueHandle) {
            publisher.callback = callback;
            publisher.context = context;
            return SYSTEM_ERROR_NONE;
        }
    }
    Publisher pub = {};
    pub.connHandle = connHandle;
    pub.valueHandle = valueHandle;
    pub.callback = callback;
    pub.context = context;
    CHECK_TRUE(publishers_.append(pub), SYSTEM_ERROR_NO_MEMORY);
    return SYSTEM_ERROR_NONE;
}

int BleGatt::removePublisher(hal_ble_conn_handle_t connHandle, hal_ble_attr_handle_t valueHandle) {
    size_t i = 0;
    for (const auto& publisher : publishers_) {
        if (publisher.connHandle == connHandle && publisher.valueHandle == valueHandle) {
            publishers_.removeAt(i);
            return SYSTEM_ERROR_NONE;
        }
        i++;
    }
    return SYSTEM_ERROR_NONE;
}

int BleGatt::removeAllPublishersOfConnection(hal_ble_conn_handle_t connHandle) {
    for (int i = 0; i < publishers_.size(); i = i) {
        const auto& publisher = publishers_[i];
        if (publisher.connHandle == connHandle) {
            publishers_.removeAt(i);
            continue;
        }
        i++;
    }
    return SYSTEM_ERROR_NONE;
}

int BleGatt::configureRemoteCCCD(const hal_ble_cccd_config_t* config) {
    CHECK_TRUE(BleGap::getInstance().valid(config->conn_handle), SYSTEM_ERROR_NOT_FOUND);
    CHECK_TRUE(config->cccd_handle != BLE_INVALID_ATTR_HANDLE, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(config->value_handle != BLE_INVALID_ATTR_HANDLE, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(config->cccd_value <= BLE_SIG_CCCD_VAL_NOTI_IND, SYSTEM_ERROR_NOT_SUPPORTED);
    uint8_t buf[2] = {0x00, 0x00};
    buf[0] = config->cccd_value;
    CHECK(writeAttribute(config->conn_handle, config->cccd_handle, buf, sizeof(buf), true));
    if (config->cccd_value > BLE_SIG_CCCD_VAL_DISABLED && config->cccd_value <= BLE_SIG_CCCD_VAL_NOTI_IND) {
        CHECK(addPublisher(config->conn_handle, config->value_handle, config->callback, config->context));
    } else {
        CHECK(removePublisher(config->conn_handle, config->value_handle));
    }
    return SYSTEM_ERROR_NONE;
}

ssize_t BleGatt::writeAttribute(hal_ble_conn_handle_t connHandle, hal_ble_attr_handle_t attrHandle, const uint8_t* buf, size_t len, bool response) {
    CHECK_TRUE(buf && len, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(BleGap::getInstance().valid(connHandle), SYSTEM_ERROR_NOT_FOUND);
    SCOPE_GUARD ({
        writeAttrHandle_ = BLE_INVALID_ATTR_HANDLE;
        currWriteConnHandle_ = BLE_INVALID_CONN_HANDLE;
    });
    T_GATT_WRITE_TYPE writeType = GATT_WRITE_TYPE_CMD;
    if (response) {
        writeType = GATT_WRITE_TYPE_REQ;
    }
    writeAttrHandle_ = attrHandle;
    currWriteConnHandle_ = connHandle;
    len = std::min(len, (size_t)BLE_ATTR_VALUE_PACKET_SIZE(BleGap::getInstance().getAttMtu(connHandle)));
    CHECK_RTL(client_attr_write(connHandle, clientId_, writeType, attrHandle, len, (uint8_t*)buf));
    if (os_semaphore_take(writeSemaphore_, BLE_OPERATION_TIMEOUT_MS, false)) {
        SPARK_ASSERT(false);
        return SYSTEM_ERROR_TIMEOUT;
    }
    return len;
}

// FIXME: Multi-link read
ssize_t BleGatt::readAttribute(hal_ble_conn_handle_t connHandle, hal_ble_attr_handle_t attrHandle, uint8_t* buf, size_t len) {
    CHECK_TRUE(buf && len, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(BleGap::getInstance().valid(connHandle), SYSTEM_ERROR_NOT_FOUND);
    SCOPE_GUARD ({
        readLen_ = 0;
        readBuf_ = nullptr;
        readAttrHandle_ = BLE_INVALID_ATTR_HANDLE;
        currReadConnHandle_ = BLE_INVALID_CONN_HANDLE;
    });
    readAttrHandle_ = attrHandle;
    currReadConnHandle_ = connHandle;
    readBuf_ = buf;
    readLen_ = std::min(len, (size_t)BLE_ATTR_VALUE_PACKET_SIZE(BleGap::getInstance().getAttMtu(connHandle)));
    CHECK_RTL(client_attr_read(connHandle, clientId_, attrHandle));
    if (os_semaphore_take(readSemaphore_, BLE_OPERATION_TIMEOUT_MS, false)) {
        SPARK_ASSERT(false);
        return SYSTEM_ERROR_TIMEOUT;
    }
    return readLen_;
}


/**********************************************
 * Particle BLE APIs
 */
uint32_t ftl_load_from_storage(void *pdata_tmp, uint16_t offset, uint16_t size) {
    LOG_DEBUG(TRACE, "ftl_load_from_storage().");
    return 1;
}

uint32_t ftl_save_to_storage(void *pdata, uint16_t offset, uint16_t size) {
    LOG_DEBUG(TRACE, "ftl_save_to_storage().");
    return 1;
}

int hal_ble_lock(void* reserved) {
    return !s_bleMutex.lock();
}

int hal_ble_unlock(void* reserved) {
    return !s_bleMutex.unlock();
}

int hal_ble_enter_locked_mode(void* reserved) {
    BleGap::getInstance().lockMode(true);
    return SYSTEM_ERROR_NONE;
}

int hal_ble_exit_locked_mode(void* reserved) {
    BleGap::getInstance().lockMode(false);
    return SYSTEM_ERROR_NONE;
}

int hal_ble_stack_init(void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_stack_init().");
    CHECK(BleGap::getInstance().init());
    return SYSTEM_ERROR_NONE;
}

int hal_ble_stack_deinit(void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_stack_deinit().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    CHECK(BleGap::getInstance().stop());
    return SYSTEM_ERROR_NONE;
}

bool hal_ble_is_initialized(void* reserved) {
    BleLock lk;
    return BleGap::getInstance().initialized();
}

int hal_ble_select_antenna(hal_ble_ant_type_t antenna, void* reserved) {
    CHECK(selectRadioAntenna((radio_antenna_type)antenna));
    return SYSTEM_ERROR_NONE;
}

int hal_ble_set_callback_on_adv_events(hal_ble_on_adv_evt_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_set_callback_on_adv_events().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    CHECK(BleGap::getInstance().onAdvEventCallback(callback, context));
    return SYSTEM_ERROR_NONE;
}

int hal_ble_cancel_callback_on_adv_events(hal_ble_on_adv_evt_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_cancel_callback_on_adv_events().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    BleGap::getInstance().cancelAdvEventCallback(callback, context);
    return SYSTEM_ERROR_NONE;
}

int hal_ble_set_callback_on_periph_link_events(hal_ble_on_link_evt_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_set_callback_on_periph_link_events().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().onPeripheralLinkEventCallback(callback, context);
}

/**********************************************
 * BLE GAP APIs
 */
int hal_ble_gap_set_device_address(const hal_ble_addr_t* address, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_device_address().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().setDeviceAddress(address);
}

int hal_ble_gap_get_device_address(hal_ble_addr_t* address, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_device_address().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().getDeviceAddress(address);
}

int hal_ble_gap_set_device_name(const char* device_name, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_device_name().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    CHECK(BleGap::getInstance().setDeviceName(device_name, len));
    return SYSTEM_ERROR_NONE;
}

int hal_ble_gap_get_device_name(char* device_name, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_device_name().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().getDeviceName(device_name, len);
}

int hal_ble_gap_set_appearance(ble_sig_appearance_t appearance, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_appearance().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().setAppearance((uint16_t)appearance);
}

int hal_ble_gap_get_appearance(ble_sig_appearance_t* appearance, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_appearance().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_set_ppcp(const hal_ble_conn_params_t* ppcp, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_ppcp().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    CHECK_FALSE(BleGap::getInstance().lockMode(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().setPpcp(ppcp);
}

int hal_ble_gap_get_ppcp(hal_ble_conn_params_t* ppcp, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_ppcp().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().getPpcp(ppcp);
}

int hal_ble_gap_add_whitelist(const hal_ble_addr_t* addr_list, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_add_whitelist().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_delete_whitelist(void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_delete_whitelist().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_set_tx_power(int8_t tx_power, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_tx_power().");
    CHECK_FALSE(BleGap::getInstance().lockMode(), SYSTEM_ERROR_INVALID_STATE);
    return SYSTEM_ERROR_NONE;
}

int hal_ble_gap_get_tx_power(int8_t* tx_power, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_tx_power().");
    return SYSTEM_ERROR_NONE;
}

int hal_ble_gap_set_advertising_parameters(const hal_ble_adv_params_t* adv_params, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_advertising_parameters().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    CHECK_FALSE(BleGap::getInstance().lockMode(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().setAdvertisingParameters(adv_params);
}

int hal_ble_gap_get_advertising_parameters(hal_ble_adv_params_t* adv_params, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_advertising_parameters().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().getAdvertisingParameters(adv_params);
}

int hal_ble_gap_set_advertising_data(const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_advertising_data().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    CHECK_FALSE(BleGap::getInstance().lockMode(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().setAdvertisingData(buf, len);
}

ssize_t hal_ble_gap_get_advertising_data(uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_advertising_data().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().getAdvertisingData(buf, len);
}

int hal_ble_gap_set_scan_response_data(const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_scan_response_data().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    CHECK_FALSE(BleGap::getInstance().lockMode(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().setScanResponseData(buf, len);
}

ssize_t hal_ble_gap_get_scan_response_data(uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_scan_response_data().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().getScanResponseData(buf, len);
}

int hal_ble_gap_start_advertising(void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_start_advertising().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().startAdvertising();
}

int hal_ble_gap_set_auto_advertise(hal_ble_auto_adv_cfg_t config, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_auto_advertise().");
    CHECK_FALSE(BleGap::getInstance().lockMode(), SYSTEM_ERROR_INVALID_STATE);
    return SYSTEM_ERROR_NONE;
}

int hal_ble_gap_get_auto_advertise(hal_ble_auto_adv_cfg_t* cfg, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_auto_advertise().");
    return SYSTEM_ERROR_NONE;
}

int hal_ble_gap_stop_advertising(void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_stop_advertising().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    CHECK_FALSE(BleGap::getInstance().lockMode(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().stopAdvertising();
}

bool hal_ble_gap_is_advertising(void* reserved) {
    BleLock lk;
    CHECK_TRUE(BleGap::getInstance().initialized(), false);
    return BleGap::getInstance().isAdvertising();
}

int hal_ble_gap_set_scan_parameters(const hal_ble_scan_params_t* scan_params, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_scan_parameters().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().setScanParams(scan_params);
}

int hal_ble_gap_get_scan_parameters(hal_ble_scan_params_t* scan_params, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_scan_parameters().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().getScanParams(scan_params);
}

int hal_ble_gap_start_scan(hal_ble_on_scan_result_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_start_scan().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().startScanning(callback, context);
}

bool hal_ble_gap_is_scanning(void* reserved) {
    BleLock lk;
    CHECK_TRUE(BleGap::getInstance().initialized(), false);
    return BleGap::getInstance().scanning();
}

int hal_ble_gap_stop_scan(void* reserved) {
    // Do not acquire the lock here, otherwise another thread cannot cancel the scanning.
    LOG_DEBUG(TRACE, "hal_ble_gap_stop_scan().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().stopScanning();
}

int hal_ble_gap_connect(const hal_ble_conn_cfg_t* config, hal_ble_conn_handle_t* conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_connect().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().connect(config, conn_handle);
}

bool hal_ble_gap_is_connecting(const hal_ble_addr_t* address, void* reserved) {
    BleLock lk;
    CHECK_TRUE(BleGap::getInstance().initialized(), false);
    return BleGap::getInstance().connecting();
}

bool hal_ble_gap_is_connected(const hal_ble_addr_t* address, void* reserved) {
    CHECK_TRUE(BleGap::getInstance().initialized(), false);
    return BleGap::getInstance().connected(address);
}

int hal_ble_gap_connect_cancel(const hal_ble_addr_t* address, void* reserved) {
    LOG_DEBUG(TRACE, "hal_ble_gap_connect_cancel().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    // Do not acquire the lock here, otherwise another thread cannot cancel the connection attempt.
    return BleGap::getInstance().connectCancel(address);
}

int hal_ble_gap_disconnect(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_disconnect().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    hal_ble_conn_info_t info = {};
    CHECK(BleGap::getInstance().getConnectionInfo(conn_handle, &info));
    CHECK_FALSE(BleGap::getInstance().lockMode() && info.role == BLE_ROLE_PERIPHERAL, SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().disconnect(conn_handle);
}

int hal_ble_gap_update_connection_params(hal_ble_conn_handle_t conn_handle, const hal_ble_conn_params_t* conn_params, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_update_connection_params().");
    hal_ble_conn_info_t info = {};
    CHECK(BleGap::getInstance().getConnectionInfo(conn_handle, &info));
    CHECK_FALSE(BleGap::getInstance().lockMode() && info.role == BLE_ROLE_PERIPHERAL, SYSTEM_ERROR_INVALID_STATE);
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_get_connection_info(hal_ble_conn_handle_t conn_handle, hal_ble_conn_info_t* info, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_connection_info().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().getConnectionInfo(conn_handle, info);
}

int hal_ble_gap_get_rssi(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_rssi().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_set_pairing_config(const hal_ble_pairing_config_t* config, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_pairing_config().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().setPairingConfig(config);
}

int hal_ble_gap_get_pairing_config(hal_ble_pairing_config_t* config, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_pairing_config().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().getPairingConfig(config);
}

int hal_ble_gap_start_pairing(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_start_pairing().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().startPairing(conn_handle);
}

int hal_ble_gap_reject_pairing(hal_ble_conn_handle_t conn_handle, void* reserved) {
    // NOT acquiring BLE lock here, we can get deadlocked if the event comes before connect() finishes
    // There is another mutex that works just for connections_ array
    LOG_DEBUG(TRACE, "hal_ble_gap_reject_pairing().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().rejectPairing(conn_handle);
}

int hal_ble_gap_set_pairing_auth_data(hal_ble_conn_handle_t conn_handle, const hal_ble_pairing_auth_data_t* auth, void* reserved) {
    // NOT acquiring BLE lock here, we can get deadlocked if the event comes before connect() finishes
    // There is another mutex that works just for connections_ array
    LOG_DEBUG(TRACE, "hal_ble_gap_set_pairing_auth_data().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().setPairingAuthData(conn_handle, auth);
}

int hal_ble_gap_set_pairing_passkey_deprecated(hal_ble_conn_handle_t conn_handle, const uint8_t* passkey, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_pairing_passkey_deprecated().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

bool hal_ble_gap_is_pairing(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    // Too noisy
    // LOG_DEBUG(TRACE, "hal_ble_gap_is_pairing().");
    CHECK_TRUE(BleGap::getInstance().initialized(), false);
    return BleGap::getInstance().isPairing(conn_handle);
}

bool hal_ble_gap_is_paired(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_is_paired().");
    CHECK_TRUE(BleGap::getInstance().initialized(), false);
    return BleGap::getInstance().isPaired(conn_handle);
}

/**********************************************
 * BLE GATT Server APIs
 */
int hal_ble_gatt_server_add_service(uint8_t type, const hal_ble_uuid_t* uuid, hal_ble_attr_handle_t* handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_add_service().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGatt::getInstance().addService(type, uuid, handle);;
}

int hal_ble_gatt_server_add_characteristic(const hal_ble_char_init_t* char_init, hal_ble_char_handles_t* char_handles, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_add_characteristic().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGatt::getInstance().addCharacteristic(char_init, char_handles);
}

int hal_ble_gatt_server_add_descriptor(const hal_ble_desc_init_t* desc_init, hal_ble_attr_handle_t* handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_add_descriptor().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

ssize_t hal_ble_gatt_server_set_characteristic_value(hal_ble_attr_handle_t value_handle, const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_set_characteristic_value().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGatt::getInstance().setValue(value_handle, buf, len);
}

ssize_t hal_ble_gatt_server_notify_characteristic_value(hal_ble_attr_handle_t value_handle, const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_notify_characteristic_value().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGatt::getInstance().notifyValue(value_handle, buf, len, false);
}

ssize_t hal_ble_gatt_server_indicate_characteristic_value(hal_ble_attr_handle_t value_handle, const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_indicate_characteristic_value().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGatt::getInstance().notifyValue(value_handle, buf, len, true);
}

ssize_t hal_ble_gatt_server_get_characteristic_value(hal_ble_attr_handle_t value_handle, uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_get_characteristic_value().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGatt::getInstance().getValue(value_handle, buf, len);
}

/**********************************************
 * BLE GATT Client APIs
 */
int hal_ble_gatt_client_discover_all_services(hal_ble_conn_handle_t conn_handle, hal_ble_on_disc_service_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_discover_all_services().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGatt::getInstance().discoverServices(conn_handle, nullptr, callback, context);
}

int hal_ble_gatt_client_discover_service_by_uuid(hal_ble_conn_handle_t conn_handle, const hal_ble_uuid_t* uuid, hal_ble_on_disc_service_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_discover_service_by_uuid().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGatt::getInstance().discoverServices(conn_handle, uuid, callback, context);
}

int hal_ble_gatt_client_discover_characteristics(hal_ble_conn_handle_t conn_handle, const hal_ble_svc_t* service, hal_ble_on_disc_char_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_discover_characteristics().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGatt::getInstance().discoverCharacteristics(conn_handle, service, nullptr, callback, context);
}

int hal_ble_gatt_client_discover_characteristics_by_uuid(hal_ble_conn_handle_t conn_handle, const hal_ble_svc_t* service, const hal_ble_uuid_t* uuid, hal_ble_on_disc_char_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_discover_characteristics_by_uuid().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGatt::getInstance().discoverCharacteristics(conn_handle, service, uuid, callback, context);
}

bool hal_ble_gatt_client_is_discovering(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    CHECK_TRUE(BleGap::getInstance().initialized(), false);
    return BleGatt::getInstance().discovering(conn_handle);
}

int hal_ble_gatt_server_set_desired_att_mtu(size_t att_mtu, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_set_desired_att_mtu().");
    return BleGatt::getInstance().setDesiredAttMtu(att_mtu);
}

ssize_t hal_ble_gatt_get_att_mtu(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_get_att_mtu().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().getAttMtu(conn_handle);
}

int hal_ble_gatt_client_att_mtu_exchange(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_att_mtu_exchange().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gatt_client_configure_cccd(const hal_ble_cccd_config_t* config, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_configure_cccd().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGatt::getInstance().configureRemoteCCCD(config);
}

ssize_t hal_ble_gatt_client_write_with_response(hal_ble_conn_handle_t conn_handle, hal_ble_attr_handle_t value_handle, const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_write_with_response().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGatt::getInstance().writeAttribute(conn_handle, value_handle, buf, len, true);
}

ssize_t hal_ble_gatt_client_write_without_response(hal_ble_conn_handle_t conn_handle, hal_ble_attr_handle_t value_handle, const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_write_without_response().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGatt::getInstance().writeAttribute(conn_handle, value_handle, buf, len, false);
}

ssize_t hal_ble_gatt_client_read(hal_ble_conn_handle_t conn_handle, hal_ble_attr_handle_t value_handle, uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_read().");
    CHECK_TRUE(BleGap::getInstance().initialized(), SYSTEM_ERROR_INVALID_STATE);
    return BleGatt::getInstance().readAttribute(conn_handle, value_handle, buf, len);
}


#if HAL_PLATFORM_BLE_BETA_COMPAT

int hal_ble_set_callback_on_events_deprecated(hal_ble_on_generic_evt_cb_deprecated_t callback, void* context, void* reserved) {
    return SYSTEM_ERROR_DEPRECATED;
}

int hal_ble_gap_connect_deprecated(const hal_ble_addr_t* address, void* reserved) {
    return SYSTEM_ERROR_DEPRECATED;
}

int hal_ble_gatt_server_add_characteristic_deprecated(const hal_ble_char_init_deprecated_t* char_init, hal_ble_char_handles_t* char_handles, void* reserved) {
    return SYSTEM_ERROR_DEPRECATED;
}

int hal_ble_gatt_client_configure_cccd_deprecated(hal_ble_conn_handle_t conn_handle, hal_ble_attr_handle_t cccd_handle, ble_sig_cccd_value_t cccd_value, void* reserved) {
    return SYSTEM_ERROR_DEPRECATED;
}

int hal_ble_gap_get_connection_params_deprecated(hal_ble_conn_handle_t conn_handle, hal_ble_conn_params_t* conn_params, void* reserved) {
    return SYSTEM_ERROR_DEPRECATED;
}

#endif // #if HAL_PLATFORM_BLE_BETA_COMPAT

#endif // HAL_PLATFORM_BLE
