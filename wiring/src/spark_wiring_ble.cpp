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

#include "spark_wiring_ble.h"

#if Wiring_BLE
#include "spark_wiring_thread.h"
#include <memory>
#include <algorithm>
#include "check.h"
#include "debug.h"
#include "scope_guard.h"
#include "hex_to_bytes.h"
#include "bytes2hexbuf.h"

#include "logging.h"
LOG_SOURCE_CATEGORY("wiring.ble")

using namespace particle::ble;

namespace particle {

#undef DEBUG // Legacy logging macro

#if BLE_WIRING_DEBUG_ENABLED
#define DEBUG(_fmt, ...) \
        do { \
            LOG_PRINTF(TRACE, _fmt "\r\n", ##__VA_ARGS__); \
        } while (false)

#define DEBUG_DUMP(_data, _size) \
        do { \
            LOG_PRINT(TRACE, "> "); \
            LOG_DUMP(TRACE, _data, _size); \
            LOG_PRINTF(TRACE, " (%u bytes)\r\n", (unsigned)(_size)); \
        } while (false)
#else
#define DEBUG(...)
#define DEBUG_DUMP(...)
#endif


namespace {

const uint8_t PARTICLE_DEFAULT_BLE_SVC_UUID[BLE_SIG_UUID_128BIT_LEN] = {
    0x7b, 0xe3, 0x27, 0x74, 0x7b, 0xf8, 0x15, 0xac, 0xdd, 0x49, 0xa9, 0x13, 0x00, 0x00, 0x72, 0xf5
};

} //anonymous namespace


namespace ble {

class WiringBleLock {
public:
    WiringBleLock() :
            locked_(false) {
        lock();
    }

    ~WiringBleLock() {
        if (locked_) {
            unlock();
        }
    }

    WiringBleLock(WiringBleLock&& lock) :
            locked_(lock.locked_) {
        lock.locked_ = false;
    }

    void lock() {
        mutex_.lock();
        locked_ = true;
    }

    void unlock() {
        mutex_.unlock();
        locked_ = false;
    }

    WiringBleLock(const WiringBleLock&) = delete;
    WiringBleLock& operator=(const WiringBleLock&) = delete;

private:
    bool locked_;
    static RecursiveMutex mutex_;
};

RecursiveMutex WiringBleLock::mutex_;

} /* namespace ble */


/*******************************************************
 * BleAddress class
 */
BleAddress::BleAddress()
        : address_{} {
    address_.addr_type = BLE_SIG_ADDR_TYPE_PUBLIC;
}

BleAddress::BleAddress(const hal_ble_addr_t& addr) {
    address_ = addr;
}

BleAddress::BleAddress(const uint8_t addr[BLE_SIG_ADDR_LEN], BleAddressType type) {
    address_.addr_type = static_cast<ble_sig_addr_type_t>(type);
    memcpy(address_.addr, addr, BLE_SIG_ADDR_LEN);
}

BleAddress::BleAddress(const char* address, BleAddressType type) {
    set(address, type);
}

BleAddress::BleAddress(const String& address, BleAddressType type) {
    set(address, type);
}

int BleAddress::type(BleAddressType type) {
    address_.addr_type = static_cast<ble_sig_addr_type_t>(type);
    return SYSTEM_ERROR_NONE;
}

BleAddressType BleAddress::type() const {
    return static_cast<BleAddressType>(address_.addr_type);
}

int BleAddress::set(const uint8_t addr[BLE_SIG_ADDR_LEN], BleAddressType type) {
    address_.addr_type = static_cast<ble_sig_addr_type_t>(type);
    memcpy(address_.addr, addr, BLE_SIG_ADDR_LEN);
    return SYSTEM_ERROR_NONE;
}

int BleAddress::set(const char* address, BleAddressType type) {
    CHECK_TRUE(address, SYSTEM_ERROR_INVALID_ARGUMENT);
    size_t len = BLE_SIG_ADDR_LEN;
    for (size_t i = 0; i < strnlen(address, BLE_SIG_ADDR_LEN * 2 + 5) && len > 0; i++) {
        int8_t hi = hexToNibble(address[i]);
        if (hi >= 0) {
            address_.addr[len - 1] = hi << 4;
            if (++i < strnlen(address, BLE_SIG_ADDR_LEN * 2 + 5)) {
                int8_t lo = hexToNibble(address[i]);
                if (lo >= 0) {
                    address_.addr[len - 1] |= lo;
                }
            }
            len--;
        }
    }
    while (len > 0) {
        address_.addr[len - 1] = 0x00;
        len--;
    }
    address_.addr_type = static_cast<ble_sig_addr_type_t>(type);
    return SYSTEM_ERROR_NONE;
}

int BleAddress::set(const String& address, BleAddressType type) {
    return set(address.c_str(), type);
}

void BleAddress::octets(uint8_t addr[BLE_SIG_ADDR_LEN]) const {
    memcpy(addr, address_.addr, BLE_SIG_ADDR_LEN);
}

hal_ble_addr_t BleAddress::halAddress() const {
    return address_;
}

String BleAddress::toString(bool stripped) const {
    char cStr[BLE_SIG_ADDR_LEN * 2 + 6];
    toString(cStr, sizeof(cStr), stripped);
    return String(cStr);
}

size_t BleAddress::toString(char* buf, size_t len, bool stripped) const {
    if (!buf || len == 0) {
        return 0;
    }
    uint8_t temp[BLE_SIG_ADDR_LEN];
    char cStr[BLE_SIG_ADDR_LEN * 2 + 5];
    toBigEndian(temp);
    if (stripped) {
        bytes2hexbuf(temp, BLE_SIG_ADDR_LEN, cStr);
    } else {
        uint8_t idx = 0;
        bytes2hexbuf(&temp[idx], 1, &cStr[idx]);
        idx++;
        cStr[idx * 2] = ':';
        bytes2hexbuf(&temp[idx], 1, &cStr[idx * 2 + 1]);
        idx++;
        cStr[idx * 2 + 1] = ':';
        bytes2hexbuf(&temp[idx], 1, &cStr[idx * 2 + 2]);
        idx++;
        cStr[idx * 2 + 2] = ':';
        bytes2hexbuf(&temp[idx], 1, &cStr[idx * 2 + 3]);
        idx++;
        cStr[idx * 2 + 3] = ':';
        bytes2hexbuf(&temp[idx], 1, &cStr[idx * 2 + 4]);
        idx++;
        cStr[idx * 2 + 4] = ':';
        bytes2hexbuf(&temp[idx], 1, &cStr[idx * 2 + 5]);
    }
    len = std::min(len - 1, sizeof(cStr));
    memcpy(buf, cStr, len);
    buf[len++] = '\0';
    return len;
}

uint8_t BleAddress::operator[](uint8_t i) const {
    if (i >= BLE_SIG_ADDR_LEN) {
        return 0;
    }
    return address_.addr[i];
}

BleAddress& BleAddress::operator=(const hal_ble_addr_t& addr) {
    address_ = addr;
    return *this;
}

BleAddress& BleAddress::operator=(const uint8_t addr[BLE_SIG_ADDR_LEN]) {
    memcpy(address_.addr, addr, BLE_SIG_ADDR_LEN);
    return *this;
}

bool BleAddress::operator==(const BleAddress& addr) const {
    if (address_.addr_type == addr.address_.addr_type && !memcmp(address_.addr, addr.address_.addr, BLE_SIG_ADDR_LEN)) {
        return true;
    }
    return false;
}

void BleAddress::toBigEndian(uint8_t buf[BLE_SIG_ADDR_LEN]) const {
    for (uint8_t i = 0, j = BLE_SIG_ADDR_LEN - 1; i < BLE_SIG_ADDR_LEN; i++, j--) {
        buf[i] = address_.addr[j];
    }
}


/*******************************************************
 * BleUuid class
 */
BleUuid::BleUuid()
        : uuid_() {
}

BleUuid::BleUuid(const hal_ble_uuid_t& uuid) {
    uuid_ = uuid;
}

BleUuid::BleUuid(const BleUuid& uuid)
        : uuid_(uuid.uuid_) {
}

BleUuid::BleUuid(const uint8_t* uuid128, BleUuidOrder order)
        : BleUuid() {
    if (!uuid128) {
        memset(uuid_.uuid128, 0x00, BLE_SIG_UUID_128BIT_LEN);
    } else {
        if (order == BleUuidOrder::LSB) {
            memcpy(uuid_.uuid128, uuid128, BLE_SIG_UUID_128BIT_LEN);
        } else {
            for (uint8_t i = 0, j = BLE_SIG_UUID_128BIT_LEN - 1; i < BLE_SIG_UUID_128BIT_LEN; i++, j--) {
                uuid_.uuid128[i] = uuid128[j];
            }
        }
        uuid_.type = BLE_UUID_TYPE_128BIT;
    }
}

BleUuid::BleUuid(uint16_t uuid16)
        : BleUuid() {
    uuid_.uuid16 = uuid16;
    uuid_.type = BLE_UUID_TYPE_16BIT;
}

BleUuid::BleUuid(const uint8_t* uuid128, uint16_t uuid16, BleUuidOrder order)
        : BleUuid(uuid128, order) {
    uuid_.uuid128[12] = (uint8_t)(uuid16 & 0x00FF);
    uuid_.uuid128[13] = (uint8_t)((uuid16 >> 8) & 0x00FF);
    uuid_.type = BLE_UUID_TYPE_128BIT;
}

BleUuid::BleUuid(const char* uuid)
        : BleUuid() {
    construct(uuid);
}

BleUuid::BleUuid(const String& uuid)
        : BleUuid(uuid.c_str()) {
}

bool BleUuid::isValid() const {
    if (type() == BleUuidType::SHORT) {
        return uuid_.uuid16 != 0x0000;
    } else {
        uint8_t temp[BLE_SIG_UUID_128BIT_LEN] = {};
        return memcmp(uuid_.uuid128, temp, BLE_SIG_UUID_128BIT_LEN);
    }
}

BleUuidType BleUuid::type() const {
    if (uuid_.type == BLE_UUID_TYPE_16BIT || uuid_.type == BLE_UUID_TYPE_128BIT_SHORTED) {
        return BleUuidType::SHORT;
    } else {
        return BleUuidType::LONG;
    }
}

hal_ble_uuid_t BleUuid::halUUID() {
    return uuid_;
}

uint16_t BleUuid::shorted() const {
    return uuid_.uuid16;
}

void BleUuid::rawBytes(uint8_t uuid128[BLE_SIG_UUID_128BIT_LEN]) const {
    memcpy(uuid128, uuid_.uuid128, BLE_SIG_UUID_128BIT_LEN);
}

const uint8_t* BleUuid::rawBytes() const {
    return uuid_.uuid128;
}

String BleUuid::toString(bool stripped) const {
    char cStr[BLE_SIG_UUID_128BIT_LEN * 2 + 5];
    toString(cStr, sizeof(cStr));
    return String(cStr);
}

size_t BleUuid::toString(char* buf, size_t len, bool stripped) const {
    if (!buf || len == 0) {
        return 0;
    }
    if (type() == BleUuidType::SHORT) {
        char cStr[BLE_SIG_UUID_16BIT_LEN * 2] = {};
        uint16_t bigEndian = uuid_.uuid16 << 8 | uuid_.uuid16 >> 8;
        bytes2hexbuf((uint8_t*)&bigEndian, 2, cStr);
        len = std::min(len - 1, sizeof(cStr));
        memcpy(buf, cStr, len);
        buf[len++] = '\0';
        return len;
    }
    uint8_t temp[BLE_SIG_UUID_128BIT_LEN];
    toBigEndian(temp);
    char cStr[BLE_SIG_UUID_128BIT_LEN * 2 + 4];
    if (stripped) {
        bytes2hexbuf(temp, BLE_SIG_UUID_128BIT_LEN, cStr);
    } else {
        uint8_t idx = 0;
        bytes2hexbuf(&temp[idx], 4, &cStr[0]);
        idx += 4;
        cStr[idx * 2] = '-';
        bytes2hexbuf(&temp[idx], 2, &cStr[idx * 2 + 1]);
        idx += 2;
        cStr[idx * 2 + 1] = '-';
        bytes2hexbuf(&temp[idx], 2, &cStr[idx * 2 + 2]);
        idx += 2;
        cStr[idx * 2 + 2] = '-';
        bytes2hexbuf(&temp[idx], 2, &cStr[idx * 2 + 3]);
        idx += 2;
        cStr[idx * 2 + 3] = '-';
        bytes2hexbuf(&temp[idx], 6, &cStr[idx * 2 + 4]);
    }
    len = std::min(len - 1, sizeof(cStr));
    memcpy(buf, cStr, len);
    buf[len++] = '\0';
    return len;
}

BleUuid& BleUuid::operator=(const BleUuid& uuid) {
    uuid_ = uuid.uuid_;
    return *this;
}

BleUuid& BleUuid::operator=(const uint8_t* uuid128) {
    if (uuid128) {
        memcpy(uuid_.uuid128, uuid128, BLE_SIG_UUID_128BIT_LEN);
        uuid_.type = BLE_UUID_TYPE_128BIT;
    }
    return *this;
}

BleUuid& BleUuid::operator=(uint16_t uuid16) {
    uuid_.uuid16 = uuid16;
    uuid_.type = BLE_UUID_TYPE_16BIT;
    return *this;
}

BleUuid& BleUuid::operator=(const String& uuid) {
    construct(uuid.c_str());
    return *this;
}

BleUuid& BleUuid::operator=(const char* uuid) {
    construct(uuid);
    return *this;
}

BleUuid& BleUuid::operator=(const hal_ble_uuid_t& uuid) {
    uuid_ = uuid;
    return *this;
}

bool BleUuid::operator==(const BleUuid& uuid) const {
    if (type() == BleUuidType::SHORT) {
        return (uuid_.uuid16 == uuid.uuid_.uuid16);
    } else {
        return !memcmp(uuid_.uuid128, uuid.uuid_.uuid128, BLE_SIG_UUID_128BIT_LEN);
    }
}

bool BleUuid::operator==(const char* uuid) const {
    return !strcmp(uuid, toString().c_str());
}

bool BleUuid::operator==(const String& uuid) const {
    return (*this == uuid.c_str());
}

bool BleUuid::operator==(uint16_t uuid) const {
    return (type() == BleUuidType::SHORT && uuid_.uuid16 == uuid);
}

bool BleUuid::operator==(const uint8_t* uuid128) const {
    return (type() == BleUuidType::LONG && !memcmp(uuid128, uuid_.uuid128, BLE_SIG_UUID_128BIT_LEN));
}

void BleUuid::construct(const char* uuid) {
    if (uuid == nullptr) {
        memset(uuid_.uuid128, 0x00, BLE_SIG_UUID_128BIT_LEN);
        uuid_.type = BLE_UUID_TYPE_128BIT;
        return;
    }
    if (strnlen(uuid, BLE_SIG_UUID_128BIT_LEN * 2 + 4) == (BLE_SIG_UUID_16BIT_LEN * 2)) {
        char buf[2] = {};
        if (hexToBytes(uuid, buf, BLE_SIG_UUID_16BIT_LEN) == BLE_SIG_UUID_16BIT_LEN) {
            uuid_.uuid16 = ((uint16_t)buf[0] << 8) | (uint16_t)buf[1];
        } else {
            uuid_.uuid16 = 0x0000;
        }
        uuid_.type = BLE_UUID_TYPE_16BIT;
        return;
    }
    size_t len = BLE_SIG_UUID_128BIT_LEN;
    for (size_t i = 0; i < strnlen(uuid, BLE_SIG_UUID_128BIT_LEN * 2 + 4) && len > 0; i++) {
        int8_t hi = hexToNibble(uuid[i]);
        if (hi >= 0) {
            uuid_.uuid128[len - 1] = hi << 4;
            if (++i < strnlen(uuid, BLE_SIG_UUID_128BIT_LEN * 2 + 4)) {
                int8_t lo = hexToNibble(uuid[i]);
                if (lo >= 0) {
                    uuid_.uuid128[len - 1] |= lo;
                }
            }
            len--;
        }
    }
    while (len > 0) {
        uuid_.uuid128[len - 1] = 0x00;
        len--;
    }
    uuid_.type = BLE_UUID_TYPE_128BIT;
}

void BleUuid::toBigEndian(uint8_t buf[BLE_SIG_UUID_128BIT_LEN]) const {
    for (uint8_t i = 0, j = BLE_SIG_UUID_128BIT_LEN - 1; i < BLE_SIG_UUID_128BIT_LEN; i++, j--) {
        buf[i] = uuid_.uuid128[j];
    }
}


/*******************************************************
 * BleAdvertisingData class
 */
BleAdvertisingData::BleAdvertisingData()
        : selfData_(),
          selfLen_(0) {
    uint8_t flag = BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    append(BleAdvertisingDataType::FLAGS, &flag, sizeof(uint8_t));
}

BleAdvertisingData::BleAdvertisingData(const iBeacon& beacon)
        : BleAdvertisingData() {
    set(beacon);
}

size_t BleAdvertisingData::set(const uint8_t* buf, size_t len) {
    if (buf == nullptr || len == 0) {
        selfLen_ = 0;
        return selfLen_;
    }
    len = std::min(len, (size_t)BLE_MAX_ADV_DATA_LEN);
    memcpy(selfData_, buf, len);
    selfLen_ = len;
    return selfLen_;
}

size_t BleAdvertisingData::set(const iBeacon& beacon) {
    clear();

    if (beacon.uuid.type() == BleUuidType::SHORT) {
        return selfLen_;
    }

    selfData_[selfLen_++] = 0x02;
    selfData_[selfLen_++] = BLE_SIG_AD_TYPE_FLAGS;
    selfData_[selfLen_++] = BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    selfData_[selfLen_++] = 0x1a; // length
    selfData_[selfLen_++] = BLE_SIG_AD_TYPE_MANUFACTURER_SPECIFIC_DATA;
    // Company ID
    uint16_t companyId = iBeacon::APPLE_COMPANY_ID;
    memcpy(&selfData_[selfLen_], (uint8_t*)&companyId, sizeof(uint16_t));
    selfLen_ += sizeof(uint16_t);
    // Beacon type: iBeacon
    selfData_[selfLen_++] = iBeacon::BEACON_TYPE_IBEACON;
    // Length of the following payload
    selfData_[selfLen_++] = 0x15;
    // 128-bits Beacon UUID, MSB
    for (size_t i = 0, j = BLE_SIG_UUID_128BIT_LEN - 1; i < BLE_SIG_UUID_128BIT_LEN; i++, j--) {
        selfData_[selfLen_ + i] = beacon.uuid.rawBytes()[j];
    }
    selfLen_ += BLE_SIG_UUID_128BIT_LEN;
    // Major, MSB
    selfData_[selfLen_++] = (uint8_t)((beacon.major >> 8) & 0x00FF);
    selfData_[selfLen_++] = (uint8_t)(beacon.major & 0x00FF);
    // Minor, MSB
    selfData_[selfLen_++] = (uint8_t)((beacon.minor >> 8) & 0x00FF);
    selfData_[selfLen_++] = (uint8_t)(beacon.minor & 0x00FF);
    // Measure power
    selfData_[selfLen_++] = beacon.measurePower;

    return selfLen_;
}

size_t BleAdvertisingData::append(BleAdvertisingDataType type, const uint8_t* buf, size_t len, bool force) {
    if (buf == nullptr) {
        return selfLen_;
    }
    size_t offset;
    size_t adsLen = locate(selfData_, selfLen_, type, &offset);
    if (!force && adsLen > 0) {
        // Update the existing AD structure.
        size_t staLen = selfLen_ - adsLen;
        if ((staLen + len + 2) <= BLE_MAX_ADV_DATA_LEN) {
            // Firstly, move the last consistent block.
            uint16_t moveLen = selfLen_ - offset - adsLen;
            memmove(&selfData_[offset + len + 2], &selfData_[offset + adsLen], moveLen);
            // Secondly, Update the AD structure.
            // The Length field is the total length of Type field and Data field.
            selfData_[offset] = len + 1;
            memcpy(&selfData_[offset + 2], buf, len);
            // An AD structure is composed of one byte length field, one byte Type field and Data field.
            selfLen_ = staLen + len + 2;
        }
    }
    else if ((selfLen_ + len + 2) <= BLE_MAX_ADV_DATA_LEN) {
        // Append the AD structure at the and of advertising data.
        selfData_[selfLen_++] = len + 1;
        selfData_[selfLen_++] = static_cast<uint8_t>(type);
        memcpy(&selfData_[selfLen_], buf, len);
        selfLen_ += len;
    }
    return selfLen_;
}

size_t BleAdvertisingData::appendLocalName(const char* name) {
    return append(BleAdvertisingDataType::COMPLETE_LOCAL_NAME, (const uint8_t*)name, strnlen(name, BLE_MAX_DEV_NAME_LEN), false);
}

size_t BleAdvertisingData::appendLocalName(const String& name) {
    return appendLocalName(name.c_str());
}

size_t BleAdvertisingData::appendCustomData(const uint8_t* buf, size_t len, bool force) {
    return append(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA, buf, len, force);
}

void BleAdvertisingData::clear() {
    selfLen_ = 0;
    memset(selfData_, 0x00, sizeof(selfData_));
}

void BleAdvertisingData::remove(BleAdvertisingDataType type) {
    size_t offset, len;
    len = locate(selfData_, selfLen_, type, &offset);
    if (len > 0) {
        size_t moveLen = selfLen_ - offset - len;
        memcpy(&selfData_[offset], &selfData_[offset + len], moveLen);
        selfLen_ -= len;
        // Recursively remove duplicated type.
        remove(type);
    }
}

size_t BleAdvertisingData::get(uint8_t* buf, size_t len) const {
    if (buf != nullptr) {
        len = std::min(len, selfLen_);
        memcpy(buf, selfData_, len);
        return len;
    }
    return selfLen_;
}

size_t BleAdvertisingData::get(BleAdvertisingDataType type, uint8_t* buf, size_t len) const {
    size_t offset;
    size_t adsLen = locate(selfData_, selfLen_, type, &offset);
    if (adsLen > 0) {
        if ((adsLen - 2) > 0) {
            adsLen -= 2;
            len = std::min(len, adsLen);
            if (buf != nullptr) {
                memcpy(buf, &selfData_[offset + 2], len);
            }
            return len;
        }
    }
    return 0;
}

uint8_t* BleAdvertisingData::data() {
    return selfData_;
}

size_t BleAdvertisingData::length() const {
    return selfLen_;
}

size_t BleAdvertisingData::deviceName(char* buf, size_t len) const {
    size_t nameLen = get(BleAdvertisingDataType::SHORT_LOCAL_NAME, reinterpret_cast<uint8_t*>(buf), len);
    if (nameLen > 0) {
        return nameLen;
    }
    return get(BleAdvertisingDataType::COMPLETE_LOCAL_NAME, reinterpret_cast<uint8_t*>(buf), len);
}

String BleAdvertisingData::deviceName() const {
    String name;
    char buf[BLE_MAX_ADV_DATA_LEN];
    size_t len = deviceName(buf, sizeof(buf));
    if (len > 0) {
        for (size_t i = 0; i < len; i++) {
            if (!name.concat(buf[i])) {
                break;
            }
        }
    }
    return name;
}

size_t BleAdvertisingData::serviceUUID(BleUuid* uuids, size_t count) const {
    size_t found = 0;
    found += serviceUUID(BleAdvertisingDataType::SERVICE_UUID_16BIT_MORE_AVAILABLE, &uuids[found], count - found);
    found += serviceUUID(BleAdvertisingDataType::SERVICE_UUID_16BIT_COMPLETE, &uuids[found], count - found);
    found += serviceUUID(BleAdvertisingDataType::SERVICE_UUID_128BIT_MORE_AVAILABLE, &uuids[found], count - found);
    found += serviceUUID(BleAdvertisingDataType::SERVICE_UUID_128BIT_COMPLETE, &uuids[found], count - found);
    return found;
}

size_t BleAdvertisingData::customData(uint8_t* buf, size_t len) const {
    return get(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA, buf, len);
}

bool BleAdvertisingData::contains(BleAdvertisingDataType type) const {
    size_t adsOffset;
    return locate(selfData_, selfLen_, type, &adsOffset) > 0;
}

size_t BleAdvertisingData::serviceUUID(BleAdvertisingDataType type, BleUuid* uuids, size_t count) const {
    size_t offset, adsLen = 0, found = 0;
    for (size_t i = 0; i < selfLen_; i += (offset + adsLen)) {
        adsLen = locate(&selfData_[i], selfLen_ - i, type, &offset);
        if (adsLen > 0 && found < count) {
            if (adsLen == 4) { // length field + type field + 16-bits UUID
                uuids[found++] = (uint16_t)selfData_[i + offset + 2] | ((uint16_t)selfData_[i + offset + 3] << 8);
            } else if (adsLen == 18) {
                uuids[found++] = &selfData_[i + offset + 2];
            }
            continue;
        }
        break;
    }
    return found;
}

size_t BleAdvertisingData::locate(const uint8_t* buf, size_t len, BleAdvertisingDataType type, size_t* offset) {
    if (offset == nullptr) {
        return 0;
    }
    uint8_t adsType = static_cast<uint8_t>(type);
    size_t adsLen;
    for (size_t i = 0; (i + 3) <= len; i = i) {
        adsLen = buf[i];
        if (buf[i + 1] == adsType) {
            // The value of adsLen doesn't include the length field of an AD structure.
            if ((i + adsLen + 1) <= len) {
                *offset = i;
                adsLen += 1;
                return adsLen;
            } else {
                return 0;
            }
        } else {
            // Navigate to the next AD structure.
            i += (adsLen + 1);
        }
    }
    return 0;
}

/*******************************************************
 * BleCharacteristicImpl definition
 */
class BleCharacteristicImpl {
public:
    BleCharacteristicImpl()
            : isLocal_(false),
              connHandle_(BLE_INVALID_CONN_HANDLE),
              properties_(BleCharacteristicProperty::NONE),
              attrHandles_(),
              charUuid_(),
              svcUuid_(),
              description_(),
              callback_(nullptr),
              context_(nullptr) {
    }

    BleCharacteristicImpl(const char* desc, BleCharacteristicProperty properties, BleOnDataReceivedCallback callback, void* context)
            : BleCharacteristicImpl() {
        properties_ = properties;
        description_ = desc;
        callback_ = callback;
        context_ = context;
    }

    BleCharacteristicImpl(const char* desc, BleCharacteristicProperty properties, BleUuid& charUuid, BleUuid& svcUuid, BleOnDataReceivedCallback callback, void* context)
            : BleCharacteristicImpl(desc, properties, callback, context) {
        charUuid_ = charUuid;
        svcUuid_ = svcUuid;
    }

    ~BleCharacteristicImpl() = default;

    bool& local() {
        return isLocal_;
    }

    BleConnectionHandle& connHandle() {
        return connHandle_;
    }

    BleCharacteristicProperty& properties() {
        return properties_;
    }

    BleCharacteristicHandles& attrHandles() {
        return attrHandles_;
    }

    BleUuid& charUUID() {
        return charUuid_;
    }

    BleUuid& svcUUID() {
        return svcUuid_;
    }

    String& description() {
        return description_;
    }

    void setCallback(BleOnDataReceivedCallback callback, void* context) {
        callback_ = callback;
        context_ = context;
    }

    void inheritCallback(BleCharacteristicImpl& charImpl) {
        if (charImpl.callback_) {
            callback_ = charImpl.callback_;
            context_ = charImpl.context_;
            charImpl.callback_ = nullptr;
            charImpl.context_ = nullptr;
        }
    }

    void assignUuidIfNeeded() {
        if (!charUuid_.isValid()) {
            LOG_DEBUG(TRACE, "Assign default characteristic UUID.");
            defaultUuidCharCount_++;
            BleUuid newUuid(PARTICLE_DEFAULT_BLE_SVC_UUID, defaultUuidCharCount_);
            charUuid_ = newUuid;
        }
    }

    BleCharacteristicImpl& operator=(const BleCharacteristicImpl& charImpl) = delete;

    bool operator==(const BleCharacteristicImpl& impl) {
        if (charUuid_ == impl.charUuid_ &&
                svcUuid_ == impl.svcUuid_ &&
                isLocal_ == impl.isLocal_ &&
                connHandle_ == impl.connHandle_) {
            return true;
        }
        return false;
    }

    static void onBleCharEvents(const hal_ble_char_evt_t *event, void* context);

private:
    bool isLocal_;
    BleConnectionHandle connHandle_; // For peer characteristic
    BleCharacteristicProperty properties_;
    BleCharacteristicHandles attrHandles_;
    BleUuid charUuid_;
    BleUuid svcUuid_;
    String description_;
    BleOnDataReceivedCallback callback_;
    void* context_;
    static uint16_t defaultUuidCharCount_;
};


/*******************************************************
 * BleServiceImpl definition
 */
class BleServiceImpl {
public:
    BleServiceImpl()
            : uuid_(),
              startHandle_(BLE_INVALID_ATTR_HANDLE),
              endHandle_(BLE_INVALID_ATTR_HANDLE) {
    }

    BleServiceImpl(const BleUuid& svcUuid)
            : BleServiceImpl() {
        uuid_ = svcUuid;
    }

    ~BleServiceImpl() = default;

    BleUuid& UUID() {
        return uuid_;
    }

    BleAttributeHandle& startHandle() {
        return startHandle_;
    }

    BleAttributeHandle& endHandle() {
        return endHandle_;
    }

private:
    BleUuid uuid_;
    BleAttributeHandle startHandle_;
    BleAttributeHandle endHandle_;
};


/*******************************************************
 * BlePeerDeviceImpl definition
 */
class BlePeerDeviceImpl {
public:
    BlePeerDeviceImpl()
            : connHandle_(BLE_INVALID_CONN_HANDLE),
              address_(),
              servicesDiscovered_(false),
              characteristicsDiscovered_(false) {
    }

    ~BlePeerDeviceImpl() = default;

    BleConnectionHandle& connHandle() {
        return connHandle_;
    }

    BleAddress& address() {
        return address_;
    }

    bool& servicesDiscovered() {
        return servicesDiscovered_;
    }

    bool& characteristicsDiscovered() {
        return characteristicsDiscovered_;
    }

    Vector<BleService>& services() {
        return services_;
    }

    Vector<BleCharacteristic>& characteristics() {
        return characteristics_;
    }

    void onDisconnected() {
        connHandle_ = BLE_INVALID_CONN_HANDLE;
        for (auto& characteristic : characteristics_) {
            characteristic.impl()->connHandle() = BLE_INVALID_CONN_HANDLE;
        }
        services_.clear();
        characteristics_.clear();
        servicesDiscovered_ = false;
        characteristicsDiscovered_ = false;
    }

private:
    BleConnectionHandle connHandle_;
    BleAddress address_;
    bool servicesDiscovered_;
    bool characteristicsDiscovered_;
    Vector<BleService> services_;
    Vector<BleCharacteristic> characteristics_;
};


/*******************************************************
 * BleLocalDeviceImpl definition
 */
class BleLocalDeviceImpl {
public:
    BleLocalDeviceImpl()
            : connectedCb_(nullptr),
              disconnectedCb_(nullptr),
              connectedContext_(nullptr),
              disconnectedContext_(nullptr) {
    }

    ~BleLocalDeviceImpl() = default;

    Vector<BleService>& services() {
        return services_;
    }

    Vector<BleCharacteristic>& characteristics() {
        return characteristics_;
    }

    Vector<BlePeerDevice>& peers() {
        return peers_;
    }

    void onConnectedCallback(BleOnConnectedCallback callback, void* context) {
        connectedCb_ = callback;
        connectedContext_ = context;
    }

    void onDisconnectedCallback(BleOnDisconnectedCallback callback, void* context) {
        disconnectedCb_ = callback;
        disconnectedContext_ = context;
    }

    BlePeerDevice* findPeerDevice(BleConnectionHandle connHandle) {
        for (auto& peer : peers_) {
            if (peer.impl()->connHandle() == connHandle) {
                return &peer;
            }
        }
        return nullptr;
    }

    static void onBleLinkEvents(const hal_ble_link_evt_t* event, void* context) {
        auto impl = static_cast<BleLocalDeviceImpl*>(context);
        WiringBleLock lk;
        switch (event->type) {
            case BLE_EVT_CONNECTED: {
                BlePeerDevice peer;
                peer.impl()->connHandle() = event->conn_handle;
                peer.impl()->address() = event->params.connected.info->address;
                if (!impl->peers_.append(peer)) {
                    LOG(ERROR, "Failed to append peer Central device.");
                    hal_ble_gap_disconnect(peer.impl()->connHandle(), nullptr);
                    return;
                }
                LOG(TRACE, "Connected by Central device.");
                if (impl->connectedCb_) {
                    impl->connectedCb_(peer, impl->connectedContext_);
                }
                break;
            }
            case BLE_EVT_DISCONNECTED: {
                BlePeerDevice* peer = impl->findPeerDevice(event->conn_handle);
                if (peer) {
                    peer->impl()->onDisconnected();
                    if (impl->disconnectedCb_) {
                        impl->disconnectedCb_(*peer, impl->disconnectedContext_);
                    }
                    LOG(TRACE, "Disconnected by remote device.");
                    impl->peers_.removeOne(*peer);
                }
                break;
            }
            default: {
                break;
            }
        }
    }

private:
    Vector<BleService> services_;
    Vector<BleCharacteristic> characteristics_;
    Vector<BlePeerDevice> peers_;
    BleOnConnectedCallback connectedCb_;
    BleOnDisconnectedCallback disconnectedCb_;
    void* connectedContext_;
    void* disconnectedContext_;
};


/*
 * WARN: This is executed from HAL ble thread. If the BLE wiring lock is acquired by a thread,
 * calling wiring APIs those acquiring the BLE wiring lock from the callback will suspend the
 * the execution of the callback, until the BLE wiring lock is released.
 */
void BleCharacteristicImpl::onBleCharEvents(const hal_ble_char_evt_t *event, void* context) {
    auto impl = static_cast<BleCharacteristicImpl*>(context);
    WiringBleLock lk;
    switch (event->type) {
        case BLE_EVT_DATA_NOTIFIED:
        case BLE_EVT_DATA_WRITTEN: {
            auto peer = BleLocalDevice::getInstance().impl()->findPeerDevice(event->conn_handle);
            if (!peer) {
                LOG(ERROR, "Peer device is missing!");
                break;
            }
            if (impl->callback_) {
                impl->callback_(event->params.data_written.data, event->params.data_written.len, *peer, impl->context_);
            }
            break;
        }
        default: {
            break;
        }
    }
}

uint16_t BleCharacteristicImpl::defaultUuidCharCount_ = 0;


/*******************************************************
 * BleCharacteristic class
 */
BleCharacteristic::BleCharacteristic()
        : impl_(std::make_shared<BleCharacteristicImpl>()) {
    if (!impl()) {
        SPARK_ASSERT(false);
    }
    DEBUG("BleCharacteristic(), 0x%08X -> 0x%08X, count: %d", this, impl(), impl_.use_count());
}

BleCharacteristic::BleCharacteristic(const BleCharacteristic& characteristic)
        : impl_(characteristic.impl_) {
    if (!impl()) {
        SPARK_ASSERT(false);
    }
    DEBUG("BleCharacteristic(copy), 0x%08X => 0x%08X -> 0x%08X, count: %d", &characteristic, this, impl(), impl_.use_count());
}

BleCharacteristic::BleCharacteristic(const char* desc, BleCharacteristicProperty properties, BleOnDataReceivedCallback callback, void* context)
        : impl_(std::make_shared<BleCharacteristicImpl>(desc, properties, callback, context)) {
    if (!impl()) {
        SPARK_ASSERT(false);
    }
    DEBUG("BleCharacteristic(...), 0x%08X -> 0x%08X, count: %d", this, impl(), impl_.use_count());
}

void BleCharacteristic::construct(const char* desc, BleCharacteristicProperty properties, BleUuid& charUuid, BleUuid& svcUuid, BleOnDataReceivedCallback callback, void* context) {
    impl_ = std::make_shared<BleCharacteristicImpl>(desc, properties, charUuid, svcUuid, callback, context);
    if (!impl()) {
        SPARK_ASSERT(false);
    }
    DEBUG("BleCharacteristic(), construct(...):0x%08X -> 0x%08X, count: %d", this, impl(), impl_.use_count());
}

BleCharacteristic& BleCharacteristic::operator=(const BleCharacteristic& characteristic) {
    DEBUG("BleCharacteristic(), operator=:0x%08X -> 0x%08X, count: %d", this, impl(), impl_.use_count());
    characteristic.impl()->inheritCallback(*this->impl());
    impl_ = characteristic.impl_;
    DEBUG("now:0x%08X -> 0x%08X, count: %d", this, impl(), impl_.use_count());
    return *this;
}

BleCharacteristic::~BleCharacteristic() {
    DEBUG("~BleCharacteristic(), 0x%08X -> 0x%08X, count: %d", this, impl(), impl_.use_count() - 1);
}

bool BleCharacteristic::valid() const {
    return (impl()->local() || impl()->connHandle() != BLE_INVALID_CONN_HANDLE);
}

BleUuid BleCharacteristic::UUID() const {
    return impl()->charUUID();
}

BleCharacteristicProperty BleCharacteristic::properties() const {
    return impl()->properties();
}

String BleCharacteristic::description() const {
    return impl()->description();
}

size_t BleCharacteristic::description(char* buf, size_t len) const {
    if (!buf || len == 0) {
        return 0;
    }
    String desc = description();
    len = std::min(len, desc.length());
    memcpy(buf, desc.c_str(), len);
    return len;
}

ssize_t BleCharacteristic::setValue(const uint8_t* buf, size_t len) {
    if (buf == nullptr || len == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    len = std::min(len, (size_t)BLE_MAX_ATTR_VALUE_PACKET_SIZE);
    if (impl()->local()) {
        return hal_ble_gatt_server_set_characteristic_value(impl()->attrHandles().value_handle, buf, len, nullptr);
    }
    if (impl()->connHandle() != BLE_INVALID_CONN_HANDLE) {
        if ((impl()->properties() & BleCharacteristicProperty::WRITE) == BleCharacteristicProperty::WRITE) {
            return hal_ble_gatt_client_write_with_response(impl()->connHandle(), impl()->attrHandles().value_handle, buf, len, nullptr);
        }
        if ((impl()->properties() & BleCharacteristicProperty::WRITE_WO_RSP) == BleCharacteristicProperty::WRITE_WO_RSP) {
            return hal_ble_gatt_client_write_without_response(impl()->connHandle(), impl()->attrHandles().value_handle, buf, len, nullptr);
        }
    }
    return SYSTEM_ERROR_INVALID_STATE;
}

ssize_t BleCharacteristic::setValue(const String& str) {
    return setValue(reinterpret_cast<const uint8_t*>(str.c_str()), str.length());
}

ssize_t BleCharacteristic::setValue(const char* str) {
    return setValue(reinterpret_cast<const uint8_t*>(str), strnlen(str, BLE_MAX_ATTR_VALUE_PACKET_SIZE));
}

ssize_t BleCharacteristic::getValue(uint8_t* buf, size_t len) const {
    if (buf == nullptr || len == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    len = std::min(len, (size_t)BLE_MAX_ATTR_VALUE_PACKET_SIZE);
    if (impl()->local()) {
        return hal_ble_gatt_server_get_characteristic_value(impl()->attrHandles().value_handle, buf, len, nullptr);
    }
    if (impl()->connHandle() != BLE_INVALID_CONN_HANDLE) {
        return hal_ble_gatt_client_read(impl()->connHandle(), impl()->attrHandles().value_handle, buf, len, nullptr);
    }
    return SYSTEM_ERROR_INVALID_STATE;
}

ssize_t BleCharacteristic::getValue(String& str) const {
    char* buf = (char*)malloc(BLE_MAX_ATTR_VALUE_PACKET_SIZE);
    if (buf) {
        SCOPE_GUARD ({
            free(buf);
        });
        int len = getValue((uint8_t*)buf, BLE_MAX_ATTR_VALUE_PACKET_SIZE);
        if (len > 0) {
            str = String(buf, len);
        }
        return len;
    }
    return 0;
}

int BleCharacteristic::subscribe(bool enable) const {
    CHECK_FALSE(impl()->local(), SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(impl()->connHandle() != BLE_INVALID_CONN_HANDLE, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(impl()->attrHandles().cccd_handle != BLE_INVALID_ATTR_HANDLE, SYSTEM_ERROR_NOT_SUPPORTED);
    hal_ble_cccd_config_t config = {};
    config.version = BLE_API_VERSION;
    config.size = sizeof(hal_ble_cccd_config_t);
    config.callback = impl()->onBleCharEvents;
    config.context = impl();
    config.conn_handle = impl()->connHandle();
    config.cccd_handle = impl()->attrHandles().cccd_handle;
    config.value_handle = impl()->attrHandles().value_handle;
    if (enable) {
        if ((impl()->properties() & BleCharacteristicProperty::NOTIFY) == BleCharacteristicProperty::NOTIFY) {
            config.cccd_value = BLE_SIG_CCCD_VAL_NOTIFICATION;
        } else {
            config.cccd_value = BLE_SIG_CCCD_VAL_INDICATION;
        }
    } else {
        config.cccd_value = BLE_SIG_CCCD_VAL_DISABLED;
    }
    return hal_ble_gatt_client_configure_cccd(&config, nullptr);
}

void BleCharacteristic::onDataReceived(BleOnDataReceivedCallback callback, void* context) {
    impl()->setCallback(callback, context);
}


/*******************************************************
 * BleService class
 */
BleService::BleService()
        : impl_(std::make_shared<BleServiceImpl>()) {
    if (!impl()) {
        SPARK_ASSERT(false);
    }
}

BleService::BleService(const BleUuid& uuid)
        : impl_(std::make_shared<BleServiceImpl>(uuid)) {
    if (!impl()) {
        SPARK_ASSERT(false);
    }
}

BleUuid BleService::UUID() const {
    return impl()->UUID();
}

BleService& BleService::operator=(const BleService& service) {
    impl_ = service.impl_;
    return *this;
}

bool BleService::operator==(const BleService& service) const {
    return (impl()->UUID() == service.impl()->UUID());
}


class BleDiscoveryDelegator {
public:
    BleDiscoveryDelegator() = default;
    ~BleDiscoveryDelegator() = default;

    int discoverAllServiceAndCharacteristics(BlePeerDevice& peer) {
        CHECK(discoverAllServices(peer));
        CHECK(discoverAllCharacteristics(peer));
        return SYSTEM_ERROR_NONE;
    }

    int discoverAllServices(BlePeerDevice& peer) {
        LOG(TRACE, "Start discovering services.");
        return hal_ble_gatt_client_discover_all_services(peer.impl()->connHandle(), onServicesDiscovered, &peer, nullptr);
    }

    int discoverAllCharacteristics(BlePeerDevice& peer) {
        LOG(TRACE, "Start discovering characteristics.");
        for (auto& service : peer.impl()->services()) {
            hal_ble_svc_t halService;
            halService.size = sizeof(hal_ble_svc_t);
            halService.start_handle = service.impl()->startHandle();
            halService.end_handle = service.impl()->endHandle();
            CHECK(hal_ble_gatt_client_discover_characteristics(peer.impl()->connHandle(), &halService, onCharacteristicsDiscovered, &peer, nullptr));
        }
        for (auto& characteristic : peer.impl()->characteristics()) {
            // Read the user description string if presented.
            if (characteristic.impl()->attrHandles().user_desc_handle != BLE_INVALID_ATTR_HANDLE) {
                char desc[BLE_MAX_DESC_LEN] = {};
                size_t len = hal_ble_gatt_client_read(peer.impl()->connHandle(), characteristic.impl()->attrHandles().user_desc_handle, (uint8_t*)desc, sizeof(desc) - 1, nullptr);
                if (len > 0) {
                    desc[len] = '\0';
                    characteristic.impl()->description() = desc;
                    LOG_DEBUG(TRACE, "User description: %s.", desc);
                }
            }
        }
        return SYSTEM_ERROR_NONE;
    }

private:
    /*
     * WARN: This is executed from HAL ble thread. The current thread which starts the service discovery procedure
     * has acquired the BLE HAL lock. Calling BLE HAL APIs those acquiring the BLE HAL lock in this function
     * will suspend the execution of the callback until the current thread release the BLE HAL lock. Or the BLE HAL
     * APIs those are invoked here must not acquire the BLE HAL lock.
     */
    static void onServicesDiscovered(const hal_ble_svc_discovered_evt_t* event, void* context) {
        BlePeerDevice* peer = static_cast<BlePeerDevice*>(context);
        for (size_t i = 0; i < event->count; i++) {
            BleService service;
            service.impl()->UUID() = event->services[i].uuid;
            service.impl()->startHandle() = event->services[i].start_handle;
            service.impl()->endHandle() = event->services[i].end_handle;
            if (!peer->impl()->services().append(service)) {
                LOG(ERROR, "Failed to append discovered service.");
            }
        }
    }

    /*
     * WARN: This is executed from HAL ble thread. The current thread which starts the characteristic discovery procedure
     * has acquired the BLE HAL lock. Calling BLE HAL APIs those acquiring the BLE HAL lock in this function
     * will suspend the execution of the callback until the current thread release the BLE HAL lock. Or the BLE HAL
     * APIs those are invoked here must not acquire the BLE HAL lock.
     */
    static void onCharacteristicsDiscovered(const hal_ble_char_discovered_evt_t* event, void* context) {
        BlePeerDevice* peer = static_cast<BlePeerDevice*>(context);
        for (size_t i = 0; i < event->count; i++) {
            BleCharacteristic characteristic;
            characteristic.impl()->connHandle() = event->conn_handle;
            if (event->characteristics[i].properties & BLE_SIG_CHAR_PROP_READ) {
                characteristic.impl()->properties() |= BleCharacteristicProperty::READ;
            }
            if (event->characteristics[i].properties & BLE_SIG_CHAR_PROP_WRITE_WO_RESP) {
                characteristic.impl()->properties() |= BleCharacteristicProperty::WRITE_WO_RSP;
            }
            if (event->characteristics[i].properties & BLE_SIG_CHAR_PROP_WRITE) {
                characteristic.impl()->properties() |= BleCharacteristicProperty::WRITE;
            }
            if (event->characteristics[i].properties & BLE_SIG_CHAR_PROP_NOTIFY) {
                characteristic.impl()->properties() |= BleCharacteristicProperty::NOTIFY;
            }
            if (event->characteristics[i].properties & BLE_SIG_CHAR_PROP_INDICATE) {
                characteristic.impl()->properties() |= BleCharacteristicProperty::INDICATE;
            }
            characteristic.impl()->charUUID() = event->characteristics[i].uuid;
            characteristic.impl()->attrHandles() = event->characteristics[i].charHandles;
            if (!peer->impl()->characteristics().append(characteristic)) {
                LOG(ERROR, "Failed to append discovered characteristic.");
            }
        }
    }
};


/*******************************************************
 * BlePeerDevice class
 */
BlePeerDevice::BlePeerDevice()
        : impl_(std::make_shared<BlePeerDeviceImpl>()) {
    if (!impl()) {
        SPARK_ASSERT(false);
    }
}

BlePeerDevice::~BlePeerDevice() {
    DEBUG("~BlePeerDevice(), 0x%08X -> 0x%08X, count: %d", this, impl(), impl_.use_count() - 1);
}

BlePeerDevice& BlePeerDevice::operator=(const BlePeerDevice& peer) {
    impl_ = peer.impl_;
    return *this;
}

Vector<BleService> BlePeerDevice::discoverAllServices() {
    if (!impl()->servicesDiscovered()) {
        BleDiscoveryDelegator discovery;
        if (discovery.discoverAllServices(*this) == SYSTEM_ERROR_NONE) {
            impl()->servicesDiscovered() = true;
        }
    }
    return services();
}

ssize_t BlePeerDevice::discoverAllServices(BleService* svcs, size_t count) {
    CHECK_TRUE(svcs && count > 0, SYSTEM_ERROR_INVALID_ARGUMENT);
    if (!impl()->servicesDiscovered()) {
        BleDiscoveryDelegator discovery;
        CHECK(discovery.discoverAllServices(*this));
        impl()->servicesDiscovered() = true;
    }
    return services(svcs, count);
}

Vector<BleCharacteristic> BlePeerDevice::discoverAllCharacteristics() {
    if (!impl()->servicesDiscovered()) {
        discoverAllServices();
    }
    if (!impl()->characteristicsDiscovered()) {
        BleDiscoveryDelegator discovery;
        if (discovery.discoverAllCharacteristics(*this) == SYSTEM_ERROR_NONE) {
            impl()->characteristicsDiscovered() = true;
        }
    }
    return characteristics();
}

ssize_t BlePeerDevice::discoverAllCharacteristics(BleCharacteristic* chars, size_t count) {
    CHECK_TRUE(chars && count > 0, SYSTEM_ERROR_INVALID_ARGUMENT);
    if (!impl()->servicesDiscovered()) {
        discoverAllServices();
    }
    if (!impl()->characteristicsDiscovered()) {
        BleDiscoveryDelegator discovery;
        CHECK(discovery.discoverAllCharacteristics(*this));
        impl()->characteristicsDiscovered() = true;
    }
    return characteristics(chars, count);
}

Vector<BleService> BlePeerDevice::services() {
    return impl()->services();
}

size_t BlePeerDevice::services(BleService* svcs, size_t count) {
    CHECK_TRUE(svcs && count > 0, SYSTEM_ERROR_INVALID_ARGUMENT);
    count = std::min((int)count, impl()->services().size());
    for (size_t i = 0; i < count; i++) {
        svcs[i] = impl()->services()[i];
    }
    return count;
}

bool BlePeerDevice::getServiceByUUID(BleService& service, const BleUuid& uuid) const {
    for (auto& existSvc : impl()->services()) {
        if (existSvc.UUID() == uuid) {
            service = existSvc;
            return true;
        }
    }
    return false;
}

Vector<BleCharacteristic> BlePeerDevice::characteristics() {
    return impl()->characteristics();
}

size_t BlePeerDevice::characteristics(BleCharacteristic* chars, size_t count) {
    CHECK_TRUE(chars && count > 0, SYSTEM_ERROR_INVALID_ARGUMENT);
    count = std::min((int)count, impl()->characteristics().size());
    for (size_t i = 0; i < count; i++) {
        chars[i] = impl()->characteristics()[i];
    }
    return count;
}

bool BlePeerDevice::getCharacteristicByDescription(BleCharacteristic& characteristic, const char* desc) const {
    CHECK_TRUE(desc, false);
    for (auto& existChar : impl()->characteristics()) {
        if (!strcmp(existChar.description().c_str(), desc)) {
            characteristic = existChar;
            return true;
        }
    }
    return true;
}

bool BlePeerDevice::getCharacteristicByDescription(BleCharacteristic& characteristic, const String& desc) const {
    return getCharacteristicByDescription(characteristic, desc.c_str());
}

bool BlePeerDevice::getCharacteristicByUUID(BleCharacteristic& characteristic, const BleUuid& uuid) const {
    for (auto& existChar : impl()->characteristics()) {
        if (existChar.UUID() == uuid) {
            characteristic = existChar;
            return true;
        }
    }
    return false;
}

int BlePeerDevice::connect(const BleAddress& addr, const BleConnectionParams* params, bool automatic) {
    hal_ble_conn_cfg_t connCfg = {};
    connCfg.version = BLE_API_VERSION;
    connCfg.size = sizeof(hal_ble_conn_cfg_t);
    connCfg.address = addr.halAddress();
    connCfg.conn_params = params;
    connCfg.callback = BleLocalDevice::getInstance().impl()->onBleLinkEvents;
    connCfg.context = BleLocalDevice::getInstance().impl();
    int ret = hal_ble_gap_connect(&connCfg, &impl()->connHandle(), nullptr);
    if (ret != SYSTEM_ERROR_NONE) {
        impl()->connHandle() = BLE_INVALID_CONN_HANDLE;
        return ret;
    }
    bind(addr);
    if (!BleLocalDevice::getInstance().impl()->peers().append(*this)) {
        LOG(ERROR, "Cannot add new peer device.");
        hal_ble_gap_disconnect(impl()->connHandle(), nullptr);
        impl()->connHandle() = BLE_INVALID_CONN_HANDLE;
        return SYSTEM_ERROR_NO_MEMORY;
    }
    LOG(TRACE, "New peripheral is connected.");
    if (automatic) {
        Vector<BleCharacteristic> characteristics = discoverAllCharacteristics();
        for (auto& characteristic : characteristics) {
            characteristic.subscribe(true);
        }
    }
    return SYSTEM_ERROR_NONE;
}

int BlePeerDevice::connect(const BleAddress& addr, uint16_t interval, uint16_t latency, uint16_t timeout, bool automatic) {
    BleConnectionParams connParams;
    connParams.version = BLE_API_VERSION;
    connParams.size = sizeof(hal_ble_conn_params_t);
    connParams.min_conn_interval = interval;
    connParams.max_conn_interval = interval;
    connParams.slave_latency = latency;
    connParams.conn_sup_timeout = timeout;
    hal_ble_gap_set_ppcp(&connParams, nullptr);
    return connect(addr, &connParams, automatic);
}

int BlePeerDevice::connect(const BleAddress& addr, bool automatic) {
    return connect(addr, nullptr, automatic);
}

int BlePeerDevice::connect(const BleConnectionParams* params, bool automatic) {
    return connect(address(), params, automatic);
}

int BlePeerDevice::connect(uint16_t interval, uint16_t latency, uint16_t timeout, bool automatic) {
    return connect(address(), interval, latency, timeout, automatic);
}

int BlePeerDevice::connect(bool automatic) {
    return connect(address(), nullptr, automatic);
}

int BlePeerDevice::disconnect() const {
    WiringBleLock lk;
    CHECK_TRUE(connected(), SYSTEM_ERROR_INVALID_STATE);
    CHECK(hal_ble_gap_disconnect(impl()->connHandle(), nullptr));
    BleLocalDevice::getInstance().impl()->peers().removeOne(*this);
    /*
     * Only the connection handle is invalid. The service and characteristics being
     * discovered previously can be re-used next time once connected if needed.
     */
    impl()->connHandle() = BLE_INVALID_CONN_HANDLE;
    return SYSTEM_ERROR_NONE;
}

bool BlePeerDevice::connected() const {
    return impl()->connHandle() != BLE_INVALID_CONN_HANDLE;
}

void BlePeerDevice::bind(const BleAddress& address) const {
    impl()->address() = address;
}

BleAddress BlePeerDevice::address() const {
    return impl()->address();
}

bool BlePeerDevice::operator==(const BlePeerDevice& device) const {
    if (impl()->connHandle() == device.impl()->connHandle() && address() == device.address()) {
        return true;
    }
    return false;
}


/*******************************************************
 * BleLocalDevice class
 */
BleLocalDevice::BleLocalDevice()
        : impl_(std::make_unique<BleLocalDeviceImpl>()) {
    if (!impl()) {
        SPARK_ASSERT(false);
    }
    SPARK_ASSERT(hal_ble_stack_init(nullptr) == SYSTEM_ERROR_NONE);
    hal_ble_set_callback_on_periph_link_events(impl()->onBleLinkEvents, impl(), nullptr);
}

BleLocalDevice& BleLocalDevice::getInstance() {
    static BleLocalDevice instance;
    return instance;
}

void BleLocalDevice::onConnected(BleOnConnectedCallback callback, void* context) const {
    impl()->onConnectedCallback(callback, context);
}

void BleLocalDevice::onDisconnected(BleOnDisconnectedCallback callback, void* context) const {
    impl()->onDisconnectedCallback(callback, context);
}

int BleLocalDevice::begin() const {
    WiringBleLock lk;
    return SYSTEM_ERROR_NONE;
}

int BleLocalDevice::end() const {
    /*
     * 1. Disconnects all the connections initiated by user application.
     * 2. Disconnects Peripheral connection if it is not in the Listening mode.
     * 3. Stops advertising if it is not in the Listening mode.
     * 4. Stops scanning if initiated.
     *
     * FIXME: If device is broadcasting before entering the Listening mode and
     * then this API is called during device in the Listening mode, device will
     * restart broadcasting automatically when device exits the Listening mode.
     */
    WiringBleLock lk;
    disconnectAll(); // BLE HAL will guard that the Peripheral connection is remained if device is in the Listening mode.
    impl()->peers().clear();
    stopAdvertising(); // BLE HAL will guard that device keeps broadcasting if device is in the Listening mode.
    stopScanning();
    return SYSTEM_ERROR_NONE;
}

int BleLocalDevice::on() const {
    WiringBleLock lk;
    CHECK(hal_ble_stack_init(nullptr));
    return SYSTEM_ERROR_NONE;
}

int BleLocalDevice::off() const {
    WiringBleLock lk;
    CHECK(hal_ble_stack_deinit(nullptr));
    impl()->peers().clear();
    return SYSTEM_ERROR_NONE;
}

int BleLocalDevice::setAddress(const BleAddress& address) const {
    hal_ble_addr_t addr = address.halAddress();
    return hal_ble_gap_set_device_address(&addr, nullptr);
}

int BleLocalDevice::setAddress(const char* address, BleAddressType type) const {
    if (!address) {
        // Restores the default random static address.
        return hal_ble_gap_set_device_address(nullptr, nullptr);
    }
    BleAddress addr(address, type);
    return setAddress(addr);
}

int BleLocalDevice::setAddress(const String& address, BleAddressType type) const {
    BleAddress addr(address, type);
    return setAddress(addr);
}

BleAddress BleLocalDevice::address() const {
    hal_ble_addr_t halAddr = {};
    hal_ble_gap_get_device_address(&halAddr, nullptr);
    return BleAddress(halAddr);
}

int BleLocalDevice::setDeviceName(const char* name, size_t len) const {
    return hal_ble_gap_set_device_name(name, len, nullptr);
}

int BleLocalDevice::setDeviceName(const char* name) const {
    return setDeviceName(name, strnlen(name, BLE_MAX_DEV_NAME_LEN));
}

int BleLocalDevice::setDeviceName(const String& name) const {
    return setDeviceName(name.c_str());
}

ssize_t BleLocalDevice::getDeviceName(char* name, size_t len) const {
    CHECK(hal_ble_gap_get_device_name(name, len, nullptr));
    return strnlen(name, BLE_MAX_DEV_NAME_LEN);
}

String BleLocalDevice::getDeviceName() const {
    String name;
    char buf[BLE_MAX_DEV_NAME_LEN] = {};
    if (getDeviceName(buf, sizeof(buf)) > 0) {
        name.concat(buf);
    }
    return name;
}

int BleLocalDevice::setTxPower(int8_t txPower) const {
    WiringBleLock lk;
    return hal_ble_gap_set_tx_power(txPower, nullptr);
}

int BleLocalDevice::txPower(int8_t* txPower) const {
    WiringBleLock lk;
    return hal_ble_gap_get_tx_power(txPower, nullptr);
}

int BleLocalDevice::selectAntenna(BleAntennaType antenna) const {
    return hal_ble_select_antenna(static_cast<hal_ble_ant_type_t>(antenna), nullptr);
}

int BleLocalDevice::setAdvertisingInterval(uint16_t interval) const {
    WiringBleLock lk;
    hal_ble_adv_params_t advParams = {};
    advParams.size = sizeof(hal_ble_adv_params_t);
    CHECK(hal_ble_gap_get_advertising_parameters(&advParams, nullptr));
    advParams.interval = interval;
    return hal_ble_gap_set_advertising_parameters(&advParams, nullptr);
}

int BleLocalDevice::setAdvertisingTimeout(uint16_t timeout) const {
    WiringBleLock lk;
    hal_ble_adv_params_t advParams = {};
    advParams.size = sizeof(hal_ble_adv_params_t);
    CHECK(hal_ble_gap_get_advertising_parameters(&advParams, nullptr));
    advParams.timeout = timeout;
    return hal_ble_gap_set_advertising_parameters(&advParams, nullptr);
}

int BleLocalDevice::setAdvertisingType(BleAdvertisingEventType type) const {
    WiringBleLock lk;
    hal_ble_adv_params_t advParams = {};
    advParams.size = sizeof(hal_ble_adv_params_t);
    CHECK(hal_ble_gap_get_advertising_parameters(&advParams, nullptr));
    advParams.type = static_cast<hal_ble_adv_evt_type_t>(type);
    return hal_ble_gap_set_advertising_parameters(&advParams, nullptr);
}

int BleLocalDevice::setAdvertisingParameters(const BleAdvertisingParams* params) const {
    return hal_ble_gap_set_advertising_parameters(params, nullptr);
}

int BleLocalDevice::setAdvertisingParameters(uint16_t interval, uint16_t timeout, BleAdvertisingEventType type) const {
    WiringBleLock lk;
    hal_ble_adv_params_t advParams = {};
    advParams.size = sizeof(hal_ble_adv_params_t);
    CHECK(hal_ble_gap_get_advertising_parameters(&advParams, nullptr));
    advParams.interval = interval;
    advParams.timeout = timeout;
    advParams.type = static_cast<hal_ble_adv_evt_type_t>(type);
    return hal_ble_gap_set_advertising_parameters(&advParams, nullptr);
}

int BleLocalDevice::getAdvertisingParameters(BleAdvertisingParams* params) const {
    return hal_ble_gap_get_advertising_parameters(params, nullptr);
}

int BleLocalDevice::setAdvertisingData(BleAdvertisingData* advertisingData) const {
    if (advertisingData == nullptr) {
        return hal_ble_gap_set_advertising_data(nullptr, 0, nullptr);
    } else {
        return hal_ble_gap_set_advertising_data(advertisingData->data(), advertisingData->length(), nullptr);
    }
}

int BleLocalDevice::setScanResponseData(BleAdvertisingData* scanResponse) const {
    if (scanResponse == nullptr) {
        return hal_ble_gap_set_scan_response_data(nullptr, 0, nullptr);
    } else {
        scanResponse->remove(BleAdvertisingDataType::FLAGS);
        return hal_ble_gap_set_scan_response_data(scanResponse->data(), scanResponse->length(), nullptr);
    }
}

ssize_t BleLocalDevice::getAdvertisingData(BleAdvertisingData* advertisingData) const {
    WiringBleLock lk;
    if (advertisingData == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    return hal_ble_gap_get_advertising_data(advertisingData->data(), BLE_MAX_ADV_DATA_LEN, nullptr);
}

ssize_t BleLocalDevice::getScanResponseData(BleAdvertisingData* scanResponse) const {
    WiringBleLock lk;
    if (scanResponse == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    return hal_ble_gap_get_scan_response_data(scanResponse->data(), BLE_MAX_ADV_DATA_LEN, nullptr);
}

int BleLocalDevice::advertise() const {
    WiringBleLock lk;
    return hal_ble_gap_start_advertising(nullptr);
}

int BleLocalDevice::advertise(BleAdvertisingData* advertisingData, BleAdvertisingData* scanResponse) const {
    WiringBleLock lk;
    CHECK(setAdvertisingData(advertisingData));
    CHECK(setScanResponseData(scanResponse));
    return advertise();
}

int BleLocalDevice::advertise(const iBeacon& beacon) const {
    WiringBleLock lk;
    BleAdvertisingData* advData = new(std::nothrow) BleAdvertisingData(beacon);
    SCOPE_GUARD ({
        if (advData) {
            free(advData);
        }
    });
    CHECK_TRUE(advData, SYSTEM_ERROR_NO_MEMORY);
    CHECK(hal_ble_gap_set_advertising_data(advData->data(), advData->length(), nullptr));
    CHECK(setAdvertisingType(BleAdvertisingEventType::SCANABLE_UNDIRECTED));
    return advertise();
}

int BleLocalDevice::stopAdvertising() const {
    return hal_ble_gap_stop_advertising(nullptr);
}

bool BleLocalDevice::advertising() const {
    return hal_ble_gap_is_advertising(nullptr);
}

class BleScanDelegator {
public:
    BleScanDelegator()
            : resultsPtr_(nullptr),
              targetCount_(0),
              foundCount_(0),
              callback_(nullptr),
              context_(nullptr) {
        resultsVector_.clear();
    }

    ~BleScanDelegator() = default;

    int start(BleOnScanResultCallback callback, void* context) {
        callback_ = callback;
        context_ = context;
        CHECK(hal_ble_gap_start_scan(onScanResultCallback, this, nullptr));
        return foundCount_;
    }

    int start(BleScanResult* results, size_t resultCount) {
        resultsPtr_ = results;
        targetCount_ = resultCount;
        CHECK(hal_ble_gap_start_scan(onScanResultCallback, this, nullptr));
        return foundCount_;
    }

    Vector<BleScanResult> start() {
        hal_ble_gap_start_scan(onScanResultCallback, this, nullptr);
        return resultsVector_;
    }

private:
    /*
     * WARN: This is executed from HAL ble thread. The current thread which starts the scanning procedure
     * has acquired the BLE HAL lock. Calling BLE HAL APIs those acquiring the BLE HAL lock in this function
     * will suspend the execution of the callback until the current thread release the BLE HAL lock. Or the BLE HAL
     * APIs those are invoked here must not acquire the BLE HAL lock.
     */
    static void onScanResultCallback(const hal_ble_scan_result_evt_t* event, void* context) {
        BleScanDelegator* delegator = static_cast<BleScanDelegator*>(context);
        BleScanResult result = {};
        result.address = event->peer_addr;
        result.rssi = event->rssi;
        result.scanResponse.set(event->sr_data, event->sr_data_len);
        result.advertisingData.set(event->adv_data, event->adv_data_len);
        if (delegator->callback_) {
            delegator->callback_(&result, delegator->context_);
            delegator->foundCount_++;
            return;
        }
        if (delegator->resultsPtr_) {
            if (delegator->foundCount_ < delegator->targetCount_) {
                delegator->resultsPtr_[delegator->foundCount_++] = result;
                if (delegator->foundCount_ >= delegator->targetCount_) {
                    hal_ble_gap_stop_scan(nullptr);
                }
            }
            return;
        }
        if (delegator->resultsVector_.append(result)) {
            delegator->foundCount_++;
        }
    }

    Vector<BleScanResult> resultsVector_;
    BleScanResult* resultsPtr_;
    size_t targetCount_;
    size_t foundCount_;
    BleOnScanResultCallback callback_;
    void* context_;
};

int BleLocalDevice::setScanTimeout(uint16_t timeout) const {
    WiringBleLock lk;
    hal_ble_scan_params_t scanParams = {};
    scanParams.size = sizeof(hal_ble_scan_params_t);
    hal_ble_gap_get_scan_parameters(&scanParams, nullptr);
    scanParams.timeout = timeout;
    return hal_ble_gap_set_scan_parameters(&scanParams, nullptr);
}

int BleLocalDevice::setScanParameters(const BleScanParams* params) const {
    WiringBleLock lk;
    return hal_ble_gap_set_scan_parameters(params, nullptr);
}

int BleLocalDevice::getScanParameters(BleScanParams* params) const {
    WiringBleLock lk;
    return hal_ble_gap_get_scan_parameters(params, nullptr);
}

int BleLocalDevice::scan(BleOnScanResultCallback callback, void* context) const {
    WiringBleLock lk;
    BleScanDelegator scanner;
    return scanner.start(callback, context);
}

int BleLocalDevice::scan(BleScanResult* results, size_t resultCount) const {
    WiringBleLock lk;
    if (results == nullptr || resultCount == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    BleScanDelegator scanner;
    return scanner.start(results, resultCount);
}

Vector<BleScanResult> BleLocalDevice::scan() const {
    WiringBleLock lk;
    BleScanDelegator scanner;
    return scanner.start();
}

int BleLocalDevice::stopScanning() const {
    return hal_ble_gap_stop_scan(nullptr);
}

int BleLocalDevice::setPPCP(uint16_t minInterval, uint16_t maxInterval, uint16_t latency, uint16_t timeout) const {
    WiringBleLock lk;
    hal_ble_conn_params_t ppcp = {};
    ppcp.size = sizeof(hal_ble_conn_params_t);
    ppcp.min_conn_interval = minInterval;
    ppcp.max_conn_interval = maxInterval;
    ppcp.slave_latency = latency;
    ppcp.conn_sup_timeout = timeout;
    return hal_ble_gap_set_ppcp(&ppcp, nullptr);
}

bool BleLocalDevice::connected() const {
    return (impl()->peers().size() > 0);
}

BlePeerDevice BleLocalDevice::connect(const BleAddress& addr, const BleConnectionParams* params, bool automatic) const {
    // Do not lock here. This thread is guarded by BLE HAL lock. But it allows the BLE thread to access the wiring data.
    BlePeerDevice peer;
    peer.connect(addr, params, automatic);
    return peer;
}

BlePeerDevice BleLocalDevice::connect(const BleAddress& addr, uint16_t interval, uint16_t latency, uint16_t timeout, bool automatic) const {
    // Do not lock here. This thread is guarded by BLE HAL lock. But it allows the BLE thread to access the wiring data.
    BlePeerDevice peer;
    peer.connect(addr, interval, latency, timeout, automatic);
    return peer;
}

BlePeerDevice BleLocalDevice::connect(const BleAddress& addr, bool automatic) const {
    // Do not lock here. This thread is guarded by BLE HAL lock. But it allows the BLE thread to access the wiring data.
    return connect(addr, nullptr, automatic);
}

int BleLocalDevice::disconnect() const {
    WiringBleLock lk;
    for (auto& p : impl()->peers()) {
        hal_ble_conn_info_t connInfo = {};
        CHECK(hal_ble_gap_get_connection_info(p.impl()->connHandle(), &connInfo, nullptr));
        if (connInfo.role == BLE_ROLE_PERIPHERAL) {
            CHECK(hal_ble_gap_disconnect(p.impl()->connHandle(), nullptr));
            impl()->peers().removeOne(p);
            return SYSTEM_ERROR_NONE;
        }
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int BleLocalDevice::disconnect(const BlePeerDevice& peer) const {
    WiringBleLock lk;
    return peer.disconnect();
}

int BleLocalDevice::disconnectAll() const {
    WiringBleLock lk;
    for (auto& p : impl()->peers()) {
        p.disconnect();
    }
    return SYSTEM_ERROR_NONE;
}

BleCharacteristic BleLocalDevice::addCharacteristic(const BleCharacteristic& characteristic) {
    WiringBleLock lk;
    auto charImpl = characteristic.impl();
    if (charImpl->properties() == BleCharacteristicProperty::NONE) {
        return characteristic;
    }
    for (const auto& existChar : impl()->characteristics()) {
        if (*charImpl == *existChar.impl()) {
            LOG(ERROR, "Duplicated characteristic cannot be added.");
            return characteristic;
        }
    }
    // If the service that the characteristic belongs to is not specified,
    // put the characteristic to the default service.
    if (!charImpl->svcUUID().isValid()) {
        LOG_DEBUG(TRACE, "Assign default service UUID.");
        charImpl->svcUUID() = PARTICLE_DEFAULT_BLE_SVC_UUID;
    }
    BleService* service = nullptr;
    for (auto& svc : impl()->services()) {
        if (svc.impl()->UUID() == charImpl->svcUUID()) {
            service = &svc;
            break;
        }
    }
    // Add service as needed.
    if (!service) {
        BleService svc(charImpl->svcUUID());
        hal_ble_uuid_t halUuid = charImpl->svcUUID().halUUID();
        if (hal_ble_gatt_server_add_service(BLE_SERVICE_TYPE_PRIMARY, &halUuid, &svc.impl()->startHandle(), nullptr) != SYSTEM_ERROR_NONE) {
            return characteristic;
        }
        if(!impl()->services().append(svc)) {
            return characteristic;
        }
        service = &impl()->services().last();
    }
    charImpl->assignUuidIfNeeded();
    hal_ble_char_init_t charInit = {};
    charInit.version = BLE_API_VERSION;
    charInit.size = sizeof(hal_ble_char_init_t);
    charInit.uuid = charImpl->charUUID().halUUID();
    charInit.properties = static_cast<uint8_t>(charImpl->properties());
    charInit.service_handle = service->impl()->startHandle();
    charInit.description = charImpl->description().c_str();
    charInit.callback = charImpl->onBleCharEvents;
    charInit.context = charImpl;
    if (hal_ble_gatt_server_add_characteristic(&charInit, &charImpl->attrHandles(), nullptr) != SYSTEM_ERROR_NONE) {
        return characteristic;
    }
    charImpl->local() = true;
    LOG_DEBUG(TRACE, "Add new local characteristic.");
    if(!impl()->characteristics().append(characteristic)) {
        LOG(ERROR, "Failed to append local characteristic.");
    }
    return characteristic;
}

BleCharacteristic BleLocalDevice::addCharacteristic(const char* desc, BleCharacteristicProperty properties, BleOnDataReceivedCallback callback, void* context) {
    WiringBleLock lk;
    BleCharacteristic characteristic(desc, properties, callback, context);
    addCharacteristic(characteristic);
    return characteristic;
}

BleCharacteristic BleLocalDevice::addCharacteristic(const String& desc, BleCharacteristicProperty properties, BleOnDataReceivedCallback callback, void* context) {
    WiringBleLock lk;
    return addCharacteristic(desc.c_str(), properties, callback, context);
}

} /* namespace particle */

#endif /* Wiring_BLE */
