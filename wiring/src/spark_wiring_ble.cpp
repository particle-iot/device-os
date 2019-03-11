/*
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

#include "spark_wiring_platform.h"
#include "spark_wiring_ble.h"

#if Wiring_BLE

#define MIN(a,b) {((a) > (b)) ? (b) : (a)}

namespace {

__attribute__((unused)) void onAdvStopped(void* event) {

}

__attribute__((unused))void onScanResult(void* event) {

}

__attribute__((unused))void onScanStopped(void* event) {

}

__attribute__((unused))void onConnected(void* event) {

}

__attribute__((unused))void onDisconnected(void* event) {

}

__attribute__((unused))void onConnParamsUpdated(void* event) {

}

__attribute__((unused))void onServiceDiscovered(void* event) {

}

__attribute__((unused))void onCharacteristicDiscovered(void* event) {

}

__attribute__((unused))void onDescriptorDiscovered(void* event) {

}

__attribute__((unused))void onDataReceived(void* event) {

}

} //anonymous namespace


/**
 * BLEUUID class
 */
BLEUUID::BLEUUID() {
}

BLEUUID::BLEUUID(uint8_t* uuid128, bleUuidOrder order) {

}

BLEUUID::BLEUUID(uint16_t uuid16, bleUuidOrder order) {

}

BLEUUID::~BLEUUID() {
}

BLEUUID::bleUuidType BLEUUID::type(void) const {
    return type_;
}

BLEUUID::bleUuidOrder BLEUUID::order(void) const {
    return order_;
}

uint16_t BLEUUID::shorted(void) const {
    return shortUuid_;
}

void BLEUUID::full(uint8_t* uuid128) const {

}


iBeacon::iBeacon() {

}

iBeacon::iBeacon(uint16_t major, uint16_t minor, uint8_t* uuid, int8_t mp) {

}

iBeacon::~iBeacon() {

}


/**
 * BLEAdvertisingData class
 */
BLEAdvertisingData::BLEAdvertisingData() {
}

BLEAdvertisingData::~BLEAdvertisingData() {
}

int BLEAdvertisingData::append(uint8_t type, const uint8_t* data, size_t len, bool sr) {
    size_t      offset;
    size_t      adsLen;
    bool        adsExist = false;

    if (sr && type == BLE_SIG_AD_TYPE_FLAGS) {
        // The scan response data should not contain the AD Flags AD structure.
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if (locate(type, &offset, &adsLen, sr) == SYSTEM_ERROR_NONE) {
        adsExist = true;
    }

    uint8_t*    advPtr;
    size_t*     advLenPtr;

    if (sr) {
        advPtr = this->sr_;
        advLenPtr = &this->srLen_;
    } else {
        advPtr = this->adv_;
        advLenPtr = &this->advLen_;
    }

    if (adsExist) {
        // Update the existing AD structure.
        uint16_t staLen = *advLenPtr - adsLen;
        if ((staLen + len + 2) <= BLE_MAX_ADV_DATA_LEN) {
            // Firstly, move the last consistent block.
            uint16_t moveLen = *advLenPtr - offset - adsLen;
            memmove(&advPtr[offset + len], &advPtr[offset + adsLen], moveLen);

            // Secondly, Update the AD structure.
            // The Length field is the total length of Type field and Data field.
            advPtr[offset + 1] = len + 1;
            memcpy(&advPtr[offset + 2], data, len);

            // An AD structure is composed of one byte length field, one byte Type field and Data field.
            *advLenPtr = staLen + len + 2;
            return SYSTEM_ERROR_NONE;
        } else {
            return SYSTEM_ERROR_LIMIT_EXCEEDED;
        }
    } else {
        // Append the AD structure at the and of advertising data.
        if ((*advLenPtr + len + 2) <= BLE_MAX_ADV_DATA_LEN) {
            advPtr[(*advLenPtr)++] = len + 1;
            advPtr[(*advLenPtr)++] = type;
            memcpy(&advPtr[*advLenPtr], data, len);
            *advLenPtr += len;
            return SYSTEM_ERROR_NONE;
        } else {
            return SYSTEM_ERROR_LIMIT_EXCEEDED;
        }
    }
}

int BLEAdvertisingData::appendLocalName(const char* name, bool sr) {
    return append(BLE_SIG_AD_TYPE_LOCAL_NAME, (const uint8_t*)name, strlen(name), sr);
}

int BLEAdvertisingData::appendCustomData(uint8_t* buf, size_t len, bool sr) {
    return append(BLE_SIG_AD_TYPE_MANU_DATA, buf, len, sr);
}

int BLEAdvertisingData::appendUuid(BLEUUID& uuid, bool sr) {
    if (uuid.type() == BLEUUID::BLE_UUID_SHORT) {
        uint16_t uuid16;
        uuid16 = uuid.shorted();
        return append(BLE_SIG_AD_TYPE_UUID, reinterpret_cast<const uint8_t*>(&uuid16), 2, sr);
    }
    else {
        uint8_t uuid128[16];
        uuid.full(uuid128);
        return append(BLE_SIG_AD_TYPE_UUID, (const uint8_t*)uuid128, 16, sr);
    }
}

int BLEAdvertisingData::remove(uint8_t type) {
    size_t      offset, len;
    bool        sr;
    uint8_t*    advPtr;
    size_t*     advLenPtr;

    if (locate(type, &offset, &len, true) == SYSTEM_ERROR_NONE) {
        sr = true;
        advPtr = this->sr_;
        advLenPtr = &this->srLen_;
    } else if (locate(type, &offset, &len, false) == SYSTEM_ERROR_NONE) {
        sr = false;
        advPtr = this->adv_;
        advLenPtr = &this->advLen_;
    } else {
        return SYSTEM_ERROR_NONE;
    }

    // The advertising data must contain the AD Flags AD structure.
    if (!sr && type == BLE_SIG_AD_TYPE_FLAGS) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }

    // Remove the AD structure from advertising data.
    uint16_t moveLen = *advLenPtr - offset - len;
    memcpy(&advPtr[offset], &advPtr[offset + len], moveLen);
    *advLenPtr = *advLenPtr - len;

    return SYSTEM_ERROR_NONE;
}

size_t BLEAdvertisingData::find(uint8_t type, bool sr) const {
    return find(type, nullptr, 0, sr);
}

size_t BLEAdvertisingData::find(uint8_t type, uint8_t* data, size_t len, bool sr) const {
    // An AD structure must consist of 1 byte length field, 1 byte type field and at least 1 byte data field
    if (this->advLen_ < 3 && this->srLen_ < 3) {
        return 0;
    }

    size_t          offset, adsLen;
    const uint8_t*  advPtr;

    if (locate(type, &offset, &adsLen, sr) == SYSTEM_ERROR_NONE) {
        if (sr) {
            advPtr = this->sr_;
        } else {
            advPtr = this->adv_;
        }
    } else {
        return 0;
    }

    adsLen = adsLen - 2;
    if (data != nullptr) {
        // Only copy the data field of the found AD structure.
        len = MIN(len, adsLen);
        memcpy(data, &advPtr[offset + 2], len);
    }

    return adsLen;
}

const uint8_t* BLEAdvertisingData::advData(void) const {
    return adv_;
}

size_t BLEAdvertisingData::advDataLen(void) const {
    return advLen_;
}

const uint8_t* BLEAdvertisingData::srData(void) const {
    return sr_;
}

size_t BLEAdvertisingData::srDataLen(void) const {
    return srLen_;
}


/**
 * BLEScanResult class
 */
BLEScanResult::BLEScanResult() {
}

BLEScanResult::~BLEScanResult() {
}

const uint8_t* BLEScanResult::advData() const {
    return data_.advData();
}

size_t BLEScanResult::advDataLen(void) const {
    return data_.advDataLen();
}

const uint8_t* BLEScanResult::srData(void) const {
    return data_.srData();
}

size_t BLEScanResult::srDataLen(void) const {
    return data_.srDataLen();
}

bool BLEScanResult::find(uint8_t type) const {
    return data_.find(type);
}

bool BLEScanResult::find(uint8_t type, uint8_t* data, size_t len, bool sr) const {
    return data_.find(type, data, len, sr);
}

void BLEScanResult::address(uint8_t* addr) const {
    memcpy(addr, address_.addr, 6);
}

const BLEAddress& BLEScanResult::address(void) const {
    return address_;
}

bleAddrType BLEScanResult::addrType(void) const {
    return address_.type;
}


/**
 * BLEAttribute class
 */
BLEAttribute::BLEAttribute() {
    properties_ = 0;
    description_ = nullptr;
    dataCb_ = nullptr;

    init();
}

BLEAttribute::BLEAttribute(const char* desc, uint8_t properties, onDataReceivedCb cb)
        : description_(desc),
          properties_(properties),
          dataCb_(cb) {
    init();

    BLEClass::local_.attributes_.append(*this);
}

BLEAttribute::BLEAttribute(const char* desc, uint8_t properties, BLEUUID& attrUuid, BLEUUID& svcUuid, onDataReceivedCb cb) {

}

BLEAttribute::~BLEAttribute() {
}

const char* BLEAttribute::description(void) const {
    return description_;
}

uint8_t BLEAttribute::properties(void) const {
    return properties_;
}

int BLEAttribute::onDataReceived(onDataReceivedCb callback) {
    if (callback != nullptr) {
        dataCb_ = callback;
    }
    return SYSTEM_ERROR_NONE;;
}

int BLEAttribute::setValue(const uint8_t* buf, size_t len) {
    return SYSTEM_ERROR_NONE;
}

int BLEAttribute::setValue(String& str) {
    return setValue(reinterpret_cast<const uint8_t*>(str.c_str()), str.length());
}

size_t BLEAttribute::getValue(uint8_t* buf, size_t len) const {
    return SYSTEM_ERROR_NONE;
}

size_t BLEAttribute::getValue(String& str) const {
    return SYSTEM_ERROR_NONE;
}


/**
 * BLEDevice class
 */
BLEDevice::BLEDevice() {

}

BLEDevice::BLEDevice(int n) : attributes_(n) {

}

BLEDevice::~BLEDevice() {

}

const connParameters& BLEDevice::params(void) const {
    return params_;
}

void BLEDevice::address(uint8_t* addr) const {
    memcpy(addr, addr_.addr, 6);
}

const BLEAddress& BLEDevice::address(void) const {
    return addr_;
}

bleAddrType BLEDevice::addrType(void) const {
    return addr_.type;
}

int BLEDevice::attribute(const char* desc, BLEPeerAttribute* attrs) {
    return 0;
}

size_t BLEDevice::attrsCount(void) const {
    return 0;
}


/**
 * BLEClass class
 */
BLEClass* BLEClass::ble_;
BLEDevice BLEClass::local_(5);

BLEClass::BLEClass() : peers_(MAX_PERIPHERAL_LINK_COUNT+MAX_CENTRAL_LINK_COUNT) {

}

BLEClass::~BLEClass() {

}

void BLEClass::on(void) {

}

void BLEClass::off(void) {

}

void BLEClass::onConnectionChangedCb(onConnectedCb connCb, onDisconnectedCb disconnCb) {
    return;
}

int BLEClass::advertisementData(BLEAdvertisingData& data) {
    return SYSTEM_ERROR_NONE;
}

int BLEClass::advertisementData(iBeacon& beacon) {
    return SYSTEM_ERROR_NONE;
}

int BLEClass::advertise(uint32_t interval) const {
    return SYSTEM_ERROR_NONE;
}

int BLEClass::advertise(uint32_t interval, uint32_t timeout) const {
    return SYSTEM_ERROR_NONE;
}

int BLEClass::scan(BLEScanResult* results, size_t count, uint16_t timeout) const {
    return SYSTEM_ERROR_NONE;
}

BLEPeerDevice BLEClass::connect(const BLEAddress& addr) {
    return nullptr;
}

int BLEClass::disconnect(void) {
    return SYSTEM_ERROR_NONE;
}

int BLEClass::disconnect(BLEPeerDevice peer) {
    return SYSTEM_ERROR_NONE;
}

bool BLEClass::connected(void) const {
    return false;
}

bool BLEClass::connected(BLEPeerDevice peer) const {
    return false;
}

BLEPeerDevice BLEClass::peripheral(const BLEAddress& addr) {
    return nullptr;
}

BLEPeerDevice BLEClass::central(void) {
    return nullptr;
}

BLEDevice& BLEClass::local(void) const {
    return local_;
}

void BLEClass::onBleEvents(void* event, void* context) {
    auto self = static_cast<BLEClass*>(context);

    // For instance.
    (void)self;
}


/* Built-in BLE instance. */
BLEClass BLE;


#endif
