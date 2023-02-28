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
using namespace std::placeholders;

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
    clear();
    address_.addr_type = BLE_SIG_ADDR_TYPE_PUBLIC;
}

BleAddress::BleAddress(const BleAddress& addr) {
    address_ = addr.address_;
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

int BleAddress::set(const hal_ble_addr_t& addr) {
    address_ = addr;
    return SYSTEM_ERROR_NONE;
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

BleAddress& BleAddress::operator=(const BleAddress& addr) {
    address_ = addr.address_;
    return *this;
}

BleAddress& BleAddress::operator=(const hal_ble_addr_t& addr) {
    return *this = BleAddress(addr);
}

BleAddress& BleAddress::operator=(const uint8_t addr[BLE_SIG_ADDR_LEN]) {
    // We just update the address value while keep the address type
    memcpy(address_.addr, addr, BLE_SIG_ADDR_LEN);
    return *this;
}

BleAddress& BleAddress::operator=(const char* address) {
    // We just update the address value while keep the address type
    set(address, static_cast<BleAddressType>(address_.addr_type));
    return *this;
}

BleAddress& BleAddress::operator=(const String& address) {
    *this = address.c_str();
    return *this;
}

bool BleAddress::operator==(const BleAddress& addr) const {
    if (address_.addr_type == addr.address_.addr_type && !memcmp(address_.addr, addr.address_.addr, BLE_SIG_ADDR_LEN)) {
        return true;
    }
    return false;
}

bool BleAddress::operator==(const hal_ble_addr_t& addr) const {
    return *this == BleAddress(addr);
}

bool BleAddress::operator==(const uint8_t addr[BLE_SIG_ADDR_LEN]) const {
    // The operator intends to compare the value only.
    return !memcmp(address_.addr, addr, BLE_SIG_ADDR_LEN);
}

bool BleAddress::operator==(const char* address) const {
    // The operator intends to compare the value only.
    return toString() == String(address);
}

bool BleAddress::operator==(const String& address) const {
    // The operator intends to compare the value only.
    return toString() == address;
}

bool BleAddress::operator!=(const BleAddress& addr) const {
    return !(*this == addr);
}

bool BleAddress::operator!=(const hal_ble_addr_t& addr) const {
    return !(*this == BleAddress(addr));
}

bool BleAddress::operator!=(const uint8_t addr[BLE_SIG_ADDR_LEN]) const {
    // The operator intends to compare the value only.
    return memcmp(address_.addr, addr, BLE_SIG_ADDR_LEN);
}

bool BleAddress::operator!=(const char* address) const {
    // The operator intends to compare the value only.
    return toString() != String(address);
}

bool BleAddress::operator!=(const String& address) const {
    // The operator intends to compare the value only.
    return toString() != address;
}

bool BleAddress::isValid() const {
    // Reference: Bluetooth Core v5.0, Vol 6, Part B, Section 1.3, Device Address.
    constexpr uint8_t bitsClear[BLE_SIG_ADDR_LEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    constexpr uint8_t bitsSet[BLE_SIG_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    if (address_.addr_type == BLE_SIG_ADDR_TYPE_PUBLIC) {
        return memcmp(address_.addr, bitsClear, BLE_SIG_ADDR_LEN) && memcmp(address_.addr, bitsSet, BLE_SIG_ADDR_LEN);
    } else {
        uint8_t temp[BLE_SIG_ADDR_LEN];
        memcpy(temp, address_.addr, BLE_SIG_ADDR_LEN);
        if (address_.addr_type == BLE_SIG_ADDR_TYPE_RANDOM_STATIC || address_.addr_type == BLE_SIG_ADDR_TYPE_RANDOM_PRIVATE_NON_RESOLVABLE) {
            temp[5] &= 0x3F; // Clear the two most significant bits
            CHECK_TRUE(memcmp(temp, bitsClear, BLE_SIG_ADDR_LEN), false);
            temp[5] |= 0xC0; // Set the two most significant bits
            CHECK_TRUE(memcmp(temp, bitsSet, BLE_SIG_ADDR_LEN), false);
            if (address_.addr_type == BLE_SIG_ADDR_TYPE_RANDOM_STATIC) {
                return (address_.addr[5] & 0xC0) == 0xC0;
            } else {
                return (address_.addr[5] & 0xC0) == 0x00;
            }
        } else if (address_.addr_type == BLE_SIG_ADDR_TYPE_RANDOM_PRIVATE_RESOLVABLE) {
            temp[5] &= 0x3F;
            CHECK_TRUE(memcmp(&temp[3], &bitsClear[3], 3), false);
            temp[5] |= 0xC0;
            CHECK_TRUE(memcmp(&temp[3], &bitsSet[3], 3), false);
            return (address_.addr[5] & 0xC0) == 0x40;
        }
    }
    // Other address type
    return true;
}

int BleAddress::clear() {
    memset(address_.addr, 0xFF, BLE_SIG_ADDR_LEN);
    return SYSTEM_ERROR_NONE;
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
        : type_(BleUuidType::LONG) {
    memcpy(uuid128_, BASE_UUID, BLE_SIG_UUID_128BIT_LEN);
}

BleUuid::BleUuid(const hal_ble_uuid_t& uuid) {
    if (uuid.type == BLE_UUID_TYPE_16BIT || uuid.type == BLE_UUID_TYPE_128BIT_SHORTED) {
        type_ = BleUuidType::SHORT;
        memcpy(uuid128_, BASE_UUID, BLE_SIG_UUID_128BIT_LEN);
        uuid128_[UUID16_LO] = (uint8_t)uuid.uuid16;
        uuid128_[UUID16_HI] = (uint8_t)(uuid.uuid16 >> 8);
    } else {
        type_ = BleUuidType::LONG;
        memcpy(uuid128_, uuid.uuid128, BLE_SIG_UUID_128BIT_LEN);
    }
}

BleUuid::BleUuid(const uint8_t* uuid128, BleUuidOrder order) {
    if (uuid128) {
        if (order == BleUuidOrder::LSB) {
            memcpy(uuid128_, uuid128, BLE_SIG_UUID_128BIT_LEN);
        } else {
            for (uint8_t i = 0, j = BLE_SIG_UUID_128BIT_LEN - 1; i < BLE_SIG_UUID_128BIT_LEN; i++, j--) {
                uuid128_[i] = uuid128[j];
            }
        }
        type_ = BleUuidType::LONG;
    } else {
        memset(uuid128_, 0x00, BLE_SIG_UUID_128BIT_LEN);
    }
}

BleUuid::BleUuid(uint16_t uuid16)
        : BleUuid() {
    type_ = BleUuidType::SHORT;
    uuid128_[UUID16_LO] = (uint8_t)(uuid16 & 0x00FF);
    uuid128_[UUID16_HI] = (uint8_t)((uuid16 >> 8) & 0x00FF);
}

BleUuid::BleUuid(const uint8_t* uuid128, uint16_t uuid16, BleUuidOrder order)
        : BleUuid(uuid128, order) {
    uuid128_[UUID16_LO] = (uint8_t)(uuid16 & 0x00FF);
    uuid128_[UUID16_HI] = (uint8_t)((uuid16 >> 8) & 0x00FF);
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
        return (uuid128_[UUID16_LO] != 0x00 || uuid128_[UUID16_HI] != 0x00);
    } else {
        return memcmp(uuid128_, BASE_UUID, BLE_SIG_UUID_128BIT_LEN);
    }
}

BleUuidType BleUuid::type() const {
    return type_;
}

hal_ble_uuid_t BleUuid::halUUID() {
    hal_ble_uuid_t uuid = {};
    if (type_ == BleUuidType::SHORT) {
        uuid.type = BLE_UUID_TYPE_16BIT;
        uuid.uuid16 = (uint16_t)uuid128_[UUID16_LO] | ((uint16_t)uuid128_[UUID16_HI] << 8);
    } else {
        uuid.type = BLE_UUID_TYPE_128BIT;
        memcpy(uuid.uuid128, uuid128_, BLE_SIG_UUID_128BIT_LEN);
    }
    return uuid;
}

uint16_t BleUuid::shorted() const {
    return ((uint16_t)uuid128_[UUID16_LO] | ((uint16_t)uuid128_[UUID16_HI] << 8));
}

size_t BleUuid::rawBytes(uint8_t uuid128[BLE_SIG_UUID_128BIT_LEN]) const {
    memcpy(uuid128, uuid128_, BLE_SIG_UUID_128BIT_LEN);
    return BLE_SIG_UUID_128BIT_LEN;
}

const uint8_t* BleUuid::rawBytes() const {
    return uuid128_;
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
        uint16_t bigEndian = (uint16_t)uuid128_[UUID16_LO] << 8 | uuid128_[UUID16_HI];
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

bool BleUuid::operator==(const BleUuid& uuid) const {
    return ((type_ == uuid.type_) && !memcmp(uuid128_, uuid.uuid128_, BLE_SIG_UUID_128BIT_LEN));
}

uint8_t BleUuid::operator[](uint8_t i) const {
    if (i >= BLE_SIG_UUID_128BIT_LEN) {
        return 0;
    }
    return uuid128_[i];
}

void BleUuid::construct(const char* uuid) {
    type_ = BleUuidType::LONG;
    memcpy(uuid128_, BASE_UUID, BLE_SIG_UUID_128BIT_LEN);
    if (uuid == nullptr) {
        return;
    }
    if (strnlen(uuid, BLE_SIG_UUID_128BIT_LEN * 2 + 4) == (BLE_SIG_UUID_16BIT_LEN * 2)) {
        char buf[2] = {};
        if (hexToBytes(uuid, buf, BLE_SIG_UUID_16BIT_LEN) == BLE_SIG_UUID_16BIT_LEN) {
            uuid128_[UUID16_LO] = buf[1];
            uuid128_[UUID16_HI] = buf[0];
        }
        type_ = BleUuidType::SHORT;
        return;
    }
    size_t len = BLE_SIG_UUID_128BIT_LEN;
    for (size_t i = 0; i < strnlen(uuid, BLE_SIG_UUID_128BIT_LEN * 2 + 4) && len > 0; i++) {
        int8_t hi = hexToNibble(uuid[i]);
        if (hi >= 0) {
            uuid128_[len - 1] = hi << 4;
            if (++i < strnlen(uuid, BLE_SIG_UUID_128BIT_LEN * 2 + 4)) {
                int8_t lo = hexToNibble(uuid[i]);
                if (lo >= 0) {
                    uuid128_[len - 1] |= lo;
                }
            }
            len--;
        }
    }
    while (len > 0) {
        uuid128_[len - 1] = 0x00;
        len--;
    }
}

void BleUuid::toBigEndian(uint8_t buf[BLE_SIG_UUID_128BIT_LEN]) const {
    for (uint8_t i = 0, j = BLE_SIG_UUID_128BIT_LEN - 1; i < BLE_SIG_UUID_128BIT_LEN; i++, j--) {
        buf[i] = uuid128_[j];
    }
}

constexpr uint8_t BleUuid::BASE_UUID[BLE_SIG_UUID_128BIT_LEN];
constexpr uint8_t BleUuid::UUID16_LO;
constexpr uint8_t BleUuid::UUID16_HI;


/*******************************************************
 * BleAdvertisingData class
 */
BleAdvertisingData::BleAdvertisingData()
        : selfData_() {
    uint8_t flag = BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    append(BleAdvertisingDataType::FLAGS, &flag, sizeof(uint8_t));
}

BleAdvertisingData::BleAdvertisingData(const iBeacon& beacon)
        : BleAdvertisingData() {
    set(beacon);
}

size_t BleAdvertisingData::set(const uint8_t* buf, size_t len) {
    if (buf == nullptr || len == 0) {
        selfData_.clear();
        return selfData_.size();
    }
    selfData_.clear();
    len = std::min(len, (size_t)BLE_MAX_ADV_DATA_LEN_EXT);
    CHECK_TRUE(selfData_.append(buf, len), 0);
    return selfData_.size();
}

size_t BleAdvertisingData::set(const iBeacon& beacon) {
    clear();

    if (beacon.UUID().type() == BleUuidType::SHORT) {
        return selfData_.size();
    }

    CHECK_TRUE(selfData_.reserve(BLE_MAX_ADV_DATA_LEN), 0);
    selfData_.append(0x02);
    selfData_.append(BLE_SIG_AD_TYPE_FLAGS);
    selfData_.append(BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    selfData_.append(0x1a); // length
    selfData_.append(BLE_SIG_AD_TYPE_MANUFACTURER_SPECIFIC_DATA);
    // Company ID
    uint16_t companyId = iBeacon::APPLE_COMPANY_ID;
    selfData_.append((const uint8_t *)&companyId, 2);
    // Beacon type: iBeacon
    selfData_.append(iBeacon::BEACON_TYPE_IBEACON);
    // Length of the following payload
    selfData_.append(0x15);
    // 128-bits Beacon UUID, MSB
    for (size_t i = 0, j = BLE_SIG_UUID_128BIT_LEN - 1; i < BLE_SIG_UUID_128BIT_LEN; i++, j--) {
        selfData_.append(beacon.UUID().rawBytes()[j]);
    }
    // Major, MSB
    selfData_.append((uint8_t)((beacon.major() >> 8) & 0x00FF));
    selfData_.append((uint8_t)(beacon.major() & 0x00FF));
    // Minor, MSB
    selfData_.append((uint8_t)((beacon.minor() >> 8) & 0x00FF));
    selfData_.append((uint8_t)(beacon.minor() & 0x00FF));
    // Measure power
    selfData_.append(beacon.measurePower());
    selfData_.trimToSize();

    return selfData_.size();
}

size_t BleAdvertisingData::append(BleAdvertisingDataType type, const uint8_t* buf, size_t len, bool force) {
    if (buf == nullptr) {
        return selfData_.size();
    }
    size_t offset;
    size_t adsLen = locate(selfData_.data(), selfData_.size(), type, &offset);
    if (!force && adsLen > 0) {
        // Update the existing AD structure.
        if ((selfData_.size() - adsLen + len + 2) <= BLE_MAX_ADV_DATA_LEN_EXT) {
            // Firstly, remove the existing AD structure.
            selfData_.removeAt(offset, adsLen);
            // Secondly, Update the AD structure.
            // Reserve the RAM in the vector, and return if it fails
            CHECK_TRUE(selfData_.reserve(selfData_.size() + len + 2), selfData_.size());
            // The Length field is the total length of Type field and Data field.
            selfData_.insert(offset, len + 1);
            // An AD structure is composed of one byte length field, one byte Type field and Data field.
            selfData_.insert(offset + 1, static_cast<uint8_t>(type));
            selfData_.insert(offset + 2, buf, len);
        }
    }
    else if ((selfData_.size() + len + 2) <= BLE_MAX_ADV_DATA_LEN_EXT) {
        // Append the AD structure at the and of advertising data.
        CHECK_TRUE(selfData_.reserve(selfData_.size() + len + 2), selfData_.size());
        selfData_.append(len + 1);
        selfData_.append(static_cast<uint8_t>(type));
        selfData_.append(buf, len);
    }
    return selfData_.size();
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

size_t BleAdvertisingData::appendAppearance(ble_sig_appearance_t appearance) {
    return append(BleAdvertisingDataType::APPEARANCE, (const uint8_t*)&appearance, 2, false);
}

size_t BleAdvertisingData::resize(size_t size) {
    selfData_.resize(std::min(size, (size_t)BLE_MAX_ADV_DATA_LEN_EXT));
    return selfData_.size();
}

void BleAdvertisingData::clear() {
    selfData_.clear();
}

void BleAdvertisingData::remove(BleAdvertisingDataType type) {
    size_t offset, len;
    len = locate(selfData_.data(), selfData_.size(), type, &offset);
    if (len > 0) {
        selfData_.removeAt(offset, len);
        // Recursively remove duplicated type.
        remove(type);
    }
}

size_t BleAdvertisingData::get(uint8_t* buf, size_t len) const {
    if (buf != nullptr) {
        len = std::min(len, (size_t)selfData_.size());
        memcpy(buf, selfData_.data(), len);
        return len;
    }
    return selfData_.size();
}

size_t BleAdvertisingData::get(BleAdvertisingDataType type, uint8_t* buf, size_t len) const {
    size_t offset;
    size_t adsLen = locate(selfData_.data(), selfData_.size(), type, &offset);
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
    return selfData_.data();
}

size_t BleAdvertisingData::length() const {
    return selfData_.size();
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
    const Vector<BleUuid>& foundUuids = serviceUUID();
    for (const auto& uuid : foundUuids) {
        if (found >= count) {
            break;
        }
        uuids[found++] = uuid;
    }
    return found;
}

Vector<BleUuid> BleAdvertisingData::serviceUUID() const {
    Vector<BleUuid> foundUuids;
    foundUuids.append(serviceUUID(BleAdvertisingDataType::SERVICE_UUID_16BIT_MORE_AVAILABLE));
    foundUuids.append(serviceUUID(BleAdvertisingDataType::SERVICE_UUID_16BIT_MORE_AVAILABLE));
    foundUuids.append(serviceUUID(BleAdvertisingDataType::SERVICE_UUID_16BIT_COMPLETE));
    foundUuids.append(serviceUUID(BleAdvertisingDataType::SERVICE_UUID_128BIT_MORE_AVAILABLE));
    foundUuids.append(serviceUUID(BleAdvertisingDataType::SERVICE_UUID_128BIT_COMPLETE));
    return foundUuids;
}

size_t BleAdvertisingData::customData(uint8_t* buf, size_t len) const {
    return get(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA, buf, len);
}

uint8_t BleAdvertisingData::operator[](uint8_t i) const {
    if (i >= selfData_.size()) {
        return 0;
    }
    return selfData_[i];
}

ble_sig_appearance_t BleAdvertisingData::appearance() const {
    uint8_t buf[2];
    size_t len = get(BleAdvertisingDataType::APPEARANCE, buf, sizeof(buf));
    if (len > 0) {
        uint16_t temp = (uint16_t)buf[1] << 8 | buf[0];
        return (ble_sig_appearance_t)temp;
    }
    return BLE_SIG_APPEARANCE_UNKNOWN;
}

bool BleAdvertisingData::contains(BleAdvertisingDataType type) const {
    size_t adsOffset;
    return locate(selfData_.data(), selfData_.size(), type, &adsOffset) > 0;
}

Vector<BleUuid> BleAdvertisingData::serviceUUID(BleAdvertisingDataType type) const {
    Vector<BleUuid> uuids;
    size_t offset, adsLen = 0;
    for (int i = 0; i < selfData_.size(); i += (offset + adsLen)) {
        adsLen = locate(&selfData_[i], selfData_.size() - i, type, &offset);
        if (adsLen > 0) {
            if (type == BleAdvertisingDataType::SERVICE_UUID_16BIT_MORE_AVAILABLE || type == BleAdvertisingDataType::SERVICE_UUID_16BIT_COMPLETE) {
                for(size_t array = 0; (array < (adsLen - 2) / BLE_SIG_UUID_16BIT_LEN); array++) {
                    BleUuid uuid = (uint16_t)selfData_[i + offset + array * BLE_SIG_UUID_16BIT_LEN + 2] | ((uint16_t)selfData_[i + offset + array * BLE_SIG_UUID_16BIT_LEN + 3] << 8);
                    uuids.append(uuid);
                }
            } else if (type == BleAdvertisingDataType::SERVICE_UUID_128BIT_MORE_AVAILABLE || type == BleAdvertisingDataType::SERVICE_UUID_128BIT_COMPLETE) {
                for(size_t array = 0; (array < (adsLen - 2) / BLE_SIG_UUID_128BIT_LEN); array++) {
                    BleUuid uuid = &selfData_[i + offset + array * BLE_SIG_UUID_128BIT_LEN + 2];
                    uuids.append(uuid);
                }
            }
            continue;
        }
        break;
    }
    return uuids;
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
              dataReceivedCallback_(nullptr) {
    }

    BleCharacteristicImpl(EnumFlags<BleCharacteristicProperty> properties, const char* desc, BleOnDataReceivedCallback callback, void* context)
            : BleCharacteristicImpl() {
        properties_ = properties;
        description_ = desc;
        dataReceivedCallback_ = callback ? std::bind(callback, _1, _2, _3, context) : (BleOnDataReceivedStdFunction)nullptr;
    }

    BleCharacteristicImpl(EnumFlags<BleCharacteristicProperty> properties, const char* desc, const BleOnDataReceivedStdFunction& callback)
            : BleCharacteristicImpl() {
        properties_ = properties;
        description_ = desc;
        dataReceivedCallback_ = callback;
    }

    BleCharacteristicImpl(const char* desc, EnumFlags<BleCharacteristicProperty> properties, BleUuid& charUuid, BleUuid& svcUuid, BleOnDataReceivedCallback callback, void* context)
            : BleCharacteristicImpl(properties, desc, callback, context) {
        charUuid_ = charUuid;
        svcUuid_ = svcUuid;
    }

    BleCharacteristicImpl(const char* desc, EnumFlags<BleCharacteristicProperty> properties, BleUuid& charUuid, BleUuid& svcUuid, const BleOnDataReceivedStdFunction& callback)
            : BleCharacteristicImpl(properties, desc, callback) {
        charUuid_ = charUuid;
        svcUuid_ = svcUuid;
    }

    ~BleCharacteristicImpl() = default;

    bool isLocal() {
        return isLocal_;
    }

    void isLocal(bool local) {
        isLocal_ = local;
    }

    BleConnectionHandle& connHandle() {
        return connHandle_;
    }

    EnumFlags<BleCharacteristicProperty>& properties() {
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
        dataReceivedCallback_ = callback ? std::bind(callback, _1, _2, _3, context) : (BleOnDataReceivedStdFunction)nullptr;
    }

    void setCallback(const BleOnDataReceivedStdFunction& callback) {
        dataReceivedCallback_ = callback;
    }

    void inheritCallback(BleCharacteristicImpl& charImpl) {
        if (charImpl.dataReceivedCallback_) {
            dataReceivedCallback_ = charImpl.dataReceivedCallback_;
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
    EnumFlags<BleCharacteristicProperty> properties_;
    BleCharacteristicHandles attrHandles_;
    BleUuid charUuid_;
    BleUuid svcUuid_;
    String description_;
    static uint16_t defaultUuidCharCount_;
    BleOnDataReceivedStdFunction dataReceivedCallback_;
};


/*******************************************************
 * BleServiceImpl definition
 */
class BleServiceImpl {
public:
    BleServiceImpl()
            : uuid_(),
              startHandle_(BLE_INVALID_ATTR_HANDLE),
              endHandle_(BLE_INVALID_ATTR_HANDLE),
              characteristicsDiscovered_(false) {
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

    bool characteristicsDiscovered() {
        return characteristicsDiscovered_;
    }

    void characteristicsDiscovered(bool discovered) {
        characteristicsDiscovered_ = discovered;
    }

    bool hasCharacteristic(const BleCharacteristic& characteristic) {
        if ( characteristic.impl()->svcUUID() == uuid_ && 
              characteristic.impl()->attrHandles().value_handle >= startHandle_ &&
              characteristic.impl()->attrHandles().value_handle <= endHandle_) {
            return true;
        }
        return false;
    }

private:
    BleUuid uuid_;
    BleAttributeHandle startHandle_;
    BleAttributeHandle endHandle_;
    bool characteristicsDiscovered_; // For peer service only
};


/*******************************************************
 * BlePeerDeviceImpl definition
 */
class BlePeerDeviceImpl {
public:
    BlePeerDeviceImpl()
            : connHandle_(BLE_INVALID_CONN_HANDLE),
              address_(),
              servicesDiscovered_(false) {
    }

    ~BlePeerDeviceImpl() = default;

    BleConnectionHandle& connHandle() {
        return connHandle_;
    }

    BleAddress& address() {
        return address_;
    }

    bool servicesDiscovered() {
        return servicesDiscovered_;
    }

    void servicesDiscovered(bool discovered) {
        servicesDiscovered_ = discovered;
    }

    Vector<BleService>& services() {
        return services_;
    }

    Vector<BleCharacteristic>& characteristics() {
        return characteristics_;
    }

    bool locateService(BleService& service, BleCharacteristicHandles handles) {
        for (const auto& svc : services_) {
            if (handles.value_handle <= svc.impl()->endHandle() && handles.value_handle >= svc.impl()->startHandle()) {
                service = svc;
                return true;
            }
        }
        return false;
    }

    void onDisconnected() {
        connHandle_ = BLE_INVALID_CONN_HANDLE;
        for (auto& characteristic : characteristics()) {
            characteristic.impl()->connHandle() = BLE_INVALID_CONN_HANDLE;
        }
        services_.clear();
        characteristics_.clear();
        servicesDiscovered_ = false;
    }

private:
    BleConnectionHandle connHandle_;
    BleAddress address_;
    bool servicesDiscovered_;
    Vector<BleService> services_;
    Vector<BleCharacteristic> characteristics_;
};


/*******************************************************
 * BleLocalDeviceImpl definition
 */
class BleLocalDeviceImpl {
public:
    BleLocalDeviceImpl()
            : connectedCallback_(nullptr),
              disconnectedCallback_(nullptr),
              pairingEventCallback_(nullptr) {
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

#if HAL_PLATFORM_BLE_ACTIVE_EVENT
    BlePeerDevice& connectingPeer() {
        return connectingPeer_;
    }

    BlePeerDevice& disconnectingPeer() {
        return disconnectingPeer_;
    }
#endif

    void onConnectedCallback(BleOnConnectedCallback callback, void* context) {
        connectedCallback_ = callback ? std::bind(callback, _1, context) : (BleOnConnectedStdFunction)nullptr;
    }

    void onConnectedCallback(const BleOnConnectedStdFunction& callback) {
        connectedCallback_ = callback;
    }

    void onDisconnectedCallback(BleOnDisconnectedCallback callback, void* context) {
        disconnectedCallback_ = callback ? std::bind(callback, _1, context) : (BleOnDisconnectedStdFunction)nullptr;
    }

    void onDisconnectedCallback(const BleOnDisconnectedStdFunction& callback) {
        disconnectedCallback_ = callback;
    }

    void onPairingEvent(BleOnPairingEventCallback callback, void* context) {
        pairingEventCallback_ = callback ? std::bind(callback, _1, context) : (BleOnPairingEventStdFunction)nullptr;
    }

    void onPairingEvent(const BleOnPairingEventStdFunction& callback) {
        pairingEventCallback_ = callback;
    }

    void onAttMtuExchangedCallback(BleOnAttMtutExchangedCallback callback, void* context) {
        attMtuExchangedCallback_ = callback ? std::bind(callback, _1, _2, context) : (BleOnAttMtuExchangedStdFunction)nullptr;
    }

    void onAttMtuExchangedCallback(const BleOnAttMtuExchangedStdFunction& callback) {
        attMtuExchangedCallback_ = callback;
    }

    BlePeerDevice* findPeerDevice(BleConnectionHandle connHandle) {
        for (auto& peer : peers_) {
            if (peer.impl()->connHandle() == connHandle) {
                return &peer;
            }
        }
        return nullptr;
    }

    /*
     * WARN: Calling BLE HAL APIs those acquiring the BLE HAL lock in this function
     * will suspend the execution of the callback until the current thread release the BLE HAL lock. Or the BLE HAL
     * APIs those are invoked here must not acquire the BLE HAL lock. Otherwise, the HAL BLE thread cannot process 
     * SoftDevice events in queue.
     */
    static void onBleLinkEvents(const hal_ble_link_evt_t* event, void* context) {
        auto impl = static_cast<BleLocalDeviceImpl*>(context);
        WiringBleLock lk;
        switch (event->type) {
            case BLE_EVT_CONNECTED: {
#if HAL_PLATFORM_BLE_ACTIVE_EVENT
                if (impl->connectingPeer_.impl()->address().isValid()) {
                    impl->connectingPeer_.impl()->connHandle() = event->conn_handle;
                    impl->peers_.append(impl->connectingPeer_);
                    impl->connectingPeer_ = {};
                } else
#endif
                {
                    BlePeerDevice peer;
                    peer.impl()->connHandle() = event->conn_handle;
                    peer.impl()->address() = event->params.connected.info->address;
                    if (!impl->peers_.append(peer)) {
                        LOG(TRACE, "Failed to append peer Central device.");
                        // FIXME: It will acquire the BLE HAL lock. If there is a thread currently invoking a
                        // BLOCKING BLE HAL API, which means that that API has acquired the BLE HAL lock and relying
                        // on the HAL BLE thread to unblock that API, while acquiring the BLE HAL lock here will prevent
                        // the HAL BLE thread from dealing with incoming event to unblock that API, dead lock happens.
                        hal_ble_gap_disconnect(peer.impl()->connHandle(), nullptr);
                        return;
                    }
                    if (impl->connectedCallback_) {
                        impl->connectedCallback_(peer);
                    }
                }
                LOG(TRACE, "Connected");
                break;
            }
            case BLE_EVT_DISCONNECTED: {
                BlePeerDevice* peer = impl->findPeerDevice(event->conn_handle);
                if (peer) {
#if HAL_PLATFORM_BLE_ACTIVE_EVENT
                    if (impl->disconnectingPeer_ == *peer) {
                        impl->peers_.removeOne(*peer);
                    } else
#endif
                    {
                        peer->impl()->onDisconnected();
                        if (impl->disconnectedCallback_) {
                            impl->disconnectedCallback_(*peer);
                        }
                        // Do not invalidate its connection handle before removing the peer,
                        // see BlePeerDevice::operator==().
                        impl->peers_.removeOne(*peer);
                    }
                    LOG(TRACE, "Disconnected");
                }
                break;
            }
            case BLE_EVT_PAIRING_REQUEST_RECEIVED:
            case BLE_EVT_PAIRING_PASSKEY_DISPLAY:
            case BLE_EVT_PAIRING_PASSKEY_INPUT:
            case BLE_EVT_PAIRING_STATUS_UPDATED: 
            case BLE_EVT_PAIRING_NUMERIC_COMPARISON: {
                BlePeerDevice* peer = impl->findPeerDevice(event->conn_handle);
                if (peer) {
                    if (impl->pairingEventCallback_) {
                        BlePairingEventPayload payload = {};
                        size_t payloadLen = 0;
                        if (event->type == BLE_EVT_PAIRING_PASSKEY_DISPLAY || event->type == BLE_EVT_PAIRING_NUMERIC_COMPARISON) {
                            payload.passkey = event->params.passkey_display.passkey;
                            payloadLen = BLE_PAIRING_PASSKEY_LEN;
                        } else if (event->type == BLE_EVT_PAIRING_STATUS_UPDATED) {
                            payload.status.status = event->params.pairing_status.status;
                            payload.status.bonded = event->params.pairing_status.bonded;
                            payload.status.lesc = event->params.pairing_status.lesc;
                            payloadLen = sizeof(BlePairingStatus);
                        }
                        BlePairingEvent pairingEvent = {
                            .peer = *peer,
                            .type = static_cast<BlePairingEventType>(event->type),
                            .payloadLen = payloadLen,
                            .payload = payload
                        };
                        impl->pairingEventCallback_(pairingEvent);
                    }
                }
                break;
            }
            case BLE_EVT_ATT_MTU_UPDATED: {
                BlePeerDevice* peer = impl->findPeerDevice(event->conn_handle);
                if (peer && impl->attMtuExchangedCallback_) {
                    impl->attMtuExchangedCallback_(*peer, event->params.att_mtu_updated.att_mtu_size);
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
#if HAL_PLATFORM_BLE_ACTIVE_EVENT
    BlePeerDevice connectingPeer_;
    BlePeerDevice disconnectingPeer_;
#endif
    BleOnConnectedStdFunction connectedCallback_;
    BleOnDisconnectedStdFunction disconnectedCallback_;
    BleOnPairingEventStdFunction pairingEventCallback_;
    BleOnAttMtuExchangedStdFunction attMtuExchangedCallback_;
};


/*
 * WARN: This is executed from HAL ble thread. If the BLE wiring lock is acquired by a thread,
 * calling wiring APIs those acquiring the BLE wiring lock from the callback will suspend the
 * the execution of the callback, until the BLE wiring lock is released.
 */
void BleCharacteristicImpl::onBleCharEvents(const hal_ble_char_evt_t *event, void* context) {
    auto impl = static_cast<BleCharacteristicImpl*>(context);
    // This callback won't modified any data in wiring.
    //WiringBleLock lk;
    switch (event->type) {
        case BLE_EVT_DATA_NOTIFIED:
        case BLE_EVT_DATA_WRITTEN: {
            auto peer = BleLocalDevice::getInstance().impl()->findPeerDevice(event->conn_handle);
            if (!peer) {
                LOG(ERROR, "Peer device is missing!");
                break;
            }
            if (impl->dataReceivedCallback_) {
                impl->dataReceivedCallback_(event->params.data_written.data, event->params.data_written.len, *peer);
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

BleCharacteristic::BleCharacteristic(EnumFlags<BleCharacteristicProperty> properties, const char* desc, BleOnDataReceivedCallback callback, void* context)
        : impl_(std::make_shared<BleCharacteristicImpl>(properties, desc, callback, context)) {
    if (!impl()) {
        SPARK_ASSERT(false);
    }
    DEBUG("BleCharacteristic(...), 0x%08X -> 0x%08X, count: %d", this, impl(), impl_.use_count());
}

BleCharacteristic::BleCharacteristic(EnumFlags<BleCharacteristicProperty> properties, const char* desc, const BleOnDataReceivedStdFunction& callback)
        : impl_(std::make_shared<BleCharacteristicImpl>(properties, desc, callback)) {
    if (!impl()) {
        SPARK_ASSERT(false);
    }
    DEBUG("BleCharacteristic(...), 0x%08X -> 0x%08X, count: %d", this, impl(), impl_.use_count());
}

void BleCharacteristic::construct(const char* desc, EnumFlags<BleCharacteristicProperty> properties, BleUuid& charUuid, BleUuid& svcUuid, BleOnDataReceivedCallback callback, void* context) {
    impl_ = std::make_shared<BleCharacteristicImpl>(desc, properties, charUuid, svcUuid, callback, context);
    if (!impl()) {
        SPARK_ASSERT(false);
    }
    DEBUG("BleCharacteristic(), construct(...):0x%08X -> 0x%08X, count: %d", this, impl(), impl_.use_count());
}

void BleCharacteristic::construct(const char* desc, EnumFlags<BleCharacteristicProperty> properties, BleUuid& charUuid, BleUuid& svcUuid, const BleOnDataReceivedStdFunction& callback) {
    impl_ = std::make_shared<BleCharacteristicImpl>(desc, properties, charUuid, svcUuid, callback);
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
    return isValid();
}

bool BleCharacteristic::isValid() const {
    return (impl()->isLocal() || impl()->connHandle() != BLE_INVALID_CONN_HANDLE);
}

BleUuid BleCharacteristic::UUID() const {
    return impl()->charUUID();
}

EnumFlags<BleCharacteristicProperty> BleCharacteristic::properties() const {
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

ssize_t BleCharacteristic::setValue(const uint8_t* buf, size_t len, BleTxRxType type) {
    if (buf == nullptr || len == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    len = std::min(len, (size_t)BLE_MAX_ATTR_VALUE_PACKET_SIZE);
    if (impl()->isLocal()) {
        int ret = SYSTEM_ERROR_NOT_SUPPORTED;
        // Updates the local characteristic value for peer to read.
        if (impl()->properties().isSet(BleCharacteristicProperty::READ)) {
            ret = CHECK(hal_ble_gatt_server_set_characteristic_value(impl()->attrHandles().value_handle, buf, len, nullptr));
        }
        if (impl()->properties().isSet(BleCharacteristicProperty::NOTIFY) && type != BleTxRxType::ACK) {
            return hal_ble_gatt_server_notify_characteristic_value(impl()->attrHandles().value_handle, buf, len, nullptr);
        }
        if (impl()->properties().isSet(BleCharacteristicProperty::INDICATE) && type != BleTxRxType::NACK) {
            return hal_ble_gatt_server_indicate_characteristic_value(impl()->attrHandles().value_handle, buf, len, nullptr);
        }
        return ret;
    }
    if (impl()->connHandle() != BLE_INVALID_CONN_HANDLE) {
        if (impl()->properties().isSet(BleCharacteristicProperty::WRITE_WO_RSP) && type != BleTxRxType::ACK) {
            return hal_ble_gatt_client_write_without_response(impl()->connHandle(), impl()->attrHandles().value_handle, buf, len, nullptr);
        }
        if (impl()->properties().isSet(BleCharacteristicProperty::WRITE) && type != BleTxRxType::NACK) {
            return hal_ble_gatt_client_write_with_response(impl()->connHandle(), impl()->attrHandles().value_handle, buf, len, nullptr);
        }
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    return SYSTEM_ERROR_INVALID_STATE;
}

ssize_t BleCharacteristic::setValue(const String& str, BleTxRxType type) {
    return setValue(reinterpret_cast<const uint8_t*>(str.c_str()), str.length(), type);
}

ssize_t BleCharacteristic::setValue(const char* str, BleTxRxType type) {
    return setValue(reinterpret_cast<const uint8_t*>(str), strnlen(str, BLE_MAX_ATTR_VALUE_PACKET_SIZE), type);
}

ssize_t BleCharacteristic::getValue(uint8_t* buf, size_t len) const {
    if (buf == nullptr || len == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    len = std::min(len, (size_t)BLE_MAX_ATTR_VALUE_PACKET_SIZE);
    if (impl()->isLocal()) {
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
    CHECK_FALSE(impl()->isLocal(), SYSTEM_ERROR_INVALID_STATE);
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
    config.cccd_value = BLE_SIG_CCCD_VAL_DISABLED;
    if (enable) {
        if (impl()->properties().isSet(BleCharacteristicProperty::INDICATE)) {
            config.cccd_value = BLE_SIG_CCCD_VAL_INDICATION;
        }
        if (impl()->properties().isSet(BleCharacteristicProperty::NOTIFY)) {
            config.cccd_value = (ble_sig_cccd_value_t)(config.cccd_value | BLE_SIG_CCCD_VAL_NOTIFICATION);
        }
    }
    return hal_ble_gatt_client_configure_cccd(&config, nullptr);
}

void BleCharacteristic::onDataReceived(BleOnDataReceivedCallback callback, void* context) {
    impl()->setCallback(callback, context);
}

void BleCharacteristic::onDataReceived(const BleOnDataReceivedStdFunction& callback) {
    impl()->setCallback(callback);
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

bool BleService::operator==(const BleService& service) const {
    return (impl()->UUID() == service.impl()->UUID());
}

bool BleService::operator!=(const BleService& service) const {
    return !(*this == service);
}


class BleDiscoveryDelegator {
public:
    BleDiscoveryDelegator() = default;
    ~BleDiscoveryDelegator() = default;

    int discoverAllServices(BlePeerDevice& peer) {
        LOG(TRACE, "Start discovering services.");
        return hal_ble_gatt_client_discover_all_services(peer.impl()->connHandle(), onServicesDiscovered, &peer, nullptr);
    }

    int discoverCharacteristics(const BlePeerDevice& peer, const BleService& service) const {
        LOG(TRACE, "Start discovering characteristics of service: %s.", service.impl()->UUID().toString().c_str());
        hal_ble_svc_t halService;
        halService.size = sizeof(hal_ble_svc_t);
        halService.start_handle = service.impl()->startHandle();
        halService.end_handle = service.impl()->endHandle();
        CHECK(hal_ble_gatt_client_discover_characteristics(peer.impl()->connHandle(), &halService, onCharacteristicsDiscovered, peer.impl(), nullptr));
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
     * APIs those are invoked here must not acquire the BLE HAL lock. Otherwise, the HAL BLE thread cannot process 
     * SoftDevice events in queue.
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
     * APIs those are invoked here must not acquire the BLE HAL lock. Otherwise, the HAL BLE thread cannot process 
     * SoftDevice events in queue.
     */
    static void onCharacteristicsDiscovered(const hal_ble_char_discovered_evt_t* event, void* context) {
        LOG(TRACE, "Characteristic discovered.");
        BlePeerDeviceImpl* peerImpl = static_cast<BlePeerDeviceImpl*>(context);
        for (size_t i = 0; i < event->count; i++) {
            BleCharacteristic characteristic;
            BleService service;
            characteristic.impl()->attrHandles() = event->characteristics[i].charHandles;
            if (peerImpl->locateService(service, characteristic.impl()->attrHandles())) {
                characteristic.impl()->svcUUID() = service.impl()->UUID();
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
                if (!peerImpl->characteristics().append(characteristic)) {
                    LOG(ERROR, "Failed to append discovered characteristic.");
                }
            } else {
                LOG(ERROR, "Discovered characteristic's handle is invalid.");
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

Vector<BleService> BlePeerDevice::discoverAllServices() {
    if (!impl()->servicesDiscovered()) {
        BleDiscoveryDelegator discovery;
        if (discovery.discoverAllServices(*this) == SYSTEM_ERROR_NONE) {
            impl()->servicesDiscovered(true);
        }
    }
    return services();
}

ssize_t BlePeerDevice::discoverAllServices(BleService* svcs, size_t count) {
    CHECK_TRUE(svcs && count > 0, SYSTEM_ERROR_INVALID_ARGUMENT);
    if (!impl()->servicesDiscovered()) {
        BleDiscoveryDelegator discovery;
        CHECK(discovery.discoverAllServices(*this));
        impl()->servicesDiscovered(true);
    }
    return services(svcs, count);
}

Vector<BleCharacteristic> BlePeerDevice::discoverAllCharacteristics() {
    if (!impl()->servicesDiscovered()) {
        discoverAllServices();
    }
    for (const auto& service : impl()->services()) {
        discoverCharacteristicsOfService(service);
    }
    return characteristics();
}

ssize_t BlePeerDevice::discoverAllCharacteristics(BleCharacteristic* chars, size_t count) {
    CHECK_TRUE(chars && count > 0, SYSTEM_ERROR_INVALID_ARGUMENT);
    discoverAllCharacteristics();
    return characteristics(chars, count);
}

Vector<BleCharacteristic> BlePeerDevice::discoverCharacteristicsOfService(const BleService& service) {
    if (!impl()->servicesDiscovered()) {
        return Vector<BleCharacteristic>();
    }
    if (!service.impl()->characteristicsDiscovered()) {
        BleDiscoveryDelegator discovery;
        if (discovery.discoverCharacteristics(*this, service) == SYSTEM_ERROR_NONE) {
            service.impl()->characteristicsDiscovered(true);
        }
    }
    return characteristics(service);
}

ssize_t BlePeerDevice::discoverCharacteristicsOfService(const BleService& service, BleCharacteristic* chars, size_t count) {
    CHECK_TRUE(chars && count > 0, SYSTEM_ERROR_INVALID_ARGUMENT);
    discoverCharacteristicsOfService(service);
    return characteristics(service, chars, count);
}

Vector<BleService> BlePeerDevice::services() const {
    WiringBleLock lk;
    return impl()->services();
}

size_t BlePeerDevice::services(BleService* svcs, size_t count) const {
    WiringBleLock lk;
    CHECK_TRUE(svcs && count > 0, SYSTEM_ERROR_INVALID_ARGUMENT);
    count = std::min((int)count, impl()->services().size());
    for (size_t i = 0; i < count; i++) {
        svcs[i] = impl()->services()[i];
    }
    return count;
}

bool BlePeerDevice::getServiceByUUID(BleService& service, const BleUuid& uuid) const {
    WiringBleLock lk;
    for (auto& existSvc : impl()->services()) {
        if (existSvc.UUID() == uuid) {
            service = existSvc;
            return true;
        }
    }
    return false;
}

Vector<BleService> BlePeerDevice::getServiceByUUID(const BleUuid& uuid) const {
    WiringBleLock lk;
    Vector<BleService> services;
    for (auto& existSvc : impl()->services()) {
        if (existSvc.UUID() == uuid) {
            services.append(existSvc);
        }
    }
    return services;
}

size_t BlePeerDevice::getServiceByUUID(BleService* svcs, size_t count, const BleUuid& uuid) const {
    WiringBleLock lk;
    CHECK_TRUE(svcs && count > 0, SYSTEM_ERROR_INVALID_ARGUMENT);
    Vector<BleService> services = getServiceByUUID(uuid);
    count = std::min((int)count, services.size());
    for (size_t i = 0; i < count; i++) {
        svcs[i] = services[i];
    }
    return count;
}

Vector<BleCharacteristic> BlePeerDevice::characteristics() const {
    WiringBleLock lk;
    return impl()->characteristics();
}

Vector<BleCharacteristic> BlePeerDevice::characteristics(const BleService& service) const {
    WiringBleLock lk;
    Vector<BleCharacteristic> characteristics;
    for (const auto& characteristic : impl()->characteristics()) {
        if (service.impl()->hasCharacteristic(characteristic)) {
            characteristics.append(characteristic);
        }
    }
    return characteristics;
}

size_t BlePeerDevice::characteristics(BleCharacteristic* characteristics, size_t count) const {
    WiringBleLock lk;
    CHECK_TRUE(characteristics && count > 0, SYSTEM_ERROR_INVALID_ARGUMENT);
    count = std::min((int)count, impl()->characteristics().size());
    for (size_t i = 0; i < count; i++) {
        characteristics[i] = impl()->characteristics()[i];
    }
    return count;
}

size_t BlePeerDevice::characteristics(const BleService& service, BleCharacteristic* characteristics, size_t count) const {
    WiringBleLock lk;
    CHECK_TRUE(characteristics && count > 0, SYSTEM_ERROR_INVALID_ARGUMENT);
    Vector<BleCharacteristic> chars = this->characteristics(service);
    count = std::min((int)count, chars.size());
    for (size_t i = 0; i < count; i++) {
        characteristics[i] = chars[i];
    }
    return count;
}

bool BlePeerDevice::getCharacteristicByDescription(BleCharacteristic& characteristic, const char* desc) const {
    WiringBleLock lk;
    CHECK_TRUE(desc, false);
    for (auto& existChar : impl()->characteristics()) {
        if (!strcmp(existChar.description().c_str(), desc)) {
            characteristic = existChar;
            return true;
        }
    }
    return false;
}

bool BlePeerDevice::getCharacteristicByDescription(BleCharacteristic& characteristic, const String& desc) const {
    return getCharacteristicByDescription(characteristic, desc.c_str());
}

bool BlePeerDevice::getCharacteristicByUUID(BleCharacteristic& characteristic, const BleUuid& uuid) const {
    WiringBleLock lk;
    for (auto& existChar : impl()->characteristics()) {
        if (existChar.UUID() == uuid) {
            characteristic = existChar;
            return true;
        }
    }
    return false;
}

Vector<BleCharacteristic> BlePeerDevice::getCharacteristicByUUID(const BleUuid& uuid) const {
    WiringBleLock lk;
    Vector<BleCharacteristic> characteristics;
    for (auto& existChar : impl()->characteristics()) {
        if (existChar.UUID() == uuid) {
            characteristics.append(existChar);
        }
    }
    return characteristics;
}

size_t BlePeerDevice::getCharacteristicByUUID(BleCharacteristic* characteristics, size_t count, const BleUuid& uuid) const {
    WiringBleLock lk;
    CHECK_TRUE(characteristics && count > 0, SYSTEM_ERROR_INVALID_ARGUMENT);
    Vector<BleCharacteristic> chars = getCharacteristicByUUID(uuid);
    count = std::min((int)count, chars.size());
    for (size_t i = 0; i < count; i++) {
        characteristics[i] = chars[i];
    }
    return count;
}

bool BlePeerDevice::getCharacteristicByDescription(const BleService& service, BleCharacteristic& characteristic, const char* desc) const {
    WiringBleLock lk;
    CHECK_TRUE(desc, false);
    for (auto& existChar : impl()->characteristics()) {
        if (service.impl()->hasCharacteristic(existChar) && !strcmp(existChar.description().c_str(), desc)) {
            characteristic = existChar;
            return true;
        }
    }
    return false;
}

bool BlePeerDevice::getCharacteristicByDescription(const BleService& service, BleCharacteristic& characteristic, const String& desc) const {
    return getCharacteristicByDescription(service, characteristic, desc.c_str());
}

bool BlePeerDevice::getCharacteristicByUUID(const BleService& service, BleCharacteristic& characteristic, const BleUuid& uuid) const {
    WiringBleLock lk;
    for (auto& existChar : impl()->characteristics()) {
        if (existChar.UUID() == uuid && service.impl()->hasCharacteristic(existChar)) {
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

#if HAL_PLATFORM_BLE_ACTIVE_EVENT
    SCOPE_GUARD ({
        BleLocalDevice::getInstance().impl()->connectingPeer() = BlePeerDevice();
    });
    bind(addr);
    BleLocalDevice::getInstance().impl()->connectingPeer() = *this;
#endif

    int ret = hal_ble_gap_connect(&connCfg, &impl()->connHandle(), nullptr);
    if (ret != SYSTEM_ERROR_NONE) {
        impl()->connHandle() = BLE_INVALID_CONN_HANDLE;
        return ret;
    }
    {
        WiringBleLock lk;
#if !HAL_PLATFORM_BLE_ACTIVE_EVENT
        bind(addr);
        if (!BleLocalDevice::getInstance().impl()->peers().append(*this)) {
#else
        if (!BleLocalDevice::getInstance().impl()->findPeerDevice(impl()->connHandle())) {
#endif
            LOG(ERROR, "Cannot add new peer device.");
            lk.unlock();
            hal_ble_gap_disconnect(impl()->connHandle(), nullptr);
            lk.lock();
            impl()->connHandle() = BLE_INVALID_CONN_HANDLE;
            return SYSTEM_ERROR_NO_MEMORY;
        }
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

int BlePeerDevice::connect(const BleAddress& addr, const BleConnectionParams& params, bool automatic) {
    return connect(addr, &params, automatic);
}

int BlePeerDevice::connect(const BleAddress& addr, uint16_t interval, uint16_t latency, uint16_t timeout, bool automatic) {
    BleConnectionParams connParams;
    connParams.version = BLE_API_VERSION;
    connParams.size = sizeof(hal_ble_conn_params_t);
    connParams.min_conn_interval = interval;
    connParams.max_conn_interval = interval;
    connParams.slave_latency = latency;
    connParams.conn_sup_timeout = timeout;
    return connect(addr, &connParams, automatic);
}

int BlePeerDevice::connect(const BleAddress& addr, bool automatic) {
    return connect(addr, nullptr, automatic);
}

int BlePeerDevice::connect(const BleConnectionParams* params, bool automatic) {
    return connect(address(), params, automatic);
}

int BlePeerDevice::connect(const BleConnectionParams& params, bool automatic) {
    return connect(address(), &params, automatic);
}

int BlePeerDevice::connect(uint16_t interval, uint16_t latency, uint16_t timeout, bool automatic) {
    return connect(address(), interval, latency, timeout, automatic);
}

int BlePeerDevice::connect(bool automatic) {
    return connect(address(), nullptr, automatic);
}

int BlePeerDevice::disconnect() const {
    CHECK_TRUE(connected(), SYSTEM_ERROR_INVALID_STATE);

#if HAL_PLATFORM_BLE_ACTIVE_EVENT
    SCOPE_GUARD ({
        BleLocalDevice::getInstance().impl()->disconnectingPeer() = BlePeerDevice();
    });
    BleLocalDevice::getInstance().impl()->disconnectingPeer() = *this;
#endif

    CHECK(hal_ble_gap_disconnect(impl()->connHandle(), nullptr));
    {
        WiringBleLock lk;
#if !HAL_PLATFORM_BLE_ACTIVE_EVENT
        BleLocalDevice::getInstance().impl()->peers().removeOne(*this);
#endif
        /*
        * Only the connection handle is invalid. The service and characteristics being
        * discovered previously can be re-used next time once connected if needed.
        */
        impl()->connHandle() = BLE_INVALID_CONN_HANDLE;
    }
    return SYSTEM_ERROR_NONE;
}

bool BlePeerDevice::connected() const {
    WiringBleLock lk;
    return impl()->connHandle() != BLE_INVALID_CONN_HANDLE;
}

void BlePeerDevice::bind(const BleAddress& address) const {
    WiringBleLock lk;
    impl()->address() = address;
}

BleAddress BlePeerDevice::address() const {
    WiringBleLock lk;
    return impl()->address();
}

bool BlePeerDevice::isValid() const {
    WiringBleLock lk;
    return impl()->connHandle() != BLE_INVALID_CONN_HANDLE;
}

bool BlePeerDevice::operator==(const BlePeerDevice& device) const {
    WiringBleLock lk;
    if (impl()->connHandle() == device.impl()->connHandle() && address() == device.address()) {
        return true;
    }
    return false;
}

bool BlePeerDevice::operator!=(const BlePeerDevice& device) const {
    WiringBleLock lk;
    return !(*this == device);
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

void BleLocalDevice::onConnected(const BleOnConnectedStdFunction& callback) const {
    impl()->onConnectedCallback(callback);
}

void BleLocalDevice::onDisconnected(BleOnDisconnectedCallback callback, void* context) const {
    impl()->onDisconnectedCallback(callback, context);
}

void BleLocalDevice::onDisconnected(const BleOnDisconnectedStdFunction& callback) const {
    impl()->onDisconnectedCallback(callback);
}

int BleLocalDevice::begin() const {
    return SYSTEM_ERROR_NONE;
}

int BleLocalDevice::end() const {
    /*
     * 1. Disconnects all the connections initiated by user application.
     * 2. Disconnects Peripheral connection if it is not in the Listening/Provisioning mode.
     * 3. Stops advertising if it is not in the Listening/Provisioning mode.
     * 4. Stops scanning if initiated.
     *
     * FIXME: If device is broadcasting before entering the Listening/Provisioning mode and
     * then this API is called during device in the Listening/Provisioning mode, device will
     * restart broadcasting automatically when device exits the Listening/Provisioning mode.
     */
    disconnectAll(); // BLE HAL will guard that the Peripheral connection is remained if device is in the Listening/Provisioning mode.
    {
        WiringBleLock lk;
        impl()->peers().clear();
    }
    stopAdvertising(); // BLE HAL will guard that device keeps broadcasting if device is in the Listening/Provisioning mode.
    stopScanning();
    return SYSTEM_ERROR_NONE;
}

int BleLocalDevice::on() const {
    CHECK(hal_ble_stack_init(nullptr));
    return SYSTEM_ERROR_NONE;
}

int BleLocalDevice::off() const {
    CHECK(hal_ble_stack_deinit(nullptr));
    {
        // Disable BLE prov mode if enabled
        if (getProvisioningStatus()) {
            provisioningMode(false);
        }
        WiringBleLock lk;
        impl()->peers().clear();
    }
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
    return setDeviceName(name, name ? strnlen(name, BLE_MAX_DEV_NAME_LEN) : 0);
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
    char buf[BLE_MAX_DEV_NAME_LEN + 1] = {}; // NULL-terminated string is returned.
    if (getDeviceName(buf, sizeof(buf)) > 0) {
        name.concat(buf);
    }
    return name;
}

int BleLocalDevice::setTxPower(int8_t txPower) const {
    return hal_ble_gap_set_tx_power(txPower, nullptr);
}

int BleLocalDevice::txPower(int8_t* txPower) const {
    return hal_ble_gap_get_tx_power(txPower, nullptr);
}

int8_t BleLocalDevice::txPower() const {
    int8_t tx = BLE_TX_POWER_INVALID;
    hal_ble_gap_get_tx_power(&tx, nullptr);
    return tx;
}

int BleLocalDevice::selectAntenna(BleAntennaType antenna) const {
    return hal_ble_select_antenna(static_cast<hal_ble_ant_type_t>(antenna), nullptr);
}

int BleLocalDevice::provisioningMode(bool enabled) const {
    return system_ble_prov_mode(enabled, nullptr);
}

bool BleLocalDevice::getProvisioningStatus() const {
    return system_ble_prov_get_status(nullptr);
}

int BleLocalDevice::setAdvertisingInterval(uint16_t interval) const {
    hal_ble_adv_params_t advParams = {};
    advParams.size = sizeof(hal_ble_adv_params_t);
    advParams.version = BLE_API_VERSION;
    CHECK(hal_ble_gap_get_advertising_parameters(&advParams, nullptr));
    advParams.interval = interval;
    return hal_ble_gap_set_advertising_parameters(&advParams, nullptr);
}

int BleLocalDevice::setAdvertisingTimeout(uint16_t timeout) const {
    hal_ble_adv_params_t advParams = {};
    advParams.size = sizeof(hal_ble_adv_params_t);
    advParams.version = BLE_API_VERSION;
    CHECK(hal_ble_gap_get_advertising_parameters(&advParams, nullptr));
    advParams.timeout = timeout;
    return hal_ble_gap_set_advertising_parameters(&advParams, nullptr);
}

int BleLocalDevice::setAdvertisingType(BleAdvertisingEventType type) const {
    hal_ble_adv_params_t advParams = {};
    advParams.size = sizeof(hal_ble_adv_params_t);
    advParams.version = BLE_API_VERSION;
    CHECK(hal_ble_gap_get_advertising_parameters(&advParams, nullptr));
    advParams.type = static_cast<hal_ble_adv_evt_type_t>(type);
    return hal_ble_gap_set_advertising_parameters(&advParams, nullptr);
}

int BleLocalDevice::setAdvertisingPhy(BlePhy phy) const {
    hal_ble_adv_params_t advParams = {};
    advParams.size = sizeof(hal_ble_adv_params_t);
    advParams.version = BLE_API_VERSION;
    CHECK(hal_ble_gap_get_advertising_parameters(&advParams, nullptr));
    advParams.primary_phy = static_cast<uint8_t>(phy);
    return hal_ble_gap_set_advertising_parameters(&advParams, nullptr);
}

int BleLocalDevice::setAdvertisingParameters(const BleAdvertisingParams* params) const {
    return hal_ble_gap_set_advertising_parameters(params, nullptr);
}

int BleLocalDevice::setAdvertisingParameters(const BleAdvertisingParams& params) const {
    return setAdvertisingParameters(&params);
}

int BleLocalDevice::setAdvertisingParameters(uint16_t interval, uint16_t timeout, BleAdvertisingEventType type) const {
    hal_ble_adv_params_t advParams = {};
    advParams.size = sizeof(hal_ble_adv_params_t);
    advParams.version = BLE_API_VERSION;
    CHECK(hal_ble_gap_get_advertising_parameters(&advParams, nullptr));
    advParams.interval = interval;
    advParams.timeout = timeout;
    advParams.type = static_cast<hal_ble_adv_evt_type_t>(type);
    return hal_ble_gap_set_advertising_parameters(&advParams, nullptr);
}

int BleLocalDevice::getAdvertisingParameters(BleAdvertisingParams* params) const {
    return hal_ble_gap_get_advertising_parameters(params, nullptr);
}

int BleLocalDevice::getAdvertisingParameters(BleAdvertisingParams& params) const {
    return getAdvertisingParameters(&params);
}

int BleLocalDevice::setAdvertisingData(BleAdvertisingData* advertisingData) const {
    if (advertisingData == nullptr) {
        return hal_ble_gap_set_advertising_data(nullptr, 0, nullptr);
    } else {
        return hal_ble_gap_set_advertising_data(advertisingData->data(), advertisingData->length(), nullptr);
    }
}

int BleLocalDevice::setAdvertisingData(BleAdvertisingData& advertisingData) const {
    return setAdvertisingData(&advertisingData);
}

int BleLocalDevice::setScanResponseData(BleAdvertisingData* scanResponse) const {
    if (scanResponse == nullptr) {
        return hal_ble_gap_set_scan_response_data(nullptr, 0, nullptr);
    } else {
        scanResponse->remove(BleAdvertisingDataType::FLAGS);
        return hal_ble_gap_set_scan_response_data(scanResponse->data(), scanResponse->length(), nullptr);
    }
}

int BleLocalDevice::setScanResponseData(BleAdvertisingData& scanResponse) const {
    return setScanResponseData(&scanResponse);
}

ssize_t BleLocalDevice::getAdvertisingData(BleAdvertisingData* advertisingData) const {
    if (advertisingData == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    hal_ble_adv_params_t advParams = {};
    advParams.size = sizeof(hal_ble_adv_params_t);
    hal_ble_gap_get_advertising_parameters(&advParams, nullptr);
    advertisingData->clear();
    CHECK_TRUE(advertisingData->resize((advParams.primary_phy == BLE_PHYS_CODED) ? BLE_MAX_ADV_DATA_LEN_EXT : BLE_MAX_ADV_DATA_LEN), SYSTEM_ERROR_NO_MEMORY);
    size_t len = CHECK(hal_ble_gap_get_advertising_data(advertisingData->data(), advertisingData->length(), nullptr));
    advertisingData->resize(len);
    return len;
}

ssize_t BleLocalDevice::getAdvertisingData(BleAdvertisingData& advertisingData) const {
    return getAdvertisingData(&advertisingData);
}

ssize_t BleLocalDevice::getScanResponseData(BleAdvertisingData* scanResponse) const {
    if (scanResponse == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    scanResponse->clear();
    CHECK_TRUE(scanResponse->resize(BLE_MAX_ADV_DATA_LEN), SYSTEM_ERROR_NO_MEMORY);
    size_t len = CHECK(hal_ble_gap_get_scan_response_data(scanResponse->data(), scanResponse->length(), nullptr));
    scanResponse->resize(len);
    return len;
}

ssize_t BleLocalDevice::getScanResponseData(BleAdvertisingData& scanResponse) const {
    return getScanResponseData(&scanResponse);
}

int BleLocalDevice::advertise() const {
    return hal_ble_gap_start_advertising(nullptr);
}

int BleLocalDevice::advertise(BleAdvertisingData* advertisingData, BleAdvertisingData* scanResponse) const {
    CHECK(setAdvertisingData(advertisingData));
    CHECK(setScanResponseData(scanResponse));
    return advertise();
}

int BleLocalDevice::advertise(BleAdvertisingData& advertisingData) const {
    return advertise(&advertisingData, nullptr);
}

int BleLocalDevice::advertise(BleAdvertisingData& advertisingData, BleAdvertisingData& scanResponse) const {
    return advertise(&advertisingData, &scanResponse);
}

int BleLocalDevice::advertise(const iBeacon& beacon) const {
    BleAdvertisingData* advData = new(std::nothrow) BleAdvertisingData(beacon);
    SCOPE_GUARD ({
        if (advData) {
            delete advData;
        }
    });
    CHECK_TRUE(advData, SYSTEM_ERROR_NO_MEMORY);
    CHECK_TRUE(advData->length() > 0, SYSTEM_ERROR_NO_MEMORY);
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
              scanResultCallback_(nullptr),
              scanResultCallbackRef_(nullptr) {
        resultsVector_.clear();
    }

    ~BleScanDelegator() = default;

    int start(BleOnScanResultCallback callback, void* context) {
        scanResultCallback_ = callback ? std::bind(callback, _1, context) : (std::function<void(const BleScanResult*)>)nullptr;
        scanResultCallbackRef_ = nullptr;
        CHECK(hal_ble_gap_start_scan(onScanResultCallback, this, nullptr));
        return foundCount_;
    }

    int start(BleOnScanResultCallbackRef callback, void* context) {
        scanResultCallback_ = nullptr;
        scanResultCallbackRef_ = callback ? std::bind(callback, _1, context) : (BleOnScanResultStdFunction)nullptr;
        CHECK(hal_ble_gap_start_scan(onScanResultCallback, this, nullptr));
        return foundCount_;
    }

    int start(BleScanResult* results, size_t resultCount) {
        scanResultCallback_ = nullptr;
        scanResultCallbackRef_ = nullptr;
        resultsPtr_ = results;
        targetCount_ = resultCount;
        CHECK(hal_ble_gap_start_scan(onScanResultCallback, this, nullptr));
        return foundCount_;
    }

    Vector<BleScanResult> start() {
        scanResultCallback_ = nullptr;
        scanResultCallbackRef_ = nullptr;
        hal_ble_gap_start_scan(onScanResultCallback, this, nullptr);
        return resultsVector_;
    }

    int start(const BleOnScanResultStdFunction& callback) {
        scanResultCallback_ = nullptr;
        scanResultCallbackRef_ = callback;
        CHECK(hal_ble_gap_start_scan(onScanResultCallback, this, nullptr));
        return foundCount_;
    }

    BleScanDelegator& setScanFilter(const BleScanFilter& filter) {
        filter_ = filter;
        return *this;
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
        result.address(event->peer_addr).rssi(event->rssi)
              .scanResponse(event->sr_data, event->sr_data_len)
              .advertisingData(event->adv_data, event->adv_data_len);

        if (!delegator->filterByRssi(result) ||
              !delegator->filterByAddress(result) ||
              !delegator->filterByDeviceName(result) ||
              !delegator->filterByServiceUUID(result) ||
              !delegator->filterByAppearance(result) ||
              !delegator->filterByCustomData(result)) {
            return;
        }

        if (delegator->scanResultCallback_) {
            delegator->foundCount_++;
            delegator->scanResultCallback_(&result);
            return;
        } else if (delegator->scanResultCallbackRef_) {
            delegator->foundCount_++;
            delegator->scanResultCallbackRef_(result);
            return;
        }
        if (delegator->resultsPtr_) {
            if (delegator->foundCount_ < delegator->targetCount_) {
                delegator->resultsPtr_[delegator->foundCount_] = result;
                delegator->foundCount_++;
                if (delegator->foundCount_ >= delegator->targetCount_) {
                    LOG_DEBUG(TRACE, "Target number of devices found. Stop scanning...");
                    hal_ble_gap_stop_scan(nullptr);
                }
            }
            return;
        }
        delegator->resultsVector_.append(result);
    }

    bool filterByRssi(const BleScanResult& result) {
        int8_t filterRssi = filter_.minRssi();
        if (filterRssi != BLE_RSSI_INVALID && result.rssi() < filterRssi) {
            LOG_DEBUG(TRACE, "Exceed min. RSSI");
            return false;
        }
        filterRssi = filter_.maxRssi();
        if (filterRssi != BLE_RSSI_INVALID && result.rssi() > filterRssi) {
            LOG_DEBUG(TRACE, "Exceed max. RSSI.");
            return false;
        }
        return true;
    }

    bool filterByAddress(const BleScanResult& result) {
        auto filerAddresses = filter_.addresses();
        if (filerAddresses.size() > 0) {
            for (const auto& address : filerAddresses) {
                if (address == result.address()) {
                    return true;
                }
            }
            LOG_DEBUG(TRACE, "Address mismatched.");
            return false;
        }
        return true;
    }

    bool filterByDeviceName(const BleScanResult& result) {
        auto filterDeviceNames = filter_.deviceNames();
        if (filterDeviceNames.size() > 0) {
            String srName = result.scanResponse().deviceName();
            String advName = result.advertisingData().deviceName();
            if (srName.length() == 0 && advName.length() == 0) {
                LOG_DEBUG(TRACE, "Device name mismatched.");
                return false;
            }
            for (const auto& name : filterDeviceNames) {
                if (name == srName || name == advName) {
                    return true;
                }
            }
            LOG_DEBUG(TRACE, "Device name mismatched.");
            return false;
        }
        return true;
    }

    bool filterByServiceUUID(const BleScanResult& result) {
        auto filterServiceUuids = filter_.serviceUUIDs();
        if (filterServiceUuids.size() > 0) {
            const Vector<BleUuid>& srUuids = result.scanResponse().serviceUUID();
            const Vector<BleUuid>& advUuids = result.advertisingData().serviceUUID();
            if (srUuids.size() <= 0 && advUuids.size() <= 0) {
                LOG_DEBUG(TRACE, "Service UUID mismatched.");
                return false;
            }
            for (const auto& uuid : filterServiceUuids) {
                for (const auto& found : srUuids) {
                    if (uuid == found) {
                        return true;
                    }
                }
                for (const auto& found : advUuids) {
                    if (uuid == found) {
                        return true;
                    }
                }
            }
            LOG_DEBUG(TRACE, "Service UUID mismatched.");
            return false;
        }
        return true;
    }

    bool filterByAppearance(const BleScanResult& result) {
        auto filterAppearances = filter_.appearances();
        if (filterAppearances.size() > 0) {
            ble_sig_appearance_t srAppearance = result.scanResponse().appearance();
            ble_sig_appearance_t advAppearance = result.advertisingData().appearance();
            for (const auto& appearance : filterAppearances) {
                if (appearance == srAppearance || appearance == advAppearance) {
                    return true;
                }
            }
            LOG_DEBUG(TRACE, "Appearance mismatched.");
            return false;
        }
        return true;
    }

    bool filterByCustomData(const BleScanResult& result) {
        size_t filterCustomDatalen;
        const uint8_t* filterCustomData = filter_.customData(&filterCustomDatalen);
        if (filterCustomData != nullptr && filterCustomDatalen > 0) {
            size_t srLen = result.scanResponse().get(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA, nullptr, BLE_MAX_ADV_DATA_LEN);
            size_t advLen = result.advertisingData().get(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA, nullptr, BLE_MAX_ADV_DATA_LEN_EXT);
            if (srLen != filterCustomDatalen && advLen != filterCustomDatalen) {
                LOG_DEBUG(TRACE, "Custom data mismatched.");
                return false;
            }
            if (srLen == filterCustomDatalen) {
                uint8_t* buf = (uint8_t*)malloc(srLen);
                SCOPE_GUARD({
                    if (buf) {
                        free(buf);
                    }
                });
                if (!buf) {
                    LOG(ERROR, "Failed to allocate memory!");
                    return false;
                }
                result.scanResponse().get(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA, buf, srLen);
                if (!memcmp(buf, filterCustomData, srLen)) {
                    return true;
                }
            }
            if (advLen == filterCustomDatalen) {
                uint8_t* buf = (uint8_t*)malloc(advLen);
                SCOPE_GUARD({
                    if (buf) {
                        free(buf);
                    }
                });
                if (!buf) {
                    LOG(ERROR, "Failed to allocate memory!");
                    return false;
                }
                result.advertisingData().get(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA, buf, advLen);
                if (!memcmp(buf, filterCustomData, advLen)) {
                    return true;
                }
            }
            LOG_DEBUG(TRACE, "Custom data mismatched.");
            return false;
        }
        return true;
    }

    Vector<BleScanResult> resultsVector_;
    BleScanResult* resultsPtr_;
    size_t targetCount_;
    size_t foundCount_;
    std::function<void(const BleScanResult*)> scanResultCallback_;
    BleOnScanResultStdFunction scanResultCallbackRef_;
    BleScanFilter filter_;
};

int BleLocalDevice::setScanTimeout(uint16_t timeout) const {
    hal_ble_scan_params_t scanParams = {};
    scanParams.size = sizeof(hal_ble_scan_params_t);
    scanParams.version = BLE_API_VERSION;
    hal_ble_gap_get_scan_parameters(&scanParams, nullptr);
    scanParams.timeout = timeout;
    return hal_ble_gap_set_scan_parameters(&scanParams, nullptr);
}

int BleLocalDevice::setScanPhy(EnumFlags<BlePhy> phy) const {
    hal_ble_scan_params_t scanParams = {};
    scanParams.size = sizeof(hal_ble_scan_params_t);
    scanParams.version = BLE_API_VERSION;
    hal_ble_gap_get_scan_parameters(&scanParams, nullptr);
    scanParams.scan_phys = static_cast<uint8_t>(phy.value());
    return hal_ble_gap_set_scan_parameters(&scanParams, nullptr);
}

int BleLocalDevice::setScanParameters(const BleScanParams* params) const {
    return hal_ble_gap_set_scan_parameters(params, nullptr);
}

int BleLocalDevice::setScanParameters(const BleScanParams& params) const {
    return setScanParameters(&params);
}

int BleLocalDevice::getScanParameters(BleScanParams* params) const {
    return hal_ble_gap_get_scan_parameters(params, nullptr);
}

int BleLocalDevice::getScanParameters(BleScanParams& params) const {
    return getScanParameters(&params);
}

int BleLocalDevice::scan(const BleOnScanResultStdFunction& callback) const {
    BleScanDelegator scanner;
    return scanner.start(callback);
}

int BleLocalDevice::scan(BleOnScanResultCallback callback, void* context) const {
    BleScanDelegator scanner;
    return scanner.start(callback, context);
}

int BleLocalDevice::scan(BleOnScanResultCallbackRef callback, void* context) const {
    BleScanDelegator scanner;
    return scanner.start(callback, context);
}

int BleLocalDevice::scan(BleScanResult* results, size_t resultCount) const {
    if (results == nullptr || resultCount == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    BleScanDelegator scanner;
    return scanner.start(results, resultCount);
}

Vector<BleScanResult> BleLocalDevice::scan() const {
    BleScanDelegator scanner;
    return scanner.start();
}

int BleLocalDevice::scanWithFilter(const BleScanFilter& filter, const BleOnScanResultStdFunction& callback) const {
    BleScanDelegator scanner;
    return scanner.setScanFilter(filter).start(callback);
}

int BleLocalDevice::scanWithFilter(const BleScanFilter& filter, BleOnScanResultCallback callback, void* context) const {
    BleScanDelegator scanner;
    return scanner.setScanFilter(filter).start(callback, context);
}

int BleLocalDevice::scanWithFilter(const BleScanFilter& filter, BleOnScanResultCallbackRef callback, void* context) const {
    BleScanDelegator scanner;
    return scanner.setScanFilter(filter).start(callback, context);
}

int BleLocalDevice::scanWithFilter(const BleScanFilter& filter, BleScanResult* results, size_t resultCount) const {
    if (results == nullptr || resultCount == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    BleScanDelegator scanner;
    return scanner.setScanFilter(filter).start(results, resultCount);
}

Vector<BleScanResult> BleLocalDevice::scanWithFilter(const BleScanFilter& filter) const {
    BleScanDelegator scanner;
    return scanner.setScanFilter(filter).start();
}

int BleLocalDevice::stopScanning() const {
    return hal_ble_gap_stop_scan(nullptr);
}

int BleLocalDevice::setPPCP(uint16_t minInterval, uint16_t maxInterval, uint16_t latency, uint16_t timeout) const {
    hal_ble_conn_params_t ppcp = {};
    ppcp.size = sizeof(hal_ble_conn_params_t);
    ppcp.min_conn_interval = minInterval;
    ppcp.max_conn_interval = maxInterval;
    ppcp.slave_latency = latency;
    ppcp.conn_sup_timeout = timeout;
    return hal_ble_gap_set_ppcp(&ppcp, nullptr);
}

int BleLocalDevice::setDesiredAttMtu(size_t mtu) const {
    return hal_ble_gatt_server_set_desired_att_mtu(mtu, nullptr);
}

ssize_t BleLocalDevice::getCurrentAttMtu(const BlePeerDevice& peer) const {
    return hal_ble_gatt_get_att_mtu(peer.impl()->connHandle(), nullptr);
}

int BleLocalDevice::updateAttMtu(const BlePeerDevice& peer) const {
    return hal_ble_gatt_client_att_mtu_exchange(peer.impl()->connHandle(), nullptr);
}

void BleLocalDevice::onAttMtuExchanged(BleOnAttMtutExchangedCallback callback, void* context) const {
    impl()->onAttMtuExchangedCallback(callback, context);
}

void BleLocalDevice::onAttMtuExchanged(const BleOnAttMtuExchangedStdFunction& callback) const {
    impl()->onAttMtuExchangedCallback(callback);
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

BlePeerDevice BleLocalDevice::connect(const BleAddress& addr, const BleConnectionParams& params, bool automatic) const {
    return connect(addr, &params, automatic);
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

int BleLocalDevice::setPairingIoCaps(BlePairingIoCaps ioCaps) const {
    hal_ble_pairing_config_t config = {};
    config.version = BLE_API_VERSION;
    config.size = sizeof(hal_ble_pairing_config_t);
    CHECK(hal_ble_gap_get_pairing_config(&config, nullptr));
    config.io_caps = static_cast<hal_ble_pairing_io_caps_t>(ioCaps);
    return hal_ble_gap_set_pairing_config(&config, nullptr);
}

int BleLocalDevice::setPairingAlgorithm(BlePairingAlgorithm algorithm) const {
    hal_ble_pairing_config_t config = {};
    config.version = BLE_API_VERSION;
    config.size = sizeof(hal_ble_pairing_config_t);
    CHECK(hal_ble_gap_get_pairing_config(&config, nullptr));
    config.algorithm = static_cast<hal_ble_pairing_algorithm_t>(algorithm);
    return hal_ble_gap_set_pairing_config(&config, nullptr);
}

int BleLocalDevice::startPairing(const BlePeerDevice& peer) const {
    return hal_ble_gap_start_pairing(peer.impl()->connHandle(), nullptr);
}

int BleLocalDevice::rejectPairing(const BlePeerDevice& peer) const {
    return hal_ble_gap_reject_pairing(peer.impl()->connHandle(), nullptr);
}

int BleLocalDevice::setPairingNumericComparison(const BlePeerDevice& peer, bool equal) const {
    hal_ble_pairing_auth_data_t auth = {};
    auth.version = BLE_API_VERSION;
    auth.size = sizeof(hal_ble_pairing_auth_data_t);
    auth.type = BLE_PAIRING_AUTH_DATA_NUMERIC_COMPARISON;
    auth.params.equal = equal;
    return hal_ble_gap_set_pairing_auth_data(peer.impl()->connHandle(), &auth, nullptr);
}

int BleLocalDevice::setPairingPasskey(const BlePeerDevice& peer, const uint8_t* passkey) const {
    hal_ble_pairing_auth_data_t auth = {};
    auth.version = BLE_API_VERSION;
    auth.size = sizeof(hal_ble_pairing_auth_data_t);
    auth.type = BLE_PAIRING_AUTH_DATA_PASSKEY;
    auth.params.passkey = passkey;
    return hal_ble_gap_set_pairing_auth_data(peer.impl()->connHandle(), &auth, nullptr);
}

bool BleLocalDevice::isPairing(const BlePeerDevice& peer) const {
    return hal_ble_gap_is_pairing(peer.impl()->connHandle(), nullptr);
}

bool BleLocalDevice::isPaired(const BlePeerDevice& peer) const {
    return hal_ble_gap_is_paired(peer.impl()->connHandle(), nullptr);
}

void BleLocalDevice::onPairingEvent(BleOnPairingEventCallback callback, void* context) const {
    impl()->onPairingEvent(callback, context);
}

void BleLocalDevice::onPairingEvent(const BleOnPairingEventStdFunction& callback) const {
    impl()->onPairingEvent(callback);
}

int BleLocalDevice::disconnect() const {
    WiringBleLock lk;
    for (auto p : impl()->peers()) {
        hal_ble_conn_info_t connInfo = {};
        connInfo.version = BLE_API_VERSION;
        connInfo.size = sizeof(hal_ble_conn_info_t);
        if (hal_ble_gap_get_connection_info(p.impl()->connHandle(), &connInfo, nullptr) != SYSTEM_ERROR_NONE) {
            continue;
        }
        if (connInfo.role == BLE_ROLE_PERIPHERAL) {
            lk.unlock(); // To allow HAL BLE thread to invoke wiring callback
            p.disconnect();
            lk.lock();
            return SYSTEM_ERROR_NONE;
        }
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int BleLocalDevice::disconnect(const BlePeerDevice& peer) const {
    return peer.disconnect();
}

int BleLocalDevice::disconnectAll() const {
    WiringBleLock lk;
    // DO NOT use auto&, otherwise, any operation using BlePeerDevice::impl() after the peer device
    // being removed from the peers() vector may not be guaranteed. See BlePeerDevice::disconnect().
    auto& peers = impl()->peers();
    while (peers.size() > 0) {
        auto p = peers[0];
        lk.unlock();
        p.disconnect();
        lk.lock();
    }
    return SYSTEM_ERROR_NONE;
}

BlePeerDevice BleLocalDevice::peerCentral() const {
    WiringBleLock lk;
    for (auto& p : impl()->peers()) {
        hal_ble_conn_info_t connInfo = {};
        connInfo.version = BLE_API_VERSION;
        connInfo.size = sizeof(hal_ble_conn_info_t);
        if (hal_ble_gap_get_connection_info(p.impl()->connHandle(), &connInfo, nullptr) != SYSTEM_ERROR_NONE) {
            continue;
        }
        if (connInfo.role == BLE_ROLE_PERIPHERAL) {
            return p;
        }
    }
    return BlePeerDevice();
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
    charInit.properties = charImpl->properties().value();
    charInit.service_handle = service->impl()->startHandle();
    charInit.description = charImpl->description().c_str();
    charInit.callback = charImpl->onBleCharEvents;
    charInit.context = charImpl;
    if (hal_ble_gatt_server_add_characteristic(&charInit, &charImpl->attrHandles(), nullptr) != SYSTEM_ERROR_NONE) {
        return characteristic;
    }
    charImpl->isLocal(true);
    LOG_DEBUG(TRACE, "Add new local characteristic.");
    if(!impl()->characteristics().append(characteristic)) {
        LOG(ERROR, "Failed to append local characteristic.");
    }
    return characteristic;
}

BleCharacteristic BleLocalDevice::addCharacteristic(EnumFlags<BleCharacteristicProperty> properties, const char* desc, BleOnDataReceivedCallback callback, void* context) {
    WiringBleLock lk;
    BleCharacteristic characteristic(properties, desc, callback, context);
    addCharacteristic(characteristic);
    return characteristic;
}

BleCharacteristic BleLocalDevice::addCharacteristic(EnumFlags<BleCharacteristicProperty> properties, const String& desc, BleOnDataReceivedCallback callback, void* context) {
    WiringBleLock lk;
    return addCharacteristic(properties, desc.c_str(), callback, context);
}

BleCharacteristic BleLocalDevice::addCharacteristic(EnumFlags<BleCharacteristicProperty> properties, const char* desc, const BleOnDataReceivedStdFunction& callback) {
    WiringBleLock lk;
    BleCharacteristic characteristic(properties, desc, callback);
    addCharacteristic(characteristic);
    return characteristic;
}

BleCharacteristic BleLocalDevice::addCharacteristic(EnumFlags<BleCharacteristicProperty> properties, const String& desc, const BleOnDataReceivedStdFunction& callback) {
    WiringBleLock lk;
    return addCharacteristic(properties, desc.c_str(), callback);
}

} /* namespace particle */

#endif /* Wiring_BLE */
