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

BLEUUID::~BLEUUID() {
}


/**
 * BLEAdvertisingData class
 */
BLEAdvertisingData::BLEAdvertisingData() {
}

BLEAdvertisingData::~BLEAdvertisingData() {
}

int BLEAdvertisingData::append(uint8_t type, const uint8_t* data, uint16_t len, bool sr) {
    uint16_t  offset;
    uint16_t  adsLen;
    bool      adsExist = false;

    if (sr && type == BLE_SIG_AD_TYPE_FLAGS) {
        // The scan response data should not contain the AD Flags AD structure.
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if (locate(type, &offset, &adsLen, sr) == SYSTEM_ERROR_NONE) {
        adsExist = true;
    }

    uint8_t *advPtr;
    uint16_t *advLenPtr;

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

int BLEAdvertisingData::appendCustomData(uint8_t* buf, uint16_t len, bool sr) {
    return append(BLE_SIG_AD_TYPE_MANU_DATA, buf, len, sr);
}

int BLEAdvertisingData::appendUuid(BLEUUID& uuid, bool sr) {
    if (uuid.type == BLEUUID::BLE_UUID_SHORT) {
        return append(BLE_SIG_AD_TYPE_UUID, reinterpret_cast<const uint8_t*>(&uuid.shortUuid), 2, sr);
    }
    else {
        return append(BLE_SIG_AD_TYPE_UUID, (const uint8_t*)uuid.fullUuid, 16, sr);
    }
}

int BLEAdvertisingData::remove(uint8_t type) {
    uint16_t  offset, len;
    bool      sr;
    uint8_t*  advPtr;
    uint16_t* advLenPtr;

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

bool BLEAdvertisingData::find(uint8_t type) const {
    return find(type, nullptr, nullptr);
}

bool BLEAdvertisingData::find(uint8_t type, uint8_t* data, uint16_t* len, bool* sr) const {
    // An AD structure must consist of 1 byte length field, 1 byte type field and at least 1 byte data field
    if (this->advLen_ < 3 && this->srLen_ < 3) {
        *len = 0;
        return SYSTEM_ERROR_NOT_FOUND;
    }

    uint16_t offset, adsLen;
    const uint8_t *advPtr;

    if (locate(type, &offset, &adsLen, true) == SYSTEM_ERROR_NONE) {
        if (sr != nullptr) {
            *sr = true;
        }
        advPtr = this->sr_;
    } else if (locate(type, &offset, &adsLen, false) == SYSTEM_ERROR_NONE) {
        if (sr != nullptr) {
            *sr = false;
        }
        advPtr = this->adv_;
    } else {
        if (len != nullptr) {
            *len = 0;
        }
        return SYSTEM_ERROR_NOT_FOUND;
    }

    if (len != nullptr) {
        adsLen = adsLen - 2;
        if (data != nullptr && *len > 0) {
            // Only copy the data field of the found AD structure.
            *len = MIN(*len, adsLen);
            memcpy(data, &advPtr[offset + 2], *len);
        }
        *len = adsLen;
    }

    return SYSTEM_ERROR_NONE;
}

int BLEAdvertisingData::advData(uint8_t* buf, uint16_t len) const {
    return 0;
}

int BLEAdvertisingData::scanRspData(uint8_t* buf, uint16_t len) const {
    return 0;
}


/**
 * BLEScanResult class
 */
BLEScanResult::BLEScanResult() {
}

BLEScanResult::~BLEScanResult() {
}

int BLEScanResult::advData(uint8_t* buf, uint16_t len) const {
    return data_.advData(buf, len);
}

int BLEScanResult::scanRspData(uint8_t* buf, uint16_t len) const {
    return data_.scanRspData(buf, len);
}

bool BLEScanResult::find(uint8_t type) const {
    return data_.find(type);
}

bool BLEScanResult::find(uint8_t type, uint8_t* data, uint16_t* len, bool* sr) const {
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
BLEAttributeClass::BLEAttributeClass() {
    properties_ = 0;
    description_ = nullptr;
    dataCb_ = nullptr;

    init();
}

BLEAttributeClass::BLEAttributeClass(const char* desc, uint8_t properties, onDataReceivedCb cb)
        : description_(desc),
          properties_(properties),
          dataCb_(cb) {
    init();

    BLEClass::local_.attributes_.append(*this);
}

BLEAttributeClass::~BLEAttributeClass() {
}

const char* BLEAttributeClass::description(void) const {
    return description_;
}

uint8_t BLEAttributeClass::properties(void) const {
    return properties_;
}

int BLEAttributeClass::onDataReceived(onDataReceivedCb callback) {
    if (callback != nullptr) {
        dataCb_ = callback;
    }
    return SYSTEM_ERROR_NONE;;
}

int BLEAttributeClass::setValue(const uint8_t* buf, uint16_t len) {
    return SYSTEM_ERROR_NONE;
}

int BLEAttributeClass::setValue(String& str) {
    return setValue(reinterpret_cast<const uint8_t*>(str.c_str()), str.length());
}

int BLEAttributeClass::getValue(uint8_t* buf, uint16_t* len) const {
    return SYSTEM_ERROR_NONE;
}

int BLEAttributeClass::getValue(String& str) const {
    return SYSTEM_ERROR_NONE;
}


/**
 * BLEDevice class
 */
BLEDeviceClass::BLEDeviceClass() {

}

BLEDeviceClass::BLEDeviceClass(int n) : attributes_(n) {

}

BLEDeviceClass::~BLEDeviceClass() {

}

const connParameters& BLEDeviceClass::params(void) const {
    return params_;
}

bool BLEDeviceClass::connected(void) const {
    return false;
}

void BLEDeviceClass::address(uint8_t* addr) const {
    memcpy(addr, addr_.addr, 6);
}

const BLEAddress& BLEDeviceClass::address(void) const {
    return addr_;
}

bleAddrType BLEDeviceClass::addrType(void) const {
    return addr_.type;
}

int BLEDeviceClass::attribute(const char* desc, BLEAttribute* attrs) {
    return 0;
}

uint8_t BLEDeviceClass::attrsCount(void) const {
    return 0;
}


/**
 * BLEClass class
 */
BLEClass* BLEClass::ble_;
BLEDeviceClass BLEClass::local_(5);

BLEClass::BLEClass() : peers_(MAX_PERIPHERAL_LINK_COUNT+MAX_CENTRAL_LINK_COUNT) {

}

BLEClass::~BLEClass() {

}

int BLEClass::advertise(uint32_t interval) const {
    return SYSTEM_ERROR_NONE;
}

int BLEClass::advertise(uint32_t interval, uint32_t timeout) const {
    return SYSTEM_ERROR_NONE;
}

int BLEClass::scan(BLEScanResult* results, uint8_t count, uint16_t timeout) const {
    return SYSTEM_ERROR_NONE;
}

int BLEClass::connect(onConnectedCb connCb, onDisconnectedCb disconnCb) {
    return SYSTEM_ERROR_NONE;
}


int BLEClass::connect(const BLEAddress& addr, onConnectedCb connCb, onDisconnectedCb disconnCb) {
    return SYSTEM_ERROR_NONE;
}

int BLEClass::disconnect(void) {
    return SYSTEM_ERROR_NONE;
}

int BLEClass::disconnect(BLEDevice peer) {
    return SYSTEM_ERROR_NONE;
}

bool BLEClass::connected(void) const {
    return false;
}

int BLEClass::peerDevices(BLEDevice* devices, uint8_t count) const {
    return 0;
}

BLEDevice BLEClass::peer(const BLEAddress& addr) {
    return nullptr;
}

void BLEClass::onBleEvents(void* event, void* context) {
    auto self = static_cast<BLEClass*>(context);

    // For instance.
    (void)self;
}


/* Built-in BLE instance. */
BLEClass BLE;


#endif
