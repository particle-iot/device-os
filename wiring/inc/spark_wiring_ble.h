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

class BLEUUID {
public:
    typedef enum {
        BLE_UUID_SHORT = 0,
        BLE_UUID_LONG = 1
    } ble_uuid_type_t;

    typedef enum {
        MSB = 0,
        LSB = 1
    } ble_uuid_byte_order_t;

    static const uint8_t LONG_UUID_LENGTH = BLE_SIG_UUID_128BIT_LEN;

private:
    ble_uuid_type_t type;
    uint16_t        shortUUID;
    uint8_t         longUUID[LONG_UUID_LENGTH];
    uint8_t         baseUUID[LONG_UUID_LENGTH];
};

class AdvertisingDataManagement {
public:
    int locateAdStructure(uint8_t adsType, const uint8_t* data, uint16_t len, uint16_t* offset, uint16_t* adsLen);
    int encodeAdStructure(uint8_t adsType, const uint8_t* data, uint16_t len, bool sr, uint8_t* advData, uint16_t* advDataLen);
}

class BLEGAPClass : protected AdvertisingDataManagement {
public:
    /* Sets local device address. */
    virtual int setDeviceAddress(const void* addr) override {return 0;}

    /* Gets local device address. */
    virtual int getDeviceAddress(void* addr) override {return 0;}

    /* Sets local device name. */
    virtual int setDeviceName(const uint8_t* name, uint16_t len) override {return 0;}
    virtual int setDeviceName(const char* str) {
        uint16_t len = strlen(str);
        return setDeviceName((const uint8_t*)str, len);
    }
    virtual int setDeviceName(String& str) {
        return setDeviceName(str.c_str());
    }

    /* Gets local device name. */
    virtual int getDeviceName(uint8_t* name, uint16_t* len) override {return 0;}

    /* Sets local device appearance. */
    virtual int setAppearance(uint16_t val) override {return 0;}

    /* Gets local device appearance. */
    virtual int getAppearance(uint16_t* val) override {return 0;}

    /* Sets Peripheral Preferred Connection Parameters. */
    virtual int setPPCP(const void* ppcp) override {return 0;}

    /* Gets Peripheral Preferred Connection Parameters. */
    virtual int getPPCP(void* ppcp) override {return 0;}

    /* Sets TX Power. */
    virtual int setTxPower(int val) override {return 0;}

    /* Gets current TX Power. */
    virtual int getTxPower(int* val) override {return 0;}

    /* Sets Advertising parameters. */
    virtual int setAdvertisingParameters(const void* params, void* reserved) override {return 0;}

    /* Sets advertising data. */
    virtual int setAdvertisingData(const uint8_t* data, uint16_t len) override {return 0;}

    /* Fetches the specific advertising structure from the given advertising data. */
    int decodeAdvertisingData(uint8_t adsType, const uint8_t* advData, uint16_t advDataLen, uint8_t* data, uint16_t* len);

    /* Sets scan response data. */
    virtual int setScanResponseData(const uint8_t* data, uint16_t len) override {return 0;}

    /* Starts advertising so that it can be observed by nearby BLE devices. */
    virtual int startAdvertising(void* reserved) override {return 0;}

    /* Stops advertising. */
    virtual int stopAdvertising(void) override {return 0;}

    /* Checks if it is advertising or not. */
    virtual bool isAdvertising(void) override {return 0;}

    /* Sets scan parameters. */
    virtual int setScanParameters(const void* params, void* reserved) override {return 0;}

    /* Starts scanning nearby BLE devices. */
    virtual int startScanning(void* reserved) override {return 0;}

    /* Stops scanning nearby BLE devices. */
    virtual int stopScanning(void) override {return 0;}

    /* Checks if it is scanning BLE devices. */
    virtual bool isScanning(void) override {return 0;}

    /* Connects to a peer BLE device. */
    virtual int connect(const void* addr, void* reserved) override {return 0;}

    /* Cancels connecting to peer BLE device if the connection hasn't been established. */
    virtual int connectCancel(void) override {return 0;}

    /* Checks if local device is connecting to a peer BLE device. */
    virtual bool isConnecting(void) override {return 0;}

    /* Checks if local device is connected to a peer BLE device. */
    virtual bool isConnected(void) override {return 0;}

    /* Disconnects from a peer BLE device. */
    virtual int disconnect(void) override {return 0;}

    /* Updates connection parameters. */
    virtual int updateConnectionParameters(const void* params, void* reserved) override {return 0;}
};

class BLEGATTServerClass {
public:
    /* Adds a BLE service. */
    virtual int addService(uint8_t type = BLE_SERVICE_TYPE_PRIMARY, const uint8_t* uuid, uint16_t* handle) override {return 0;}
    virtual int addService(uint8_t type = BLE_SERVICE_TYPE_PRIMARY, uint16_t uuid, uint16_t* handle) override {return 0;}
    virtual int addService(uint8_t type = BLE_SERVICE_TYPE_PRIMARY, BLEUUID& uuid, uint16_t* handle) override {return 0;}
    virtual int addService(uint8_t type = BLE_SERVICE_TYPE_PRIMARY, String& uuid, uint16_t* handle) override {return 0;}
    virtual int addService(BLEService& service) override {return 0;}

    /* Adds a BLE characteristic under the specific service. */
    virtual int addCharacteristic(BLECharacteristic& characteristic) override {return 0;}
    virtual int addCharacteristic(const uint8_t* uuid, BLECharacteristic& characteristic) override {return 0;}
    virtual int addCharacteristic(uint16_t uuid, BLECharacteristic& characteristic) override {return 0;}
    virtual int addCharacteristic(uint16_t svcHandle, uint8_t properties, const uint8_t* uuid, const char* desc, BLECharacteristic& characteristic) override {return 0;}
    virtual int addCharacteristic(uint16_t svcHandle, uint8_t properties, uint16_t uuid, const char* desc, BLECharacteristic& characteristic) override {return 0;}

    /* Writes local characteristic value. */
    virtual int write(BLECharacteristic& characteristic, const uint8_t* data, uint16_t len) override {return 0;}
    virtual int write(uint16_t valueHandle, const uint8_t* data, uint16_t len) override {return 0;}

    /* Reads local characteristic value. */
    virtual int read(BLECharacteristic& characteristic, uint8_t* data, uint16_t* len) override {return 0;}
    virtual int read(uint16_t valueHandle, uint8_t* data, uint16_t* len) override {return 0;}
};

class BLEGATTClientClass {
public:
    /* Discovers all services on peer device. */
    virtual int discoverAllServices(uint16_t connHandle, BLEService* services, uint8_t* svcCount) override {return 0;}

    /* Discovers all services and characteristics on peer device. */
    virtual int discoverAllServices(uint16_t connHandle, BLEService* services, uint8_t* svcCount, BLECharacteristic* characteristic, uint8_t* charCount) override {return 0;}

    /* Discovers specific service on peer device by UUID. */
    virtual int discoverServiceByUUID(uint16_t connHandle, BLEUUID& uuid, BLEService* services) override {return 0;}
    virtual int discoverServiceByUUID128(uint16_t connHandle, const uint8_t* uuid, BLEService* services) override {return 0;}
    virtual int discoverServiceByUUID128(uint16_t connHandle, const char* uuid, BLEService* services) override {return 0;}
    virtual int discoverServiceByUUID128(uint16_t connHandle, String& uuid, BLEService* services) override {return 0;}
    virtual int discoverServiceByUUID16(uint16_t connHandle, uint16_t uuid, BLEService* services) override {return 0;}

    /* Discovers specific service on peer device by UUID and all the characteristics under this service. */
    virtual int discoverServiceByUUID(uint16_t connHandle, BLEUUID& uuid, BLEService* services, BLECharacteristic* characteristic, uint8_t* count) override {return 0;}
    virtual int discoverServiceByUUID128(uint16_t connHandle, const uint8_t* uuid, BLEService* services, BLECharacteristic* characteristic, uint8_t* count) override {return 0;}
    virtual int discoverServiceByUUID128(uint16_t connHandle, const char* uuid, BLEService* services, BLECharacteristic* characteristic, uint8_t* count) override {return 0;}
    virtual int discoverServiceByUUID128(uint16_t connHandle, String& uuid, BLEService* services, BLECharacteristic* characteristic, uint8_t* count) override {return 0;}
    virtual int discoverServiceByUUID16(uint16_t connHandle, uint16_t uuid, BLEService* services, BLECharacteristic* characteristic, uint8_t* count) override {return 0;}

    /* Discovers all characteristics under specific service on peer device and the descriptors under the characteristics. */
    virtual int discoverAllCharacteristics(uint16_t connHandle, BLEService& service, BLECharacteristic* characteristic, uint8_t* count) override {return 0;}
    virtual int discoverAllCharacteristics(uint16_t connHandle, uint16_t startHandle, uint16_t endHandle, BLECharacteristic* characteristic, uint8_t* count) override {return 0;}

    /* Checks if it is discovering service or characteristics. */
    virtual int isDiscovering(void) override {return 0;}

    /* Configures the Client Characteristic Configuration Descriptor. */
    virtual int configureCCCD(uint16_t connHandle, uint16_t attrHandle, uint8_t val) override {return 0;}
    virtual int configureCCCD(uint16_t connHandle, BLECharacteristic* characteristic, uint8_t val) override {return 0;}
    virtual int enableNotification(uint16_t connHandle, BLECharacteristic* characteristic) override {return 0;}
    virtual int enableNotification(uint16_t connHandle, uint16_t attrHandle) override {return 0;}
    virtual int disableNotification(uint16_t connHandle, BLECharacteristic* characteristic) override {return 0;}
    virtual int disableNotification(uint16_t connHandle, uint16_t attrHandle) override {return 0;}

    /* Writes local characteristic value. */
    virtual int write(uint16_t connHandle, BLECharacteristic& characteristic, const uint8_t* data, uint16_t len) override {return 0;}
    virtual int write(uint16_t connHandle, uint16_t valueHandle, const uint8_t* data, uint16_t len) override {return 0;}

    /* Reads local characteristic value. */
    virtual int read(uint16_t connHandle, BLECharacteristic& characteristic, uint8_t* data, uint16_t* len) override {return 0;}
    virtual int read(uint16_t connHandle, uint16_t valueHandle, uint8_t* data, uint16_t* len) override {return 0;}
};

class BLEDeviceClass :
        public BLEGAPClass,
        public BLEGATTServerClass,
        public BLEGATTClientClass {
public:
    BLEDeviceClass() { }
    ~BLEDeviceClass() { }

    /* Turns BLE on. */
    virtual int on(void) override {return 0;}

    /* Turns BLE off. */
    virtual int off(void) override {return 0;}

private:

};


extern BLEDeviceClass BLE;


#endif /* Wiring_BLE */

#endif /* SPARK_WIRING_BLE_H */
