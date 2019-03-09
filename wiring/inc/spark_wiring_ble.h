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

#define Wiring_BLE  1

#if Wiring_BLE

#include "system_error.h"
#include "spark_wiring_string.h"
#include "spark_wiring_vector.h"
#include "spark_wiring_flags.h"

using spark::Vector;
using particle::Flags;

// Temporary code. Use HAL definitions.
#ifndef BLE_SIG_AD_TYPE_FLAGS
#define BLE_SIG_AD_TYPE_FLAGS 0x02
#endif

#ifndef BLE_SIG_AD_TYPE_LOCAL_NAME
#define BLE_SIG_AD_TYPE_LOCAL_NAME 0x03
#endif

#ifndef BLE_SIG_AD_TYPE_MANU_DATA
#define BLE_SIG_AD_TYPE_MANU_DATA 0x04
#endif

#ifndef BLE_SIG_AD_TYPE_UUID
#define BLE_SIG_AD_TYPE_UUID 0x05
#endif

#ifndef BLE_MAX_ADV_DATA_LEN
#define BLE_MAX_ADV_DATA_LEN (31)
#endif


class BLEClass;
class BLEDevice;
class BLEAttribute;


typedef struct {
    uint16_t interval;
    uint16_t latency;
    uint16_t timeout;
} connParameters;

typedef enum {
    BLE_ADDR_PUBLIC = 0,
    BLE_ADDR_RANDOM_STATIC = 1,
} bleAddrType;

typedef enum {
    PERIPHERAL = 0,
    CENTRAL = 1,
    UNKNOWN = 0xFF,
} bleDeviceRole;

typedef enum {
    READ = 0x01,
    WRITE = 0x02,
    WRITE_WO_RSP = 0x04,
    NOTIFY = 0x08,
    INDICATE = 0x10,
} attrProperty;

typedef uint16_t bleConnHandle;


/* BLE UUID class */
class BLEUUID {
public:
    typedef enum {
        BLE_UUID_SHORT = 0,
        BLE_UUID_LONG = 1
    } bleUuidType;

    typedef enum {
        MSB = 0,
        LSB = 1
    } bleUuidOrder;

    static const uint8_t LONG_UUID_LENGTH = 16;

    BLEUUID();
    BLEUUID(uint8_t* uuid128, bleUuidOrder order = LSB);
    BLEUUID(uint16_t uuid16, bleUuidOrder order = LSB);
    ~BLEUUID();

    bleUuidType type(void) const;
    bleUuidOrder order(void) const;
    uint16_t shorted(void) const;
    void full(uint8_t* uuid128) const;

private:
    bleUuidType type_;
    bleUuidOrder order_;
    uint16_t shortUuid_;
    uint8_t fullUuid_[LONG_UUID_LENGTH];
};


class iBeacon {
public:
    uint16_t major;
    uint16_t minor;
    uint8_t* uuid;
    int8_t   measurePower;

    iBeacon();
    iBeacon(uint16_t major, uint16_t minor, uint8_t* uuid, int8_t mp);
    ~iBeacon();
};


/* BLE advertising data class */
class BLEAdvertisingData {
public:
    BLEAdvertisingData();
    ~BLEAdvertisingData();

    /* Add or update a type of data snippet to advertising data or scan response data. */
    int append(uint8_t type, const uint8_t* data, size_t len, bool sr = false);
    int appendLocalName(const char* name, bool sr = false);
    int appendCustomData(uint8_t* buf, size_t len, bool sr = false);
    int appendUuid(BLEUUID& uuid, bool sr = false);

    int remove(uint8_t type);

    /* Filter the specified type of data from advertising or scan response data. */
    size_t find(uint8_t type, bool sr = false) const;
    size_t find(uint8_t type, uint8_t* data, size_t len, bool sr = false) const;

    const uint8_t* advData(void) const;
    size_t advDataLen(void) const;

    const uint8_t* srData(void) const;
    size_t srDataLen(void) const;

private:
    int locate(uint8_t type, size_t* offset, size_t* len, bool sr) const {
        const uint8_t*  data;
        size_t          dataLen;

        // A valid AD structure is composed of Length field, Type field and Data field.
        // Each field should be filled with at least one byte.
        if (!sr) {
            data = adv_;
            dataLen = this->advLen_;
        } else {
            data = sr_;
            dataLen = this->srLen_;
        }

        for (size_t i = 0; (i + 3) <= dataLen; i = i) {
            *len = data[i];

            uint8_t adsType = data[i + 1];
            if (adsType == type) {
                // The value of adsLen doesn't include the length field of an AD structure.
                if ((i + *len + 1) <= dataLen) {
                    *offset = i;
                    *len += 1;
                    return SYSTEM_ERROR_NONE;
                } else {
                    return SYSTEM_ERROR_INTERNAL;
                }
            } else {
                // Navigate to the next AD structure.
                i += (*len + 1);
            }
        }

        return SYSTEM_ERROR_NOT_FOUND;
    }

    uint8_t adv_[31];
    size_t advLen_;
    uint8_t sr_[31];
    size_t srLen_;
};


/* BLE Address */
class BLEAddress {
public:
    static const uint8_t BLE_ADDR_LEN = 6;

    uint8_t addr[BLE_ADDR_LEN];
    bleAddrType type;
};


/* BLE scan result class */
class BLEScanResult {
public:
    BLEScanResult();
    ~BLEScanResult();

    const uint8_t* advData(void) const;
    size_t advDataLen(void) const;

    const uint8_t* srData(void) const;
    size_t srDataLen(void) const;

    bool find(uint8_t type) const;
    bool find(uint8_t type, uint8_t* data, size_t len, bool sr = false) const;

    void address(uint8_t* addr) const;
    const BLEAddress& address(void) const;
    bleAddrType addrType(void) const;

private:
    BLEAddress address_;
    BLEAdvertisingData data_;
    int rssi_;
};


/* BLE attribute class */
class BLEAttribute {
public:
    static const uint16_t MAX_ATTR_VALUE_LEN = 20;

    typedef void (*onDataReceivedCb)(uint8_t* data, size_t len);

    BLEAttribute();
    BLEAttribute(const char* desc, uint8_t properties, onDataReceivedCb cb = nullptr);
    BLEAttribute(const char* desc, uint8_t properties, BLEUUID& attrUuid, BLEUUID& svcUuid, onDataReceivedCb cb = nullptr);
    ~BLEAttribute();

    const char* description(void) const;

    uint8_t properties(void) const;

    int onDataReceived(onDataReceivedCb callback);

    /**
     * Update the attribute value and send to peer automatically if connected.
     */
    int setValue(const uint8_t* buf, size_t len);
    int setValue(String& str);
    int setValue(const char* str);

    template<typename T> int setValue(T val) {
        return setValue(reinterpret_cast<const uint8_t*>(&val), sizeof(T));
    }

    /**
     * Get the attribute value
     */
    size_t getValue(uint8_t* buf, size_t len) const;
    size_t getValue(String& str) const;

    template<typename T> size_t getValue(T* val) const {
        size_t len = sizeof(T);
        return getValue(reinterpret_cast<uint8_t*>(val), len);
    }

private:
    void init(void) {
        configured_ = false;
        len_ = 0;
        attrHandle_ = 0;
    }

    const char* description_;
    uint8_t properties_;

    /**
     * Internally used for the attribute with NOTIFY or INDICATE property.
     * For local attribute, if configured, it automatically sending data to peer device.
     * For peer attribute, if not configured, it will automatically configure it.
     */
    bool configured_;

    uint8_t value_[MAX_ATTR_VALUE_LEN];
    size_t len_;

    /**
     * For local attribute that with WRITE or WRITE_WITHOUT_RESPONSE property.
     * For peer attribute that with NOTIFY or INDICATE property.
     */
    onDataReceivedCb dataCb_;

    uint16_t attrHandle_;
    BLEUUID attrUuid_;
    BLEUUID svcUuid_;
};

typedef BLEAttribute* BLEPeerAttribute;


/* BLE device class */
class BLEDevice {
    friend BLEAttribute;

public:
    BLEDevice();
    explicit BLEDevice(int n);
    ~BLEDevice();

    const connParameters& params(void) const;

    void address(uint8_t* addr) const;
    const BLEAddress& address(void) const;
    bleAddrType addrType(void) const;

    /**
     * There might have more than two attributes with the same description.
     */
    int attribute(const char* desc, BLEPeerAttribute* attrs);

    size_t attrsCount(void) const;

private:
    bleConnHandle handle_;
    connParameters params_;
    BLEAddress addr_;
    int rssi_;
    Vector<BLEAttribute> attributes_;
};

typedef BLEDevice* BLEPeerDevice;


/* BLE class */
class BLEClass {
    friend BLEAttribute;

public:
    static const uint8_t  MAX_PERIPHERAL_LINK_COUNT = 1; // When acting as Peripheral, the maximum number of Central it can connect to in parallel.
    static const uint8_t  MAX_CENTRAL_LINK_COUNT = 5; // When acting as Central, the maximum number of Peripheral it can connect to in parallel.

    static const uint32_t DEFAULT_ADVERTISING_INTERVAL = 100; // In milliseconds.
    static const uint32_t DEFAULT_ADVERTISING_TIMEOUT = 0; // In milliseconds, 0 for advertising infinitely
    static const uint32_t DEFAULT_SCANNING_TIMEOUT = 0; // In milliseconds, 0 for scanning infinitely

    typedef void (*onConnectedCb)(BLEPeerDevice peer);
    typedef void (*onDisconnectedCb)(BLEPeerDevice peer);

    BLEClass();
    ~BLEClass();

    void on(void);
    void off(void);

    void onConnectionChangedCb(onConnectedCb connCb, onDisconnectedCb disconnCb);

    int advertisementData(BLEAdvertisingData& data);
    int advertisementData(iBeacon& beacon);

    /**
     * Start advertising.
     */
    int advertise(uint32_t interval = DEFAULT_ADVERTISING_INTERVAL) const;
    int advertise(uint32_t interval, uint32_t timeout = DEFAULT_ADVERTISING_TIMEOUT) const;

    /**
     * Start scanning. It will stop once the advertiser count is reached or timeout expired.
     */
    ssize_t scan(BLEScanResult* result, size_t count, uint16_t timeout = DEFAULT_SCANNING_TIMEOUT) const;

    /**
     * Connect to a peer Peripheral device.
     */
    BLEPeerDevice connect(const BLEAddress& addr);

    /**
     * Disconnect from peer Central.
     */
    int disconnect(void);

    /**
     * Disconnect from peer Peripheral.
     */
    int disconnect(BLEPeerDevice peer);

    /**
     * Check if connected with peer Central.
     */
    bool connected(void) const;

    /**
     * Check if connected with peer Peripheral.
     */
    bool connected(BLEPeerDevice peer) const;

    /**
     * Retrieve specific peer Peripheral if found.
     */
    BLEPeerDevice peripheral(const BLEAddress& addr);

    /**
     * Retrieve peer Central if connected.
     */
    BLEPeerDevice central(void);

    /**
     * Retrieve local device.
     */
    BLEDevice& local(void) const;

private:
    BLEClass* getInstance(void) {
        if (ble_ == nullptr) {
            ble_ = new BLEClass;
        }
        return ble_;
    }

    static void onBleEvents(void* event, void* context);

    /**
     * Retrieve peer device with given connection handle.
     */
    BLEPeerDevice getPeerDeviceByConnHandle(bleConnHandle) {
        return nullptr;
    }

    static BLEClass* ble_;
    static BLEDevice local_;
    Vector<BLEDevice> peers_;
    onConnectedCb connectedCb_;
    onDisconnectedCb disconnectCb_;
};


extern BLEClass BLE;


#endif /* Wiring_BLE */

#endif /* SPARK_WIRING_BLE_H */
