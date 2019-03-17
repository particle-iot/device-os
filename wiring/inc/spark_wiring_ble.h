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

#define Wiring_BLE 1
#if Wiring_BLE

#include "system_error.h"
#include "spark_wiring_string.h"
#include "spark_wiring_vector.h"
#include "spark_wiring_flags.h"
#include "ble_hal.h"
#include "check.h"

using spark::Vector;
using particle::Flags;


class BleAttribute;
class BleDevice;
class BleClass;

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
typedef Flags<BleAttrPropType, uint8_t> BleAttrProps;
typedef BleAttrProps::FlagType BleAttrProp;

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
    BleUuid(const uint8_t* uuid128, BleUuidOrder order = BleUuidOrder::LSB);
    BleUuid(uint16_t uuid16, BleUuidOrder order = BleUuidOrder::LSB);
    BleUuid(const uint8_t* uuid128, uint16_t uuid16, BleUuidOrder order = BleUuidOrder::LSB);
    BleUuid(const String& str);
    BleUuid(const char* string);
    ~BleUuid();

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
    iBeacon() : major_(0),minor_(0),uuid_(nullptr),measurePower_(0) {

    }
    iBeacon(uint16_t major, uint16_t minor, uint8_t* uuid, int8_t mp) : major_(major),minor_(minor),uuid_(uuid),measurePower_(mp) {

    }
    ~iBeacon() {

    }

    uint16_t major(void) const {
        return major_;
    }

    uint16_t minor(void) const {
        return minor_;
    }

    uint8_t* uuid(void) const {
        return uuid_;
    }

    int8_t measurePower(void) {
        return measurePower_;
    }

private:
    uint16_t major_;
    uint16_t minor_;
    uint8_t* uuid_;
    int8_t   measurePower_;
};


class BleAdvData {
public:
    BleAdvData() : len(0) {

    }
    ~BleAdvData() {

    }

    uint8_t data[BLE_MAX_ADV_DATA_LEN];
    size_t len;

    bool contain(uint8_t type);
    bool contain(uint8_t type, const uint8_t* buf, size_t len);

    size_t fetch(uint8_t type, uint8_t* buf, size_t len);

    size_t locate(uint8_t type, size_t* offset);
};


class BleBroadcaster {
public:
    BleBroadcaster();
    ~BleBroadcaster();

    int appendAdvData(uint8_t type, const uint8_t* data, size_t len);
    int appendAdvDataLocalName(const char* name);
    int appendAdvDataCustomData(const uint8_t* buf, size_t len);
    int appendAdvDataUuid(const BleUuid& uuid);

    int appendScanRspData(uint8_t type, const uint8_t* buf, size_t len);
    int appendScanRspDataLocalName(const char* name);
    int appendScanRspDataCustomData(const uint8_t* buf, size_t len);
    int appendScanRspDataUuid(const BleUuid& uuid);

    int advDataBeacon(const iBeacon& beacon);

    int clearAdvData(void);
    int removeFromAdvData(uint8_t type);

    int clearScanRspData(void);
    int removeFromScanRspData(uint8_t type);

    int setTxPower(int8_t val) const;
    int8_t txPower(void) const;

    int advertise(void);
    int advertise(uint32_t interval);
    int advertise(uint32_t interval, uint32_t timeout);
    int advertise(const BleAdvParams& params);

    int stopAdvertise(void) const;

protected:
    void bleBroadcasterCallback(hal_ble_gap_on_adv_stopped_evt_t* event);

private:
    BleAdvParams advParams_;
    BleAdvData advData_;
    BleAdvData srData_;

    int append(uint8_t type, const uint8_t* buf, size_t len, BleAdvData& data);
};


class BleScannedDevice {
public:
    BleAddress address;
    BleAdvData advData;
    BleAdvData srData;
    int8_t rssi;
};


class BleObserver {
public:
    BleObserver();
    ~BleObserver();

    static const size_t DEFAULT_COUNT = 5;

    typedef void (*BleScanCallback)(const BleScannedDevice *device);

    int scan(BleScanCallback callback);
    int scan(BleScannedDevice* results, size_t resultCount);
    int scan(BleScannedDevice* results, size_t resultCount, uint16_t timeout);
    int scan(BleScannedDevice* results, size_t resultCount, const BleScanParams& params);

    int stopScan(void);

protected:
    void bleObserverScanResultCallback(const hal_ble_gap_on_scan_result_evt_t* event);
    void bleObserverScanStoppedCallback(const hal_ble_gap_on_scan_stopped_evt_t* event);

private:
    BleScanParams scanParams_;
    size_t targetCount_;
    BleScanCallback callback_;
    BleScannedDevice* results_;
    size_t count_;
};


/* BLE device class */
class BleDevice {
    friend BleClass;
    friend BleAttribute;

public:
    BleDevice();
    BleDevice(int n);
    ~BleDevice();

    const BleConnParams& params(void) const { return connParams_; }

    const BleAddress& address(void) const { return address_; }

    BleDevRoles role(void) const { return role_; }

    size_t attrCount(void) const { return attributes_.size(); }

    Vector<BleAttribute> getAttrList(const char* desc);
    Vector<BleAttribute> getAttrList(const BleUuid& svcUuid);

    BleAttribute* getAttr(uint16_t attrHandle);
    BleAttribute* getAttr(const BleUuid& charUuid);
    BleAttribute* getAttr(size_t i);

    bool operator == (const BleDevice& dev) const;

private:
    BleDevRoles role_;
    BleConnHandle connHandle_;
    BleConnParams connParams_;
    BleAddress address_;
    int rssi_;
    Vector<BleAttribute> attributes_;

    int preAddAttr(const BleAttribute& attr);
    void executeAddAttr(void);
};


/* BLE attribute class */
class BleAttribute {
    friend BleClass;
    friend BleDevice;

public:
    typedef void (*onDataReceivedCb)(const uint8_t* data, size_t len);

    BleAttribute();
    BleAttribute(const char* desc, BleAttrProps properties, onDataReceivedCb cb = nullptr);
    BleAttribute(const char* desc, BleAttrProps properties, BleUuid& attrUuid, BleUuid& svcUuid, onDataReceivedCb cb = nullptr);
    ~BleAttribute();

    const char* description(void) const { return description_; }

    BleAttrProps properties(void) const { return properties_; }

    const BleUuid& serviceUuid(void) const { return svcUuid_; }

    const BleUuid& charUuid(void) const { return charUuid_; }

    size_t getValue(uint8_t* buf, size_t len) const;
    size_t getValue(String& str) const;
    template<typename T> size_t getValue(T* val) const {
        size_t len = sizeof(T);
        return getValue(reinterpret_cast<uint8_t*>(val), len);
    }

    int setValue(const uint8_t* buf, size_t len);
    int setValue(const String& str);
    int setValue(const char* str);

    template<typename T> int setValue(T val) {
        uint8_t buf[BLE_MAX_CHAR_VALUE_LEN];
        size_t len = sizeof(T) > BLE_MAX_CHAR_VALUE_LEN ? BLE_MAX_CHAR_VALUE_LEN : sizeof(T);

        for (size_t i = 0, j = len - 1; i < len; i++, j--) {
            buf[i] = reinterpret_cast<const uint8_t*>(&val)[j];
        }

        return setValue(buf, len);
    }

    void onDataReceived(onDataReceivedCb callback);

private:
    BleAttrProps properties_;
    BleCharHandles handles_;
    uint16_t svcHandle_;
    BleUuid charUuid_;
    BleUuid svcUuid_;
    const char* description_;
    onDataReceivedCb dataCb_;

    bool valid_;
    bool cccdConfigured_;
    static uint16_t defaultAttrCount_;
    BleDevice* owner_;
    BleAttribute* extRef_;

    void init(void);
    bool contain(uint16_t handle);
    bool isLocalAttr(void) const;
    void syncExtRef(void) const;
    BleUuid generateDefaultAttrUuid(void) const;
    BleUuid assignDefaultSvcUuid(void) const;
    void processReceivedData(uint16_t handle, const uint8_t* data, size_t len);
};


/* BLE class */
class BleClass : public BleBroadcaster,
                 public BleObserver {
public:
    typedef void (*onConnectedCb)(BleDevice &peer);
    typedef void (*onDisconnectedCb)(BleDevice &peer);

    int on(void);
    void off(void);

    void onConnectionChangedCb(onConnectedCb connCb, onDisconnectedCb disconnCb);

    BleDevice* connect(const BleAddress& addr);

    int disconnect(void);
    int disconnect(const BleDevice& peer);

    bool connected(void) const { return (peerCount() > 0); }

    size_t peerCount(void) const { return peers_.size(); }

    const BleDevice* peer(size_t i) const;

    BleDevice& local(void);

    static BleClass* getInstance(void);

private:
    Vector<BleDevice> peers_;
    onConnectedCb connectedCb_;
    onDisconnectedCb disconnectedCb_;

    BleClass() : connectedCb_(nullptr), disconnectedCb_(nullptr) { }
    ~BleClass() { }

    const BleDevice* findPeer(BleConnHandle handle) const;
    int addPeer(const BleDevice& device);
    int removePeer(BleConnHandle handle);
    void updateLocal(void);
    static void onBleEvents(hal_ble_evts_t *event, void* context);
};


extern BleClass& _fetch_ble();
#define BLE _fetch_ble()


#endif /* Wiring_BLE */

#endif /* SPARK_WIRING_BLE_H */
