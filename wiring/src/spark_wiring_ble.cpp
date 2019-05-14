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

#include "spark_wiring_ble.h"

#if Wiring_BLE
#include "spark_wiring_thread.h"
#include <memory>
#include <algorithm>
#include "hex_to_bytes.h"

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
 * BleUuid class
 */
BleUuid::BleUuid(const BleUuid& uuid)
        : uuid_(uuid.uuid_) {
}

BleUuid::BleUuid(const uint8_t* uuid128, BleUuidOrder order)
        : BleUuid() {
    if (uuid128 == nullptr) {
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
    if (uuid == nullptr) {
        memset(uuid_.uuid128, 0x00, BLE_SIG_UUID_128BIT_LEN);
    } else {
        size_t len = BLE_SIG_UUID_128BIT_LEN;
        for (size_t i = 0; i < strlen(uuid); i++) {
            int8_t hi = hexToNibble(uuid[i]);
            if (hi >= 0) {
                uuid_.uuid128[len - 1] = hi << 4;
                if (++i < strlen(uuid)) {
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
    }
    uuid_.type = BLE_UUID_TYPE_128BIT;
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

bool BleUuid::operator==(const BleUuid& uuid) const {
    if (type() == BleUuidType::SHORT) {
        return (uuid_.uuid16 == uuid.uuid_.uuid16);
    } else {
        return !memcmp(uuid_.uuid128, uuid.uuid_.uuid128, BLE_SIG_UUID_128BIT_LEN);
    }
}


/*******************************************************
 * BleAdvertisingData class
 */
BleAdvertisingData::BleAdvertisingData()
        : selfData_(),
          selfLen_(0) {
}

size_t BleAdvertisingData::set(const uint8_t* buf, size_t len) {
    if (buf == nullptr) {
        return selfLen_;
    }
    len = std::min(len, (size_t)BLE_MAX_ADV_DATA_LEN);
    memcpy(selfData_, buf, len);
    selfLen_ = len;
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
    return append(BleAdvertisingDataType::COMPLETE_LOCAL_NAME, (const uint8_t*)name, strlen(name), false);
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

const uint8_t* BleAdvertisingData::data() const {
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
                uint16_t temp = (uint16_t)selfData_[i + offset + 2] | ((uint16_t)selfData_[i + offset + 3] << 8);
                BleUuid uuid(temp);
                uuids[found++] = uuid;
            } else if (adsLen == 18) {
                BleUuid uuid(&selfData_[i + offset + 2]);
                uuids[found++] = uuid;
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
            : properties(BleCharacteristicProperty::NONE),
              uuid(),
              svcUuid(),
              description(),
              isLocal(true),
              dataCb(nullptr),
              context(nullptr),
              connHandle(BLE_INVALID_CONN_HANDLE),
              svcImpl(nullptr),
              valid_(false) {
    }

    BleCharacteristicImpl(const char* desc, BleCharacteristicProperty properties, BleOnDataReceivedCallback callback, void* context)
            : BleCharacteristicImpl() {
        this->description = desc;
        this->properties = properties;
        this->dataCb = callback;
        this->context = context;
    }

    BleCharacteristicImpl(const char* desc, BleCharacteristicProperty properties, BleUuid& charUuid, BleUuid& svcUuid, BleOnDataReceivedCallback callback, void* context)
            : BleCharacteristicImpl(desc, properties, callback, context) {
        this->uuid = charUuid;
        this->svcUuid = svcUuid;
    }

    ~BleCharacteristicImpl() = default;

    ssize_t getValue(uint8_t* buf, size_t len) const {
        if (buf == nullptr || len == 0) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        len = len > BLE_MAX_ATTR_VALUE_PACKET_SIZE ? BLE_MAX_ATTR_VALUE_PACKET_SIZE : len;
        if (isLocal) {
            return hal_ble_gatt_server_get_characteristic_value(attrHandles.value_handle, buf, len, nullptr);
        } else if(valid_) {
            return hal_ble_gatt_client_read(connHandle, attrHandles.value_handle, buf, len, nullptr);
        } else {
            return SYSTEM_ERROR_INVALID_STATE;
        }
    }

    ssize_t setValue(const uint8_t* buf, size_t len) {
        if (buf == nullptr || len == 0) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        len = len > BLE_MAX_ATTR_VALUE_PACKET_SIZE ? BLE_MAX_ATTR_VALUE_PACKET_SIZE : len;
        if (isLocal) {
            return hal_ble_gatt_server_set_characteristic_value(attrHandles.value_handle, buf, len, nullptr);
        } else if(valid_) {
            if ((properties & BleCharacteristicProperty::WRITE) == BleCharacteristicProperty::WRITE) {
                return hal_ble_gatt_client_write_with_response(connHandle, attrHandles.value_handle, buf, len, nullptr);
            } else if ((properties & BleCharacteristicProperty::WRITE_WO_RSP) == BleCharacteristicProperty::WRITE_WO_RSP) {
                return hal_ble_gatt_client_write_without_response(connHandle, attrHandles.value_handle, buf, len, nullptr);
            } else {
                return SYSTEM_ERROR_NOT_SUPPORTED;
            }
        } else {
            return SYSTEM_ERROR_INVALID_STATE;
        }
    }

    void assignUuidIfNeeded() {
        if (!uuid.isValid()) {
            LOG_DEBUG(TRACE, "Assign default characteristic UUID.");
            defaultUuidCharCount_++;
            BleUuid newUuid(PARTICLE_DEFAULT_BLE_SVC_UUID, defaultUuidCharCount_);
            uuid = newUuid;
        }
    }

    void setValid(bool valid) {
        valid_ = valid;
    }

    void processReceivedData(BleAttributeHandle attrHandle, const uint8_t* data, size_t len, const BlePeerDevice& peer) {
        if (data == nullptr || len == 0) {
            return;
        }
        if (dataCb && attrHandle == attrHandles.value_handle) {
            dataCb(data, len, peer, context);
        }
    }

    bool operator==(const BleCharacteristicImpl& impl) {
        if (uuid == impl.uuid
                && svcUuid == impl.svcUuid
                && isLocal == impl.isLocal) {
            return true;
        }
        return false;
    }

    BleCharacteristicProperty properties;
    BleUuid uuid;
    BleUuid svcUuid;
    String description;
    bool isLocal;
    BleCharacteristicHandles attrHandles;
    BleOnDataReceivedCallback dataCb;
    void* context;
    BleConnectionHandle connHandle; // For peer characteristic
    BleServiceImpl* svcImpl; // Related service

private:
    static uint16_t defaultUuidCharCount_;
    bool valid_;
};

uint16_t BleCharacteristicImpl::defaultUuidCharCount_ = 0;


/*******************************************************
 * BleCharacteristic class
 */
BleCharacteristic::BleCharacteristic()
        : impl_(std::make_shared<BleCharacteristicImpl>()) {
    DEBUG("BleCharacteristic(), 0x%08X -> 0x%08X, count: %d", this, impl(), impl_.use_count());
}

BleCharacteristic::BleCharacteristic(const BleCharacteristic& characteristic)
        : impl_(characteristic.impl_) {
    DEBUG("BleCharacteristic(copy), 0x%08X => 0x%08X -> 0x%08X, count: %d", &characteristic, this, impl(), impl_.use_count());
}

BleCharacteristic::BleCharacteristic(const char* desc, BleCharacteristicProperty properties, BleOnDataReceivedCallback callback, void* context)
        : impl_(std::make_shared<BleCharacteristicImpl>(desc, properties, callback, context)) {
    DEBUG("BleCharacteristic(...), 0x%08X -> 0x%08X, count: %d", this, impl(), impl_.use_count());
}

void BleCharacteristic::construct(const char* desc, BleCharacteristicProperty properties, BleUuid& charUuid, BleUuid& svcUuid, BleOnDataReceivedCallback callback, void* context) {
    impl_ = std::make_shared<BleCharacteristicImpl>(desc, properties, charUuid, svcUuid, callback, context);
    DEBUG("BleCharacteristic(), construct(...):0x%08X -> 0x%08X, count: %d", this, impl(), impl_.use_count());
}

BleCharacteristic& BleCharacteristic::operator=(const BleCharacteristic& characteristic) {
    DEBUG("BleCharacteristic(), operator=:0x%08X -> 0x%08X, count: %d", this, impl(), impl_.use_count());
    BleOnDataReceivedCallback preDataCb = nullptr;
    void* preContext = nullptr;
    if (impl() != nullptr) {
        if (impl()->dataCb != nullptr) {
            preDataCb = impl()->dataCb;
            preContext = impl()->context;
        }
    }

    impl_ = characteristic.impl_;
    if (impl()->dataCb == nullptr) {
        impl()->dataCb = preDataCb;
        impl()->context = preContext;
    }
    DEBUG("now:0x%08X -> 0x%08X, count: %d", this, impl(), impl_.use_count());
    return *this;
}

BleCharacteristic::~BleCharacteristic() {
    DEBUG("~BleCharacteristic(), 0x%08X -> 0x%08X, count: %d", this, impl(), impl_.use_count() - 1);
}

BleUuid BleCharacteristic::UUID() const {
    if (impl() != nullptr) {
        return impl()->uuid;
    }
    BleUuid uuid((uint16_t)0x0000);
    return uuid;
}

BleCharacteristicProperty BleCharacteristic::properties() const {
    if (impl() != nullptr) {
        return impl()->properties;
    }
    return BleCharacteristicProperty::NONE;
}

ssize_t BleCharacteristic::setValue(const uint8_t* buf, size_t len) {
    if (impl() != nullptr) {
        return impl()->setValue(buf, len);
    }
    return 0;
}

ssize_t BleCharacteristic::setValue(const String& str) {
    return setValue(reinterpret_cast<const uint8_t*>(str.c_str()), str.length());
}

ssize_t BleCharacteristic::setValue(const char* str) {
    return setValue(reinterpret_cast<const uint8_t*>(str), strlen(str));
}

ssize_t BleCharacteristic::getValue(uint8_t* buf, size_t len) const {
    if (impl() != nullptr) {
        return impl()->getValue(buf, len);
    }
    return 0;
}

ssize_t BleCharacteristic::getValue(String& str) const {
    return 0;
}

void BleCharacteristic::onDataReceived(BleOnDataReceivedCallback callback, void* context) {
    if (impl() != nullptr) {
        impl()->dataCb = callback;
        impl()->context = context;
    }
}


/*******************************************************
 * BleServiceImpl definition
 */
class BleServiceImpl {
public:
    BleServiceImpl()
            : uuid(),
              startHandle(BLE_INVALID_ATTR_HANDLE),
              endHandle(BLE_INVALID_ATTR_HANDLE) {
    }
    BleServiceImpl(const BleUuid& svcUuid)
            : BleServiceImpl() {
        uuid = svcUuid;
    }
    ~BleServiceImpl() = default;

    Vector<BleCharacteristic>& characteristics() {
        return characteristics_;
    }

    BleCharacteristic* getCharacteristic(const char* desc) {
        if (desc == nullptr) {
            return nullptr;
        }
        for (auto& characteristic : characteristics_) {
            if (characteristic.impl() != nullptr && !strcmp(characteristic.impl()->description.c_str(), desc)) {
                return &characteristic;
            }
        }
        return nullptr;
    }

    BleCharacteristic* getCharacteristic(BleAttributeHandle attrHandle) {
        for (auto& characteristic : characteristics_) {
            BleCharacteristicImpl* charImpl = characteristic.impl();
            if (charImpl != nullptr) {
                if (   charImpl->attrHandles.decl_handle == attrHandle
                    || charImpl->attrHandles.value_handle == attrHandle
                    || charImpl->attrHandles.user_desc_handle == attrHandle
                    || charImpl->attrHandles.cccd_handle == attrHandle
                    || charImpl->attrHandles.sccd_handle == attrHandle) {
                    return &characteristic;
                }
            }
        }
        return nullptr;
    }

    BleCharacteristic* getCharacteristic(const BleUuid& charUuid) {
        for (auto& characteristic : characteristics_) {
            if (characteristic.impl() != nullptr && characteristic.impl()->uuid == charUuid) {
                return &characteristic;
            }
        }
        return nullptr;
    }

    int addCharacteristic(BleCharacteristic& characteristic) {
        BleCharacteristicImpl* charImpl = characteristic.impl();
        if (charImpl == nullptr || contains(characteristic) || charImpl->properties == BleCharacteristicProperty::NONE) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        if (characteristic.impl()->isLocal) {
            characteristic.impl()->assignUuidIfNeeded();
            hal_ble_char_init_t char_init = {};
            char_init.size = sizeof(hal_ble_char_init_t);
            char_init.uuid = characteristic.impl()->uuid.UUID();
            char_init.properties = static_cast<uint8_t>(characteristic.impl()->properties);
            char_init.service_handle = startHandle;
            char_init.description = characteristic.impl()->description.c_str();
            int ret = hal_ble_gatt_server_add_characteristic(&char_init, &characteristic.impl()->attrHandles, nullptr);
            if (ret != SYSTEM_ERROR_NONE) {
                return ret;
            }
        }
        characteristic.impl()->svcImpl = this;
        characteristic.impl()->setValid(true);
        LOG_DEBUG(TRACE, "characteristics.append(characteristic)");
        characteristics_.append(characteristic);
        return SYSTEM_ERROR_NONE;
    }

    BleUuid uuid;
    BleAttributeHandle startHandle;
    BleAttributeHandle endHandle;

private:
    bool contains(const BleCharacteristic& characteristic) {
        if (characteristic.impl() != nullptr) {
            for (const auto& stubChar : characteristics_) {
                if (*characteristic.impl() == *stubChar.impl()) {
                    return true;
                }
            }
        }
        return false;
    }

    Vector<BleCharacteristic> characteristics_;
};


/*******************************************************
 * BleService class
 */
BleService::BleService()
        : impl_(std::make_shared<BleServiceImpl>()) {
}

BleService::BleService(const BleUuid& uuid)
        : impl_(std::make_shared<BleServiceImpl>(uuid)) {
}


/*******************************************************
 * BleGattServerImpl definition
 */
class BleGattServerImpl {
public:
    BleGattServerImpl() = delete;
    BleGattServerImpl(bool local)
            : local_(local) {
    }
    ~BleGattServerImpl() = default;

    Vector<BleService>& services() {
        return services_;
    }

    int addService(BleService& svc) {
        if (getService(svc.impl()->uuid) != nullptr) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        if (local_) {
            hal_ble_uuid_t halUuid = svc.impl()->uuid.UUID();
            int ret = hal_ble_gatt_server_add_service(BLE_SERVICE_TYPE_PRIMARY, &halUuid, &svc.impl()->startHandle, nullptr);
            if (ret != SYSTEM_ERROR_NONE) {
                return ret;
            }
        }
        DEBUG("services.append(service)");
        services_.append(svc);
        return SYSTEM_ERROR_NONE;
    }

    int addCharacteristic(BleCharacteristic& characteristic) {
        if (characteristic.impl() == nullptr) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        characteristic.impl()->isLocal = local_;
        if (local_) {
            LOG_DEBUG(TRACE, "< LOCAL CHARACTERISTIC >");
            if (!characteristic.impl()->svcUuid.isValid()) {
                BleUuid newUuid(PARTICLE_DEFAULT_BLE_SVC_UUID);
                LOG_DEBUG(TRACE, "Assign default service UUID.");
                characteristic.impl()->svcUuid = newUuid;
            }
        }
        BleService* service = getService(characteristic.impl()->svcUuid);
        if (service != nullptr) {
            if (service->impl() != nullptr) {
                return service->impl()->addCharacteristic(characteristic);
            }
            return SYSTEM_ERROR_INTERNAL;
        } else {
            BleService service(characteristic.impl()->svcUuid);
            if (addService(service) == SYSTEM_ERROR_NONE) {
                return services_.last().impl()->addCharacteristic(characteristic);
            }
            return SYSTEM_ERROR_INTERNAL;
        }
    }

    template <typename T>
    BleCharacteristic getCharacteristic(T type) const {
        for (auto& service : services_) {
            BleCharacteristic* characteristic = service.impl()->getCharacteristic(type);
            if (characteristic != nullptr) {
                return *characteristic;
            }
        }
        BleCharacteristic temp;
        return temp;
    }

    void gattsProcessDisconnected(const BlePeerDevice& peer) {
    }

    void gattsProcessDataWritten(BleAttributeHandle attrHandle, const uint8_t* buf, size_t len, const BlePeerDevice& peer) {
        for (auto& service : services_) {
            BleCharacteristic* characteristic = service.impl()->getCharacteristic(attrHandle);
            if (characteristic != nullptr) {
                characteristic->impl()->processReceivedData(attrHandle, buf, len, peer);
            }
        }
    }

private:
    BleService* getService(const BleUuid& uuid) {
        for (auto& service : services_) {
            if (service.impl()->uuid == uuid) {
                return &service;
            }
        }
        return nullptr;
    }

    Vector<BleService> services_;
    bool local_;
};


/*******************************************************
 * BlePeerDeviceImpl definition
 */
class BlePeerDeviceImpl {
public:
    BlePeerDeviceImpl()
            : role(BLE_ROLE_INVALID),
              address(),
              connParams(),
              connHandle(BLE_INVALID_CONN_HANDLE),
              rssi(0x7F),
              gattsProxy(std::make_unique<BleGattServerImpl>(false)) {
    }
    ~BlePeerDeviceImpl() = default;

    void invalidate() {
        role = BLE_ROLE_INVALID;
        connHandle = BLE_INVALID_CONN_HANDLE;
        rssi = 0x7F;
        connParams = {};
    }

    hal_ble_role_t role;
    BleAddress address;
    BleConnectionParams connParams;
    BleConnectionHandle connHandle;
    int8_t rssi;
    std::unique_ptr<BleGattServerImpl> gattsProxy;
};

/*******************************************************
 * BleGattClientImpl definition
 */
class BleGattClientImpl {
public:
    BleGattClientImpl() = default;
    ~BleGattClientImpl() = default;

    int discoverAllServices(BlePeerDevice& peer) {
        LOG(TRACE, "Start discovering services.");
        int ret = hal_ble_gatt_client_discover_all_services(peer.impl()->connHandle, onServicesDiscovered, &peer, nullptr);
        if (ret == SYSTEM_ERROR_NONE) {
            for (auto& service : peer.impl()->gattsProxy->services()) {
                hal_ble_svc_t halService;
                halService.size = sizeof(hal_ble_svc_t);
                halService.start_handle = service.impl()->startHandle;
                halService.end_handle = service.impl()->endHandle;
                LOG(TRACE, "Start discovering characteristics.");
                ret = hal_ble_gatt_client_discover_characteristics(peer.impl()->connHandle, &halService, onCharacteristicsDiscovered, &service, nullptr);
                if (ret != SYSTEM_ERROR_NONE) {
                    return ret;
                }
                for (auto& characteristic : service.impl()->characteristics()) {
                    // Enable notification or indication if presented.
                    if (characteristic.impl()->attrHandles.cccd_handle != BLE_INVALID_ATTR_HANDLE) {
                        if ((characteristic.impl()->properties & BleCharacteristicProperty::NOTIFY) == BleCharacteristicProperty::NOTIFY) {
                            hal_ble_gatt_client_configure_cccd(peer.impl()->connHandle, characteristic.impl()->attrHandles.cccd_handle, BLE_SIG_CCCD_VAL_NOTIFICATION, nullptr);
                        } else if ((characteristic.impl()->properties & BleCharacteristicProperty::INDICATE) == BleCharacteristicProperty::INDICATE) {
                            hal_ble_gatt_client_configure_cccd(peer.impl()->connHandle, characteristic.impl()->attrHandles.cccd_handle, BLE_SIG_CCCD_VAL_INDICATION, nullptr);
                        }
                    }
                    // Read the user description string if presented.
                    if (characteristic.impl()->attrHandles.user_desc_handle != BLE_INVALID_ATTR_HANDLE) {
                        char desc[32]; // FIXME: use macro definition instead.
                        size_t len = hal_ble_gatt_client_read(peer.impl()->connHandle, characteristic.impl()->attrHandles.user_desc_handle, (uint8_t*)desc, sizeof(desc) - 1, nullptr);
                        if (len > 0) {
                            desc[len] = '\0';
                            characteristic.impl()->description = desc;
                            LOG_DEBUG(TRACE, "User description: %s.", desc);
                        }
                    }
                }
            }
        }
        else {
            LOG(ERROR, "hal_ble_gatt_client_discover_all_services() failed; %d", ret);
        }
        return ret;
    }

    void gattcProcessDataNotified(BleAttributeHandle attrHandle, const uint8_t* buf, size_t len, const BlePeerDevice& peer) {
        peer.impl()->gattsProxy->gattsProcessDataWritten(attrHandle, buf, len, peer);
    }

    /*
     * WARN: This is executed from HAL ble thread. The current thread which starts the service discovery procedure
     * has acquired the BLE HAL lock. Calling BLE HAL APIs those acquiring the BLE HAL lock in this function
     * will suspend the execution of the callback until the current thread release the BLE HAL lock. Or the BLE HAL
     * APIs those are invoked here must not acquire the BLE HAL lock.
     */
    static void onServicesDiscovered(const hal_ble_gattc_on_svc_disc_evt_t* event, void* context) {
        BlePeerDevice* peer = static_cast<BlePeerDevice*>(context);
        for (size_t i = 0; i < event->count; i++) {
            BleUuid svcUUID;
            svcUUID.UUID() = event->services[i].uuid;
            BleService service(svcUUID);
            service.impl()->startHandle = event->services[i].start_handle;
            service.impl()->endHandle = event->services[i].end_handle;
            if (peer->impl()->gattsProxy->addService(service) == SYSTEM_ERROR_NONE) {
                LOG_DEBUG(TRACE, "New service found.");
            }
        }
    }

    /*
     * WARN: This is executed from HAL ble thread. The current thread which starts the characteristic discovery procedure
     * has acquired the BLE HAL lock. Calling BLE HAL APIs those acquiring the BLE HAL lock in this function
     * will suspend the execution of the callback until the current thread release the BLE HAL lock. Or the BLE HAL
     * APIs those are invoked here must not acquire the BLE HAL lock.
     */
    static void onCharacteristicsDiscovered(const hal_ble_gattc_on_char_disc_evt_t* event, void* context) {
        BleService* service = static_cast<BleService*>(context);
        for (size_t i = 0; i < event->count; i++) {
            BleCharacteristic characteristic;
            characteristic.impl()->isLocal = false;
            characteristic.impl()->connHandle = event->conn_handle;
            characteristic.impl()->svcUuid = service->impl()->uuid;
            if (event->characteristics[i].properties & BLE_SIG_CHAR_PROP_READ) {
                characteristic.impl()->properties |= BleCharacteristicProperty::READ;
            }
            if (event->characteristics[i].properties & BLE_SIG_CHAR_PROP_WRITE_WO_RESP) {
                characteristic.impl()->properties |= BleCharacteristicProperty::WRITE_WO_RSP;
            }
            if (event->characteristics[i].properties & BLE_SIG_CHAR_PROP_WRITE) {
                characteristic.impl()->properties |= BleCharacteristicProperty::WRITE;
            }
            if (event->characteristics[i].properties & BLE_SIG_CHAR_PROP_NOTIFY){
                characteristic.impl()->properties |= BleCharacteristicProperty::NOTIFY;
            }
            if (event->characteristics[i].properties & BLE_SIG_CHAR_PROP_INDICATE) {
                characteristic.impl()->properties |= BleCharacteristicProperty::INDICATE;
            }
            BleUuid charUUID;
            charUUID.UUID() = event->characteristics[i].uuid;
            characteristic.impl()->uuid = charUUID;
            characteristic.impl()->attrHandles = event->characteristics[i].charHandles;
            if (service->impl()->addCharacteristic(characteristic) == SYSTEM_ERROR_NONE) {
                LOG_DEBUG(TRACE, "New characteristic found.");
            }
        }
    }
};


/*******************************************************
 * BleBroadcasterImpl definition
 */
class BleBroadcasterImpl {
public:
    BleBroadcasterImpl() = default;
    ~BleBroadcasterImpl() = default;

    int setTxPower(int8_t txPower) const {
        return hal_ble_gap_set_tx_power(txPower);
    }

    int txPower(int8_t* txPower) const {
        return hal_ble_gap_get_tx_power(txPower, nullptr);
    }

    int setAdvertisingInterval(uint16_t interval) {
        hal_ble_adv_params_t advParams;
        advParams.size = sizeof(hal_ble_adv_params_t);
        int ret = hal_ble_gap_get_advertising_parameters(&advParams, nullptr);
        if (ret == SYSTEM_ERROR_NONE) {
            advParams.interval = interval;
            ret = hal_ble_gap_set_advertising_parameters(&advParams, nullptr);
        }
        return ret;
    }

    int setAdvertisingTimeout(uint16_t timeout) {
        hal_ble_adv_params_t advParams;
        advParams.size = sizeof(hal_ble_adv_params_t);
        int ret = hal_ble_gap_get_advertising_parameters(&advParams, nullptr);
        if (ret == SYSTEM_ERROR_NONE) {
            advParams.timeout = timeout;
            ret = hal_ble_gap_set_advertising_parameters(&advParams, nullptr);
        }
        return ret;
    }

    int setAdvertisingType(BleAdvertisingEventType type) {
        hal_ble_adv_params_t advParams;
        advParams.size = sizeof(hal_ble_adv_params_t);
        int ret = hal_ble_gap_get_advertising_parameters(&advParams, nullptr);
        if (ret == SYSTEM_ERROR_NONE) {
            advParams.type = static_cast<hal_ble_adv_evt_type_t>(type);
            ret = hal_ble_gap_set_advertising_parameters(&advParams, nullptr);
        }
        return ret;
    }

    int setAdvertisingParams(const BleAdvertisingParams* params) {
        return hal_ble_gap_set_advertising_parameters(params, nullptr);
    }

    int setIbeacon(const iBeacon& iBeacon) {
        if (iBeacon.uuid.type() == BleUuidType::SHORT) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }

        BleAdvertisingData advertisingData;
        advertisingData.clear();

        // AD flags
        uint8_t flags = BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
        advertisingData.append(BleAdvertisingDataType::FLAGS, &flags, 1);

        uint8_t mfgData[BLE_MAX_ADV_DATA_LEN];
        size_t mfgDataLen = 0;

        // Company ID
        memcpy(&mfgData[mfgDataLen], (uint8_t*)&iBeacon::APPLE_COMPANY_ID, sizeof(iBeacon::APPLE_COMPANY_ID));
        mfgDataLen += sizeof(iBeacon::APPLE_COMPANY_ID);
        // Beacon type: iBeacon
        mfgData[mfgDataLen++] = iBeacon::BEACON_TYPE_IBEACON;
        // Length of the following payload
        mfgData[mfgDataLen++] = 0x15;
        // 128-bits Beacon UUID, MSB
        for (size_t i = 0, j = BLE_SIG_UUID_128BIT_LEN - 1; i < BLE_SIG_UUID_128BIT_LEN; i++, j--) {
            mfgData[mfgDataLen + i] = iBeacon.uuid.full()[j];
        }
        mfgDataLen += BLE_SIG_UUID_128BIT_LEN;
        // Major, MSB
        mfgData[mfgDataLen++] = (uint8_t)((iBeacon.major >> 8) & 0x00FF);
        mfgData[mfgDataLen++] = (uint8_t)(iBeacon.major & 0x00FF);
        // Minor, MSB
        mfgData[mfgDataLen++] = (uint8_t)((iBeacon.minor >> 8) & 0x00FF);
        mfgData[mfgDataLen++] = (uint8_t)(iBeacon.minor & 0x00FF);
        // Measure power
        mfgData[mfgDataLen++] = iBeacon.measurePower;
        advertisingData.appendCustomData(mfgData, mfgDataLen);

        int ret = hal_ble_gap_set_advertising_data(advertisingData.data(), advertisingData.length(), nullptr);
        if (ret == SYSTEM_ERROR_NONE) {
            // Clear the scan response data.
            ret = hal_ble_gap_set_scan_response_data(nullptr, 0, nullptr);
        }
        return ret;
    }

    int setAdvertisingData(const BleAdvertisingData* advData) {
        int ret;
        if (advData == nullptr) {
            ret = hal_ble_gap_set_advertising_data(nullptr, 0, nullptr);
        } else {
            if (advData->contains(BleAdvertisingDataType::FLAGS)) {
                ret = hal_ble_gap_set_advertising_data(advData->data(), advData->length(), nullptr);
            } else {
                uint8_t temp[BLE_MAX_ADV_DATA_LEN];
                hal_ble_gap_get_advertising_data(temp, sizeof(temp), nullptr);
                // Keep the previous AD Flags
                size_t len = advData->get(&temp[3], sizeof(temp) - 3);
                ret = hal_ble_gap_set_advertising_data(temp, len + 3, nullptr);
            }
        }
        if (ret != SYSTEM_ERROR_NONE) {
            LOG_DEBUG(TRACE, "ble_gap_set_advertising_data failed: %d", ret);
        }
        return ret;
    }

    int setScanResponseData(const BleAdvertisingData* srData) {
        int ret;
        if (srData == nullptr) {
            ret = hal_ble_gap_set_scan_response_data(nullptr, 0, nullptr);
            // TODO: keep the Particle vendor ID
        } else {
            BleAdvertisingData scanRespData = *srData;
            scanRespData.remove(BleAdvertisingDataType::FLAGS);
            ret = hal_ble_gap_set_scan_response_data(scanRespData.data(), scanRespData.length(), nullptr);
        }
        if (ret != SYSTEM_ERROR_NONE) {
            LOG_DEBUG(TRACE, "ble_gap_set_scan_response_data failed: %d", ret);
        }
        return ret;
    }

    int advertise() {
        return hal_ble_gap_start_advertising(nullptr);
    }

    int advertise(bool connectable) {
        int ret;
        if (connectable) {
            ret = setAdvertisingType(BleAdvertisingEventType::CONNECTABLE_SCANNABLE_UNDIRECRED);
        } else {
            ret = setAdvertisingType(BleAdvertisingEventType::SCANABLE_UNDIRECTED);
        }
        if (ret == SYSTEM_ERROR_NONE) {
            ret = advertise();
        }
        return ret;
    }

    int stopAdvertising() const {
        return hal_ble_gap_stop_advertising();
    }

    bool advertising() const {
        return hal_ble_gap_is_advertising();
    }

    void broadcasterProcessStopped() {
        return;
    }
};


/*******************************************************
 * BleObserverImpl definition
 */
class BleObserverImpl {
public:
    BleObserverImpl()
            : callback_(nullptr),
              context_(nullptr),
              resultsPtr_(nullptr),
              targetCount_(0),
              count_(0) {
    }
    ~BleObserverImpl() = default;

    int scan(BleOnScanResultCallback callback, void* context) {
        init();
        callback_ = callback;
        context_ = context;
        hal_ble_gap_start_scan(observerProcessScanResult, this, nullptr);
        return count_;
    }

    int scan(BleOnScanResultCallback callback, uint16_t timeout, void* context) {
        hal_ble_scan_params_t params;
        params.size = sizeof(hal_ble_scan_params_t);
        hal_ble_gap_get_scan_parameters(&params, nullptr);
        params.timeout = timeout;
        hal_ble_gap_set_scan_parameters(&params, nullptr);
        return scan(callback, context);
    }

    int scan(BleScanResult* results, size_t resultCount) {
        init();
        resultsPtr_ = results;
        targetCount_ = resultCount;
        hal_ble_gap_start_scan(observerProcessScanResult, this, nullptr);
        return count_;
    }

    int scan(BleScanResult* results, size_t resultCount, uint16_t timeout) {
        hal_ble_scan_params_t params;
        params.size = sizeof(hal_ble_scan_params_t);
        hal_ble_gap_get_scan_parameters(&params, nullptr);
        params.timeout = timeout;
        hal_ble_gap_set_scan_parameters(&params, nullptr);
        return scan(results, resultCount);
    }

    int scan(BleScanResult* results, size_t resultCount, const BleScanParams& params) {
        hal_ble_gap_set_scan_parameters(&params, nullptr);
        return scan(results, resultCount);
    }

    Vector<BleScanResult> scan() {
        init();
        hal_ble_gap_start_scan(observerProcessScanResult, this, nullptr);
        return resultsVector_;
    }

    Vector<BleScanResult> scan(uint16_t timeout) {
        hal_ble_scan_params_t params;
        params.size = sizeof(hal_ble_scan_params_t);
        hal_ble_gap_get_scan_parameters(&params, nullptr);
        params.timeout = timeout;
        hal_ble_gap_set_scan_parameters(&params, nullptr);
        return scan();
    }

    Vector<BleScanResult> scan(const BleScanParams& params) {
        hal_ble_gap_set_scan_parameters(&params, nullptr);
        return scan();
    }

    int stopScanning() {
        return hal_ble_gap_stop_scan();
    }

    /*
     * WARN: This is executed from HAL ble thread. The current thread which starts the scanning procedure
     * has acquired the BLE HAL lock. Calling BLE HAL APIs those acquiring the BLE HAL lock in this function
     * will suspend the execution of the callback until the current thread release the BLE HAL lock. Or the BLE HAL
     * APIs those are invoked here must not acquire the BLE HAL lock.
     */
    static void observerProcessScanResult(const hal_ble_gap_on_scan_result_evt_t* event, void* context) {
        BleObserverImpl* impl = static_cast<BleObserverImpl*>(context);
        BleScanResult result;
        result.address = event->peer_addr;
        result.rssi = event->rssi;
        result.scanResponse.set(event->sr_data, event->sr_data_len);
        result.advertisingData.set(event->adv_data, event->adv_data_len);

        if (impl->callback_ != nullptr) {
            impl->callback_(&result, impl->context_);
        } else if (impl->resultsPtr_ != nullptr) {
            if (impl->count_ < impl->targetCount_) {
                impl->resultsPtr_[impl->count_++] = result;
                if (impl->count_ >= impl->targetCount_) {
                    impl->stopScanning();
                }
            }
        } else {
            impl->resultsVector_.append(result);
        }
    }

private:
    void init() {
        count_ = 0;
        callback_ = nullptr;
        context_=nullptr;
        resultsPtr_ = nullptr;
        resultsVector_.clear();
    }

    Vector<BleScanResult> resultsVector_;
    BleOnScanResultCallback callback_;
    void* context_;
    BleScanResult* resultsPtr_;
    size_t targetCount_;
    size_t count_;
};


/*******************************************************
 * BlePeripheralImpl definition
 */
class BlePeripheralImpl {
public:
    BlePeripheralImpl() = default;
    ~BlePeripheralImpl() = default;

    Vector<BlePeerDevice>& centrals() {
        return centrals_;
    }

    int setPPCP(uint16_t minInterval, uint16_t maxInterval, uint16_t latency, uint16_t timeout) {
        hal_ble_conn_params_t ppcp;
        ppcp.size = sizeof(hal_ble_conn_params_t);
        ppcp.min_conn_interval = minInterval;
        ppcp.max_conn_interval = maxInterval;
        ppcp.slave_latency = latency;
        ppcp.conn_sup_timeout = timeout;
        return hal_ble_gap_set_ppcp(&ppcp, nullptr);
    }

    int disconnect() {
        for (const BlePeerDevice& central : centrals_) {
            hal_ble_gap_disconnect(central.impl()->connHandle, nullptr);
        }
        centrals_.clear();
        return SYSTEM_ERROR_NONE;
    }

    bool connected() const {
        return centrals_.size() > 0;
    }

    void peripheralProcessConnected(const BlePeerDevice& peer) {
        if (centrals_.size() < BLE_MAX_PERIPHERAL_COUNT) {
            centrals_.append(peer);
        }
    }

    void peripheralProcessDisconnected(const BlePeerDevice& peer) {
        for (auto& service : peer.impl()->gattsProxy->services()) {
            for (auto& characteristic : service.impl()->characteristics()) {
                characteristic.impl()->setValid(false);
            }
        }
        peer.impl()->invalidate();
        centrals_.clear();
    }

private:
    Vector<BlePeerDevice> centrals_;
};


/*******************************************************
 * BleCentralImpl definition
 */
class BleCentralImpl {
public:
    BleCentralImpl() = default;
    ~BleCentralImpl() = default;

    Vector<BlePeerDevice>& peripherals() {
        return peripherals_;
    }

    BlePeerDevice connect(const BleAddress& addr) {
        BlePeerDevice pseudo;
        int ret = hal_ble_gap_connect(&addr, nullptr);
        if (ret != SYSTEM_ERROR_NONE) {
            LOG_DEBUG(TRACE, "hal_ble_gap_connect failed: %d", ret);
            return pseudo;
        }
        for (auto& peripheral : peripherals_) {
            if (peripheral.impl()->address == addr) {
                LOG(TRACE, "New peripheral connected.");
                return peripheral;
            }
        }
        return pseudo;
    }

    int disconnect(const BlePeerDevice& peer) {
        for (auto& peripheral : peripherals_) {
            if (peripheral.impl()->connHandle == peer.impl()->connHandle) {
                hal_ble_gap_disconnect(peer.impl()->connHandle, nullptr);
                return SYSTEM_ERROR_NONE;
            }
        }
        return SYSTEM_ERROR_NONE;
    }

    bool connected() const {
        return peripherals_.size() > 0;
    }

    void centralProcessConnected(const BlePeerDevice& peer) {
        if (peripherals_.size() < BLE_MAX_CENTRAL_COUNT) {
            peripherals_.append(peer);
        }
    }

    void centralProcessDisconnected(const BlePeerDevice& peer) {
        for (auto& service : peer.impl()->gattsProxy->services()) {
            for (auto& characteristic : service.impl()->characteristics()) {
                characteristic.impl()->setValid(false);
            }
        }
        peer.impl()->invalidate();
        peripherals_.removeOne(peer);
    }

private:
    Vector<BlePeerDevice> peripherals_;
};


/*******************************************************
 * BlePeerDevice class
 */
BlePeerDevice::BlePeerDevice()
        : impl_(std::make_shared<BlePeerDeviceImpl>()) {
}

BlePeerDevice::~BlePeerDevice() {
    DEBUG("~BlePeerDevice(), 0x%08X -> 0x%08X, count: %d", this, impl(), impl_.use_count() - 1);
}

BleCharacteristic BlePeerDevice::getCharacteristicByDescription(const char* desc) {
    return impl()->gattsProxy->getCharacteristic(desc);
}

BleCharacteristic BlePeerDevice::getCharacteristicByDescription(const String& desc) {
    return impl()->gattsProxy->getCharacteristic(desc.c_str());
}

BleCharacteristic BlePeerDevice::getCharacteristicByUUID(const BleUuid& uuid) {
    return impl()->gattsProxy->getCharacteristic(uuid);
}

bool BlePeerDevice::connected() {
    return impl()->connHandle != BLE_INVALID_CONN_HANDLE;
}

const BleAddress& BlePeerDevice::address() const {
    return impl()->address;
}

bool BlePeerDevice::operator==(const BlePeerDevice& device) {
    if (impl()->connHandle == device.impl()->connHandle && impl()->address == device.impl()->address) {
        return true;
    }
    return false;
}


/*******************************************************
 * BleLocalDevice class
 */
BleLocalDevice::BleLocalDevice()
        : connectedCb_(nullptr),
          disconnectedCb_(nullptr),
          connectedContext(nullptr),
          disconnectedContext(nullptr) {
    hal_ble_stack_init(nullptr);

    // The following members must not be in the initializer list, since it may call
    // BLE HAL APIs and BLE stack must be initialized previous to these APIs.
    gattsProxy_ = std::make_unique<BleGattServerImpl>(true);
    gattcProxy_ = std::make_unique<BleGattClientImpl>();
    broadcasterProxy_ = std::make_unique<BleBroadcasterImpl>();
    observerProxy_ = std::make_unique<BleObserverImpl>();
    peripheralProxy_ = std::make_unique<BlePeripheralImpl>();
    centralProxy_ = std::make_unique<BleCentralImpl>();

    hal_ble_set_callback_on_events(onBleEvents, this);
}

BleLocalDevice::~BleLocalDevice() {

}

BleLocalDevice& BleLocalDevice::getInstance() {
    static BleLocalDevice instance;
    return instance;
}

void BleLocalDevice::onConnected(BleOnConnectedCallback callback, void* context) {
    connectedCb_ = callback;
    connectedContext = context;
}

void BleLocalDevice::onDisconnected(BleOnDisconnectedCallback callback, void* context) {
    disconnectedCb_ = callback;
    disconnectedContext = context;
}

int BleLocalDevice::on() {
    WiringBleLock lk;
    return SYSTEM_ERROR_NONE;
}

int BleLocalDevice::off() {
    WiringBleLock lk;
    return SYSTEM_ERROR_NONE;
}

const BleAddress BleLocalDevice::address() const {
    BleAddress addr = {};
    hal_ble_gap_get_device_address(&addr);
    return addr;
}

int BleLocalDevice::setTxPower(int8_t val) const {
    WiringBleLock lk;
    return broadcasterProxy_->setTxPower(val);
}

int BleLocalDevice::txPower(int8_t* txPower) const {
    WiringBleLock lk;
    return broadcasterProxy_->txPower(txPower);
}

int BleLocalDevice::advertise() const {
    WiringBleLock lk;
    return broadcasterProxy_->advertise();
}

int BleLocalDevice::advertise(const BleAdvertisingData* advertisingData, const BleAdvertisingData* scanResponse) const {
    WiringBleLock lk;
    broadcasterProxy_->setAdvertisingData(advertisingData);
    broadcasterProxy_->setScanResponseData(scanResponse);
    return broadcasterProxy_->advertise();
}

int BleLocalDevice::advertise(uint16_t interval) const {
    WiringBleLock lk;
    broadcasterProxy_->setAdvertisingInterval(interval);
    return broadcasterProxy_->advertise();
}

int BleLocalDevice::advertise(uint16_t interval, const BleAdvertisingData* advertisingData, const BleAdvertisingData* scanResponse) const {
    WiringBleLock lk;
    broadcasterProxy_->setAdvertisingData(advertisingData);
    broadcasterProxy_->setScanResponseData(scanResponse);
    broadcasterProxy_->setAdvertisingInterval(interval);
    return broadcasterProxy_->advertise();
}

int BleLocalDevice::advertise(uint16_t interval, uint16_t timeout) const {
    WiringBleLock lk;
    broadcasterProxy_->setAdvertisingInterval(interval);
    broadcasterProxy_->setAdvertisingTimeout(timeout);
    return broadcasterProxy_->advertise();
}

int BleLocalDevice::advertise(uint16_t interval, uint16_t timeout, const BleAdvertisingData* advertisingData, const BleAdvertisingData* scanResponse) const {
    WiringBleLock lk;
    broadcasterProxy_->setAdvertisingData(advertisingData);
    broadcasterProxy_->setScanResponseData(scanResponse);
    broadcasterProxy_->setAdvertisingInterval(interval);
    broadcasterProxy_->setAdvertisingTimeout(timeout);
    return broadcasterProxy_->advertise();
}

int BleLocalDevice::advertise(const BleAdvertisingParams& params) const {
    WiringBleLock lk;
    broadcasterProxy_->setAdvertisingParams(&params);
    return broadcasterProxy_->advertise();
}

int BleLocalDevice::advertise(const BleAdvertisingParams& params, const BleAdvertisingData* advertisingData, const BleAdvertisingData* scanResponse) const {
    WiringBleLock lk;
    broadcasterProxy_->setAdvertisingData(advertisingData);
    broadcasterProxy_->setScanResponseData(scanResponse);
    broadcasterProxy_->setAdvertisingParams(&params);
    return broadcasterProxy_->advertise();
}

int BleLocalDevice::advertise(const iBeacon& iBeacon, bool connectable) const {
    WiringBleLock lk;
    broadcasterProxy_->setIbeacon(iBeacon);
    return broadcasterProxy_->advertise(connectable);
}

int BleLocalDevice::advertise(uint16_t interval, const iBeacon& iBeacon, bool connectable) const {
    WiringBleLock lk;
    broadcasterProxy_->setIbeacon(iBeacon);
    broadcasterProxy_->setAdvertisingInterval(interval);
    return broadcasterProxy_->advertise(connectable);
}

int BleLocalDevice::advertise(uint16_t interval, uint16_t timeout, const iBeacon& iBeacon, bool connectable) const {
    WiringBleLock lk;
    broadcasterProxy_->setIbeacon(iBeacon);
    broadcasterProxy_->setAdvertisingInterval(interval);
    broadcasterProxy_->setAdvertisingTimeout(timeout);
    return broadcasterProxy_->advertise(connectable);
}

int BleLocalDevice::advertise(const BleAdvertisingParams& params, const iBeacon& iBeacon, bool connectable) const {
    WiringBleLock lk;
    broadcasterProxy_->setIbeacon(iBeacon);
    broadcasterProxy_->setAdvertisingParams(&params);
    return broadcasterProxy_->advertise(connectable);
}

int BleLocalDevice::stopAdvertising() const {
    return broadcasterProxy_->stopAdvertising();
}

bool BleLocalDevice::advertising() const {
    return broadcasterProxy_->advertising();
}

int BleLocalDevice::scan(BleOnScanResultCallback callback, void* context) const {
    WiringBleLock lk;
    return observerProxy_->scan(callback, context);
}

int BleLocalDevice::scan(BleOnScanResultCallback callback, uint16_t timeout, void* context) const {
    WiringBleLock lk;
    return observerProxy_->scan(callback, timeout, context);
}

int BleLocalDevice::scan(BleScanResult* results, size_t resultCount) const {
    WiringBleLock lk;
    return observerProxy_->scan(results, resultCount);
}

int BleLocalDevice::scan(BleScanResult* results, size_t resultCount, uint16_t timeout) const {
    WiringBleLock lk;
    return observerProxy_->scan(results, resultCount, timeout);
}

int BleLocalDevice::scan(BleScanResult* results, size_t resultCount, const BleScanParams& params) const {
    return observerProxy_->scan(results, resultCount, params);
}

Vector<BleScanResult> BleLocalDevice::scan() const {
    WiringBleLock lk;
    return observerProxy_->scan();
}

Vector<BleScanResult> BleLocalDevice::scan(uint16_t timeout) const {
    WiringBleLock lk;
    return observerProxy_->scan(timeout);
}

Vector<BleScanResult> BleLocalDevice::scan(const BleScanParams& params) const {
    WiringBleLock lk;
    return observerProxy_->scan(params);
}

int BleLocalDevice::stopScanning() const {
    return observerProxy_->stopScanning();
}

int BleLocalDevice::setPPCP(uint16_t minInterval, uint16_t maxInterval, uint16_t latency, uint16_t timeout) const {
    WiringBleLock lk;
    return peripheralProxy_->setPPCP(minInterval, maxInterval, latency, timeout);
}

bool BleLocalDevice::connected() const {
    return (peripheralProxy_->connected() || centralProxy_->connected());
}

BlePeerDevice BleLocalDevice::connect(const BleAddress& addr, uint16_t interval, uint16_t latency, uint16_t timeout) const {
    WiringBleLock lk;
    hal_ble_conn_params_t connParams;
    connParams.size = sizeof(hal_ble_conn_params_t);
    connParams.min_conn_interval = interval;
    connParams.max_conn_interval = interval;
    connParams.slave_latency = latency;
    connParams.conn_sup_timeout = timeout;
    hal_ble_gap_set_ppcp(&connParams, nullptr);
    return connect(addr);
}

BlePeerDevice BleLocalDevice::connect(const BleAddress& addr) const {
    WiringBleLock lk;
    BlePeerDevice peer = centralProxy_->connect(addr);
    // Automatically discover services and characteristics.
    if (peer.connected()) {
        gattcProxy_->discoverAllServices(peer);
    }
    return peer;
}

int BleLocalDevice::disconnect() const {
    WiringBleLock lk;
    return peripheralProxy_->disconnect();
}

int BleLocalDevice::disconnect(const BlePeerDevice& peripheral) const {
    WiringBleLock lk;
    return centralProxy_->disconnect(peripheral);
}

int BleLocalDevice::addCharacteristic(BleCharacteristic& characteristic) const {
    WiringBleLock lk;
    return gattsProxy_->addCharacteristic(characteristic);
}

int BleLocalDevice::addCharacteristic(const char* desc, BleCharacteristicProperty properties, BleOnDataReceivedCallback callback) const {
    WiringBleLock lk;
    BleCharacteristic characteristic(desc, properties, callback);
    return gattsProxy_->addCharacteristic(characteristic);
}

int BleLocalDevice::addCharacteristic(const String& desc, BleCharacteristicProperty properties, BleOnDataReceivedCallback callback) const {
    WiringBleLock lk;
    return addCharacteristic(desc.c_str(), properties, callback);
}

BlePeerDevice* BleLocalDevice::findPeerDevice(BleConnectionHandle connHandle) {
    for (auto& central : peripheralProxy_->centrals()) {
        if (central.impl()->connHandle == connHandle) {
            return &central;
        }
    }
    for (auto& peripheral : centralProxy_->peripherals()) {
        if (peripheral.impl()->connHandle == connHandle) {
            return &peripheral;
        }
    }
    return nullptr;
}

/*
 * WARN: This is executed from HAL ble thread. If the BLE wiring lock is acquired by a thread,
 * calling wiring APIs those acquiring the BLE wiring lock from the callback will suspend the
 * the execution of the callback, until the BLE wiring lock is released.
 */
void BleLocalDevice::onBleEvents(const hal_ble_evts_t *event, void* context) {
    if (context == nullptr) {
        return;
    }

    auto bleInstance = static_cast<BleLocalDevice*>(context);

    switch (event->type) {
        case BLE_EVT_ADV_STOPPED: {
            bleInstance->broadcasterProxy_->broadcasterProcessStopped();
            break;
        }
        case BLE_EVT_CONNECTED: {
            BlePeerDevice peer;

            peer.impl()->connParams.conn_sup_timeout = event->params.connected.conn_sup_timeout;
            peer.impl()->connParams.slave_latency = event->params.connected.slave_latency;
            peer.impl()->connParams.max_conn_interval = event->params.connected.conn_interval;
            peer.impl()->connParams.min_conn_interval = event->params.connected.conn_interval;
            peer.impl()->connHandle = event->params.connected.conn_handle;
            peer.impl()->address = event->params.connected.peer_addr;

            if (bleInstance->connectedCb_) {
                bleInstance->connectedCb_(peer, bleInstance->connectedContext);
            }
            if (event->params.connected.role == BLE_ROLE_PERIPHERAL) {
                peer.impl()->role = BLE_ROLE_CENTRAL;
                bleInstance->peripheralProxy_->peripheralProcessConnected(peer);
            } else {
                peer.impl()->role = BLE_ROLE_PERIPHERAL;
                bleInstance->centralProxy_->centralProcessConnected(peer);
            }
            break;
        }
        case BLE_EVT_DISCONNECTED: {
            BlePeerDevice* peer = bleInstance->findPeerDevice(event->params.disconnected.conn_handle);
            if (peer != nullptr) {
                bleInstance->gattsProxy_->gattsProcessDisconnected(*peer);

                if (bleInstance->disconnectedCb_) {
                    bleInstance->disconnectedCb_(*peer, bleInstance->disconnectedContext);
                }
                if (peer->impl()->role == BLE_ROLE_PERIPHERAL) {
                    bleInstance->centralProxy_->centralProcessDisconnected(*peer);
                } else {
                    bleInstance->peripheralProxy_->peripheralProcessDisconnected(*peer);
                }
            }
            break;
        }
        case BLE_EVT_CONN_PARAMS_UPDATED: {
            break;
        }
        case BLE_EVT_DATA_WRITTEN: {
            LOG_DEBUG(TRACE, "onDataWritten, connection: %d, attribute: %d", event->params.data_rec.conn_handle, event->params.data_rec.attr_handle);

            BlePeerDevice* peer = bleInstance->findPeerDevice(event->params.data_rec.conn_handle);
            if (peer != nullptr) {
                bleInstance->gattsProxy_->gattsProcessDataWritten(event->params.data_rec.attr_handle,
                        event->params.data_rec.data, event->params.data_rec.data_len, *peer);
            }
            break;
        }
        case BLE_EVT_DATA_NOTIFIED: {
            LOG_DEBUG(TRACE, "onDataNotified, connection: %d, attribute: %d", event->params.data_rec.conn_handle, event->params.data_rec.attr_handle);

            BlePeerDevice* peer = bleInstance->findPeerDevice(event->params.data_rec.conn_handle);
            if (peer != nullptr) {
                bleInstance->gattcProxy_->gattcProcessDataNotified(event->params.data_rec.attr_handle,
                        event->params.data_rec.data, event->params.data_rec.data_len, *peer);
            }
            break;
        }
        default:{
            break;
        }
    }
}

} /* namespace particle */

#endif /* Wiring_BLE */
