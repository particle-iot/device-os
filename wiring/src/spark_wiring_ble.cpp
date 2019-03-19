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

LOG_SOURCE_CATEGORY("wiring.ble")


namespace {

const uint16_t PARTICLE_COMPANY_ID = 0x0662;

const uint8_t BLE_CTRL_REQ_SVC_UUID[BLE_SIG_UUID_128BIT_LEN] = {
    0xfc, 0x36, 0x6f, 0x54, 0x30, 0x80, 0xf4, 0x94, 0xa8, 0x48, 0x4e, 0x5c, 0x01, 0x00, 0xa9, 0x6f
};

void convertToHalUuid(const BleUuid& uuid, hal_ble_uuid_t* halUuid) {
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


BleUuid::BleUuid() : type_(BleUuidType::SHORT), order_(BleUuidOrder::LSB), shortUuid_(0x0000) {
}

BleUuid::BleUuid(const uint8_t* uuid128, BleUuidOrder order) : order_(order), shortUuid_(0x0000) {
    type_ = BleUuidType::LONG;
    memcpy(fullUuid_, uuid128, BLE_SIG_UUID_128BIT_LEN);
}

BleUuid::BleUuid(uint16_t uuid16, BleUuidOrder order) : order_(order), shortUuid_(uuid16) {
    type_ = BleUuidType::SHORT;
    memset(fullUuid_, 0x00, BLE_SIG_UUID_128BIT_LEN);
}

BleUuid::BleUuid(const uint8_t* uuid128, uint16_t uuid16, BleUuidOrder order) : order_(order), shortUuid_(0x0000) {
    type_ = BleUuidType::LONG;
    memcpy(fullUuid_, uuid128, BLE_SIG_UUID_128BIT_LEN);
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
    String str(string);
    setUuid(str);
}

BleUuid::~BleUuid() {

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


/**
 * BleAdvData class
 */
BleAdvData::BleAdvData() : len(0) {

}

BleAdvData::~BleAdvData() {

}

size_t BleAdvData::locate(uint8_t type, size_t* offset) {
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


/**
 * BleCharacteristic class
 */
uint16_t BleCharacteristic::defaultUuidCharCount_ = 0;

BleCharacteristic::BleCharacteristic() : properties(PROPERTY::NONE), description(nullptr), dataCb_(nullptr) {
    init();
}

BleCharacteristic::BleCharacteristic(const char* desc, BleCharProps properties, onDataReceivedCb cb)
        : properties(properties), description(desc), dataCb_(cb) {
    init();

    BleUuid svcUuid(PARTICLE_BLE::BLE_USER_DEFAULT_SVC_UUID);
    BleService* service = BleLocalDevice::getInstance()->addService(svcUuid);
    if (service != nullptr) {
        stub_ = this;
        uuid = generateDefaultCharUuid();
        service->addCharacteristic(*this);
    }
}

BleCharacteristic::BleCharacteristic(const char* desc, BleCharProps properties, BleUuid& charUuid, BleUuid& svcUuid, onDataReceivedCb cb)
        : properties(properties), uuid(charUuid), description(desc), dataCb_(cb) {
    init();

    BleService* service = BleLocalDevice::getInstance()->addService(svcUuid);
    if (service != nullptr) {
        stub_ = this;
        service->addCharacteristic(*this);
    }
}

BleCharacteristic::~BleCharacteristic() {

}

void BleCharacteristic::init(void) {
    valid = false;
    stub_ = nullptr;
}

int BleCharacteristic::setValue(const uint8_t* buf, size_t len) {
    if (!valid) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (isLocal) {
        return BleGattServer::write(*this, buf, len);
    }
    else {
        return BleGattClient::write(*this, buf, len);
    }
}

int BleCharacteristic::setValue(const String& str) {
    return setValue(reinterpret_cast<const uint8_t*>(str.c_str()), str.length());
}

int BleCharacteristic::setValue(const char* str) {
    return setValue(reinterpret_cast<const uint8_t*>(str), strlen(str));
}

size_t BleCharacteristic::getValue(uint8_t* buf, size_t len) const {
    if (!valid) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (isLocal) {
        return BleGattServer::read(*this, buf, len);
    }
    else {
        return BleGattClient::read(*this, buf, len);
    }
}

size_t BleCharacteristic::getValue(String& str) const {
    return 0;
}

void BleCharacteristic::onDataReceived(onDataReceivedCb callback) {
    if (callback != nullptr) {
        dataCb_ = callback;
    }
}

void BleCharacteristic::syncStub(void) const {
    if (stub_ != nullptr) {
        *stub_ = *this;
    }
}

BleUuid BleCharacteristic::generateDefaultCharUuid(void) const {
    defaultUuidCharCount_++;
    uint8_t uuid[BLE_SIG_UUID_128BIT_LEN];
    memcpy(uuid, PARTICLE_BLE::BLE_USER_DEFAULT_SVC_UUID, BLE_SIG_UUID_128BIT_LEN);
    uuid[12] = (uint8_t)(defaultUuidCharCount_ & 0x00FF);
    uuid[13] = (uint8_t)((defaultUuidCharCount_ >> 8) & 0x00FF);

    BleUuid charUuid(uuid);
    return charUuid;
}

void BleCharacteristic::processReceivedData(uint16_t attrHandle, const uint8_t* data, size_t len, const BlePeerDevice& peer) {
    if (isLocal) {
        if (attrHandle == attrHandles.cccd_handle) {
            if (data[0] > 0) {
                LOG(TRACE, "Configure CCCD: 0x%02x%02x", data[0],data[1]);
                cccdOfServer.append(peer.connHandle);
            }
            else {
                cccdOfServer.removeAll(peer.connHandle);
            }
            syncStub();
        }
    }
    if (attrHandle == attrHandles.value_handle) {
        if (dataCb_ != nullptr) {
            dataCb_(data, len);
        }
    }
}


/**
 * BleService class
 */
BleService::BleService() {

}

BleService::~BleService() {

}

BleCharacteristic* BleService::findCharacteristic(const char* desc) {
    for (size_t i = 0; i < characteristicCount(); i++) {
        BleCharacteristic& characteristic = characteristics_.at(i);
        if (!strcmp(characteristic.description, desc)) {
            return &characteristic;
        }
    }
    return nullptr;
}

BleCharacteristic* BleService::findCharacteristic(uint16_t attrHandle) {
    for (size_t i = 0; i < characteristicCount(); i++) {
        BleCharacteristic& characteristic = characteristics_.at(i);
        if (   characteristic.attrHandles.decl_handle == attrHandle
            || characteristic.attrHandles.value_handle == attrHandle
            || characteristic.attrHandles.user_desc_handle == attrHandle
            || characteristic.attrHandles.cccd_handle == attrHandle
            || characteristic.attrHandles.sccd_handle == attrHandle) {
            return &characteristic;
        }
    }
    return nullptr;
}

BleCharacteristic* BleService::findCharacteristic(const BleUuid& charUuid) {
    for (size_t i = 0; i < characteristicCount(); i++) {
        BleCharacteristic& characteristic = characteristics_.at(i);
        if (characteristic.uuid == charUuid) {
            return &characteristic;
        }
    }
    return nullptr;
}

BleCharacteristic* BleService::findCharacteristic(size_t i) {
    if (i >= characteristicCount()) {
        return nullptr;
    }
    return &characteristics_.at(i);
}

int BleService::addCharacteristic(BleCharacteristic& characteristic) {
    BleCharacteristic* charPtr = findCharacteristic(characteristic.uuid);
    if (charPtr == nullptr) {
        characteristic.service = this;
        if (characteristics_.append(characteristic)) {
            return SYSTEM_ERROR_NONE;
        }
        return SYSTEM_ERROR_INTERNAL;
    }
    return SYSTEM_ERROR_NONE;
}


/**
 * BleGattServer class
 */
BleGattServer::BleGattServer() {

}

BleGattServer::~BleGattServer() {

}

BleService* BleGattServer::findService(const BleUuid& svcUuid) {
    for (size_t i = 0; i < serviceCount(); i++) {
        BleService& service = services_.at(i);
        if (service.uuid == svcUuid) {
            return &service;
        }
    }
    return nullptr;
}

BleService* BleGattServer::findService(size_t i) {
    if (i >= serviceCount()) {
        return nullptr;
    }
    return &services_.at(i);
}

BleService* BleGattServer::addService(const BleUuid& svcUuid) {
    BleService* svcPtr = findService(svcUuid);
    if (svcPtr == nullptr) {
        BleService service;
        service.uuid = svcUuid;
        service.server = this;
        if (services_.append(service)) {
            return &services_.last();
        }
        else {
            return nullptr;
        }
    }
    return svcPtr;
}

size_t BleGattServer::characteristicCount(void) const {
    size_t total = 0;
    for (size_t i = 0; i < serviceCount(); i++) {
        total += services_.at(i).characteristicCount();
    }
    return total;
}

int BleGattServer::addCharacteristic(BleService& service, BleCharacteristic& characteristic) {
    return service.addCharacteristic(characteristic);
}

int BleGattServer::write(const BleCharacteristic& characteristic, const uint8_t* buf, size_t len) {
    size_t writeLen = len > BLE_MAX_CHAR_VALUE_LEN ? BLE_MAX_CHAR_VALUE_LEN : len;

    ble_gatt_server_set_characteristic_value(characteristic.attrHandles.value_handle, buf, writeLen, nullptr);

    size_t count = characteristic.cccdOfServer.size();
    for (size_t i = 0; i < count; i++) {
        if (characteristic.properties & PROPERTY::NOTIFY) {
            return ble_gatt_server_notify_characteristic_value( characteristic.cccdOfServer.at(i),
                    characteristic.attrHandles.value_handle, buf, writeLen, nullptr);
        }
        else if (characteristic.properties & PROPERTY::INDICATE) {
            return ble_gatt_server_indicate_characteristic_value( characteristic.cccdOfServer.at(i),
                    characteristic.attrHandles.value_handle, buf, writeLen, nullptr);
        }
    }
    return SYSTEM_ERROR_NONE;
}

int BleGattServer::read(const BleCharacteristic& characteristic, uint8_t* buf, size_t len) {
    uint16_t readLen = len > BLE_MAX_CHAR_VALUE_LEN ? BLE_MAX_CHAR_VALUE_LEN : len;
    ble_gatt_server_get_characteristic_value(characteristic.attrHandles.value_handle, buf, &readLen, nullptr);
    return readLen;
}

void BleGattServer::finalizeLocalGattServer(void) {
    size_t svcCount = serviceCount();
    LOG_DEBUG(TRACE, "Total local service: %d", svcCount);

    for (size_t i = 0; i < svcCount; i++) {
        BleService* service = findService(i);
        if (service != nullptr) {
            // Add service
            hal_ble_uuid_t halUuid;
            convertToHalUuid(service->uuid, &halUuid);
            ble_gatt_server_add_service(BLE_SERVICE_TYPE_PRIMARY, &halUuid, &service->startHandle, NULL);

            // Add Characteristics
            size_t count = service->characteristicCount();
            for (size_t j = 0; j < count; j++) {
                BleCharacteristic* characteristic = service->findCharacteristic(j);
                if (characteristic != nullptr) {
                    hal_ble_char_init_t char_init;
                    memset(&char_init, 0x00, sizeof(hal_ble_char_init_t));

                    convertToHalUuid(characteristic->uuid, &char_init.uuid);
                    char_init.properties = static_cast<uint8_t>(characteristic->properties);
                    char_init.service_handle = service->startHandle;
                    char_init.description = characteristic->description;

                    ble_gatt_server_add_characteristic(&char_init, &characteristic->attrHandles, NULL);

                    characteristic->isLocal = true;
                    characteristic->valid = true;
                    characteristic->syncStub();
                }
            }
        }
    }
}

void BleGattServer::gattServerProcessDataWritten(uint16_t attrHandle, const uint8_t* buf, size_t len, BlePeerDevice& peer) {
    BleCharacteristic* characteristic = findCharacteristic(attrHandle);
    if (characteristic != nullptr) {
        characteristic->processReceivedData(attrHandle, buf, len, peer);
    }
}

void BleGattServer::gattServerProcessDisconnected(const BlePeerDevice& peer) {
    for (size_t i = 0; i < characteristicCount(); i++) {
        BleCharacteristic* characteristic = findCharacteristic(i);
        characteristic->cccdOfServer.removeAll(peer.connHandle);
    }
}


/**
 * BleGattClient class
 */
BleGattClient::BleGattClient() {

}

BleGattClient::~BleGattClient() {

}

int BleGattClient::write(const BleCharacteristic& characteristic, const uint8_t* buf, size_t len) {
    size_t writeLen = len > BLE_MAX_CHAR_VALUE_LEN ? BLE_MAX_CHAR_VALUE_LEN : len;
    if (characteristic.properties & PROPERTY::INDICATE) {
        return ble_gatt_client_write_with_response( static_cast<BlePeerDevice*>(characteristic.service->server->device)->connHandle,
                characteristic.attrHandles.value_handle, buf, writeLen, nullptr);
    }
    else {
        return ble_gatt_client_write_without_response( static_cast<BlePeerDevice*>(characteristic.service->server->device)->connHandle,
                characteristic.attrHandles.value_handle, buf, writeLen, nullptr);
    }
}

int BleGattClient::read(const BleCharacteristic& characteristic, uint8_t* buf, size_t len) {
    uint16_t readLen = len > BLE_MAX_CHAR_VALUE_LEN ? BLE_MAX_CHAR_VALUE_LEN : len;
    ble_gatt_client_read( static_cast<BlePeerDevice*>(characteristic.service->server->device)->connHandle,
            characteristic.attrHandles.value_handle, buf, &readLen, nullptr);
    return readLen;
}

void BleGattClient::gattClientProcessDataNotified(uint16_t attrHandle, const uint8_t* buf, size_t len, BlePeerDevice& peer) {
    BleCharacteristic* characteristic = peer.findCharacteristic(attrHandle);
    if (characteristic != nullptr) {
        characteristic->processReceivedData(attrHandle, buf, len, peer);
    }
}


/**
 * BleBroadcaster class
 */
BleBroadcaster::BleBroadcaster() {
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
    advParams_.type          = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT;
    advParams_.filter_policy = BLE_ADV_FP_ANY;
    advParams_.interval      = BLE_DEFAULT_ADVERTISING_INTERVAL;
    advParams_.timeout       = BLE_DEFAULT_ADVERTISING_TIMEOUT;
    advParams_.inc_tx_power  = false;
}

BleBroadcaster::~BleBroadcaster() {

}

int BleBroadcaster::advDataBeacon(const iBeacon& beacon) {
    return 0;
}

int BleBroadcaster::append(uint8_t type, const uint8_t* buf, size_t len, BleAdvData& advData) {
    size_t offset;
    size_t adsLen = advData.locate(type, &offset);

    if (adsLen > 0) {
        // Update the existing AD structure.
        uint16_t staLen = advData.len - adsLen;
        if ((staLen + len + 2) <= BLE_MAX_ADV_DATA_LEN) {
            // Firstly, move the last consistent block.
            uint16_t moveLen = advData.len - offset - adsLen;
            memmove(&advData.data[offset + len], &advData.data[offset + adsLen], moveLen);

            // Secondly, Update the AD structure.
            // The Length field is the total length of Type field and Data field.
            advData.data[offset] = len + 1;
            memcpy(&advData.data[offset + 2], buf, len);

            // An AD structure is composed of one byte length field, one byte Type field and Data field.
            advData.len = staLen + len + 2;
        }
        else {
            return SYSTEM_ERROR_LIMIT_EXCEEDED;
        }
    }
    else {
        // Append the AD structure at the and of advertising data.
        if ((advData.len + len + 2) <= BLE_MAX_ADV_DATA_LEN) {
            advData.data[advData.len++] = len + 1;
            advData.data[advData.len++] = type;
            memcpy(&advData.data[advData.len], buf, len);
            advData.len += len;
        }
        else {
            return SYSTEM_ERROR_LIMIT_EXCEEDED;
        }
    }
    return SYSTEM_ERROR_NONE;
}

int BleBroadcaster::appendAdvData(uint8_t type, const uint8_t* buf, size_t len) {
    return append(type, buf, len, advData_);
}

int BleBroadcaster::appendAdvDataLocalName(const char* name) {
    return appendAdvData(BLE_SIG_AD_TYPE_COMPLETE_LOCAL_NAME, (const uint8_t*)name, strlen(name));
}

int BleBroadcaster::appendAdvDataCustomData(const uint8_t* buf, size_t len) {
    return appendAdvData(BLE_SIG_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, buf, len);
}

int BleBroadcaster::appendAdvDataUuid(const BleUuid& uuid) {
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

int BleBroadcaster::appendScanRspData(uint8_t type, const uint8_t* buf, size_t len) {
    if (type == BLE_SIG_AD_TYPE_FLAGS) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    return append(type, buf, len, srData_);
}

int BleBroadcaster::appendScanRspDataLocalName(const char* name) {
    return appendScanRspData(BLE_SIG_AD_TYPE_COMPLETE_LOCAL_NAME, (const uint8_t*)name, strlen(name));
}

int BleBroadcaster::appendScanRspDataCustomData(const uint8_t* buf, size_t len) {
    return appendScanRspData(BLE_SIG_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, buf, len);
}

int BleBroadcaster::appendScanRspDataUuid(const BleUuid& uuid) {
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

int BleBroadcaster::removeFromAdvData(uint8_t type) {
    size_t offset, len;
    len = advData_.locate(type, &offset);
    if (len > 0) {
        uint16_t moveLen = advData_.len - offset - len;
        memcpy(&advData_.data[offset], &advData_.data[offset + len], moveLen);
        advData_.len = advData_.len - len;
        return SYSTEM_ERROR_NONE;
    }
    else {
        return SYSTEM_ERROR_NONE;
    }
}

int BleBroadcaster::clearAdvData(void) {
    advData_.len = 0;
    memset(advData_.data, 0x00, sizeof(advData_.data));
    return SYSTEM_ERROR_NONE;
}

int BleBroadcaster::removeFromScanRspData(uint8_t type) {
    // The advertising data must contain the AD Flags AD structure.
    if (type == BLE_SIG_AD_TYPE_FLAGS) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }

    size_t offset, len;
    len = srData_.locate(type, &offset);
    if (len > 0) {
        uint16_t moveLen = srData_.len - offset - len;
        memcpy(&srData_.data[offset], &srData_.data[offset + len], moveLen);
        srData_.len = srData_.len - len;
        return SYSTEM_ERROR_NONE;
    }
    else {
        return SYSTEM_ERROR_NONE;
    }
}

int BleBroadcaster::clearScanRspData(void) {
    srData_.len = 0;
    memset(srData_.data, 0x00, sizeof(srData_.data));
    return SYSTEM_ERROR_NONE;
}

int BleBroadcaster::setTxPower(int8_t val) const {
    return ble_gap_set_tx_power(val);
}

int8_t BleBroadcaster::txPower(void) const {
    int8_t txPower;

    if (ble_gap_get_tx_power(&txPower) == SYSTEM_ERROR_NONE) {
        return txPower;
    }
    else {
        return 0x7F;
    }
}

int BleBroadcaster::advertise(void) {
    return advertise(advParams_);
}

int BleBroadcaster::advertise(uint32_t interval) {
    advParams_.interval = interval;
    return advertise(advParams_);
}

int BleBroadcaster::advertise(uint32_t interval, uint32_t timeout) {
    advParams_.interval = interval;
    advParams_.timeout = timeout;
    return advertise(advParams_);
}

int BleBroadcaster::advertise(const BleAdvParams& params) {
    advParams_ = params;
    ble_gap_set_advertising_parameters(&advParams_, NULL);

    if (advData_.len > 0) {
        ble_gap_set_advertising_data(advData_.data, advData_.len, nullptr);
    }
    if (srData_.len > 0) {
        ble_gap_set_scan_response_data(srData_.data, srData_.len, nullptr);
    }
    return ble_gap_start_advertising(NULL);
}

int BleBroadcaster::stopAdvertise(void) const {
    return ble_gap_stop_advertising();
}

void BleBroadcaster::broadcasterProcessStopped(void) {
    return;
}


/**
 * BleObserver class
 */
BleObserver::BleObserver() : targetCount_(0), callback_(nullptr), results_(nullptr), count_(0) {
    scanParams_.active = true;
    scanParams_.filter_policy = BLE_SCAN_FP_ACCEPT_ALL;
    scanParams_.interval = BLE_DEFAULT_SCANNING_INTERVAL;
    scanParams_.window = BLE_DEFAULT_SCANNING_WINDOW;
    scanParams_.timeout = BLE_DEFAULT_SCANNING_TIMEOUT;
}

BleObserver::~BleObserver() {

}

int BleObserver::scan(BleScanCallback callback) {
    callback_ = callback;
    ble_gap_set_scan_parameters(&scanParams_, NULL);
    ble_gap_start_scan(nullptr);
    return count_;
}

int BleObserver::scan(BleScanCallback callback, uint16_t timeout) {
    callback_ = callback;
    scanParams_.timeout  = timeout;
    ble_gap_set_scan_parameters(&scanParams_, NULL);
    ble_gap_start_scan(nullptr);
    return count_;
}

int BleObserver::scan(BleScannedDevice* results, size_t resultCount) {
    return scan(results, resultCount, scanParams_);
}

int BleObserver::scan(BleScannedDevice* results, size_t resultCount, uint16_t timeout) {
    scanParams_.timeout  = timeout;
    return scan(results, resultCount, scanParams_);
}

int BleObserver::scan(BleScannedDevice* results, size_t resultCount, const BleScanParams& params) {
    results_ = results;
    targetCount_ = resultCount;
    scanParams_ = params;
    ble_gap_set_scan_parameters(&scanParams_, NULL);
    ble_gap_start_scan(nullptr);
    return count_;
}

int BleObserver::stopScan(void) {
    return ble_gap_stop_scan();
}

void BleObserver::observerProcessScanResult(const hal_ble_gap_on_scan_result_evt_t* event) {
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

    if (callback_ != nullptr) {
        callback_(&device);
    }
    else if (results_ != nullptr) {
        results_[count_++] = device;
        if (count_ >= targetCount_) {
            stopScan();
        }
    }
}

void BleObserver::observerProcessScanStopped(const hal_ble_gap_on_scan_stopped_evt_t* event) {

}


/**
 * BlePeripheral class
 */
BlePeripheral::BlePeripheral() {
    ppcp_.min_conn_interval = BLE_DEFAULT_MIN_CONN_INTERVAL;
    ppcp_.max_conn_interval = BLE_DEFAULT_MAX_CONN_INTERVAL;
    ppcp_.slave_latency     = BLE_DEFAULT_SLAVE_LATENCY;
    ppcp_.conn_sup_timeout  = BLE_DEFAULT_CONN_SUP_TIMEOUT;
}

BlePeripheral::~BlePeripheral() {

}

int BlePeripheral::setPpcp(void) {
    return ble_gap_set_ppcp(&ppcp_, NULL);
}

int BlePeripheral::setPpcp(uint16_t minInterval, uint16_t maxInterval, uint16_t latency, uint16_t timeout) {
    ppcp_.min_conn_interval = minInterval;
    ppcp_.max_conn_interval = maxInterval;
    ppcp_.slave_latency = latency;
    ppcp_.conn_sup_timeout = timeout;
    return setPpcp();
}

int BlePeripheral::disconnect(void) {
    size_t count = centrals_.size();
    for (size_t i = 0; i < count; i++) {
        BlePeerDevice& central = centrals_.at(i);
        ble_gap_disconnect(central.connHandle, NULL);
        centrals_.clear();
    }
    return SYSTEM_ERROR_NONE;
}

BlePeerDevice* BlePeripheral::centralAt(size_t i) {
    if (i >= centralCount()) {
        return nullptr;
    }
    return &centrals_.at(i);
}

void BlePeripheral::peripheralProcessConnected(const BlePeerDevice& peer) {
    if (centralCount() < BLE_MAX_PERIPHERAL_COUNT) {
        centrals_.append(peer);
    }
}

void BlePeripheral::peripheralProcessDisconnected(const BlePeerDevice& peer) {
    centrals_.clear();
}


/**
 * BleCentral class
 */
BleCentral::BleCentral() {
    connParams_.min_conn_interval = BLE_DEFAULT_MIN_CONN_INTERVAL;
    connParams_.max_conn_interval = BLE_DEFAULT_MIN_CONN_INTERVAL;
    connParams_.slave_latency     = BLE_DEFAULT_SLAVE_LATENCY;
    connParams_.conn_sup_timeout  = BLE_DEFAULT_CONN_SUP_TIMEOUT;
}

BleCentral::~BleCentral() {

}

BlePeerDevice* BleCentral::connect(const BleAddress& addr, uint16_t interval, uint16_t latency, uint16_t timeout) {
    connParams_.min_conn_interval = interval;
    connParams_.max_conn_interval = interval;
    connParams_.slave_latency = latency;
    connParams_.conn_sup_timeout = timeout;
    ble_gap_connect(&addr, &connParams_, nullptr);
    return nullptr;
}

int BleCentral::disconnect(const BlePeerDevice& peripheral) {
    for (size_t i = 0; i < peripheralCount(); i++) {
        BlePeerDevice& peer = peripherals_.at(i);
        if (peer.connHandle == peripheral.connHandle) {
            ble_gap_disconnect(peer.connHandle, NULL);
            peripherals_.removeOne(peer);

            // clear CCCD
            return SYSTEM_ERROR_NONE;
        }
    }
    return SYSTEM_ERROR_NONE;
}

BlePeerDevice* BleCentral::peripheralAt(size_t i) {
    if (i >= peripheralCount()) {
        return nullptr;
    }
    return &peripherals_.at(i);
}

void BleCentral::centralProcessConnected(const BlePeerDevice& peer) {
    if (peripheralCount() < BLE_MAX_CENTRAL_COUNT) {
        peripherals_.append(peer);
    }
}

void BleCentral::centralProcessDisconnected(const BlePeerDevice& peer) {
    peripherals_.removeAll(peer);
}


/**
 * BlePeerDevice class
 */
BlePeerDevice::BlePeerDevice() {
    device = this;
    role = ROLE::INVALID;
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


/**
 * BleLocalDevice class
 */
BleLocalDevice* BleLocalDevice::getInstance(void) {
    static BleLocalDevice instance;
    return &instance;
}

void BleLocalDevice::onConnectionChangedCb(onConnectedCb connCb, onDisconnectedCb disconnCb) {
    connectedCb_ = connCb;
    disconnectedCb_ = disconnCb;
}

int BleLocalDevice::on(void) {
    if (!ble_stack_is_initialized()) {
        ble_stack_init(NULL);
    }

    BleAddress addr;
    ble_gap_get_device_address(&addr);
    address = addr;

    char devName[32] = {};
    CHECK(get_device_name(devName, sizeof(devName)));
    ble_gap_set_device_name((const uint8_t*)devName, strlen(devName));

    setPpcp();

    finalizeLocalGattServer();

    ble_set_callback_on_events(onBleEvents, getInstance());

    return SYSTEM_ERROR_NONE;
}

void BleLocalDevice::off(void) {

}

BlePeerDevice* BleLocalDevice::findPeerDevice(BleConnHandle connHandle) {
    for (size_t i = 0; i < centralCount(); i++) {
        BlePeerDevice* peer = centralAt(i);
        if (peer != nullptr) {
            if (peer->connHandle == connHandle) {
                return peer;
            }
        }
    }
    for (size_t i = 0; i < peripheralCount(); i++) {
        BlePeerDevice* peer = peripheralAt(i);
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
            bleInstance->broadcasterProcessStopped();
        } break;

        case BLE_EVT_SCAN_RESULT: {
            bleInstance->observerProcessScanResult(&event->params.scan_result);
        } break;

        case BLE_EVT_SCAN_STOPPED: {
            bleInstance->observerProcessScanStopped(&event->params.scan_stopped);
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
                bleInstance->peripheralProcessConnected(peer);
            }
            else {
                peer.role = ROLE::PERIPHERAL;
                bleInstance->centralProcessConnected(peer);
            }
        } break;

        case BLE_EVT_DISCONNECTED: {
            BlePeerDevice* peer = bleInstance->findPeerDevice(event->params.disconnected.conn_handle);
            if (peer != nullptr) {
                bleInstance->gattServerProcessDisconnected(*peer);

                if (peer->role & ROLE::PERIPHERAL) {
                    bleInstance->peripheralProcessDisconnected(*peer);
                }
                else {
                    bleInstance->centralProcessDisconnected(*peer);
                }
            }
        } break;

        case BLE_EVT_CONN_PARAMS_UPDATED: {

        } break;

        case BLE_EVT_DATA_WRITTEN: {
            LOG(TRACE, "onDataReceived: %d, %d", event->params.data_rec.conn_handle, event->params.data_rec.attr_handle);

            BlePeerDevice* peer = bleInstance->findPeerDevice(event->params.data_rec.conn_handle);
            if (peer != nullptr) {
                bleInstance->gattServerProcessDataWritten(event->params.data_rec.attr_handle,
                        event->params.data_rec.data, event->params.data_rec.data_len, *peer);
            }
        } break;

        case BLE_EVT_DATA_NOTIFIED: {
            LOG(TRACE, "onDataNotified: %d, %d", event->params.data_rec.conn_handle, event->params.data_rec.attr_handle);

            BlePeerDevice* peer = bleInstance->findPeerDevice(event->params.data_rec.conn_handle);
            if (peer != nullptr) {
                bleInstance->gattClientProcessDataNotified(event->params.data_rec.attr_handle,
                        event->params.data_rec.data, event->params.data_rec.data_len, *peer);
            }
        } break;

        default: break;
    }
}


BleLocalDevice& _fetch_ble() {
    return *BleLocalDevice::getInstance();
}


#endif
