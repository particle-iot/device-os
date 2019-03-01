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

BLEAttributeList BLEClass::localAttrs_(5);


BLEUUID::BLEUUID() {

}

BLEUUID::~BLEUUID() {

}

BLEAdvertisingData::BLEAdvertisingData() {

}

BLEAdvertisingData::~BLEAdvertisingData() {

}

int BLEAdvertisingData::locate(uint8_t type, uint16_t* offset, uint16_t* len, bool sr) const {
    // A valid AD structure is composed of Length field, Type field and Data field.
    // Each field should be filled with at least one byte.
    if (!sr) {
        for (uint16_t i = 0; (i + 3) <= this->advLen_; i = i) {
            *len = this->adv_[i];

            uint8_t type = this->adv_[i + 1];
            if (type == type) {
                // The value of adsLen doesn't include the length field of an AD structure.
                if ((i + *len + 1) <= this->advLen_) {
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
    }
    else {
        for (uint16_t i = 0; (i + 3) <= this->srLen_; i = i) {
            *len = this->sr_[i];

            uint8_t type = this->sr_[i + 1];
            if (type == type) {
                // The value of adsLen doesn't include the length field of an AD structure.
                if ((i + *len + 1) <= this->srLen_) {
                    *offset = i;
                    *len += 1;
                    if (!sr) {
                        sr = true;
                    }
                    return SYSTEM_ERROR_NONE;
                } else {
                    return SYSTEM_ERROR_INTERNAL;
                }
            } else {
                // Navigate to the next AD structure.
                i += (*len + 1);
            }
        }
    }

    return SYSTEM_ERROR_NOT_FOUND;
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

BLEDevice::BLEDevice() {

}

BLEDevice::~BLEDevice() {

}

BLEScanResult::BLEScanResult() {

}

BLEScanResult::~BLEScanResult() {

}

const BLEAdvertisingData& BLEScanResult::data(void) const {
    return data_;
}

const BLEDevice& BLEScanResult::device(void) const {
    return peer_;
}

BLEConnection::BLEConnection() : peerAttrs_(10) {

}

BLEConnection::~BLEConnection() {

}

const bleConnHandle BLEConnection::handle(void) const {
    return handle_;
}

BLEAttributeList& BLEConnection::peerAttrs(void) {
    return peerAttrs_;
}

BLEAttribute::BLEAttribute() {
    properties_ = 0;
    description_ = nullptr;
    dataCb_ = nullptr;

    init();
}

BLEAttribute::BLEAttribute(uint8_t properties, const char* desc, onDataReceivedCb cb)
        : properties_(properties),
          description_(desc),
          dataCb_(cb) {
    init();

    BLEClass::localAttrs_.attributes_.append(*this);
}

BLEAttribute::~BLEAttribute() {

}

void BLEAttribute::init(void) {
    valid_ = false;
    configured_ = false;
    len_ = 0;
    attrHandle_ = 0;
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

int BLEAttribute::update(const uint8_t* buf, uint16_t len) {
    return SYSTEM_ERROR_NONE;
}

int BLEAttribute::value(uint8_t* buf, uint16_t* len) const {
    return SYSTEM_ERROR_NONE;
}

BLEAttributeList::BLEAttributeList() {

}

BLEAttributeList::BLEAttributeList(int n) : attributes_(n) {

}

BLEAttributeList::~BLEAttributeList() {

}

int BLEAttributeList::fetch(const char* desc, BLEAttribute** attrs) {
    return 0;
}

uint8_t BLEAttributeList::count(void) const {
    return attributes_.size();
}

BLEClass::BLEClass() : connections_(MAX_PERIPHERAL_LINK_COUNT+MAX_CENTRAL_LINK_COUNT) {

}

BLEClass::~BLEClass() {

}

int BLEClass::on(void) {
    return SYSTEM_ERROR_NONE;
}

int BLEClass::off(void) {
    return SYSTEM_ERROR_NONE;
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

BLEConnection* BLEClass::connect(onConnectedCb connCb, onDisconnectedCb disconnCb) {
    return nullptr;
}


BLEConnection* BLEClass::connect(const BLEDevice& peer, onConnectedCb connCb, onDisconnectedCb disconnCb) {
    return nullptr;
}

int BLEClass::disconnect(BLEConnection* conn) {
    return SYSTEM_ERROR_NONE;
}

bool BLEClass::connected(BLEConnection* conn) const {
    return conn->handle() != 0xFFFF;
}


/* Built-in BLE instance. */
BLEClass BLE;


#endif
