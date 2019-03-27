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

#ifndef SPARK_WIRING_BLE_H
#define SPARK_WIRING_BLE_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "spark_wiring_platform.h"

#if Wiring_BLE

#include "system_error.h"
#include "spark_wiring_string.h"
#include "spark_wiring_vector.h"
#include "spark_wiring_flags.h"
#include "ble_hal.h"
#include "check.h"
#include <memory>

using spark::Vector;
using particle::Flags;

class BleScannedDevice;
class BlePeerDevice;

// Forward declaration
class BleCharacteristicImpl;
class BleServiceImpl;
class BleGattServerImpl;
class BleGattClientImpl;
class BleBroadcasterImpl;
class BleObserverImpl;
class BlePeripheralImpl;
class BleCentralImpl;

enum class BleUuidType {
    SHORT = 0,
    LONG = 1
};

enum class BleUuidOrder {
    MSB = 0,
    LSB = 1
};

struct BleDevRoleType; // Tag type for BLE device role flags
typedef Flags<BleDevRoleType, uint8_t> BleDevRoles;
typedef BleDevRoles::FlagType BleDevRole;
namespace ROLE {
    const BleDevRole INVALID(BLE_ROLE_INVALID);
    const BleDevRole PERIPHERAL(BLE_ROLE_PERIPHERAL);
    const BleDevRole CENTRAL(BLE_ROLE_CENTRAL);
    const BleDevRole ALL(0xff);
}

struct BleAttrPropType; // Tag type for BLE attribute property flags
typedef Flags<BleAttrPropType, uint8_t> BleCharProps;
typedef BleCharProps::FlagType BleAttrProp;
namespace PROPERTY {
    const BleAttrProp NONE(0);
    const BleAttrProp READ(BLE_SIG_CHAR_PROP_READ);
    const BleAttrProp WRITE(BLE_SIG_CHAR_PROP_WRITE);
    const BleAttrProp WRITE_WO_RSP(BLE_SIG_CHAR_PROP_WRITE_WO_RESP);
    const BleAttrProp NOTIFY(BLE_SIG_CHAR_PROP_NOTIFY);
    const BleAttrProp INDICATE(BLE_SIG_CHAR_PROP_INDICATE);
}

namespace PARTICLE_BLE {
    const uint8_t BLE_USER_DEFAULT_SVC_UUID[BLE_SIG_UUID_128BIT_LEN] = {
        0x2d, 0x49, 0xc0, 0x26, 0xf0, 0xdb, 0xce, 0x06, 0x7a, 0x33, 0x6f, 0x12, 0x00, 0x00, 0xf3, 0x5d
    };
}

typedef uint16_t BleConnHandle;
typedef uint16_t BleAttrHandle;

typedef void (*onDataReceivedCb)(const uint8_t* data, size_t len);
typedef void (*BleScanCallback)(const BleScannedDevice *device);
typedef void (*onConnectedCb)(BlePeerDevice &peer);
typedef void (*onDisconnectedCb)(BlePeerDevice &peer);

class BleAddress : public hal_ble_addr_t {
public:
    void operator = (hal_ble_addr_t addr) {
        this->addr_type = addr.addr_type;
        memcpy(this->addr, addr.addr, BLE_SIG_ADDR_LEN);
    }

    bool operator == (const BleAddress& addr) const {
        if (this->addr_type == addr.addr_type && !memcmp(this->addr, addr.addr, BLE_SIG_ADDR_LEN)) {
            return true;
        }
        return false;
    }
};

class BleAdvParams : public hal_ble_adv_params_t {
};

class BleConnParams : public hal_ble_conn_params_t {
};

class BleScanParams : public hal_ble_scan_params_t {
};

class BleCharHandles : public hal_ble_char_handles_t {
};


class BleUuid {
public:
    BleUuid();
    BleUuid(const BleUuid& uuid);
    BleUuid(const uint8_t* uuid128, BleUuidOrder order = BleUuidOrder::LSB);
    BleUuid(uint16_t uuid16, BleUuidOrder order = BleUuidOrder::LSB);
    BleUuid(const uint8_t* uuid128, uint16_t uuid16, BleUuidOrder order = BleUuidOrder::LSB);
    BleUuid(const String& str);
    BleUuid(const char* string);
    ~BleUuid();

    bool isValid(void) const;

    BleUuidType type(void) const {
        return type_;
    }

    BleUuidOrder order(void) const {
        return order_;
    }

    uint16_t shortUuid(void) const {
        return shortUuid_;
    }

    void fullUuid(uint8_t uuid128[BLE_SIG_UUID_128BIT_LEN]) const {
        memcpy(uuid128, fullUuid_, BLE_SIG_UUID_128BIT_LEN);
    }

    const uint8_t* fullUuid(void) const {
        return fullUuid_;
    }

    bool operator == (const BleUuid& uuid) const;

private:
    BleUuidType type_;
    BleUuidOrder order_;
    uint16_t shortUuid_;
    uint8_t fullUuid_[BLE_SIG_UUID_128BIT_LEN];

    int8_t toInt(char c);
    void setUuid(const String& str);
};


class iBeacon {
public:
    uint16_t major;
    uint16_t minor;
    BleUuid uuid;
    int8_t measurePower;

    static const uint16_t APPLE_COMPANY_ID = 0x004C;
    static const uint8_t BEACON_TYPE_IBEACON = 0x02;

    iBeacon() : major(0), minor(0), measurePower(0) {
    }

    template<typename T>
    iBeacon(uint16_t major, uint16_t minor, T uuid, int8_t measurePower)
        : major(major), minor(minor), uuid(uuid), measurePower(measurePower) {
    }

    ~iBeacon() {
    }
};


class BleAdvData {
public:
    static const size_t MAX_LEN = BLE_MAX_ADV_DATA_LEN;

    BleAdvData();
    ~BleAdvData();

    size_t set(const uint8_t* buf, size_t len);

    size_t append(uint8_t type, const uint8_t* buf, size_t len, bool force = false);
    size_t appendLocalName(const char* name, bool force = false);
    size_t appendCustomData(const uint8_t* buf, size_t len, bool force = false);
    size_t appendServiceUuid(const BleUuid& uuid, bool force = false);

    template<typename T>
    size_t appendServiceUuid(T uuid, bool force = false) {
        BleUuid tempUuid(uuid);
        return appendServiceUuid(tempUuid, force);
    }

    void clear(void);
    void remove(uint8_t type);

    size_t get(uint8_t* buf, size_t len) const;
    size_t get(uint8_t type, uint8_t* buf, size_t len) const;

    String deviceName(void) const;
    size_t deviceName(uint8_t* buf, size_t len) const;
    size_t serviceUuid(BleUuid* uuids, size_t count) const;
    size_t customData(uint8_t* buf, size_t len) const;

    bool contains (uint8_t type) const;

private:
    uint8_t selfData[BLE_MAX_ADV_DATA_LEN];
    size_t selfLen;

    size_t serviceUuid(uint8_t type, BleUuid* uuids, size_t count) const;
    static size_t locate(const uint8_t* buf, size_t len, uint8_t type, size_t* offset);
};


class BleCharacteristic {
public:
    BleCharacteristic();
    BleCharacteristic(const BleCharacteristic& characteristic);
    BleCharacteristic(const char* desc, BleCharProps properties, onDataReceivedCb cb = nullptr);
    BleCharacteristic(const char* desc, BleCharProps properties, BleUuid& charUuid, BleUuid& svcUuid, onDataReceivedCb cb = nullptr);
    ~BleCharacteristic();

    BleCharacteristic& operator=(const BleCharacteristic&);

    bool valid(void) {
        return impl() != nullptr;
    }

    BleUuid uuid(void) const;
    BleCharProps properties(void) const;

    size_t getValue(uint8_t* buf, size_t len) const;
    size_t getValue(String& str) const;

    template<typename T>
    size_t getValue(T* val) const {
        size_t len = sizeof(T);
        return getValue(reinterpret_cast<uint8_t*>(val), len);
    }

    size_t setValue(const uint8_t* buf, size_t len);
    size_t setValue(const String& str);
    size_t setValue(const char* str);

    template<typename T>
    size_t setValue(T val) {
        uint8_t buf[BLE_MAX_CHAR_VALUE_LEN];
        size_t len = sizeof(T) > BLE_MAX_CHAR_VALUE_LEN ? BLE_MAX_CHAR_VALUE_LEN : sizeof(T);
        for (size_t i = 0, j = len - 1; i < len; i++, j--) {
            buf[i] = reinterpret_cast<const uint8_t*>(&val)[j];
        }
        return setValue(buf, len);
    }

    void onDataReceived(onDataReceivedCb callback);

    BleCharacteristicImpl* impl(void) const {
        return impl_.get();
    }

private:
    std::shared_ptr<BleCharacteristicImpl> impl_;
};


class BleService {
public:
    BleService();
    BleService(const BleUuid& uuid);
    ~BleService();

    BleServiceImpl* impl(void) const {
        return impl_.get();
    }

private:
    std::shared_ptr<BleServiceImpl> impl_;
};


class BleScannedDevice {
public:
    BleAddress address;
    BleAdvData advData;
    BleAdvData srData;
    int8_t rssi;
};


class BleGattServerImpl;

class BlePeerDevice {
public:
    BleDevRoles role;
    BleAddress address;
    BleConnParams connParams;
    BleConnHandle connHandle;
    int8_t rssi;

    BlePeerDevice();
    BlePeerDevice(const BleAddress& addr);
    ~BlePeerDevice();

    BleGattServerImpl* gattsProxy(void) {
        return gattsProxy_.get();
    }

    bool operator == (const BlePeerDevice& device);

private:
    std::shared_ptr<BleGattServerImpl> gattsProxy_;
};


class BleLocalDevice {
public:
    BleAddress address;

    int on(void);
    void off(void);

    int setTxPower(int8_t val) const;
    int8_t txPower(void) const;

    int advertise(void);
    int advertise(BleAdvData* advData, BleAdvData* srData);
    int advertise(uint32_t interval);
    int advertise(uint32_t interval, BleAdvData* advData, BleAdvData* srData);
    int advertise(uint32_t interval, uint32_t timeout);
    int advertise(uint32_t interval, uint32_t timeout, BleAdvData* advData, BleAdvData* srData);
    int advertise(const BleAdvParams& params);
    int advertise(const BleAdvParams& params, BleAdvData* advData, BleAdvData* srData);

    int advertise(const iBeacon& iBeacon, bool connectable = false);
    int advertise(uint32_t interval, const iBeacon& iBeacon, bool connectable = false);
    int advertise(uint32_t interval, uint32_t timeout, const iBeacon& iBeacon, bool connectable = false);
    int advertise(const BleAdvParams& params, const iBeacon& iBeacon, bool connectable = false);

    int stopAdvertising(void) const;

    int scan(BleScanCallback callback);
    int scan(BleScanCallback callback, uint16_t timeout);
    int scan(BleScannedDevice* results, size_t resultCount);
    int scan(BleScannedDevice* results, size_t resultCount, uint16_t timeout);
    int scan(BleScannedDevice* results, size_t resultCount, const BleScanParams& params);
    int stopScanning(void);

    int addCharacteristic(BleCharacteristic& characteristic);
    int addCharacteristic(const char* desc, BleCharProps properties, onDataReceivedCb cb);
    int addCharacteristic(const char* desc, BleCharProps properties, BleUuid& charUuid, BleUuid& svcUuid, onDataReceivedCb cb);

    int setPpcp(void);
    int setPpcp(uint16_t minInterval, uint16_t maxInterval, uint16_t latency = BLE_DEFAULT_SLAVE_LATENCY, uint16_t timeout = BLE_DEFAULT_CONN_SUP_TIMEOUT);

    BlePeerDevice* connect(const BleAddress& addr,
            uint16_t interval = BLE_DEFAULT_MIN_CONN_INTERVAL,
            uint16_t latency = BLE_DEFAULT_SLAVE_LATENCY,
            uint16_t timeout = BLE_DEFAULT_CONN_SUP_TIMEOUT);

    int disconnect(void);
    int disconnect(const BlePeerDevice& peripheral);

    bool connected(void) const;
    bool connectedAsCentral(void) const;

    void onConnectionChangedCb(onConnectedCb connCb, onDisconnectedCb disconnCb);

    static BleLocalDevice& getInstance(void);

private:
    onConnectedCb connectedCb_;
    onDisconnectedCb disconnectedCb_;
    std::shared_ptr<BleGattServerImpl> gattsProxy_;
    std::shared_ptr<BleGattClientImpl> gattcProxy_;
    std::shared_ptr<BleBroadcasterImpl> broadcasterProxy_;
    std::shared_ptr<BleObserverImpl> observerProxy_;
    std::shared_ptr<BlePeripheralImpl> peripheralProxy_;
    std::shared_ptr<BleCentralImpl> centralProxy_;

    BleLocalDevice();
    ~BleLocalDevice();
    static void onBleEvents(hal_ble_evts_t *event, void* context);
    BlePeerDevice* findPeerDevice(BleConnHandle connHandle);
};


extern BleLocalDevice& _fetch_ble();
#define BLE _fetch_ble()


#endif /* Wiring_BLE */

#endif /* SPARK_WIRING_BLE_H */
