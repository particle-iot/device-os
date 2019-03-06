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
class BLEAttribute;
class BLEPeerAttrList;
class BLEConnection;


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
    ~BLEUUID();

    bleUuidType type;
    uint16_t    shortUuid;
    uint8_t     fullUuid[LONG_UUID_LENGTH];
    uint8_t     baseUuid[LONG_UUID_LENGTH];
};


/* BLE advertising data class */
class BLEAdvertisingData {
public:
    BLEAdvertisingData();
    ~BLEAdvertisingData();

    /* Add or update a type of data snippet to advertising data or scan response data. */
    int append(uint8_t type, const uint8_t* data, uint16_t len, bool sr = false);
    int appendLocalName(const char* name, bool sr = false);
    int appendCustomData(uint8_t* buf, uint16_t len, bool sr = false);
    int appendUuid(BLEUUID& uuid, bool sr = false);

    int remove(uint8_t type);

    /* Filter the specified type of data from advertising or scan response data. */
    bool find(uint8_t type) const;
    bool find(uint8_t type, uint8_t* data, uint16_t* len, bool* sr = nullptr) const;

    int advData(uint8_t* buf, uint16_t len) const;

    int scanRspData(uint8_t* buf, uint16_t len) const;

private:
    int locate(uint8_t type, uint16_t* offset, uint16_t* len, bool sr) const;

    uint8_t  adv_[31];
    uint16_t advLen_;
    uint8_t  sr_[31];
    uint16_t srLen_;
};


/* BLE Address */
class BLEAddress {
public:
    static const uint8_t BLE_ADDR_LEN = 6;

    uint8_t     addr[BLE_ADDR_LEN];
    bleAddrType type;
};


/* BLE scan result class */
class BLEScanResult {
public:
    BLEScanResult();
    ~BLEScanResult();

    int advData(uint8_t* buf, uint16_t len) const;

    int scanRspData(uint8_t* buf, uint16_t len) const;

    bool find(uint8_t type) const;
    bool find(uint8_t type, uint8_t* data, uint16_t* len, bool* sr = nullptr) const;

    BLEAddress address;

private:
    BLEAdvertisingData data_;
};


/* BLE attribute class */
class BLEAttribute {
public:
    static const uint16_t MAX_ATTR_VALUE_LEN = 20;

    typedef void (*onDataReceivedCb)(uint8_t* data, uint16_t len);

    BLEAttribute();
    BLEAttribute(const char* desc, uint8_t properties, onDataReceivedCb cb = nullptr);

    ~BLEAttribute();

    const char* description(void) const;

    uint8_t properties(void) const;

    int onDataReceived(onDataReceivedCb callback);

    /* Update the attribute value and send to peer automatically if connected.*/
    int setValue(const uint8_t* buf, uint16_t len);
    int setValue(String& str);

    template<typename T> int setValue(T val) {
        return setValue(reinterpret_cast<const uint8_t*>(&val), sizeof(T));
    }

    /* Get the attribute value */
    int getValue(uint8_t* buf, uint16_t* len) const;

private:
    void init(void);

    /**
     * For local attribute, it always be valid.
     * For peer attribute, it will be valid only if the corresponding connection is valid.
     */
    bool             valid_;

    const char*      description_;
    uint8_t          properties_;

    /**
     * Internally used for the attribute with NOTIFY or INDICATE property.
     * For local attribute, if configured, it automatically sending data to peer device.
     * For peer attribute, if not configured, it will automatically configure it.
     */
    bool             configured_;

    uint8_t          value_[MAX_ATTR_VALUE_LEN];
    uint16_t         len_;

    /**
     * For local attribute that with WRITE or WRITE_WITHOUT_RESPONSE property.
     * For peer attribute that with NOTIFY or INDICATE property.
     */
    onDataReceivedCb dataCb_;

    uint16_t         attrHandle_;
    BLEUUID          attrUuid_;
    BLEUUID          svcUuid_;
};


/* BLE device class */
class BLEDevice : public BLEAddress {
    friend BLEAttribute;

public:
    BLEDevice();
    explicit BLEDevice(int n);
    ~BLEDevice();

    // There might have more than two attributes with the same description.
    int attribute(const char* desc, BLEAttribute** attrs);

    uint8_t attrsCount(void) const;

private:
    Vector<BLEAttribute> attributes_;
};


/* BLE connection class */
class BLEConnection {
public:
    bleDeviceRole role;

    BLEConnection();
    BLEConnection(bleDeviceRole role, int n);
    ~BLEConnection();

    const bleConnHandle handle(void) const;

    const connParameters& params(void) const;

    BLEDevice& peer(void);

    static BLEDevice& local(void);

private:
    bleConnHandle  handle_;
    connParameters params_;

    /**
     * It should be static since it may be invoked when constructing an attribute in user application.
     * Attributes can only be added after BLE stack being initialized.
     */
    static BLEDevice local_;
    BLEDevice        peer_;
};


/* BLE class */
class BLEClass {
public:
    static const uint8_t  MAX_PERIPHERAL_LINK_COUNT = 1; // When acting as Peripheral, the maximum number of Central it can connect to in parallel.
    static const uint8_t  MAX_CENTRAL_LINK_COUNT = 5; // When acting as Central, the maximum number of Peripheral it can connect to in parallel.

    static const uint32_t DEFAULT_ADVERTISING_INTERVAL = 100; // In milliseconds.
    static const uint32_t DEFAULT_ADVERTISING_TIMEOUT = 0; // In milliseconds, 0 for advertising infinitely
    static const uint32_t DEFAULT_SCANNING_TIMEOUT = 0; // In milliseconds, 0 for scanning infinitely

    typedef void (*onConnectedCb)(BLEConnection* conn);
    typedef void (*onDisconnectedCb)(BLEConnection* conn);

    BLEClass();
    ~BLEClass();

    /* Start advertising. */
    int advertise(uint32_t interval = DEFAULT_ADVERTISING_INTERVAL) const;
    int advertise(uint32_t interval, uint32_t timeout = DEFAULT_ADVERTISING_TIMEOUT) const;

    /**
     * Start scanning. It will stop once the advertiser count is reached or timeout expired.
     */
    int scan(BLEScanResult* result, uint8_t count, uint16_t timeout = DEFAULT_SCANNING_TIMEOUT) const;

    /**
     * By calling these methods, local device will be Peripheral once connected.
     */
    BLEConnection* connect(onConnectedCb connCb = nullptr, onDisconnectedCb disconnCb = nullptr);
    /**
     * If peer is local device, it will be BLE Peripheral once connected.
     * Otherwise, it will be BLE Central once connected.
     */
    BLEConnection* connect(const BLEAddress& addr, onConnectedCb connCb = nullptr, onDisconnectedCb disconnCb = nullptr);

    /* Disconnect from peer Central device. */
    int disconnect(void);
    /* Disconnect from specific peer Central or Peripheral device. */
    int disconnect(BLEConnection* conn);

    bool connected(BLEConnection* conn) const;
    bool connected(void) const;

private:
    BLEClass* getInstance(void) {
        if (ble_ == nullptr) {
            ble_ = new BLEClass;
        }
        return ble_;
    }

    static BLEClass* ble_;

    static void onBleEvents(void* event, void* context);

    /* Retrieve the connection with given connection handle.  */
    BLEConnection* connection(bleConnHandle);

    Vector<BLEConnection> connections_;

    onConnectedCb    connectedCb_;
    onDisconnectedCb disconnectCb_;
};


extern BLEClass BLE;


#endif /* Wiring_BLE */

#endif /* SPARK_WIRING_BLE_H */
