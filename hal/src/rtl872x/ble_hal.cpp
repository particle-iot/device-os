/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#undef LOG_COMPILE_TIME_LEVEL
#include "logging.h"
LOG_SOURCE_CATEGORY("hal.ble");

#include "ble_hal.h"

#if HAL_PLATFORM_BLE

#ifdef __cplusplus
extern "C" {
#endif
#include "rtl8721d.h"
void bt_coex_init(void);
#ifdef __cplusplus
} // extern "C"
#endif

#include <platform_opts_bt.h>
#include <os_sched.h>
#include <string.h>
#include <trace_app.h>
#include <gap.h>
#include <gap_adv.h>
#include <gap_bond_le.h>
#include <gap_conn_le.h>
#include <profile_server.h>
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
#include "rtl8721d.h"

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

#include "mbedtls/ecdh.h"
#include "mbedtls_util.h"

// FIXME
#undef OFF
#undef ON
#include "network/ncp/wifi/ncp.h"
#include "network/ncp/wifi/wifi_network_manager.h"
#include "network/ncp/wifi/wifi_ncp_client.h"

using spark::Vector;
using namespace particle;
using namespace particle::ble;

namespace {

#define CHECK_RTL(_expr) \
        ({ \
            const auto _ret = _expr; \
            if (_ret != GAP_CAUSE_SUCCESS) { \
                _LOG_CHECKED_ERROR(_expr, rtl_system_error(_ret)); \
                return rtl_ble_error_to_system(_ret); \
            } \
            _ret; \
        })

StaticRecursiveMutex s_bleMutex;

bool s_bleStackInit = false;

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

} //anonymous namespace


class BleEventDispatcher {
public:
    int init();
    int start() const;

    static BleEventDispatcher& getInstance() {
        static BleEventDispatcher instance;
        return instance;
    }

private:
    BleEventDispatcher()
            : thread_(nullptr),
              allEvtQueue_(nullptr),
              ioEvtQueue_(nullptr) {
    }
    ~BleEventDispatcher() = default;

    void handleIoMessage(T_IO_MSG message) const;
    static void bleEventDispatchThread(void *context);

    void* thread_;
    void* allEvtQueue_;
    void* ioEvtQueue_;
    static constexpr uint32_t BLE_EVENT_THREAD_STACK_SZIE = 10 * 1024;
    static constexpr uint8_t BLE_EVENT_THREAD_PRIORITY = 2; // Higher value, higher priority
    static constexpr uint8_t MAX_NUMBER_OF_GAP_MESSAGE = 0x20;
    static constexpr uint8_t MAX_NUMBER_OF_IO_MESSAGE = 0x20;
    static constexpr uint8_t MAX_NUMBER_OF_EVENT_MESSAGE = (MAX_NUMBER_OF_GAP_MESSAGE + MAX_NUMBER_OF_IO_MESSAGE);
};


class BleGap {
public:
    int init();
    int start();
    int setAppearance(uint16_t appearance) const;
    int setDeviceName(const char* deviceName, size_t len);
    int getDeviceName(char* deviceName, size_t len) const;

    int setAdvertisingParameters(const hal_ble_adv_params_t* params);
    int getAdvertisingParameters(hal_ble_adv_params_t* params) const;
    int setAdvertisingData(const uint8_t* buf, size_t len);
    ssize_t getAdvertisingData(uint8_t* buf, size_t len) const;
    int setScanResponseData(const uint8_t* buf, size_t len);
    ssize_t getScanResponseData(uint8_t* buf, size_t len) const;
    int startAdvertising(bool wait) const;
    int stopAdvertising(bool wait) const;

    bool isAdvertising() const {
        return state_.gap_adv_state == GAP_ADV_STATE_ADVERTISING;
    }

    int disconnect() const;

    bool connected() const {
        return connHandle_ != BLE_INVALID_CONN_HANDLE;
    }

    bool connecting() const {
        return connecting_;
    }

    void handleDevStateChanged(T_GAP_DEV_STATE state, uint16_t cause);
    void handleConnectionStateChanged(uint8_t connHandle, T_GAP_CONN_STATE newState, uint16_t discCause);
    void handleMtuUpdated(uint8_t connHandle, uint16_t mtuSize);
    void handleConnParamsUpdated(uint8_t connHandle, uint8_t status, uint16_t cause);
    void handleAuthenStateChanged(uint8_t connHandle, uint8_t state, uint16_t cause);

    int onPeripheralLinkEventCallback(hal_ble_on_link_evt_cb_t cb, void* context) {
        std::pair<hal_ble_on_link_evt_cb_t, void*> callback(cb, context);
        CHECK_TRUE(periphEvtCallbacks_.append(callback), SYSTEM_ERROR_NO_MEMORY);
        return SYSTEM_ERROR_NONE;
    }

    static BleGap& getInstance() {
        static BleGap instance;
        return instance;
    }

private:
    BleGap()
            : state_{},
              advParams_{},
              connParams_{},
              periphEvtCallbacks_{},
              connHandle_(BLE_INVALID_CONN_HANDLE),
              connecting_(false),
              devNameLen_(0) {
        state_.gap_init_state = GAP_INIT_STATE_INIT;

        advParams_.version = BLE_API_VERSION;
        advParams_.size = sizeof(hal_ble_adv_params_t);
        advParams_.interval = BLE_DEFAULT_ADVERTISING_INTERVAL;
        advParams_.timeout = BLE_DEFAULT_ADVERTISING_TIMEOUT;
        advParams_.type = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT;
        advParams_.filter_policy = BLE_ADV_FP_ANY;
        advParams_.inc_tx_power = false;
        advParams_.primary_phy = 0; // Not used

        memset(advData_, 0x00, sizeof(advData_));
        memset(scanRespData_, 0x00, sizeof(scanRespData_));
        advDataLen_ = scanRespDataLen_ = 0;
        /* Default advertising data. */
        // FLags
        advData_[advDataLen_++] = 0x02; // Length field of an AD structure, it is the length of the rest AD structure data.
        advData_[advDataLen_++] = BLE_SIG_AD_TYPE_FLAGS; // Type field of an AD structure.
        advData_[advDataLen_++] = BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE; // Payload field of an AD structure.

        connParams_.version = BLE_API_VERSION;
        connParams_.size = sizeof(hal_ble_conn_params_t);
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

    static T_APP_RESULT gapEventCallback(uint8_t type, void *data);

    T_GAP_DEV_STATE state_;
    hal_ble_adv_params_t advParams_;
    hal_ble_conn_params_t connParams_;
    Vector<std::pair<hal_ble_on_link_evt_cb_t, void*>> periphEvtCallbacks_;
    hal_ble_conn_handle_t connHandle_;
    bool connecting_;
    uint8_t advData_[BLE_MAX_ADV_DATA_LEN];         /**< Current advertising data. */
    size_t advDataLen_;                             /**< Current advertising data length. */
    uint8_t scanRespData_[BLE_MAX_ADV_DATA_LEN];    /**< Current scan response data. */
    size_t scanRespDataLen_;                        /**< Current scan response data length. */
    char devName_[BLE_MAX_DEV_NAME_LEN + 1];        // null-terminated
    size_t devNameLen_;
    static constexpr uint8_t MAX_LINK_COUNT = 4;
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
    int addService(uint8_t type, const hal_ble_uuid_t* uuid, hal_ble_attr_handle_t* svcHandle);
    int addCharacteristic(const hal_ble_char_init_t* charInit, hal_ble_char_handles_t* charHandles);
    int registerAttributeTable();
    ssize_t setValue(hal_ble_attr_handle_t attrHandle, const uint8_t* buf, size_t len);
    ssize_t getValue(hal_ble_attr_handle_t attrHandle, uint8_t* buf, size_t len);
    ssize_t notifyValue(hal_ble_attr_handle_t attrHandle, const uint8_t* buf, size_t len, bool ack);
    int removeSubscriber(hal_ble_conn_handle_t connHandle);

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
              callbacks_() {
        callbacks_.read_attr_cb = gattReadAttrCallback;
        callbacks_.write_attr_cb = gattWriteAttrCallback;
        callbacks_.cccd_update_cb = gattCccdUpdatedCallback;
    }
    ~BleGatt() = default;

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

    static T_APP_RESULT gattServerEventCallback(T_SERVER_ID serviceId, void *pData);
    static T_APP_RESULT gattReadAttrCallback(uint8_t connHandle, T_SERVER_ID serviceId, uint16_t index, uint16_t offset, uint16_t *length, uint8_t **value);
    static T_APP_RESULT gattWriteAttrCallback(uint8_t connHandle, T_SERVER_ID serviceId, uint16_t index, T_WRITE_TYPE type, uint16_t length, uint8_t *value, P_FUN_WRITE_IND_POST_PROC *postProc);
    static void gattCccdUpdatedCallback(uint8_t connHandle, T_SERVER_ID serviceId, uint16_t index, uint16_t bits);

    bool serviceRegistered_;
    T_FUN_GATT_SERVICE_CBS callbacks_;
    Vector<BleService> services_;
    static constexpr uint8_t MAX_ALLOWED_BLE_SERVICES = 5;
    static constexpr uint16_t CUSTOMER_SERVICE_START_HANDLE = 30;
    static constexpr uint8_t SERVICE_HANDLE_RANGE_RESERVED = 20;
};

int BleEventDispatcher::init() {
    if (!thread_) {
        CHECK_TRUE(os_msg_queue_create(&ioEvtQueue_, MAX_NUMBER_OF_IO_MESSAGE, sizeof(T_IO_MSG)), SYSTEM_ERROR_INTERNAL);
        CHECK_TRUE(os_msg_queue_create(&allEvtQueue_, MAX_NUMBER_OF_EVENT_MESSAGE, sizeof(uint8_t)), SYSTEM_ERROR_INTERNAL);
        CHECK_TRUE(os_task_create(&thread_, "bleEvtThread", bleEventDispatchThread, this, BLE_EVENT_THREAD_STACK_SZIE, BLE_EVENT_THREAD_PRIORITY), SYSTEM_ERROR_INTERNAL);
    }
    return SYSTEM_ERROR_NONE;
}

int BleEventDispatcher::start() const {
    CHECK_TRUE(gap_start_bt_stack(allEvtQueue_, ioEvtQueue_, MAX_NUMBER_OF_GAP_MESSAGE), SYSTEM_ERROR_INTERNAL);
    return SYSTEM_ERROR_NONE;
}

void BleEventDispatcher::handleIoMessage(T_IO_MSG message) const {
    switch (message.type) {
        case IO_MSG_TYPE_BT_STATUS: {
            T_LE_GAP_MSG gap_msg;
            uint8_t conn_id;
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
                    conn_id = gap_msg.msg_data.gap_bond_just_work_conf.conn_id;
                    le_bond_just_work_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
                    break;
                }
                case GAP_MSG_LE_BOND_PASSKEY_DISPLAY: {
                    uint32_t display_value = 0;
                    conn_id = gap_msg.msg_data.gap_bond_passkey_display.conn_id;
                    le_bond_get_display_key(conn_id, &display_value);
                    LOG_DEBUG(TRACE, "handleIoMessage, GAP_MSG_LE_BOND_PASSKEY_DISPLAY, passkey %d", display_value);
                    le_bond_passkey_display_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
                    break;
                }
                case GAP_MSG_LE_BOND_USER_CONFIRMATION: {
                    uint32_t display_value = 0;
                    conn_id = gap_msg.msg_data.gap_bond_user_conf.conn_id;
                    le_bond_get_display_key(conn_id, &display_value);
                    LOG_DEBUG(TRACE, "handleIoMessage, GAP_MSG_LE_BOND_USER_CONFIRMATION, passkey %d", display_value);
                    //le_bond_user_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
                    break;
                }
                case GAP_MSG_LE_BOND_PASSKEY_INPUT: {
                    //uint32_t passkey = 888888;
                    conn_id = gap_msg.msg_data.gap_bond_passkey_input.conn_id;
                    LOG_DEBUG(TRACE, "handleIoMessage: GAP_MSG_LE_BOND_PASSKEY_INPUT, conn_id %d", conn_id);
                    //le_bond_passkey_input_confirm(conn_id, passkey, GAP_CFM_CAUSE_ACCEPT);
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
            } else {
                // Handled by BT lib, which will finally invoke the registered callback.
                // Do not do time consuming stuff in the callbacks.
                gap_handle_msg(event);
            }
        }
    }
}

int BleGap::init() {
    CHECK_RTL(le_get_gap_param(GAP_PARAM_DEV_STATE , &state_));
    if (state_.gap_init_state == GAP_INIT_STATE_STACK_READY) {
        return SYSTEM_ERROR_NONE;
    }
    gap_config_max_le_link_num(MAX_LINK_COUNT);
    gap_config_max_le_paired_device(MAX_LINK_COUNT);
    CHECK_TRUE(bte_init(), SYSTEM_ERROR_INTERNAL);
    {
        int ret = SYSTEM_ERROR_INTERNAL;
        SCOPE_GUARD ({
            if (ret != SYSTEM_ERROR_NONE) {
                bte_deinit();
            }
        });
        CHECK_TRUE(le_gap_init(MAX_LINK_COUNT), SYSTEM_ERROR_INTERNAL);
        uint8_t mtuReq = false;
        CHECK_RTL(le_set_gap_param(GAP_PARAM_SLAVE_INIT_GATT_MTU_REQ, sizeof(mtuReq), &mtuReq));

        CHECK(setAppearance(GAP_GATT_APPEARANCE_UNKNOWN));

        if (devNameLen_ == 0) {
            int len = get_device_name(devName_, sizeof(devName_)); // null-terminated
            CHECK(len);
            devNameLen_ = std::min(len, (int)(sizeof(devName_) - 1));
        }
        CHECK(setDeviceName(devName_, strlen(devName_)));

        CHECK(setAdvertisingParameters(&advParams_));

        /* register gap message callback */
        le_register_app_cb(gapEventCallback);

        ret = SYSTEM_ERROR_NONE;
    }
    return SYSTEM_ERROR_NONE;
}

int BleGap::start() {
    if (state_.gap_init_state == GAP_INIT_STATE_INIT) {
        CHECK(BleEventDispatcher::getInstance().start());
        LOG_DEBUG(TRACE, "Waiting ready...");
        do {
            HAL_Delay_Milliseconds(100);
            le_get_gap_param(GAP_PARAM_DEV_STATE , &state_);
        } while (state_.gap_init_state != GAP_INIT_STATE_STACK_READY);
    }
    return SYSTEM_ERROR_NONE;
}

int BleGap::setAppearance(uint16_t appearance) const {
    CHECK_RTL(le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance));
    return SYSTEM_ERROR_NONE;
}

int BleGap::setDeviceName(const char* deviceName, size_t len) {
    if (deviceName == nullptr || len == 0) {
        int ret = get_device_name(devName_, sizeof(devName_)); // null-terminated
        CHECK(ret);
        devNameLen_ = std::min(ret, (int)(sizeof(devName_) - 1));
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
            startAdvertising(true);
        }
    });
    CHECK(stopAdvertising(true));
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
            startAdvertising(true);
        }
    });
    CHECK(stopAdvertising(true));
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
            startAdvertising(true);
        }
    });
    CHECK(stopAdvertising(true));
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

int BleGap::startAdvertising(bool wait) const {
    // The attribute table needs to be registered before BT stack starts.
    // Now we assume that the attribute table is finalized.
    // Register it to BT stack
    BleGatt::getInstance().registerAttributeTable();
    // Starts the BT stack
    CHECK(BleGap::getInstance().start());

    if (state_.gap_adv_state != GAP_ADV_STATE_ADVERTISING && state_.gap_adv_state != GAP_ADV_STATE_START) {
        CHECK(le_adv_start());
    }
    if (wait) {
        LOG_DEBUG(TRACE, "Starting advertising...");
        while (state_.gap_adv_state != GAP_ADV_STATE_ADVERTISING) {
            HAL_Delay_Milliseconds(100);
        }
    }
    LOG_DEBUG(TRACE, "Advertising started");
    return SYSTEM_ERROR_NONE;
}

int BleGap::stopAdvertising(bool wait) const {
    if (state_.gap_adv_state != GAP_ADV_STATE_ADVERTISING && state_.gap_adv_state != GAP_ADV_STATE_START) {
        return SYSTEM_ERROR_NONE;
    }
    CHECK(le_adv_stop());
    if (wait) {
        LOG_DEBUG(TRACE, "Stopping advertising...");
        while (state_.gap_adv_state != GAP_ADV_STATE_IDLE) {
            HAL_Delay_Milliseconds(100);
        }
    }
    LOG_DEBUG(TRACE, "Advertising stopped");
    return SYSTEM_ERROR_NONE;
}

int BleGap::disconnect() const {
    if (connHandle_ != BLE_INVALID_CONN_HANDLE) {
        CHECK(le_disconnect(connHandle_));
    }
    return SYSTEM_ERROR_NONE;
}

void BleGap::handleDevStateChanged(T_GAP_DEV_STATE newState, uint16_t cause) {
    LOG_DEBUG(TRACE, "GAP state updated, init:%d, adv:%d, adv-sub:%d, scan:%d, conn:%d, cause:%d",
                        newState.gap_init_state, newState.gap_adv_state, newState.gap_adv_sub_state, newState.gap_scan_state, newState.gap_conn_state, cause);
    if (state_.gap_init_state == GAP_INIT_STATE_INIT && newState.gap_init_state == GAP_INIT_STATE_STACK_READY) {
        LOG_DEBUG(TRACE, "Start BT co-existence.");
        bt_coex_init();
        wifi_btcoex_set_bt_on();
    }
    state_ = newState;
}

void BleGap::handleConnectionStateChanged(uint8_t connHandle, T_GAP_CONN_STATE newState, uint16_t discCause) {
    if (newState == GAP_CONN_STATE_CONNECTING) {
        connecting_ = true;
    } else {
        connecting_ = false;
    }
    switch (newState) {
        case GAP_CONN_STATE_CONNECTING: {
            LOG_DEBUG(TRACE, "Connecting...");
            break;
        }
        case GAP_CONN_STATE_DISCONNECTING: {
            LOG_DEBUG(TRACE, "Disconnecting...");
            break;
        }
        case GAP_CONN_STATE_DISCONNECTED: {
            connHandle_ = BLE_INVALID_CONN_HANDLE;
            if ((discCause != (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE)) && (discCause != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE))) {
                // placeholder
            }
            LOG_DEBUG(TRACE, "[BLE peripheral] Disconnected, handle:%d, cause:0x%x, re-start ADV", connHandle, discCause);
            for (const auto& callback : periphEvtCallbacks_) {
                if (callback.first) {
                    hal_ble_link_evt_t evt = {};
                    evt.type = BLE_EVT_DISCONNECTED;
                    evt.conn_handle = connHandle;
                    evt.params.disconnected.reason = (uint8_t)discCause;
                    callback.first(&evt, callback.second);
                }
            }
            BleGatt::getInstance().removeSubscriber(connHandle);
            startAdvertising(false);
            break;
        }
        case GAP_CONN_STATE_CONNECTED: {
            connHandle_ = connHandle;
            hal_ble_addr_t peerAddr = {};
            peerAddr.addr_type = BLE_SIG_ADDR_TYPE_PUBLIC;
            le_get_conn_addr(connHandle, peerAddr.addr, (uint8_t *)&peerAddr.addr_type);
            le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &connParams_.min_conn_interval, connHandle);
            le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &connParams_.max_conn_interval, connHandle);
            le_get_conn_param(GAP_PARAM_CONN_LATENCY, &connParams_.slave_latency, connHandle);
            le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &connParams_.conn_sup_timeout, connHandle);
            LOG_DEBUG(TRACE, "[BLE peripheral] Connected, interval:0x%x, latency:0x%x, timeout:0x%x",
                            connParams_.max_conn_interval, connParams_.slave_latency, connParams_.conn_sup_timeout);
            for (const auto& callback : periphEvtCallbacks_) {
                if (callback.first) {
                    hal_ble_link_evt_t evt = {};
                    hal_ble_conn_info_t info = {};
                    info.version = BLE_API_VERSION;
                    info.size = sizeof(hal_ble_conn_info_t);
                    info.role = BLE_ROLE_PERIPHERAL;
                    info.conn_handle = connHandle;
                    info.conn_params = connParams_;
                    info.address = peerAddr;
                    info.att_mtu = BLE_DEFAULT_ATT_MTU_SIZE; // Use the default ATT_MTU on connected.
                    evt.type = BLE_EVT_CONNECTED;
                    evt.conn_handle = info.conn_handle;
                    evt.params.connected.info = &info;
                    callback.first(&evt, callback.second);
                }
            }
            break;
        }
        default: break;
    }
}

void BleGap::handleMtuUpdated(uint8_t connHandle, uint16_t mtuSize) {
    LOG_DEBUG(TRACE, "handleMtuUpdated: handle:%d, mtu_size:%d", connHandle, mtuSize);
    for (const auto& callback : periphEvtCallbacks_) {
        if (callback.first) {
            hal_ble_link_evt_t evt = {};
            evt.type = BLE_EVT_ATT_MTU_UPDATED;
            evt.conn_handle = connHandle;
            evt.params.att_mtu_updated.att_mtu_size = mtuSize;
            callback.first(&evt, callback.second);
        }
    }
}

void BleGap::handleConnParamsUpdated(uint8_t connHandle, uint8_t status, uint16_t cause) {
    switch (status) {
        case GAP_CONN_PARAM_UPDATE_STATUS_SUCCESS: {
            le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &connParams_.min_conn_interval, connHandle);
            le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &connParams_.max_conn_interval, connHandle);
            le_get_conn_param(GAP_PARAM_CONN_LATENCY, &connParams_.slave_latency, connHandle);
            le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &connParams_.conn_sup_timeout, connHandle);
            LOG_DEBUG(TRACE, "handleConnParamsUpdated: update success: interval:0x%x, latency:0x%x, timeout:0x%x",
                            connParams_.max_conn_interval, connParams_.slave_latency, connParams_.conn_sup_timeout);
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

void BleGap::handleAuthenStateChanged(uint8_t connHandle, uint8_t state, uint16_t cause) {
    LOG_DEBUG(TRACE, "handleAuthenStateChanged: handle: %d, cause: 0x%x", connHandle, cause);
    switch (state) {
        case GAP_AUTHEN_STATE_STARTED: {
            LOG_DEBUG(TRACE, "GAP_AUTHEN_STATE_STARTED");
            break;
        }
        case GAP_AUTHEN_STATE_COMPLETE: {
            if (cause == GAP_SUCCESS) {
                LOG_DEBUG(TRACE, "GAP_AUTHEN_STATE_COMPLETE pair success");
            } else {
                LOG_DEBUG(TRACE, "GAP_AUTHEN_STATE_COMPLETE pair failed");
            }
            break;
        }
        default: {
            LOG_DEBUG(TRACE, "unknown newstate %d", state);
            break;
        }
    }
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

        default: {
            LOG_DEBUG(TRACE, "gapEventCallback: unhandled type 0x%x", type);
            break;
        }
    }
    return result;
}

int BleGatt::init() {
    server_init(MAX_ALLOWED_BLE_SERVICES);
    server_register_app_cb(gattServerEventCallback);
    serviceRegistered_ = false;
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
                LOG_DEBUG(TRACE, "PROFILE_EVT_SEND_DATA_COMPLETE: conn_id %d, cause: 0x%x, serviceId: %d, index: 0x%x, credits: %d",
                            pParam->event_data.send_data_result.conn_id,
                            pParam->event_data.send_data_result.cause,
                            pParam->event_data.send_data_result.service_id,
                            pParam->event_data.send_data_result.attrib_idx,
                            pParam->event_data.send_data_result.credits);
                if (pParam->event_data.send_data_result.cause == GAP_SUCCESS) {
                    LOG_DEBUG(TRACE, "PROFILE_EVT_SEND_DATA_COMPLETE success");
                } else {
                    LOG_DEBUG(TRACE, "PROFILE_EVT_SEND_DATA_COMPLETE failed");
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

    CHECK_TRUE(services_.append(service), SYSTEM_ERROR_NO_MEMORY);
    *svcHandle = service.startHandle;
    return ret= SYSTEM_ERROR_NONE;
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

    BleService* service = nullptr;
    for (auto& svc : services_) {
        if (svc.startHandle == charInit->service_handle) {
            for (const auto& charact : svc.characteristics) {
                if (!memcmp(svc.attrTable[charact.index].type_value, charUuid, sizeof(charUuid))) {
                    LOG_DEBUG(TRACE, "Characteristic is already exist.");
                    return SYSTEM_ERROR_ALREADY_EXISTS;
                }
            }
            service = &svc;
            break;
        }
    }
    if (!service) {
        LOG_DEBUG(TRACE, "Service not found.");
        return SYSTEM_ERROR_NOT_FOUND;
    }

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
    T_ATTRIB_APPL attribute = {};
    attribute.flags = ATTRIB_FLAG_VALUE_INCL; // type_value -> attribute value
    attribute.type_value[0] = LO_WORD(BLE_SIG_UUID_CHAR_DECL);
    attribute.type_value[1] = HI_WORD(BLE_SIG_UUID_CHAR_DECL);
    attribute.type_value[2] = charInit->properties;
    attribute.value_len = sizeof(uint8_t);
    attribute.p_value_context = nullptr;
    attribute.permissions = GATT_PERM_READ;
    CHECK_TRUE(service->attrTable.append(attribute), SYSTEM_ERROR_NO_MEMORY);
    service->endHandle++;
    charHandles->decl_handle = service->endHandle;

    // Characteristic value attribute
    attribute = {};
    attribute.flags = ATTRIB_FLAG_VALUE_APPL; // gattReadAttrCallback -> attribute value
    if (charInit->uuid.type == BLE_UUID_TYPE_128BIT) {
        attribute.flags |= ATTRIB_FLAG_UUID_128BIT;
    }
    memcpy(attribute.type_value, charUuid, BLE_SIG_UUID_128BIT_LEN);
    value = (uint8_t*)malloc(BLE_MAX_ATTR_VALUE_PACKET_SIZE);
    CHECK_TRUE(value, SYSTEM_ERROR_NO_MEMORY);
    attribute.value_len = 0; // variable length
    attribute.p_value_context = value; // gattReadAttrCallback will use the content pointed by this pointer to provide value for BT stack.
    attribute.permissions = GATT_PERM_READ | GATT_PERM_WRITE;
    CHECK_TRUE(service->attrTable.append(attribute), SYSTEM_ERROR_NO_MEMORY);
    service->endHandle++;
    charHandles->value_handle = service->endHandle;
    BleCharacteristic characteristic = {};
    characteristic.handle = charHandles->value_handle;
    characteristic.index = service->attrTable.size() - 1;
    characteristic.callback = charInit->callback;
    characteristic.context = charInit->context;
    CHECK_TRUE(service->characteristics.append(characteristic), SYSTEM_ERROR_NO_MEMORY);

    // Characteristic CCCD descriptor attribute
    if ((charInit->properties & BLE_SIG_CHAR_PROP_NOTIFY) || (charInit->properties & BLE_SIG_CHAR_PROP_INDICATE)) {
        attribute = {};
        attribute.flags = ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_CCCD_APPL; // type_value -> attribute value
        attribute.type_value[0] = LO_WORD(BLE_SIG_UUID_CLIENT_CHAR_CONFIG_DESC);
        attribute.type_value[1] = HI_WORD(BLE_SIG_UUID_CLIENT_CHAR_CONFIG_DESC);
        attribute.type_value[2] = LO_WORD(BLE_SIG_CCCD_VAL_DISABLED);
        attribute.type_value[3] = HI_WORD(BLE_SIG_CCCD_VAL_DISABLED);
        attribute.value_len = sizeof(ble_sig_cccd_value_t);
        attribute.p_value_context = nullptr;
        attribute.permissions = GATT_PERM_READ | GATT_PERM_WRITE;
        CHECK_TRUE(service->attrTable.append(attribute), SYSTEM_ERROR_NO_MEMORY);
        service->endHandle++;
        charHandles->cccd_handle = service->endHandle;
        CccdConfig config = {};
        config.index = characteristic.index;
        config.subscriber.connHandle = BLE_INVALID_CONN_HANDLE;
        config.subscriber.config = BLE_SIG_CCCD_VAL_DISABLED;
        CHECK_TRUE(service->cccdConfigs.append(config), SYSTEM_ERROR_NO_MEMORY);
    } else {
        charHandles->cccd_handle = BLE_INVALID_ATTR_HANDLE;
    }
    charHandles->sccd_handle = BLE_INVALID_ATTR_HANDLE; // FIXME: not supported for now
    
    // User description descriptor attribute
    if (charInit->description) {
        attribute = {};
        attribute.flags = ATTRIB_FLAG_VOID | ATTRIB_FLAG_ASCII_Z; // p_value_context -> attribute value
        attribute.type_value[0] = LO_WORD(BLE_SIG_UUID_CHAR_USER_DESCRIPTION_DESC);
        attribute.type_value[1] = HI_WORD(BLE_SIG_UUID_CHAR_USER_DESCRIPTION_DESC);
        attribute.value_len = std::min((size_t)BLE_MAX_DESC_LEN, strlen(charInit->description));
        attribute.p_value_context = (void*)charInit->description;
        attribute.permissions = GATT_PERM_READ;
        CHECK_TRUE(service->attrTable.append(attribute), SYSTEM_ERROR_NO_MEMORY);
        service->endHandle++;
        charHandles->user_desc_handle = service->endHandle;
    } else {
        charHandles->user_desc_handle = BLE_INVALID_ATTR_HANDLE;
    }

    return ret = SYSTEM_ERROR_NONE;
}

int BleGatt::registerAttributeTable() {
    if (!serviceRegistered_) {
        serviceRegistered_ = true;
        for (auto& svc : services_) {
            if (!server_add_service_by_start_handle(&svc.id, (uint8_t *)svc.attrTable.data(), svc.attrTable.size() * sizeof(T_ATTRIB_APPL), callbacks_, svc.startHandle)) {
                LOG_DEBUG(TRACE, "Failed to add service.");
                svc.id = 0xFF;
                return SYSTEM_ERROR_INTERNAL;
            }
        }
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
                // FIXME: this seems to help avoid getting errors from server_send_data
                LOG(INFO, "notify %x %x %x %u %x", config.subscriber.connHandle, svc.id, config.index, attribute.value_len, type);
                CHECK_TRUE(server_send_data(config.subscriber.connHandle, svc.id, config.index, (uint8_t*)attribute.p_value_context, attribute.value_len, type), SYSTEM_ERROR_INTERNAL);
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
    return SYSTEM_ERROR_NONE;
}

int hal_ble_exit_locked_mode(void* reserved) {
    return SYSTEM_ERROR_NONE;
}

int hal_ble_stack_init(void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_stack_init().");

    if (!s_bleStackInit) {
        RCC_PeriphClockCmd(APBPeriph_UART1, APBPeriph_UART1_CLOCK, ENABLE);

        const auto mgr = wifiNetworkManager();
        CHECK_TRUE(mgr, SYSTEM_ERROR_INTERNAL);
        const auto client = mgr->ncpClient();
        CHECK_TRUE(client, SYSTEM_ERROR_INTERNAL);
        if (client->ncpPowerState() != NcpPowerState::ON) {
            CHECK(client->on());
        }

        CHECK(BleEventDispatcher::getInstance().init());
        CHECK(BleGap::getInstance().init());
        CHECK(BleGatt::getInstance().init());
        s_bleStackInit = true;
    }
    return SYSTEM_ERROR_NONE;
}

int hal_ble_stack_deinit(void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_stack_deinit().");
    CHECK_TRUE(s_bleStackInit, SYSTEM_ERROR_INVALID_STATE);
    CHECK(BleGap::getInstance().stopAdvertising(true));
    CHECK(BleGap::getInstance().disconnect());
    bte_deinit();
    s_bleStackInit = false;
    return SYSTEM_ERROR_NONE;
}

int hal_ble_select_antenna(hal_ble_ant_type_t antenna, void* reserved) {
    CHECK(selectRadioAntenna((radio_antenna_type)antenna));
    return SYSTEM_ERROR_NONE;
}

int hal_ble_set_callback_on_adv_events(hal_ble_on_adv_evt_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_set_callback_on_adv_events().");
    return SYSTEM_ERROR_NONE;
}

int hal_ble_cancel_callback_on_adv_events(hal_ble_on_adv_evt_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_cancel_callback_on_adv_events().");
    return SYSTEM_ERROR_NONE;
}

int hal_ble_set_callback_on_periph_link_events(hal_ble_on_link_evt_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_set_callback_on_periph_link_events().");
    CHECK_TRUE(s_bleStackInit, SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().onPeripheralLinkEventCallback(callback, context);
}

/**********************************************
 * BLE GAP APIs
 */
int hal_ble_gap_set_device_address(const hal_ble_addr_t* address, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_device_address().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_get_device_address(hal_ble_addr_t* address, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_device_address().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_set_device_name(const char* device_name, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_device_name().");
    CHECK_TRUE(s_bleStackInit, SYSTEM_ERROR_INVALID_STATE);
    CHECK(BleGap::getInstance().setDeviceName(device_name, len));
    return SYSTEM_ERROR_NONE;
}

int hal_ble_gap_get_device_name(char* device_name, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_device_name().");
    CHECK_TRUE(s_bleStackInit, SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().getDeviceName(device_name, len);
}

int hal_ble_gap_set_appearance(ble_sig_appearance_t appearance, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_appearance().");
    CHECK_TRUE(s_bleStackInit, SYSTEM_ERROR_INVALID_STATE);
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
    return SYSTEM_ERROR_NONE;
}

int hal_ble_gap_get_ppcp(hal_ble_conn_params_t* ppcp, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_ppcp().");
    return SYSTEM_ERROR_NONE;
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
    return SYSTEM_ERROR_NONE;
}

int hal_ble_gap_get_tx_power(int8_t* tx_power, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_tx_power().");
    return 0;
}

int hal_ble_gap_set_advertising_parameters(const hal_ble_adv_params_t* adv_params, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_advertising_parameters().");
    CHECK_TRUE(s_bleStackInit, SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().setAdvertisingParameters(adv_params);
}

int hal_ble_gap_get_advertising_parameters(hal_ble_adv_params_t* adv_params, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_advertising_parameters().");
    CHECK_TRUE(s_bleStackInit, SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().getAdvertisingParameters(adv_params);
}

int hal_ble_gap_set_advertising_data(const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_advertising_data().");
    CHECK_TRUE(s_bleStackInit, SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().setAdvertisingData(buf, len);
}

ssize_t hal_ble_gap_get_advertising_data(uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_advertising_data().");
    CHECK_TRUE(s_bleStackInit, SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().getAdvertisingData(buf, len);
}

int hal_ble_gap_set_scan_response_data(const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_scan_response_data().");
    CHECK_TRUE(s_bleStackInit, SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().setScanResponseData(buf, len);
}

ssize_t hal_ble_gap_get_scan_response_data(uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_scan_response_data().");
    CHECK_TRUE(s_bleStackInit, SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().getScanResponseData(buf, len);
}

int hal_ble_gap_start_advertising(void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_start_advertising().");
    CHECK_TRUE(s_bleStackInit, SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().startAdvertising(true);
}

int hal_ble_gap_set_auto_advertise(hal_ble_auto_adv_cfg_t config, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_auto_advertise().");
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
    CHECK_TRUE(s_bleStackInit, SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().stopAdvertising(true);
}

bool hal_ble_gap_is_advertising(void* reserved) {
    BleLock lk;
    CHECK_TRUE(s_bleStackInit, false);
    return BleGap::getInstance().isAdvertising();
}

int hal_ble_gap_set_scan_parameters(const hal_ble_scan_params_t* scan_params, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_scan_parameters().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_get_scan_parameters(hal_ble_scan_params_t* scan_params, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_scan_parameters().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_start_scan(hal_ble_on_scan_result_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_start_scan().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

bool hal_ble_gap_is_scanning(void* reserved) {
    BleLock lk;
    return false;
}

int hal_ble_gap_stop_scan(void* reserved) {
    // Do not acquire the lock here, otherwise another thread cannot cancel the scanning.
    LOG_DEBUG(TRACE, "hal_ble_gap_stop_scan().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_connect(const hal_ble_conn_cfg_t* config, hal_ble_conn_handle_t* conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_connect().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

bool hal_ble_gap_is_connecting(const hal_ble_addr_t* address, void* reserved) {
    BleLock lk;
    CHECK_TRUE(s_bleStackInit, false);
    return BleGap::getInstance().connecting();
}

bool hal_ble_gap_is_connected(const hal_ble_addr_t* address, void* reserved) {
    CHECK_TRUE(s_bleStackInit, false);
    return BleGap::getInstance().connected();
}

int hal_ble_gap_connect_cancel(const hal_ble_addr_t* address, void* reserved) {
    LOG_DEBUG(TRACE, "hal_ble_gap_connect_cancel().");
    CHECK_TRUE(s_bleStackInit, SYSTEM_ERROR_INVALID_STATE);
    // Do not acquire the lock here, otherwise another thread cannot cancel the connection attempt.
    return BleGap::getInstance().disconnect();
}

int hal_ble_gap_disconnect(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_disconnect().");
    CHECK_TRUE(s_bleStackInit, SYSTEM_ERROR_INVALID_STATE);
    return BleGap::getInstance().disconnect();
}

int hal_ble_gap_update_connection_params(hal_ble_conn_handle_t conn_handle, const hal_ble_conn_params_t* conn_params, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_update_connection_params().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_get_connection_info(hal_ble_conn_handle_t conn_handle, hal_ble_conn_info_t* info, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_connection_info().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_get_rssi(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_rssi().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_set_pairing_config(const hal_ble_pairing_config_t* config, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_pairing_config().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_get_pairing_config(hal_ble_pairing_config_t* config, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_pairing_config().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_start_pairing(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_start_pairing().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_reject_pairing(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_reject_pairing().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_set_pairing_auth_data(hal_ble_conn_handle_t conn_handle, const hal_ble_pairing_auth_data_t* auth, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_pairing_auth_data().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_set_pairing_passkey_deprecated(hal_ble_conn_handle_t conn_handle, const uint8_t* passkey, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_pairing_passkey_deprecated().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

bool hal_ble_gap_is_pairing(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_is_pairing().");
    return false;
}

bool hal_ble_gap_is_paired(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_is_paired().");
    return false;
}

/**********************************************
 * BLE GATT Server APIs
 */
int hal_ble_gatt_server_add_service(uint8_t type, const hal_ble_uuid_t* uuid, hal_ble_attr_handle_t* handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_add_service().");
    CHECK_TRUE(s_bleStackInit, SYSTEM_ERROR_INVALID_STATE);
    return BleGatt::getInstance().addService(type, uuid, handle);;
}

int hal_ble_gatt_server_add_characteristic(const hal_ble_char_init_t* char_init, hal_ble_char_handles_t* char_handles, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_add_characteristic().");
    CHECK_TRUE(s_bleStackInit, SYSTEM_ERROR_INVALID_STATE);
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
    CHECK_TRUE(s_bleStackInit, SYSTEM_ERROR_INVALID_STATE);
    return BleGatt::getInstance().setValue(value_handle, buf, len);
}

ssize_t hal_ble_gatt_server_notify_characteristic_value(hal_ble_attr_handle_t value_handle, const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_notify_characteristic_value().");
    CHECK_TRUE(s_bleStackInit, SYSTEM_ERROR_INVALID_STATE);
    return BleGatt::getInstance().notifyValue(value_handle, buf, len, false);
}

ssize_t hal_ble_gatt_server_indicate_characteristic_value(hal_ble_attr_handle_t value_handle, const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_indicate_characteristic_value().");
    CHECK_TRUE(s_bleStackInit, SYSTEM_ERROR_INVALID_STATE);
    return BleGatt::getInstance().notifyValue(value_handle, buf, len, true);
}

ssize_t hal_ble_gatt_server_get_characteristic_value(hal_ble_attr_handle_t value_handle, uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_get_characteristic_value().");
    CHECK_TRUE(s_bleStackInit, SYSTEM_ERROR_INVALID_STATE);
    return BleGatt::getInstance().getValue(value_handle, buf, len);
}

/**********************************************
 * BLE GATT Client APIs
 */
int hal_ble_gatt_client_discover_all_services(hal_ble_conn_handle_t conn_handle, hal_ble_on_disc_service_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_discover_all_services().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gatt_client_discover_service_by_uuid(hal_ble_conn_handle_t conn_handle, const hal_ble_uuid_t* uuid, hal_ble_on_disc_service_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_discover_service_by_uuid().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gatt_client_discover_characteristics(hal_ble_conn_handle_t conn_handle, const hal_ble_svc_t* service, hal_ble_on_disc_char_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_discover_characteristics().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gatt_client_discover_characteristics_by_uuid(hal_ble_conn_handle_t conn_handle, const hal_ble_svc_t* service, const hal_ble_uuid_t* uuid, hal_ble_on_disc_char_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

bool hal_ble_gatt_client_is_discovering(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    return false;
}

int hal_ble_gatt_set_att_mtu(size_t att_mtu, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "ble_gatt_client_set_att_mtu().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gatt_client_configure_cccd(const hal_ble_cccd_config_t* config, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_configure_cccd().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

ssize_t hal_ble_gatt_client_write_with_response(hal_ble_conn_handle_t conn_handle, hal_ble_attr_handle_t value_handle, const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_write_with_response().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

ssize_t hal_ble_gatt_client_write_without_response(hal_ble_conn_handle_t conn_handle, hal_ble_attr_handle_t value_handle, const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_write_without_response().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

ssize_t hal_ble_gatt_client_read(hal_ble_conn_handle_t conn_handle, hal_ble_attr_handle_t value_handle, uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_read().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
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
