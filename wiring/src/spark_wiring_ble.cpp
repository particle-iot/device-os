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

#include "device_code.h"
#include "logging.h"
#include <memory>

LOG_SOURCE_CATEGORY("wiring.ble")


namespace {

const uint16_t PARTICLE_COMPANY_ID = 0x0662;

const uint8_t BLE_CTRL_REQ_SVC_UUID[BLE_SIG_UUID_128BIT_LEN] = {
    0xfc, 0x36, 0x6f, 0x54, 0x30, 0x80, 0xf4, 0x94, 0xa8, 0x48, 0x4e, 0x5c, 0x01, 0x00, 0xa9, 0x6f
};

void convertToHalUuid(const BleUuid& uuid, hal_ble_uuid_t* halUuid) {
    if (halUuid == nullptr) {
        return;
    }

    if (uuid.type() == BleUuidType::SHORT) {
        halUuid->type = BLE_UUID_TYPE_16BIT;
        halUuid->uuid16 = uuid.shortUuid();
    }
    else {
        halUuid->type = BLE_UUID_TYPE_128BIT;
        if (uuid.order() == BleUuidOrder::LSB) {
            uuid.fullUuid(halUuid->uuid128);
        }
        else {
            for (size_t i = 0, j = BLE_SIG_UUID_128BIT_LEN - 1; i < BLE_SIG_UUID_128BIT_LEN; i++, j--) {
                halUuid->uuid128[i] = uuid.fullUuid()[j];
            }
        }
    }
}

} //anonymous namespace


/*******************************************************
 * BleUuid class
 */
BleUuid::BleUuid() : type_(BleUuidType::SHORT), order_(BleUuidOrder::LSB), shortUuid_(0x0000) {
}

BleUuid::BleUuid(const uint8_t* uuid128, BleUuidOrder order) : order_(order), shortUuid_(0x0000) {
    if (uuid128 == nullptr) {
        memset(fullUuid_, 0x00, BLE_SIG_UUID_128BIT_LEN);
    }
    else {
        memcpy(fullUuid_, uuid128, BLE_SIG_UUID_128BIT_LEN);
    }
    type_ = BleUuidType::LONG;
}

BleUuid::BleUuid(uint16_t uuid16, BleUuidOrder order) : order_(order), shortUuid_(uuid16) {
    type_ = BleUuidType::SHORT;
    memset(fullUuid_, 0x00, BLE_SIG_UUID_128BIT_LEN);
}

BleUuid::BleUuid(const uint8_t* uuid128, uint16_t uuid16, BleUuidOrder order) : order_(order), shortUuid_(0x0000) {
    type_ = BleUuidType::LONG;
    if (uuid128 == nullptr) {
        memset(fullUuid_, 0x00, BLE_SIG_UUID_128BIT_LEN);
    }
    else {
        memcpy(fullUuid_, uuid128, BLE_SIG_UUID_128BIT_LEN);
    }
    if (order == BleUuidOrder::LSB) {
        fullUuid_[12] = (uint8_t)(uuid16 & 0x00FF);
        fullUuid_[13] = (uint8_t)((uuid16 >> 8) & 0x00FF);
    }
    else {
        fullUuid_[13] = (uint8_t)(uuid16 & 0x00FF);
        fullUuid_[12] = (uint8_t)((uuid16 >> 8) & 0x00FF);
    }
}

BleUuid::BleUuid(const String& str) : type_(BleUuidType::LONG), order_(BleUuidOrder::LSB), shortUuid_(0x0000) {
    setUuid(str);
}

BleUuid::BleUuid(const char* string) : type_(BleUuidType::LONG), order_(BleUuidOrder::LSB), shortUuid_(0x0000) {
    if (string == nullptr) {
        memset(fullUuid_, 0x00, BLE_SIG_UUID_128BIT_LEN);
    }
    else {
        String str(string);
        setUuid(str);
    }
}

BleUuid::~BleUuid() {

}

bool BleUuid::isValid(void) const {
    if (type_ == BleUuidType::SHORT) {
        return shortUuid_ != 0x0000;
    }
    else {
        uint8_t temp[BLE_SIG_UUID_128BIT_LEN];
        memset(temp, 0x00, sizeof(temp));
        return memcmp(fullUuid_, temp, BLE_SIG_UUID_128BIT_LEN);
    }
}

int8_t BleUuid::toInt(char c) {
    if (c >= '0' && c <= '9') {
        return (c - '0');
    } else if (c >= 'a' && c <= 'f') {
        return (c - 'a' + 10);
    } else if (c >= 'A' && c <= 'F') {
        return (c - 'A' + 10);
    } else {
        return -1;
    }
}

void BleUuid::setUuid(const String& str) {
    size_t len = BLE_SIG_UUID_128BIT_LEN;
    for (size_t i = 0; i < str.length(); i++) {
        int8_t hi = toInt(str.charAt(i));
        if (hi >= 0) {
            fullUuid_[len - 1] = hi << 4;
            if (++i < str.length()) {
                int8_t lo = toInt(str.charAt(i));
                if (lo >= 0) {
                    fullUuid_[len - 1] |= lo;
                }
            }
            len--;
        }
    }
    while (len > 0) {
        fullUuid_[len - 1] = 0x00;
        len--;
    }
}

bool BleUuid::operator == (const BleUuid& uuid) const {
    if (type_ == uuid.type() && order_ == uuid.order()) {
        if (type_ == BleUuidType::SHORT) {
            return (shortUuid_ == uuid.shortUuid());
        }
        else {
            return !memcmp(fullUuid(), uuid.fullUuid(), BLE_SIG_UUID_128BIT_LEN);
        }
    }
    return false;
}


/*******************************************************
 * BleAdvData class
 */
BleAdvData::BleAdvData() : len(0) {

}

BleAdvData::~BleAdvData() {

}

size_t BleAdvData::locate(uint8_t type, size_t* offset) {
    if (offset == nullptr) {
        return 0;
    }

    size_t adsLen;
    for (size_t i = 0; (i + 3) <= len; i = i) {
        adsLen = data[i];
        if (data[i + 1] == type) {
            // The value of adsLen doesn't include the length field of an AD structure.
            if ((i + adsLen + 1) <= len) {
                *offset = i;
                adsLen += 1;
                return adsLen;
            }
            else {
                return 0;
            }
        }
        else {
            // Navigate to the next AD structure.
            i += (adsLen + 1);
        }
    }
    return 0;
}

size_t BleAdvData::fetch(uint8_t type, uint8_t* buf, size_t len) {
    size_t adsOffset;
    size_t adsLen = locate(type, &adsOffset);
    if (adsLen > 0) {
        adsLen -= 2;
        adsOffset += 2;
        len = len > adsLen ? adsLen : len;
        if (buf != nullptr) {
            memcpy(buf, &data[adsOffset], len);
        }

        return adsLen;
    }
    return 0;
}

bool BleAdvData::contain(uint8_t type, const uint8_t* buf, size_t len) {
    if (buf == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    uint8_t temp[BLE_MAX_ADV_DATA_LEN];
    size_t tempLen = fetch(type, temp, sizeof(temp));
    if (tempLen > 0) {
        return !memcmp(buf, temp, len);
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

bool BleAdvData::contain(uint8_t type) {
    return (fetch(type, nullptr, 0)  > 0);
}


class BleCharacteristicRef {
public:
    BleCharacteristic* charPtr;
    bool isStub;

    BleCharacteristicRef() {
        charPtr = nullptr;
        isStub = false;
    }

    BleCharacteristicRef(BleCharacteristic* characteristic, bool stub)
        : charPtr(characteristic),
          isStub(stub) {
    }

    ~BleCharacteristicRef() {
    }

    bool operator == (const BleCharacteristicRef& reference) {
        return (reference.charPtr == this->charPtr);
    }
};


/*******************************************************
 * BleCharacteristicImpl definition
 */
class BleCharacteristicImpl {
public:
    BleCharProps properties;
    BleUuid uuid;
    BleUuid svcUuid;
    const char* description;
    bool isLocal;
    BleCharHandles attrHandles;
    onDataReceivedCb dataCb;
    Vector<BleConnHandle> cccdOfServer; // For local characteristic
    bool cccdOfClient; // For peer characteristic
    BleConnHandle connHandle; // For peer characteristic
    BleServiceImpl* svcImpl; // Related service
    Vector<BleCharacteristicRef> references;

    BleCharacteristicImpl() {
        init();
    }

    BleCharacteristicImpl(const char* desc, BleCharProps properties, onDataReceivedCb cb) {
        init();
        this->description = desc;
        this->properties = properties;
        this->dataCb = cb;
    }

    BleCharacteristicImpl(const char* desc, BleCharProps properties, BleUuid& charUuid, BleUuid& svcUuid, onDataReceivedCb cb) {
        init();
        this->description = desc;
        this->properties = properties;
        this->uuid = charUuid;
        this->svcUuid = svcUuid;
        this->dataCb = cb;
    }

    ~BleCharacteristicImpl() {
    }

    void init(void) {
        properties = PROPERTY::NONE;
        description = nullptr;
        isLocal = true;
        cccdOfClient = false;
        connHandle = BLE_INVALID_CONN_HANDLE;
        svcImpl = nullptr;
        dataCb = nullptr;
    }

    size_t getValue(uint8_t* buf, size_t len) const {
        if (buf == nullptr) {
            return 0;
        }
        uint16_t readLen = len > BLE_MAX_CHAR_VALUE_LEN ? BLE_MAX_CHAR_VALUE_LEN : len;
        int ret;
        if (isLocal) {
            ret = ble_gatt_server_get_characteristic_value(attrHandles.value_handle, buf, &readLen, nullptr);
        }
        else {
            ret = ble_gatt_client_read(connHandle, attrHandles.value_handle, buf, &readLen, nullptr);
        }
        if (ret == SYSTEM_ERROR_NONE) {
            return readLen;
        }
        return 0;
    }

    size_t setValue(const uint8_t* buf, size_t len) {
        if (buf == nullptr) {
            return 0;
        }
        len = len > BLE_MAX_CHAR_VALUE_LEN ? BLE_MAX_CHAR_VALUE_LEN : len;
        int ret = SYSTEM_ERROR_INVALID_STATE;
        if (isLocal) {
            ret = ble_gatt_server_set_characteristic_value(attrHandles.value_handle, buf, len, nullptr);
            if (ret != SYSTEM_ERROR_NONE) {
                return 0;
            }
            for (int i = 0; i < cccdOfServer.size(); i++) {
                if (properties & PROPERTY::NOTIFY) {
                    ret = ble_gatt_server_notify_characteristic_value(cccdOfServer[i], attrHandles.value_handle, buf, len, nullptr);
                }
                else if (properties & PROPERTY::INDICATE) {
                    ret = ble_gatt_server_indicate_characteristic_value(cccdOfServer[i], attrHandles.value_handle, buf, len, nullptr);
                }
                if (ret != SYSTEM_ERROR_NONE) {
                    return 0;
                }
            }
        }
        else {
            if (properties & PROPERTY::WRITE) {
                ret = ble_gatt_client_write_with_response(connHandle, attrHandles.value_handle, buf, len, nullptr);
            }
            else if (properties & PROPERTY::WRITE_WO_RSP) {
                ret = ble_gatt_client_write_without_response(connHandle, attrHandles.value_handle, buf, len, nullptr);
            }
            if (ret != SYSTEM_ERROR_NONE) {
                return 0;
            }
        }
        return len;
    }

    void configureCccd(BleConnHandle handle, uint8_t enable) {
        if (isLocal) {
            if (enable) {
                cccdOfServer.append(handle);
            }
            else {
                cccdOfServer.removeOne(handle);
            }
            LOG_DEBUG(TRACE, "CCCD configured count: %d", cccdOfServer.size());
        }
        else {
            // Gatt Client configure peer CCCD.
        }
    }

    void assignUuidIfNeeded(void) {
        if (!uuid.isValid()) {
            defaultUuidCharCount_++;
            BleUuid newUuid(BLE_CTRL_REQ_SVC_UUID, defaultUuidCharCount_);
            uuid = newUuid;
        }
    }

    void addReference(BleCharacteristic* characteristic, bool stub) {
        BleCharacteristicRef reference(characteristic, stub);
        references.append(reference);
        LOG_DEBUG(TRACE, "0x%08X added references: %d", this, references.size());
    }

    void removeReference(BleCharacteristic* characteristic) {
        for (int i = 0; i < references.size(); i++) {
            BleCharacteristicRef& reference = references[i];
            if (reference.charPtr == characteristic) {
                references.removeOne(reference);
                LOG_DEBUG(TRACE, "0x%08X removed references: %d", this, references.size());
            }
        }
    }

    bool isReferenceStub(const BleCharacteristic* characteristic) {
        for (int i = 0; i < references.size(); i++) {
            BleCharacteristicRef& reference = references[i];
            if (reference.charPtr == characteristic) {
                return reference.isStub;
            }
        }
        return false;
    }

    size_t referenceStubCount(void) {
        size_t total = 0;
        for (int i = 0; i < references.size(); i++) {
            BleCharacteristicRef& reference = references[i];
            if (reference.isStub) {
                total++;
            }
        }
        return total;
    }

    void markReferenceAsStub(BleCharacteristic* characteristic) {
        for (int i = 0; i < references.size(); i++) {
            BleCharacteristicRef& reference = references[i];
            if (reference.charPtr == characteristic) {
                reference.isStub = true;
                LOG_DEBUG(TRACE, "0x%08X STUB references: 0x%08X", this, characteristic);
            }
        }
    }

    void processReceivedData(BleAttrHandle attrHandle, const uint8_t* data, size_t len, const BlePeerDevice& peer) {
        if (data == nullptr) {
            return;
        }
        if (isLocal) {
            if (attrHandle == attrHandles.cccd_handle) {
                LOG_DEBUG(TRACE, "Configure CCCD: 0x%02x%02x", data[0],data[1]);
                configureCccd(peer.connHandle, data[0]);
            }
        }
        if (attrHandle == attrHandles.value_handle) {
            if (dataCb != nullptr) {
                dataCb(data, len);
            }
        }
    }

private:
    static uint16_t defaultUuidCharCount_;
};

uint16_t BleCharacteristicImpl::defaultUuidCharCount_ = 0;


/*******************************************************
 * BleCharacteristic class
 */
BleCharacteristic::BleCharacteristic()
    : impl_(std::make_shared<BleCharacteristicImpl>()) {
    impl()->addReference(this, false);
    LOG_DEBUG(TRACE, "BleCharacteristic::BleCharacteristic():0x%08X -> 0x%08X", this, impl());
    LOG_DEBUG(TRACE, "shared_ptr count: %d", impl_.use_count());
}

BleCharacteristic::BleCharacteristic(const BleCharacteristic& characteristic)
    : impl_(characteristic.impl_) {
    LOG_DEBUG(TRACE, "BleCharacteristic::BleCharacteristic(copy):0x%08X => 0x%08X -> 0x%08X", &characteristic, this, impl());
    LOG_DEBUG(TRACE, "shared_ptr count: %d", impl_.use_count());
    if (impl() != nullptr) {
        impl()->addReference(this, impl()->isReferenceStub(&characteristic));
    }
}

BleCharacteristic::BleCharacteristic(const char* desc, BleCharProps properties, onDataReceivedCb cb)
    : impl_(std::make_shared<BleCharacteristicImpl>(desc, properties, cb)) {
    impl()->addReference(this, false);

    LOG_DEBUG(TRACE, "BleCharacteristic::BleCharacteristic(...):0x%08X -> 0x%08X", this, impl());
    LOG_DEBUG(TRACE, "shared_ptr count: %d", impl_.use_count());
}

BleCharacteristic::BleCharacteristic(const char* desc, BleCharProps properties, BleUuid& charUuid, BleUuid& svcUuid, onDataReceivedCb cb)
    : impl_(std::make_shared<BleCharacteristicImpl>(desc, properties, charUuid, svcUuid, cb)) {
    impl()->addReference(this, false);

    LOG_DEBUG(TRACE, "BleCharacteristic::BleCharacteristic(...):0x%08X -> 0x%08X", this, impl());
    LOG_DEBUG(TRACE, "shared_ptr count: %d", impl_.use_count());
}

BleCharacteristic& BleCharacteristic::operator=(const BleCharacteristic& characteristic) {
    LOG_DEBUG(TRACE, "BleCharacteristic::operator=:0x%08X -> 0x%08X", this, impl());
    LOG_DEBUG(TRACE, "shared_ptr pre-count1: %d", impl_.use_count());
    onDataReceivedCb preDataCb = nullptr;
    if (impl() != nullptr) {
        if (impl()->dataCb != nullptr) {
            preDataCb = impl()->dataCb;
        }
        if (impl()->isReferenceStub(this) && impl()->referenceStubCount() == 1) {
            for (int i = 0; i < impl()->references.size(); i++) {
                BleCharacteristicRef reference = impl()->references[i];
                if (reference.charPtr != this) {
                    reference.charPtr->impl_ = nullptr;
                }
            }
            LOG_DEBUG(TRACE, "shared_ptr pre-count2: %d", impl_.use_count());
        }
        else {
            impl()->removeReference(this);
        }
    }

    impl_ = characteristic.impl_;
    if (impl()->dataCb == nullptr) {
        impl()->dataCb = preDataCb;
    }
    LOG_DEBUG(TRACE, "now:0x%08X -> 0x%08X", this, impl());
    LOG_DEBUG(TRACE, "shared_ptr curr-count: %d", impl_.use_count());
    if (impl() != nullptr) {
        impl()->addReference(this, impl()->isReferenceStub(&characteristic));
    }
    return *this;
}

BleCharacteristic::~BleCharacteristic() {
    LOG_DEBUG(TRACE, "BleCharacteristic::~BleCharacteristic:0x%08X -> 0x%08X", this, impl());
    if (impl() != nullptr) {
        if (impl()->isReferenceStub(this) && impl()->referenceStubCount() == 1) {
            LOG_DEBUG(TRACE, "shared_ptr count1: %d", impl_.use_count());
            for (int i = 0; i < impl()->references.size(); i++) {
                BleCharacteristicRef reference = impl()->references[i];
                if (reference.charPtr != this) {
                    reference.charPtr->impl_ = nullptr;
                }
            }
            LOG_DEBUG(TRACE, "shared_ptr count2: %d", impl_.use_count());
        }
        else {
            LOG_DEBUG(TRACE, "shared_ptr count1: %d", impl_.use_count());
            impl()->removeReference(this);
        }
    }
}

BleUuid BleCharacteristic::uuid(void) const {
    if (impl() != nullptr) {
        return impl()->uuid;
    }
    BleUuid uuid((uint16_t)0x0000);
    return uuid;
}

BleCharProps BleCharacteristic::properties(void) const {
    if (impl() != nullptr) {
        return impl()->properties;
    }
    return PROPERTY::NONE;
}

size_t BleCharacteristic::setValue(const uint8_t* buf, size_t len) {
    if (impl() != nullptr) {
        return impl()->setValue(buf, len);
    }
    return 0;
}

size_t BleCharacteristic::setValue(const String& str) {
    return setValue(reinterpret_cast<const uint8_t*>(str.c_str()), str.length());
}

size_t BleCharacteristic::setValue(const char* str) {
    return setValue(reinterpret_cast<const uint8_t*>(str), strlen(str));
}

size_t BleCharacteristic::getValue(uint8_t* buf, size_t len) const {
    if (impl() != nullptr) {
        return impl()->getValue(buf, len);
    }
    return 0;
}

size_t BleCharacteristic::getValue(String& str) const {
    return 0;
}

void BleCharacteristic::onDataReceived(onDataReceivedCb callback) {
    if (impl() != nullptr) {
        impl()->dataCb = callback;
    }
}


/*******************************************************
 * BleServiceImpl definition
 */
class BleServiceImpl {
public:
    BleUuid uuid;
    BleAttrHandle startHandle;
    BleAttrHandle endHandle;
    Vector<BleCharacteristic> characteristics;
    BleGattServerImpl* gattsProxy; // Related GATT Server

    BleServiceImpl() {
        init();
    }

    BleServiceImpl(const BleUuid& uuid)
        : uuid(uuid) {
        init();
    }

    ~BleServiceImpl() {
    }

    void init(void) {
        startHandle = 0;
        endHandle = 0;
        gattsProxy = nullptr;
    }

    size_t characteristicCount(void) const {
        return characteristics.size();
    }

    bool contains(const BleCharacteristic& characteristic) {
        if (characteristic.impl() != nullptr) {
            for (size_t i = 0; i < characteristicCount(); i++) {
                BleCharacteristic& stubChar = characteristics[i];
                if (characteristic.impl() == stubChar.impl()) {
                    return true;
                }
            }
        }
        return false;
    }

    BleCharacteristic* characteristic(const char* desc) {
        if (desc == nullptr) {
            return nullptr;
        }
        for (size_t i = 0; i < characteristicCount(); i++) {
            BleCharacteristic& characteristic = characteristics[i];
            if (characteristic.impl() != nullptr && !strcmp(characteristic.impl()->description, desc)) {
                return &characteristic;
            }
        }
        return nullptr;
    }

    BleCharacteristic* characteristic(BleAttrHandle attrHandle) {
        for (size_t i = 0; i < characteristicCount(); i++) {
            BleCharacteristic& characteristic = characteristics[i];
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

    BleCharacteristic* characteristic(const BleUuid& charUuid) {
        for (size_t i = 0; i < characteristicCount(); i++) {
            BleCharacteristic& characteristic = characteristics[i];
            if (characteristic.impl() != nullptr && characteristic.impl()->uuid == charUuid) {
                return &characteristic;
            }
        }
        return nullptr;
    }

    int addCharacteristic(BleCharacteristic& characteristic) {
        BleCharacteristicImpl* charImpl = characteristic.impl();
        if (charImpl == nullptr || contains(characteristic) || !charImpl->properties) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        if (characteristic.impl()->isLocal) {
            if (ble_stack_is_initialized()) {
                characteristic.impl()->assignUuidIfNeeded();
                hal_ble_char_init_t char_init;
                memset(&char_init, 0x00, sizeof(hal_ble_char_init_t));
                convertToHalUuid(characteristic.impl()->uuid, &char_init.uuid);
                char_init.properties = static_cast<uint8_t>(characteristic.impl()->properties);
                char_init.service_handle = startHandle;
                char_init.description = characteristic.impl()->description;
                int ret = ble_gatt_server_add_characteristic(&char_init, &characteristic.impl()->attrHandles, NULL);
                if (ret != SYSTEM_ERROR_NONE) {
                    return ret;
                }
            }
            else {
                return SYSTEM_ERROR_INVALID_STATE;
            }
        }
        characteristic.impl()->svcImpl = this;
        LOG_DEBUG(TRACE, "characteristics.append(characteristic)");
        characteristics.append(characteristic);
        BleCharacteristic& lastChar = characteristics.last();
        lastChar.impl()->markReferenceAsStub(&lastChar);
        return SYSTEM_ERROR_NONE;
    }
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

BleService::~BleService() {
}


/*******************************************************
 * BleGattServerImpl definition
 */
class BleGattServerImpl {
public:
    Vector<BleService> services;

    // Related device
    BleAddress address;

    BleGattServerImpl(const BleAddress& addr)
        : address(addr) {
    }

    ~BleGattServerImpl() {
    }

    size_t serviceCount(void) const {
        return services.size();
    }

    BleService* service(const BleUuid& uuid) {
        for (size_t i = 0; i < serviceCount(); i++) {
            BleService& stubSvc = services[i];
            if (stubSvc.impl()->uuid == uuid) {
                return &stubSvc;
            }
        }
        return nullptr;
    }

    bool isLocal(void) {
        BleAddress addr;
        ble_gap_get_device_address(&addr);
        return addr == address;
    }

    int addService(BleService& svc) {
        if (service(svc.impl()->uuid) != nullptr) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        if (isLocal()) {
            if (ble_stack_is_initialized()) {
                hal_ble_uuid_t halUuid;
                convertToHalUuid(svc.impl()->uuid, &halUuid);
                int ret = ble_gatt_server_add_service(BLE_SERVICE_TYPE_PRIMARY, &halUuid, &svc.impl()->startHandle, NULL);
                if (ret != SYSTEM_ERROR_NONE) {
                    return ret;
                }
            }
            else {
                return SYSTEM_ERROR_INVALID_STATE;
            }
        }
        LOG_DEBUG(TRACE, "services.append(service)");
        services.append(svc);
        return SYSTEM_ERROR_NONE;
    }

    int addCharacteristic(BleCharacteristic& characteristic) {
        if (characteristic.impl() == nullptr) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        characteristic.impl()->isLocal = isLocal();
        if (isLocal()) {
            LOG_DEBUG(TRACE, "< LOCAL CHARACTERISTIC >");
            if (!characteristic.impl()->svcUuid.isValid()) {
                BleUuid newUuid(BLE_CTRL_REQ_SVC_UUID);
                characteristic.impl()->svcUuid = newUuid;
            }
        }
        BleService* stubSvc = service(characteristic.impl()->svcUuid);
        if (stubSvc != nullptr) {
            if (stubSvc->impl() != nullptr) {
                return stubSvc->impl()->addCharacteristic(characteristic);
            }
            return SYSTEM_ERROR_INTERNAL;
        }
        else {
            BleService service(characteristic.impl()->svcUuid);
            if (addService(service) == SYSTEM_ERROR_NONE) {
                return services.last().impl()->addCharacteristic(characteristic);
            }
            return SYSTEM_ERROR_INTERNAL;
        }
    }

    int addCharacteristic(const char* desc, BleCharProps properties, onDataReceivedCb cb) {
        BleCharacteristic characteristic(desc, properties, cb);
        return addCharacteristic(characteristic);
    }

    int addCharacteristic(const char* desc, BleCharProps properties, BleUuid& charUuid, BleUuid& svcUuid, onDataReceivedCb cb) {
        BleCharacteristic characteristic(desc, properties, charUuid, svcUuid, cb);
        return addCharacteristic(characteristic);
    }

    template <typename T>
    BleCharacteristic characteristic(T& type) const {
        for (size_t i = 0; i < serviceCount(); i++) {
            BleCharacteristic* characteristic = services[i].impl()->characteristic(type);
            if (characteristic != nullptr) {
                return *characteristic;
            }
        }
        BleCharacteristic temp;
        return temp;
    }

    void gattsProcessDisconnected(const BlePeerDevice& peer) {
        for (size_t i = 0; i < serviceCount(); i++) {
            BleService& service = services[i];
            for (size_t j = 0; j < service.impl()->characteristicCount(); j++) {
                BleCharacteristic& characteristic = service.impl()->characteristics[j];
                characteristic.impl()->configureCccd(peer.connHandle, false);
            }
        }
    }

    void gattsProcessDataWritten(BleAttrHandle attrHandle, const uint8_t* buf, size_t len, BlePeerDevice& peer) {
        for (size_t i = 0; i < serviceCount(); i++) {
            BleCharacteristic* characteristic = services[i].impl()->characteristic(attrHandle);
            if (characteristic != nullptr) {
                characteristic->impl()->processReceivedData(attrHandle, buf, len, peer);
            }
        }
    }
};


/*******************************************************
 * BleGattClientImpl definition
 */
class BleGattClientImpl {
public:
    BleGattClientImpl() {

    }

    ~BleGattClientImpl() {

    }

    void gattcProcessDataNotified(BleAttrHandle attrHandle, const uint8_t* buf, size_t len, BlePeerDevice& peer) {
        peer.gattsProxy()->gattsProcessDataWritten(attrHandle, buf, len, peer);
    }
};


/*******************************************************
 * BleBroadcasterImpl definition
 */
class BleBroadcasterImpl {
public:
    BleAdvParams advParams;
    BleAdvData advData;
    BleAdvData srData;

    BleBroadcasterImpl() {
        // AD flag
        uint8_t flag = BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
        appendAdvData(BLE_SIG_AD_TYPE_FLAGS, &flag, 1);

        // Complete local name
        char devName[32] = {};
        get_device_name(devName, sizeof(devName));
        appendAdvDataLocalName(devName);

        // Particle specific Manufacture data
        uint8_t mfgData[BLE_MAX_ADV_DATA_LEN];
        size_t mfgDataLen = 0;
        uint16_t platformID = PLATFORM_ID;
        memcpy(&mfgData[mfgDataLen], (uint8_t*)&PARTICLE_COMPANY_ID, sizeof(PARTICLE_COMPANY_ID));
        mfgDataLen += sizeof(PARTICLE_COMPANY_ID);
        memcpy(&mfgData[mfgDataLen], (uint8_t*)&platformID, sizeof(platformID));
        mfgDataLen += sizeof(platformID);
        appendAdvDataCustomData(mfgData, mfgDataLen);

        // Particle Control Request Service 128-bits UUID
        BleUuid svcUUID(BLE_CTRL_REQ_SVC_UUID);
        appendScanRspDataUuid(svcUUID);

        // Default advertising parameters
        advParams.type          = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT;
        advParams.filter_policy = BLE_ADV_FP_ANY;
        advParams.interval      = BLE_DEFAULT_ADVERTISING_INTERVAL;
        advParams.timeout       = BLE_DEFAULT_ADVERTISING_TIMEOUT;
        advParams.inc_tx_power  = false;
    }

    ~BleBroadcasterImpl() {
    }

    int appendAdvData(uint8_t type, const uint8_t* buf, size_t len) {
        return append(type, buf, len, advData);
    }

    int appendAdvDataLocalName(const char* name) {
        return appendAdvData(BLE_SIG_AD_TYPE_COMPLETE_LOCAL_NAME, (const uint8_t*)name, strlen(name));
    }

    int appendAdvDataCustomData(const uint8_t* buf, size_t len) {
        return appendAdvData(BLE_SIG_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, buf, len);
    }

    int appendAdvDataUuid(const BleUuid& uuid) {
        if (uuid.type() == BleUuidType::SHORT) {
            uint16_t uuid16;
            uuid16 = uuid.shortUuid();
            return appendAdvData(BLE_SIG_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE, reinterpret_cast<const uint8_t*>(&uuid16), sizeof(uint16_t));
        }
        else {
            uint8_t uuid128[16];
            uuid.fullUuid(uuid128);
            return appendAdvData(BLE_SIG_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE, (const uint8_t*)uuid128, 16);
        }
    }

    int appendScanRspData(uint8_t type, const uint8_t* buf, size_t len) {
        if (type == BLE_SIG_AD_TYPE_FLAGS) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        return append(type, buf, len, srData);
    }

    int appendScanRspDataLocalName(const char* name) {
        return appendScanRspData(BLE_SIG_AD_TYPE_COMPLETE_LOCAL_NAME, (const uint8_t*)name, strlen(name));
    }

    int appendScanRspDataCustomData(const uint8_t* buf, size_t len) {
        return appendScanRspData(BLE_SIG_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, buf, len);
    }

    int appendScanRspDataUuid(const BleUuid& uuid) {
        if (uuid.type() == BleUuidType::SHORT) {
            uint16_t uuid16;
            uuid16 = uuid.shortUuid();
            return appendScanRspData(BLE_SIG_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE, reinterpret_cast<const uint8_t*>(&uuid16), sizeof(uint16_t));
        }
        else {
            uint8_t uuid128[16];
            uuid.fullUuid(uuid128);
            return appendScanRspData(BLE_SIG_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE, (const uint8_t*)uuid128, 16);
        }
    }

    int advDataBeacon(const iBeacon& beacon) {
        return 0;
    }

    int clearAdvData(void) {
        advData.len = 0;
        memset(advData.data, 0x00, sizeof(advData.data));
        return SYSTEM_ERROR_NONE;
    }

    int removeFromAdvData(uint8_t type) {
        size_t offset, len;
        len = advData.locate(type, &offset);
        if (len > 0) {
            uint16_t moveLen = advData.len - offset - len;
            memcpy(&advData.data[offset], &advData.data[offset + len], moveLen);
            advData.len = advData.len - len;
            return SYSTEM_ERROR_NONE;
        }
        else {
            return SYSTEM_ERROR_NONE;
        }
    }

    int clearScanRspData(void) {
        srData.len = 0;
        memset(srData.data, 0x00, sizeof(srData.data));
        return SYSTEM_ERROR_NONE;
    }

    int removeFromScanRspData(uint8_t type) {
        // The advertising data must contain the AD Flags AD structure.
        if (type == BLE_SIG_AD_TYPE_FLAGS) {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }

        size_t offset, len;
        len = srData.locate(type, &offset);
        if (len > 0) {
            uint16_t moveLen = srData.len - offset - len;
            memcpy(&srData.data[offset], &srData.data[offset + len], moveLen);
            srData.len = srData.len - len;
            return SYSTEM_ERROR_NONE;
        }
        else {
            return SYSTEM_ERROR_NONE;
        }
    }

    int setTxPower(int8_t val) const {
        return ble_gap_set_tx_power(val);
    }

    int8_t txPower(void) const {
        int8_t txPower;
        if (ble_gap_get_tx_power(&txPower) == SYSTEM_ERROR_NONE) {
            return txPower;
        }
        else {
            return 0x7F;
        }
    }

    int advertise(void) {
        return advertise(advParams);
    }

    int advertise(uint32_t interval) {
        advParams.interval = interval;
        return advertise(advParams);
    }

    int advertise(uint32_t interval, uint32_t timeout) {
        advParams.interval = interval;
        advParams.timeout = timeout;
        return advertise(advParams);
    }

    int advertise(const BleAdvParams& params) {
        advParams = params;
        ble_gap_set_advertising_parameters(&advParams, NULL);

        if (advData.len > 0) {
            ble_gap_set_advertising_data(advData.data, advData.len, nullptr);
        }
        if (srData.len > 0) {
            ble_gap_set_scan_response_data(srData.data, srData.len, nullptr);
        }
        return ble_gap_start_advertising(NULL);
    }

    int stopAdvertising(void) const {
        return ble_gap_stop_advertising();
    }

    int append(uint8_t type, const uint8_t* buf, size_t len, BleAdvData& target) {
        if (buf == nullptr) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }

        size_t offset;
        size_t adsLen = target.locate(type, &offset);

        if (adsLen > 0) {
            // Update the existing AD structure.
            uint16_t staLen = target.len - adsLen;
            if ((staLen + len + 2) <= BLE_MAX_ADV_DATA_LEN) {
                // Firstly, move the last consistent block.
                uint16_t moveLen = target.len - offset - adsLen;
                memmove(&target.data[offset + len], &target.data[offset + adsLen], moveLen);

                // Secondly, Update the AD structure.
                // The Length field is the total length of Type field and Data field.
                target.data[offset] = len + 1;
                memcpy(&target.data[offset + 2], buf, len);

                // An AD structure is composed of one byte length field, one byte Type field and Data field.
                target.len = staLen + len + 2;
            }
            else {
                return SYSTEM_ERROR_LIMIT_EXCEEDED;
            }
        }
        else {
            // Append the AD structure at the and of advertising data.
            if ((target.len + len + 2) <= BLE_MAX_ADV_DATA_LEN) {
                target.data[target.len++] = len + 1;
                target.data[target.len++] = type;
                memcpy(&target.data[target.len], buf, len);
                target.len += len;
            }
            else {
                return SYSTEM_ERROR_LIMIT_EXCEEDED;
            }
        }
        return SYSTEM_ERROR_NONE;
    }

    void broadcasterProcessStopped(void) {
        return;
    }
};


/*******************************************************
 * BleObserverImpl definition
 */
class BleObserverImpl {
public:
    BleScanParams scanParams;
    size_t targetCount;
    BleScanCallback callback;
    BleScannedDevice* results;
    size_t count;

    static const size_t DEFAULT_COUNT = 5;

    BleObserverImpl() : targetCount(0), callback(nullptr), results(nullptr), count(0) {
        scanParams.active = true;
        scanParams.filter_policy = BLE_SCAN_FP_ACCEPT_ALL;
        scanParams.interval = BLE_DEFAULT_SCANNING_INTERVAL;
        scanParams.window = BLE_DEFAULT_SCANNING_WINDOW;
        scanParams.timeout = BLE_DEFAULT_SCANNING_TIMEOUT;
    }

    ~BleObserverImpl() {

    }

    int scan(BleScanCallback callback) {
        this->callback = callback;
        ble_gap_set_scan_parameters(&scanParams, NULL);
        ble_gap_start_scan(nullptr);
        return count;
    }

    int scan(BleScanCallback callback, uint16_t timeout) {
        this->callback = callback;
        scanParams.timeout  = timeout;
        ble_gap_set_scan_parameters(&scanParams, NULL);
        ble_gap_start_scan(nullptr);
        return count;
    }

    int scan(BleScannedDevice* results, size_t resultCount) {
        return scan(results, resultCount, scanParams);
    }

    int scan(BleScannedDevice* results, size_t resultCount, uint16_t timeout) {
        scanParams.timeout  = timeout;
        return scan(results, resultCount, scanParams);
    }

    int scan(BleScannedDevice* results, size_t resultCount, const BleScanParams& params) {
        this->results = results;
        targetCount = resultCount;
        scanParams = params;
        ble_gap_set_scan_parameters(&scanParams, NULL);
        ble_gap_start_scan(nullptr);
        return count;
    }

    int stopScanning(void) {
        return ble_gap_stop_scan();
    }

    void observerProcessScanResult(const hal_ble_gap_on_scan_result_evt_t* event) {
        BleScannedDevice device;
        device.address = event->peer_addr;
        device.rssi = event->rssi;

        if (event->type.scan_response) {
            memcpy(device.srData.data, event->data, event->data_len);
            device.srData.len = event->data_len;
        }
        else {
            memcpy(device.advData.data, event->data, event->data_len);
            device.advData.len = event->data_len;
        }

        if (callback != nullptr) {
            callback(&device);
        }
        else if (results != nullptr) {
            results[count++] = device;
            if (count >= targetCount) {
                stopScanning();
            }
        }
    }

    void observerProcessScanStopped(const hal_ble_gap_on_scan_stopped_evt_t* event) {
        return;
    }
};


/*******************************************************
 * BlePeripheralImpl definition
 */
class BlePeripheralImpl {
public:
    BleConnParams ppcp;
    Vector<BlePeerDevice> centrals;

    BlePeripheralImpl() {
        ppcp.min_conn_interval = BLE_DEFAULT_MIN_CONN_INTERVAL;
        ppcp.max_conn_interval = BLE_DEFAULT_MAX_CONN_INTERVAL;
        ppcp.slave_latency     = BLE_DEFAULT_SLAVE_LATENCY;
        ppcp.conn_sup_timeout  = BLE_DEFAULT_CONN_SUP_TIMEOUT;
    }

    ~BlePeripheralImpl() {
    }

    size_t centralCount(void) const {
        return centrals.size();
    }

    int setPpcp(void) {
        return ble_gap_set_ppcp(&ppcp, NULL);
    }

    int setPpcp(uint16_t minInterval, uint16_t maxInterval, uint16_t latency, uint16_t timeout) {
        ppcp.min_conn_interval = minInterval;
        ppcp.max_conn_interval = maxInterval;
        ppcp.slave_latency = latency;
        ppcp.conn_sup_timeout = timeout;
        return setPpcp();
    }

    int disconnect(void) {
        for (size_t i = 0; i < centralCount(); i++) {
            BlePeerDevice& central = centrals[i];
            ble_gap_disconnect(central.connHandle, NULL);
            centrals.clear();
        }
        return SYSTEM_ERROR_NONE;
    }

    bool connected(void) const {
        return centrals.size() > 0;
    }

    void peripheralProcessConnected(const BlePeerDevice& peer) {
        if (centralCount() < BLE_MAX_PERIPHERAL_COUNT) {
            centrals.append(peer);
        }
    }

    void peripheralProcessDisconnected(const BlePeerDevice& peer) {
        centrals.clear();
    }
};


/*******************************************************
 * BleCentralImpl definition
 */
class BleCentralImpl {
public:
    BleConnParams connParams;
    Vector<BlePeerDevice> peripherals;

    BleCentralImpl() {
        connParams.min_conn_interval = BLE_DEFAULT_MIN_CONN_INTERVAL;
        connParams.max_conn_interval = BLE_DEFAULT_MIN_CONN_INTERVAL;
        connParams.slave_latency     = BLE_DEFAULT_SLAVE_LATENCY;
        connParams.conn_sup_timeout  = BLE_DEFAULT_CONN_SUP_TIMEOUT;
    }

    ~BleCentralImpl() {
    }

    size_t peripheralCount(void) const {
        return peripherals.size();
    }

    BlePeerDevice* connect(const BleAddress& addr, uint16_t interval, uint16_t latency, uint16_t timeout) {
        connParams.min_conn_interval = interval;
        connParams.max_conn_interval = interval;
        connParams.slave_latency = latency;
        connParams.conn_sup_timeout = timeout;
        ble_gap_connect(&addr, &connParams, nullptr);
        return nullptr;
    }

    int disconnect(const BlePeerDevice& peripheral) {
        for (size_t i = 0; i < peripheralCount(); i++) {
            BlePeerDevice& peer = peripherals[i];
            if (peer.connHandle == peripheral.connHandle) {
                ble_gap_disconnect(peer.connHandle, NULL);
                peripherals.removeOne(peer);

                // clear CCCD
                return SYSTEM_ERROR_NONE;
            }
        }
        return SYSTEM_ERROR_NONE;
    }

    bool connectedAsCentral(void) const {
        return peripherals.size() > 0;
    }

    void centralProcessConnected(const BlePeerDevice& peer) {
        if (peripheralCount() < BLE_MAX_CENTRAL_COUNT) {
            peripherals.append(peer);
        }
    }

    void centralProcessDisconnected(const BlePeerDevice& peer) {
        peripherals.removeOne(peer);
    }
};


/*******************************************************
 * BlePeerDevice class
 */
BlePeerDevice::BlePeerDevice() {
    role = ROLE::INVALID;
    connHandle = BLE_INVALID_CONN_HANDLE;
    rssi = 0x7F;
    gattsProxy_ = std::make_shared<BleGattServerImpl>(address);
}

BlePeerDevice::BlePeerDevice(const BleAddress& addr) {
    role = ROLE::INVALID;
    connHandle = BLE_INVALID_CONN_HANDLE;
    rssi = 0x7F;
    gattsProxy_ = std::make_shared<BleGattServerImpl>(addr);
}

BlePeerDevice::~BlePeerDevice() {

}

bool BlePeerDevice::operator == (const BlePeerDevice& device) {
    if (connHandle == device.connHandle &&
        address == device.address) {
        return true;
    }
    return false;
}


/*******************************************************
 * BleLocalDevice class
 */
BleLocalDevice::BleLocalDevice()
    : connectedCb_(nullptr),
      disconnectedCb_(nullptr) {
    if (!ble_stack_is_initialized()) {
        ble_stack_init(NULL);
    }

    ble_gap_get_device_address(&address);

    gattsProxy_ = std::make_shared<BleGattServerImpl>(address);
    gattcProxy_ = std::make_shared<BleGattClientImpl>();
    broadcasterProxy_ = std::make_shared<BleBroadcasterImpl>();
    observerProxy_ = std::make_shared<BleObserverImpl>();
    peripheralProxy_ = std::make_shared<BlePeripheralImpl>();
    centralProxy_ = std::make_shared<BleCentralImpl>();

    char devName[32] = {};
    get_device_name(devName, sizeof(devName));
    ble_gap_set_device_name((const uint8_t*)devName, strlen(devName));

    setPpcp();

    ble_set_callback_on_events(onBleEvents, this);
}

BleLocalDevice::~BleLocalDevice() {

}

BleLocalDevice& BleLocalDevice::getInstance(void) {
    static BleLocalDevice instance;
    return instance;
}

void BleLocalDevice::onConnectionChangedCb(onConnectedCb connCb, onDisconnectedCb disconnCb) {
    connectedCb_ = connCb;
    disconnectedCb_ = disconnCb;
}

int BleLocalDevice::on(void) {
    return SYSTEM_ERROR_NONE;
}

void BleLocalDevice::off(void) {

}

int BleLocalDevice::appendAdvData(uint8_t type, const uint8_t* buf, size_t len) {
    return broadcasterProxy_->appendAdvData(type, buf, len);
}

int BleLocalDevice::appendAdvDataLocalName(const char* name) {
    return broadcasterProxy_->appendAdvDataLocalName(name);
}

int BleLocalDevice::appendAdvDataCustomData(const uint8_t* buf, size_t len) {
    return broadcasterProxy_->appendAdvDataCustomData(buf, len);
}

int BleLocalDevice::appendAdvDataUuid(const BleUuid& uuid) {
    return broadcasterProxy_->appendAdvDataUuid(uuid);
}

int BleLocalDevice::appendScanRspData(uint8_t type, const uint8_t* buf, size_t len) {
    return broadcasterProxy_->appendScanRspData(type, buf, len);
}

int BleLocalDevice::appendScanRspDataLocalName(const char* name) {
    return broadcasterProxy_->appendScanRspDataLocalName(name);
}

int BleLocalDevice::appendScanRspDataCustomData(const uint8_t* buf, size_t len) {
    return broadcasterProxy_->appendScanRspDataCustomData(buf, len);
}

int BleLocalDevice::appendScanRspDataUuid(const BleUuid& uuid) {
    return broadcasterProxy_->appendScanRspDataUuid(uuid);
}

int BleLocalDevice::advDataBeacon(const iBeacon& beacon) {
    return broadcasterProxy_->advDataBeacon(beacon);
}

int BleLocalDevice::clearAdvData(void) {
    return broadcasterProxy_->clearAdvData();
}

int BleLocalDevice::removeFromAdvData(uint8_t type) {
    return broadcasterProxy_->removeFromAdvData(type);
}

int BleLocalDevice::clearScanRspData(void) {
    return broadcasterProxy_->clearScanRspData();
}

int BleLocalDevice::removeFromScanRspData(uint8_t type) {
    return broadcasterProxy_->removeFromScanRspData(type);
}

int BleLocalDevice::setTxPower(int8_t val) const {
    return broadcasterProxy_->setTxPower(val);
}
int8_t BleLocalDevice::txPower(void) const {
    return broadcasterProxy_->txPower();
}

int BleLocalDevice::advertise(void) {
    return broadcasterProxy_->advertise();
}

int BleLocalDevice::advertise(uint32_t interval) {
    return broadcasterProxy_->advertise(interval);
}

int BleLocalDevice::advertise(uint32_t interval, uint32_t timeout) {
    return broadcasterProxy_->advertise(interval, timeout);
}

int BleLocalDevice::advertise(const BleAdvParams& params) {
    return broadcasterProxy_->advertise(params);
}

int BleLocalDevice::stopAdvertising(void) const {
    return broadcasterProxy_->stopAdvertising();
}

int BleLocalDevice::scan(BleScanCallback callback) {
    return observerProxy_->scan(callback);
}

int BleLocalDevice::scan(BleScanCallback callback, uint16_t timeout) {
    return observerProxy_->scan(callback, timeout);
}

int BleLocalDevice::scan(BleScannedDevice* results, size_t resultCount) {
    return observerProxy_->scan(results, resultCount);
}

int BleLocalDevice::scan(BleScannedDevice* results, size_t resultCount, uint16_t timeout) {
    return observerProxy_->scan(results, resultCount, timeout);
}

int BleLocalDevice::scan(BleScannedDevice* results, size_t resultCount, const BleScanParams& params) {
    return observerProxy_->scan(results, resultCount, params);
}

int BleLocalDevice::stopScanning(void) {
    return observerProxy_->stopScanning();
}

int BleLocalDevice::setPpcp(void) {
    return peripheralProxy_->setPpcp();
}

int BleLocalDevice::setPpcp(uint16_t minInterval, uint16_t maxInterval, uint16_t latency, uint16_t timeout) {
    return peripheralProxy_->setPpcp(minInterval, maxInterval, latency, timeout);
}

int BleLocalDevice::disconnect(void) {
    return peripheralProxy_->disconnect();
}

bool BleLocalDevice::connected(void) const {
    return peripheralProxy_->connected();
}

BlePeerDevice* BleLocalDevice::connect(const BleAddress& addr, uint16_t interval, uint16_t latency, uint16_t timeout) {
    return centralProxy_->connect(addr, interval, latency, timeout);
}

int BleLocalDevice::disconnect(const BlePeerDevice& peripheral) {
    return centralProxy_->disconnect(peripheral);
}

bool BleLocalDevice::connectedAsCentral(void) const {
    return centralProxy_->connectedAsCentral();
}

int BleLocalDevice::addCharacteristic(BleCharacteristic& characteristic) {
    return gattsProxy_->addCharacteristic(characteristic);
}

int BleLocalDevice::addCharacteristic(const char* desc, BleCharProps properties, onDataReceivedCb cb) {
    return gattsProxy_->addCharacteristic(desc, properties, cb);
}

int BleLocalDevice::addCharacteristic(const char* desc, BleCharProps properties, BleUuid& charUuid, BleUuid& svcUuid, onDataReceivedCb cb) {
    return gattsProxy_->addCharacteristic(desc, properties, charUuid, svcUuid, cb);
}

BlePeerDevice* BleLocalDevice::findPeerDevice(BleConnHandle connHandle) {
    for (size_t i = 0; i < peripheralProxy_->centralCount(); i++) {
        BlePeerDevice* peer = &peripheralProxy_->centrals[i];
        if (peer != nullptr) {
            if (peer->connHandle == connHandle) {
                return peer;
            }
        }
    }
    for (size_t i = 0; i < centralProxy_->peripheralCount(); i++) {
        BlePeerDevice* peer = &centralProxy_->peripherals[i];
        if (peer != nullptr) {
            if (peer->connHandle == connHandle) {
                return peer;
            }
        }
    }
    return nullptr;
}

void BleLocalDevice::onBleEvents(hal_ble_evts_t *event, void* context) {
    if (context == nullptr) {
        return;
    }

    auto bleInstance = static_cast<BleLocalDevice*>(context);

    switch (event->type) {
        case BLE_EVT_ADV_STOPPED: {
            bleInstance->broadcasterProxy_->broadcasterProcessStopped();
        } break;

        case BLE_EVT_SCAN_RESULT: {
            bleInstance->observerProxy_->observerProcessScanResult(&event->params.scan_result);
        } break;

        case BLE_EVT_SCAN_STOPPED: {
            bleInstance->observerProxy_->observerProcessScanStopped(&event->params.scan_stopped);
        } break;

        case BLE_EVT_CONNECTED: {
            BlePeerDevice peer;

            peer.connParams.conn_sup_timeout = event->params.connected.conn_sup_timeout;
            peer.connParams.slave_latency = event->params.connected.slave_latency;
            peer.connParams.max_conn_interval = event->params.connected.conn_interval;
            peer.connParams.min_conn_interval = event->params.connected.conn_interval;
            peer.connHandle = event->params.connected.conn_handle;
            peer.address = event->params.connected.peer_addr;

            if (event->params.connected.role == BLE_ROLE_PERIPHERAL) {
                peer.role = ROLE::CENTRAL;
                bleInstance->peripheralProxy_->peripheralProcessConnected(peer);
            }
            else {
                peer.role = ROLE::PERIPHERAL;
                bleInstance->centralProxy_->centralProcessConnected(peer);
            }
        } break;

        case BLE_EVT_DISCONNECTED: {
            BlePeerDevice* peer = bleInstance->findPeerDevice(event->params.disconnected.conn_handle);
            if (peer != nullptr) {
                bleInstance->gattsProxy_->gattsProcessDisconnected(*peer);

                if (peer->role & ROLE::PERIPHERAL) {
                    bleInstance->centralProxy_->centralProcessDisconnected(*peer);
                }
                else {
                    bleInstance->peripheralProxy_->peripheralProcessDisconnected(*peer);
                }
            }
        } break;

        case BLE_EVT_CONN_PARAMS_UPDATED: {

        } break;

        case BLE_EVT_DATA_WRITTEN: {
            LOG_DEBUG(TRACE, "onDataWritten, connection: %d, attribute: %d", event->params.data_rec.conn_handle, event->params.data_rec.attr_handle);

            BlePeerDevice* peer = bleInstance->findPeerDevice(event->params.data_rec.conn_handle);
            if (peer != nullptr) {
                bleInstance->gattsProxy_->gattsProcessDataWritten(event->params.data_rec.attr_handle,
                        event->params.data_rec.data, event->params.data_rec.data_len, *peer);
            }
        } break;

        case BLE_EVT_DATA_NOTIFIED: {
            LOG_DEBUG(TRACE, "onDataNotified, connection: %d, attribute: %d", event->params.data_rec.conn_handle, event->params.data_rec.attr_handle);

            BlePeerDevice* peer = bleInstance->findPeerDevice(event->params.data_rec.conn_handle);
            if (peer != nullptr) {
                bleInstance->gattcProxy_->gattcProcessDataNotified(event->params.data_rec.attr_handle,
                        event->params.data_rec.data, event->params.data_rec.data_len, *peer);
            }
        } break;

        default: break;
    }
}


BleLocalDevice& _fetch_ble() {
    return BleLocalDevice::getInstance();
}


#endif
