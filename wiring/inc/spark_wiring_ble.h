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

private:
    bleUuidType type_;
    uint16_t    short_;
    uint8_t     long_[LONG_UUID_LENGTH];
    uint8_t     base_[LONG_UUID_LENGTH];
};


/* BLE advertising data class */
class BLEAdvertisingData {
public:
    BLEAdvertisingData();
    ~BLEAdvertisingData();

    /* Add or update a type of data snippet to advertising data or scan response data. */
    virtual int append(uint8_t type, const uint8_t* data, uint16_t len, bool sr = false) = 0;

    virtual int remove(uint8_t type) = 0;

    /* Filter the specified type of data from advertising or scan response data. */
    bool find(uint8_t type) const;
    bool find(uint8_t type, uint8_t* data, uint16_t* len, bool* sr = nullptr) const;

    void advData(uint8_t* buf, uint16_t* len) const;

    void scanRespData(uint8_t* buf, uint16_t* len) const;

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


/* BLE device class */
class BLEDevice : public BLEAddress {
public:
    bleDeviceRole role;

    BLEDevice();
    explicit BLEDevice(bleDeviceRole role);
    ~BLEDevice();
};


/* BLE advertiser class */
class BLEScanResult : public BLEAdvertisingData {
public:
    BLEAddress address;

    BLEScanResult();
    ~BLEScanResult();

    /**
     * Cannot edit the advertising data in scan result.
     */
    int append(uint8_t type, const uint8_t* data, uint16_t len, bool sr = false) override { return SYSTEM_ERROR_NOT_SUPPORTED; }
    int remove(uint8_t type) override { return SYSTEM_ERROR_NOT_SUPPORTED; }
};


/* BLE attribute class */
class BLEAttribute {
public:
    static const uint16_t MAX_ATTR_VALUE_LEN = 20;

    typedef void (*onDataReceivedCb)(uint8_t* data, uint16_t len);

    BLEAttribute();
    BLEAttribute(uint8_t properties, const char* desc, onDataReceivedCb cb = nullptr);

    ~BLEAttribute();

    const char* description(void) const;

    uint8_t properties(void) const;

    int onDataReceived(onDataReceivedCb callback);

    /* Update the attribute value and send to peer automatically if connected.*/
    int update(const uint8_t* buf, uint16_t len);

    /* Get the attribute value */
    int value(uint8_t* buf, uint16_t* len) const;

private:
    void init(void);

    /**
     * For local attribute, it always be valid.
     * For peer attribute, it will be valid only if the corresponding connection is valid.
     */
    bool             valid_;

    uint8_t          properties_;
    const char*      description_;

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


class BLEAttributeList {
public:
    BLEAttributeList();
    explicit BLEAttributeList(int n);
    ~BLEAttributeList();

    // There might have more than two attributes with the same description.
    int fetch(const char* desc, BLEAttribute** attrs);

    uint8_t count(void) const;

private:
    Vector<BLEAttribute> peerAttrs_;
};


/* BLE connection class */
class BLEConnection : public BLEAttributeList {
    friend BLEClass;
    friend BLEAttribute;

public:
    BLEConnection();
    explicit BLEConnection(int peerAttrCount);
    ~BLEConnection();

    const bleConnHandle handle(void) const;
    const connParameters& params(void) const;
    const BLEDevice& peer(void) const;
    const BLEDevice& local(void) const;

private:
    int setHandle(bleConnHandle handle);
    int setParams(connParameters& params);
    int setPeer(BLEDevice& peer);
    int setLocal(BLEDevice& local);

    bleConnHandle  handle_;
    connParameters params_;
    BLEDevice      peer_;

    static BLEDevice local_;

    /**
     * It should be static since it may be invoked when constructing an attribute in user application.
     * Attributes can only be added after BLE stack being initialized.
     */
    static Vector<BLEAttribute> localAttrs_;
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

    /* Initialize BLE stack and add pre-defined local attributes. */
    int on(void);

    /* Turn BLE off. */
    int off(void);

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

    int disconnect(BLEConnection* conn);

    bool connected(BLEConnection* conn) const;

private:
    static void onBleEvents(void* event, void* context);

    /* BLE event handlers. */
    void onAdvStopped(void* event);
    void onScanResult(void* event);
    void onScanStopped(void* event);
    void onConnected(void* event);
    void onDisconnected(void* event);
    void onConnParamsUpdated(void* event);
    void onServiceDiscovered(void* event);
    void onCharacteristicDiscovered(void* event);
    void onDescriptorDiscovered(void* event);
    void onDataReceived(void* event);

    /* Retrieve the connection with given connection handle.  */
    BLEConnection* connection(bleConnHandle);

    Vector<BLEConnection> connections_;

    onConnectedCb    connectedCb_;
    onDisconnectedCb disconnectCb_;
};


extern BLEClass BLE;


#endif /* Wiring_BLE */

#endif /* SPARK_WIRING_BLE_H */
