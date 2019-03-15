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


void onAdvStopped(hal_ble_gap_on_adv_stopped_evt_t* event, void* context) {
    return;
}

__attribute__((unused))void onScanResult(void* event, void* context) {

}

__attribute__((unused))void onScanStopped(void* event, void* context) {

}

void onConnected(hal_ble_gap_on_connected_evt_t* event, void* context) {

}

void onDisconnected(hal_ble_gap_on_disconnected_evt_t* event, void* context) {

}

void onConnParamsUpdated(hal_ble_gap_on_conn_params_evt_t* event, void* context) {

}

__attribute__((unused))void onServiceDiscovered(void* event, void* context) {

}

__attribute__((unused))void onCharacteristicDiscovered(void* event, void* context) {

}

__attribute__((unused))void onDescriptorDiscovered(void* event, void* context) {

}

void onDataReceived(hal_ble_gatt_on_data_evt_t* event, void* context) {

}

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

    while(len > 0) {
        fullUuid_[len - 1] = 0x00;
        len--;
    }
}

bool BleUuid::operator == (const BleUuid& uuid) const {
    if (   type_ == uuid.type()
        && order_ == uuid.order()) {
        if (type_ == BleUuidType::SHORT) {
            return (shortUuid_ == uuid.shortUuid());
        } else {
            return !memcmp(fullUuid(), uuid.fullUuid(), BLE_SIG_UUID_128BIT_LEN);
        }
    }
    return false;
}


/**
 * BLEAdvertisingData class
 */
int BLEAdvertisingData::append(uint8_t type, const uint8_t* data, size_t len, bool sr) {
    size_t offset;
    size_t adsLen;
    bool adsExist = false;

    if (sr && type == BLE_SIG_AD_TYPE_FLAGS) {
        // The scan response data should not contain the AD Flags AD structure.
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if (locate(type, &offset, &adsLen, sr) == SYSTEM_ERROR_NONE) {
        adsExist = true;
    }

    uint8_t* advPtr;
    size_t* advLenPtr;

    if (sr) {
        advPtr = sr_;
        advLenPtr = &srLen_;
    } else {
        advPtr = adv_;
        advLenPtr = &advLen_;
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
            goto execute;
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
            goto execute;
        } else {
            return SYSTEM_ERROR_LIMIT_EXCEEDED;
        }
    }

execute:
    if (!sr) {
        ble_gap_set_advertising_data(adv_, advLen_, nullptr);
    }
    else {
        ble_gap_set_scan_response_data(sr_, srLen_, nullptr);
    }
    return SYSTEM_ERROR_NONE;
}

int BLEAdvertisingData::appendLocalName(const char* name, bool sr) {
    return append(BLE_SIG_AD_TYPE_COMPLETE_LOCAL_NAME, (const uint8_t*)name, strlen(name), sr);
}

int BLEAdvertisingData::appendCustomData(uint8_t* buf, size_t len, bool sr) {
    return append(BLE_SIG_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, buf, len, sr);
}

int BLEAdvertisingData::appendUuid(BleUuid& uuid, bool sr) {
    if (uuid.type() == BleUuidType::SHORT) {
        uint16_t uuid16;
        uuid16 = uuid.shortUuid();
        return append(BLE_SIG_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE, reinterpret_cast<const uint8_t*>(&uuid16), sizeof(uint16_t), sr);
    }
    else {
        uint8_t uuid128[16];
        uuid.fullUuid(uuid128);
        return append(BLE_SIG_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE, (const uint8_t*)uuid128, 16, sr);
    }
}

int BLEAdvertisingData::remove(uint8_t type) {
    size_t offset, len;
    bool sr;
    uint8_t* advPtr;
    size_t* advLenPtr;

    if (this->locate(type, &offset, &len, true) == SYSTEM_ERROR_NONE) {
        sr = true;
        advPtr = sr_;
        advLenPtr = &srLen_;
    } else if (locate(type, &offset, &len, false) == SYSTEM_ERROR_NONE) {
        sr = false;
        advPtr = adv_;
        advLenPtr = &advLen_;
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

size_t BLEAdvertisingData::find(uint8_t type, uint8_t* data, size_t len, bool sr) const {
    // An AD structure must consist of 1 byte length field, 1 byte type field and at least 1 byte data field
    if (advLen_ < 3 && srLen_ < 3) {
        return 0;
    }

    size_t offset, adsLen;
    const uint8_t* advPtr;

    if (locate(type, &offset, &adsLen, sr) == SYSTEM_ERROR_NONE) {
        if (sr) {
            advPtr = sr_;
        } else {
            advPtr = adv_;
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

size_t BLEAdvertisingData::find(uint8_t type, bool sr) const {
    return find(type, nullptr, 0, sr);
}

int BLEAdvertisingData::locate(uint8_t type, size_t* offset, size_t* len, bool sr) const {
    const uint8_t*  data;
    size_t dataLen;

    // A valid AD structure is composed of Length field, Type field and Data field.
    // Each field should be filled with at least one byte.
    if (!sr) {
        data = adv_;
        dataLen = advLen_;
    } else {
        data = sr_;
        dataLen = srLen_;
    }

    for (size_t i = 0; (i + 3) <= dataLen; i = i) {
        *len = data[i];

        uint8_t adsType = data[i + 1];
        if (adsType == type) {
            // The value of adsLen doesn't include the length field of an AD structure.
            if ((i + *len + 1) <= dataLen) {
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

    return SYSTEM_ERROR_NOT_FOUND;
}

size_t BLEAdvertisingData::advData(uint8_t* data, size_t len) const {
    len = (len > advLen_) ? advLen_ : len;
    memcpy(data, adv_, len);
    return len;
}

size_t BLEAdvertisingData::srData(uint8_t* data, size_t len) const {
    len = (len > srLen_) ? srLen_ : len;
    memcpy(data, sr_, len);
    return len;
}


/**
 * BleAttribute class
 */
uint16_t BleAttribute::defaultAttrCount_ = 0;

BleAttribute::BleAttribute() : properties_(PROPERTY::NONE), description_(nullptr), dataCb_(nullptr) {
    init();
}

BleAttribute::BleAttribute(const char* desc, BleAttrProps properties, onDataReceivedCb cb)
        : properties_(properties), description_(desc), dataCb_(cb) {
    init();
    svcUuid_ = assignDefaultSvcUuid();
    charUuid_ = generateDefaultAttrUuid();
    extRef_ = this;
    BleClass::getInstance()->local().preAddAttr(*this);
}

BleAttribute::BleAttribute(const char* desc, BleAttrProps properties, BleUuid& attrUuid, BleUuid& svcUuid, onDataReceivedCb cb)
        : properties_(properties), charUuid_(attrUuid), svcUuid_(svcUuid), description_(desc), dataCb_(cb) {
    init();
    extRef_ = this;
    BleClass::getInstance()->local().preAddAttr(*this);
}

BleAttribute::~BleAttribute() {

}

void BleAttribute::init(void) {
    valid_ = false;
    cccdConfigured_ = false;
    owner_ = nullptr;
    svcHandle_ = 0x00;
    extRef_ = nullptr;
    memset(&handles_, 0x00, sizeof(handles_));
}

bool BleAttribute::isLocalAttr(void) const {
    if (owner_ == nullptr) {
        return false;
    }
    BleAddress addr;
    ble_gap_get_device_address(&addr);
    return (owner_->address() == addr);
}

bool BleAttribute::contain(uint16_t handle) {
    if (   handles_.decl_handle == handle
        || handles_.value_handle == handle
        || handles_.user_desc_handle == handle
        || handles_.cccd_handle == handle
        || handles_.sccd_handle == handle) {
        return true;
    }
    return false;
}

BleUuid BleAttribute::generateDefaultAttrUuid(void) const {
    defaultAttrCount_++;
    uint8_t uuid[BLE_SIG_UUID_128BIT_LEN];
    memcpy(uuid, PARTICLE_BLE::BLE_USER_DEFAULT_SVC_UUID, BLE_SIG_UUID_128BIT_LEN);
    uuid[12] = (uint8_t)(defaultAttrCount_ & 0x00FF);
    uuid[13] = (uint8_t)((defaultAttrCount_ >> 8) & 0x00FF);

    BleUuid charUuid(uuid);
    return charUuid;
}

BleUuid BleAttribute::assignDefaultSvcUuid(void) const {
    BleUuid svcUuid(PARTICLE_BLE::BLE_USER_DEFAULT_SVC_UUID);
    return svcUuid;
}

int BleAttribute::setValue(const uint8_t* buf, size_t len) {
    if (!valid_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    uint16_t writeLen = len > BLE_MAX_CHAR_VALUE_LEN ? BLE_MAX_CHAR_VALUE_LEN : len;
    if (isLocalAttr()) {
        // FIXME: each attribute should have a vector to store the CCCD value for each connection.
        if (cccdConfigured_) {
            for (size_t i = 0; i < BleClass::getInstance()->peerCount(); i++) {
                const BleDevice* peer = BleClass::getInstance()->peer(i);

                // It will update the database if succeed.
                if (properties_ & PROPERTY::NOTIFY) {
                    return ble_gatt_server_notify_characteristic_value(peer->connHandle_, handles_.value_handle, buf, writeLen, nullptr);
                }
                else if (properties_ & PROPERTY::INDICATE) {
                    return ble_gatt_server_indicate_characteristic_value(peer->connHandle_, handles_.value_handle, buf, writeLen, nullptr);
                }
            }
        }
        else {
            ble_gatt_server_set_characteristic_value(handles_.value_handle, buf, writeLen, nullptr);
        }

        return SYSTEM_ERROR_NONE;
    }
    else {
        LOG(TRACE, "setValue: peer attribbute.");
        return SYSTEM_ERROR_NONE;
    }
}

int BleAttribute::setValue(const String& str) {
    return setValue(reinterpret_cast<const uint8_t*>(str.c_str()), str.length());
}

int BleAttribute::setValue(const char* str) {
    return setValue(reinterpret_cast<const uint8_t*>(str), strlen(str));
}

size_t BleAttribute::getValue(uint8_t* buf, size_t len) const {
    if (!valid_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (isLocalAttr()) {
        uint16_t readLen = len > BLE_MAX_CHAR_VALUE_LEN ? BLE_MAX_CHAR_VALUE_LEN : len;
        ble_gatt_server_get_characteristic_value(handles_.value_handle, buf, &readLen, nullptr);
        return readLen;
    }
    else {
        return 0;
    }
}

size_t BleAttribute::getValue(String& str) const {
    return 0;
}

void BleAttribute::onDataReceived(onDataReceivedCb callback) {
    if (callback != nullptr) {
        dataCb_ = callback;
    }
}

void BleAttribute::syncExtRef(void) const {
    if (extRef_ != nullptr) {
        *extRef_ = *this;
    }
}

void BleAttribute::processReceivedData(uint16_t handle, const uint8_t* data, size_t len) {
    if (isLocalAttr()) {
        if (handle == handles_.cccd_handle) {
            if (data[0] > 0) {
                LOG(TRACE, "Configure CCCD: 0x%02x%02x", data[0],data[1]);
                cccdConfigured_ = true;
            }
            else {
                cccdConfigured_ = false;
            }
            syncExtRef();
        }
    }
    if (handle == handles_.value_handle) {
        if (dataCb_ != nullptr) {
            dataCb_(data, len);
        }
    }
}


/**
 * BleDevice class
 */
BleDevice::BleDevice() : role_(ROLE::INVALID), connHandle_(BLE_INVALID_CONN_HANDLE), rssi_(-99) {
    memset(&connParams_, 0x00, sizeof(BleConnParams));
}

BleDevice::BleDevice(int n) : role_(ROLE::INVALID), connHandle_(BLE_INVALID_CONN_HANDLE), rssi_(-99), attributes_(n) {
    memset(&connParams_, 0x00, sizeof(BleConnParams));
}

Vector<BleAttribute> BleDevice::getAttrList(const char* desc) {
    Vector<BleAttribute> attrs;

    return attrs;
}

BleDevice::~BleDevice() {

}

Vector<BleAttribute> BleDevice::getAttrList(const BleUuid& svcUuid) {
    Vector<BleAttribute> attrs;
    for (size_t i = 0; i < attrCount(); i++) {
        BleAttribute& attr = attributes_.at(i);
        if (attr.serviceUuid() == svcUuid) {
            attrs.append(attr);
        }
    }
    return attrs;
}

BleAttribute* BleDevice::getAttr(uint16_t attrHandle) {
    for (size_t i = 0; i < attrCount(); i++) {
        BleAttribute& attr = attributes_.at(i);
        if (attr.contain(attrHandle)) {
            return &attr;
        }
    }
    return nullptr;
}

BleAttribute* BleDevice::getAttr(const BleUuid& charUuid) {
    for (size_t i = 0; i < attrCount(); i++) {
        BleAttribute& attr = attributes_.at(i);
        if (attr.charUuid() == charUuid) {
            return &attr;
        }
    }
    return nullptr;
}

BleAttribute* BleDevice::getAttr(size_t i) {
    if (i > attrCount()) {
        return nullptr;
    }
    return &attributes_.at(i);
}

int BleDevice::preAddAttr(const BleAttribute& attr) {
    if (attributes_.size() >= BLE_MAX_CHAR_COUNT) {
        return SYSTEM_ERROR_LIMIT_EXCEEDED;
    }
    attributes_.append(attr);
    return SYSTEM_ERROR_NONE;
}

void BleDevice::executeAddAttr(void) {
    size_t totalAttr = attrCount();
    LOG_DEBUG(TRACE, "Total local attribute: %d", totalAttr);

    for (size_t i = 0; i < totalAttr; i++) {
        BleAttribute* iterator = getAttr(i);
        if (iterator != nullptr) {
            if (iterator->valid_) {
                continue;
            }

            const BleUuid& svcUuid = iterator->serviceUuid();

            // A copy of attributes those have the same service UUID
            Vector<BleAttribute> foundAttrs = getAttrList(svcUuid);
            size_t attrCount = foundAttrs.size();

            if (attrCount > 0) {
                BleAttribute& attr = foundAttrs.at(0);
                if (!attr.valid_) {
                    uint16_t svcHandle;
                    hal_ble_uuid_t halUuid;

                    // Add service
                    convertToHalUuid(iterator->serviceUuid(), &halUuid);
                    ble_gatt_server_add_service(BLE_SERVICE_TYPE_PRIMARY, &halUuid, &svcHandle, NULL);

                    for (size_t j = 0; j < attrCount; j++) {
                        BleAttribute& attr = foundAttrs.at(j);
                        hal_ble_char_init_t char_init;
                        BleCharHandles handles;

                        // Add characteristic
                        memset(&char_init, 0x00, sizeof(hal_ble_char_init_t));
                        convertToHalUuid(attr.charUuid(), &halUuid);
                        char_init.properties = static_cast<uint8_t>(attr.properties());
                        char_init.service_handle = svcHandle;
                        char_init.uuid = halUuid;
                        char_init.description = attr.description();
                        ble_gatt_server_add_characteristic(&char_init, &handles, NULL);

                        // Update internal corresponding attribute
                        BleAttribute* internalAttr = getAttr(attr.charUuid());
                        if (internalAttr != nullptr) {
                            internalAttr->handles_ = handles;
                            internalAttr->owner_ = this;
                            internalAttr->valid_ = true;
                            internalAttr->syncExtRef();
                        }
                    }
                }
            }
        }
        else {
            continue;
        }
    }
}

bool BleDevice::operator == (const BleDevice& dev) const {
    if (connHandle_ == dev.connHandle_ && address_ == dev.address()) {
        return true;
    }
    return false;
}


/**
 * BleClass class
 */
BleClass* BleClass::getInstance(void) {
    static BleClass instance;
    return &instance;
}

void BleClass::onConnectionChangedCb(onConnectedCb connCb, onDisconnectedCb disconnCb) {
    connectedCb_ = connCb;
    disconnectedCb_ = disconnCb;
}

int BleClass::on(void) {
    if (!ble_stack_is_initialized()) {
        ble_stack_init(NULL);
    }

    /**
     * Set local device address
     */
    BleAddress addr;
    ble_gap_get_device_address(&addr);
    local().address_ = addr;

    /**
     * Set default device name
     */
    char devName[32] = {};
    CHECK(get_device_name(devName, sizeof(devName)));
    ble_gap_set_device_name((const uint8_t*)devName, strlen(devName));

    /**
     * Set default preferred connection parameters
     */
    hal_ble_conn_params_t connParams;
    connParams.min_conn_interval = BLE_DEFAULT_MIN_CONN_INTERVAL;
    connParams.max_conn_interval = BLE_DEFAULT_MAX_CONN_INTERVAL;
    connParams.slave_latency     = BLE_DEFAULT_SLAVE_LATENCY;
    connParams.conn_sup_timeout  = BLE_DEFAULT_CONN_SUP_TIMEOUT;
    ble_gap_set_ppcp(&connParams, NULL);

    /**
     * Set default advertising parameters
     */
    hal_ble_adv_params_t advParams;
    advParams.type          = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT;
    advParams.filter_policy = BLE_ADV_FP_ANY;
    advParams.interval      = BLE_DEFAULT_ADVERTISING_INTERVAL;
    advParams.timeout       = BLE_DEFAULT_ADVERTISING_TIMEOUT;
    advParams.inc_tx_power  = false;
    ble_gap_set_advertising_parameters(&advParams, NULL);

    /**
     * Set default scan parameters
     */
    hal_ble_scan_params_t scanParams;
    scanParams.active = true;
    scanParams.filter_policy = BLE_SCAN_FP_ACCEPT_ALL;
    scanParams.interval = BLE_DEFAULT_SCANNING_INTERVAL;
    scanParams.window   = BLE_DEFAULT_SCANNING_WINDOW;
    scanParams.timeout  = BLE_DEFAULT_SCANNING_TIMEOUT;
    ble_gap_set_scan_parameters(&scanParams, NULL);

    /**
     * Set default advertising and scan response data
     */
    BLEAdvertisingData advData;
    uint8_t  mfgData[31];
    size_t   mfgDataLen = 0;
    uint16_t platformID = PLATFORM_ID;
    uint8_t  flag = BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    BleUuid  svcUUID(BLE_CTRL_REQ_SVC_UUID);

    memcpy(&mfgData[mfgDataLen], (uint8_t*)&PARTICLE_COMPANY_ID, sizeof(PARTICLE_COMPANY_ID));
    mfgDataLen += sizeof(PARTICLE_COMPANY_ID);
    memcpy(&mfgData[mfgDataLen], (uint8_t*)&platformID, sizeof(platformID));
    mfgDataLen += sizeof(platformID);

    advData.append(BLE_SIG_AD_TYPE_FLAGS, &flag, 1);    // AD flag
    advData.appendLocalName(devName);   // Complete local name
    advData.appendCustomData(mfgData, mfgDataLen);  // Manufacture data
    advData.appendUuid(svcUUID, true);  // Full 128-bits UUID

    /**
     * Add local services and characteristics.
     */
    local().executeAddAttr();

    ble_set_callback_on_events(onBleEvents, getInstance());

    return SYSTEM_ERROR_NONE;
}

void BleClass::off(void) {

}

int BleClass::advertisementData(BLEAdvertisingData& data) {
    return SYSTEM_ERROR_NONE;
}

int BleClass::advertisementData(iBeacon& beacon) {
    return SYSTEM_ERROR_NONE;
}

int BleClass::advertise(void) const {
    return ble_gap_start_advertising(NULL);
}

int BleClass::advertise(uint32_t interval) const {
    BleAdvParams advParams;
    advParams.type          = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT;
    advParams.filter_policy = BLE_ADV_FP_ANY;
    advParams.interval      = interval;
    advParams.timeout       = BLE_DEFAULT_ADVERTISING_TIMEOUT;
    advParams.inc_tx_power  = false;
    ble_gap_set_advertising_parameters(&advParams, NULL);
    return advertise(advParams);
}

int BleClass::advertise(uint32_t interval, uint32_t timeout) const {
    BleAdvParams advParams;
    advParams.type          = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT;
    advParams.filter_policy = BLE_ADV_FP_ANY;
    advParams.interval      = interval;
    advParams.timeout       = timeout;
    advParams.inc_tx_power  = false;
    ble_gap_set_advertising_parameters(&advParams, NULL);
    return advertise(advParams);
}

int BleClass::advertise(BleAdvParams& params) const {
    ble_gap_set_advertising_parameters(&params, NULL);
    return ble_gap_start_advertising(NULL);
}

int BleClass::scan(BLEScanResult* results, size_t count, uint16_t timeout) const {
    return SYSTEM_ERROR_NONE;
}

BleDevice* BleClass::connect(const BleAddress& addr) {
    return nullptr;
}

int BleClass::disconnect(void) {
    for (size_t i = 0; i < peerCount(); i++) {
        BleDevice& peer = peers_.at(i);
        ble_gap_disconnect(peer.connHandle_, NULL);
        removePeer(peer.connHandle_);
    }
    return SYSTEM_ERROR_NONE;
}

int BleClass::disconnect(const BleDevice& peer) {
    ble_gap_disconnect(peer.connHandle_, NULL);
    removePeer(peer.connHandle_);
    return SYSTEM_ERROR_NONE;
}

const BleDevice* BleClass::peer(size_t i) const {
    if (i >= peerCount()) {
        return nullptr;
    }
    return &peers_.at(i);
}

const BleDevice* BleClass::findPeer(BleConnHandle handle) const {
    for (size_t i = 0; i < peerCount(); i++) {
        const BleDevice& dev = peers_.at(i);
        if (dev.connHandle_ == handle) {
            return &dev;
        }
    }
    return nullptr;
}

BleDevice& BleClass::local(void) {
    static BleDevice local;
    return local;
}

int BleClass::addPeer(const BleDevice& device) {
    if (peerCount() >= (BLE_MAX_PERIPHERAL_COUNT + BLE_MAX_CENTRAL_COUNT)) {
        return SYSTEM_ERROR_LIMIT_EXCEEDED;
    }
    peers_.append(device);
    return SYSTEM_ERROR_NONE;
}

int BleClass::removePeer(BleConnHandle handle) {
    const BleDevice* dev = findPeer(handle);
    if (dev != nullptr) {
        peers_.removeOne(*dev);
    }
    return SYSTEM_ERROR_NONE;
}

void BleClass::updateLocal(void) {
    for (size_t i = 0; i < local().attrCount(); i++) {
        BleAttribute* attr = local().getAttr(i);
        attr->cccdConfigured_ = false;
        attr->syncExtRef();
    }
}

void BleClass::onBleEvents(hal_ble_evts_t *event, void* context) {
    if (context == nullptr) {
        return;
    }

    auto bleInstance = static_cast<BleClass*>(context);

    switch (event->type) {
        case BLE_EVT_ADV_STOPPED:
            onAdvStopped(&event->params.adv_stopped, context);
            break;

        case BLE_EVT_CONNECTED: {
            onConnected(&event->params.connected, context);

            BleDevice peer;
            BleConnParams params;

            params.conn_sup_timeout = event->params.connected.conn_sup_timeout;
            params.slave_latency = event->params.connected.slave_latency;
            params.max_conn_interval = event->params.connected.conn_interval;
            params.min_conn_interval = event->params.connected.conn_interval;
            peer.connHandle_ = event->params.connected.conn_handle;
            peer.connParams_ = params;
            peer.address_ = static_cast<const BleAddress&>(event->params.connected.peer_addr);
            if (event->params.connected.role == BLE_ROLE_PERIPHERAL) {
                peer.role_ = ROLE::CENTRAL;
            }
            else {
                peer.role_ = ROLE::PERIPHERAL;
            }

            bleInstance->addPeer(peer);
        } break;

        case BLE_EVT_DISCONNECTED: {
            onDisconnected(&event->params.disconnected, context);

            bleInstance->removePeer(event->params.disconnected.conn_handle);
            bleInstance->updateLocal();
        } break;

        case BLE_EVT_CONN_PARAMS_UPDATED:
            onConnParamsUpdated(&event->params.conn_params_updated, context);
            break;

        case BLE_EVT_DATA_WRITTEN: {
            onDataReceived(&event->params.data_rec, context);

            LOG(TRACE, "onDataReceived: %d, %d", event->params.data_rec.conn_handle, event->params.data_rec.attr_handle);
            const BleDevice* peerPtr = bleInstance->findPeer(event->params.data_rec.conn_handle);
            if (peerPtr == nullptr) {
                return;
            }

            if (peerPtr->role() & ROLE::CENTRAL) {
                // Received data from peer Central
                BleAttribute* attr = BleClass::getInstance()->local().getAttr(event->params.data_rec.attr_handle);
                if (attr != nullptr) {
                    attr->processReceivedData(event->params.data_rec.attr_handle, event->params.data_rec.data, event->params.data_rec.data_len);
                }
            }
            else if (peerPtr->role() & ROLE::PERIPHERAL) {

            }
        } break;

        default: break;
    }
}


BleClass& _fetch_ble() {
    return *BleClass::getInstance();
}


#endif
