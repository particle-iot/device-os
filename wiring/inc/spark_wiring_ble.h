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

#if Wiring_BLE

#include "system_error.h"
#include "spark_wiring_string.h"
#include "spark_wiring_vector.h"
#include "spark_wiring_flags.h"
#include "ble_hal.h"
#include <memory>

namespace particle {

class BleScanResult;
class BlePeerDevice;

// Forward declaration
class BleCharacteristicImpl;
class BleServiceImpl;
class BlePeerDeviceImpl;
class BleLocalDeviceImpl;

enum class BleUuidType {
    SHORT = 0,
    LONG = 1
};

enum class BleUuidOrder {
    MSB = 0,
    LSB = 1
};

enum class BleCharacteristicProperty : uint8_t {
    NONE                = 0,
    BROADCAST           = BLE_SIG_CHAR_PROP_BROADCAST,
    READ                = BLE_SIG_CHAR_PROP_READ,
    WRITE_WO_RSP        = BLE_SIG_CHAR_PROP_WRITE_WO_RESP,
    WRITE               = BLE_SIG_CHAR_PROP_WRITE,
    NOTIFY              = BLE_SIG_CHAR_PROP_NOTIFY,
    INDICATE            = BLE_SIG_CHAR_PROP_INDICATE,
    AUTH_SIGN_WRITES    = BLE_SIG_CHAR_PROP_AUTH_SIGN_WRITES,
    EXTENDED_PROP       = BLE_SIG_CHAR_PROP_EXTENDED_PROP
};

inline BleCharacteristicProperty operator&(BleCharacteristicProperty lhs, BleCharacteristicProperty rhs) {
    return static_cast<BleCharacteristicProperty>(
        static_cast<std::underlying_type<BleCharacteristicProperty>::type>(lhs) |
        static_cast<std::underlying_type<BleCharacteristicProperty>::type>(rhs)
    );
}

inline BleCharacteristicProperty& operator|=(BleCharacteristicProperty& lhs, BleCharacteristicProperty rhs) {
    lhs = static_cast<BleCharacteristicProperty> (
        static_cast<std::underlying_type<BleCharacteristicProperty>::type>(lhs) |
        static_cast<std::underlying_type<BleCharacteristicProperty>::type>(rhs)
    );
    return lhs;
}

enum class BleAdvertisingDataType : uint8_t {
    FLAGS                               = BLE_SIG_AD_TYPE_FLAGS,
    SERVICE_UUID_16BIT_MORE_AVAILABLE   = BLE_SIG_AD_TYPE_16BIT_SERVICE_UUID_MORE_AVAILABLE,
    SERVICE_UUID_16BIT_COMPLETE         = BLE_SIG_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE,
    SERVICE_UUID_32BIT_MORE_AVAILABLE   = BLE_SIG_AD_TYPE_32BIT_SERVICE_UUID_MORE_AVAILABLE,
    SERVICE_UUID_32BIT_COMPLETE         = BLE_SIG_AD_TYPE_32BIT_SERVICE_UUID_COMPLETE,
    SERVICE_UUID_128BIT_MORE_AVAILABLE  = BLE_SIG_AD_TYPE_128BIT_SERVICE_UUID_MORE_AVAILABLE,
    SERVICE_UUID_128BIT_COMPLETE        = BLE_SIG_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE,
    SHORT_LOCAL_NAME                    = BLE_SIG_AD_TYPE_SHORT_LOCAL_NAME,
    COMPLETE_LOCAL_NAME                 = BLE_SIG_AD_TYPE_COMPLETE_LOCAL_NAME,
    TX_POWER_LEVEL                      = BLE_SIG_AD_TYPE_TX_POWER_LEVEL,
    CLASS_OF_DEVICE                     = BLE_SIG_AD_TYPE_CLASS_OF_DEVICE,
    SIMPLE_PAIRING_HASH_C               = BLE_SIG_AD_TYPE_SIMPLE_PAIRING_HASH_C,
    SIMPLE_PAIRING_RANDOMIZER_R         = BLE_SIG_AD_TYPE_SIMPLE_PAIRING_RANDOMIZER_R,
    SECURITY_MANAGER_TK_VALUE           = BLE_SIG_AD_TYPE_SECURITY_MANAGER_TK_VALUE,
    SECURITY_MANAGER_OOB_FLAGS          = BLE_SIG_AD_TYPE_SECURITY_MANAGER_OOB_FLAGS,
    SLAVE_CONNECTION_INTERVAL_RANGE     = BLE_SIG_AD_TYPE_SLAVE_CONNECTION_INTERVAL_RANGE,
    SOLICITED_SERVICE_UUIDS_16BIT       = BLE_SIG_AD_TYPE_SOLICITED_SERVICE_UUIDS_16BIT,
    SOLICITED_SERVICE_UUIDS_128BIT      = BLE_SIG_AD_TYPE_SOLICITED_SERVICE_UUIDS_128BIT,
    SERVICE_DATA                        = BLE_SIG_AD_TYPE_SERVICE_DATA,
    PUBLIC_TARGET_ADDRESS               = BLE_SIG_AD_TYPE_PUBLIC_TARGET_ADDRESS,
    RANDOM_TARGET_ADDRESS               = BLE_SIG_AD_TYPE_RANDOM_TARGET_ADDRESS,
    APPEARANCE                          = BLE_SIG_AD_TYPE_APPEARANCE,
    ADVERTISING_INTERVAL                = BLE_SIG_AD_TYPE_ADVERTISING_INTERVAL,
    LE_BLUETOOTH_DEVICE_ADDRESS         = BLE_SIG_AD_TYPE_LE_BLUETOOTH_DEVICE_ADDRESS,
    LE_ROLE                             = BLE_SIG_AD_TYPE_LE_ROLE,
    SIMPLE_PAIRING_HASH_C256            = BLE_SIG_AD_TYPE_SIMPLE_PAIRING_HASH_C256,
    SIMPLE_PAIRING_RANDOMIZER_R256      = BLE_SIG_AD_TYPE_SIMPLE_PAIRING_RANDOMIZER_R256,
    SERVICE_SOLICITATION_32BIT_UUID     = BLE_SIG_AD_TYPE_32BIT_SERVICE_SOLICITATION_UUID,
    SERVICE_DATA_32BIT_UUID             = BLE_SIG_AD_TYPE_SERVICE_DATA_32BIT_UUID,
    SERVICE_DATA_128BIT_UUID            = BLE_SIG_AD_TYPE_SERVICE_DATA_128BIT_UUID,
    LESC_CONFIRMATION_VALUE             = BLE_SIG_AD_TYPE_LESC_CONFIRMATION_VALUE,
    LESC_RANDOM_VALUE                   = BLE_SIG_AD_TYPE_LESC_RANDOM_VALUE,
    URI                                 = BLE_SIG_AD_TYPE_URI,
    INDOOR_POSITIONING                  = BLE_SIG_AD_TYPE_INDOOR_POSITIONING,
    TRANSPORT_DISCOVERY_DATA            = BLE_SIG_AD_TYPE_TRANSPORT_DISCOVERY_DATA,
    LE_SUPPORTED_FEATURES               = BLE_SIG_AD_TYPE_LE_SUPPORTED_FEATURES,
    CHANNEL_MAP_UPDATE_INDICATION       = BLE_SIG_AD_TYPE_CHANNEL_MAP_UPDATE_INDICATION,
    PB_ADV                              = BLE_SIG_AD_TYPE_PB_ADV,
    MESH_MESSAGE                        = BLE_SIG_AD_TYPE_MESH_MESSAGE,
    MESH_BEACON                         = BLE_SIG_AD_TYPE_MESH_BEACON,
    THREE_D_INFORMATION_DATA            = BLE_SIG_AD_TYPE_3D_INFORMATION_DATA,
    MANUFACTURER_SPECIFIC_DATA          = BLE_SIG_AD_TYPE_MANUFACTURER_SPECIFIC_DATA,
};

enum class BleAdvertisingEventType : uint8_t {
    CONNECTABLE_SCANNABLE_UNDIRECRED        = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT,
    CONNECTABLE_UNDIRECTED                  = BLE_ADV_CONNECTABLE_UNDIRECTED_EVT,
    CONNECTABLE_DIRECTED                    = BLE_ADV_CONNECTABLE_DIRECTED_EVT,
    NON_CONNECTABLE_NON_SCANABLE_UNDIRECTED = BLE_ADV_NON_CONNECTABLE_NON_SCANABLE_UNDIRECTED_EVT,
    NON_CONNECTABLE_NON_SCANABLE_DIRECTED   = BLE_ADV_NON_CONNECTABLE_NON_SCANABLE_DIRECTED_EVT,
    SCANABLE_UNDIRECTED                     = BLE_ADV_SCANABLE_UNDIRECTED_EVT,
    SCANABLE_DIRECTED                       = BLE_ADV_SCANABLE_DIRECTED_EVT
};

enum class BleAntennaType : uint8_t {
    DEFAULT = BLE_ANT_DEFAULT,
    INTERNAL = BLE_ANT_INTERNAL,
    EXTERNAL = BLE_ANT_EXTERNAL
};

enum class BleAddressType : uint8_t {
    PUBLIC                          = BLE_SIG_ADDR_TYPE_PUBLIC,
    RANDOM_STATIC                   = BLE_SIG_ADDR_TYPE_RANDOM_STATIC,
    RANDOM_PRIVATE_RESOLVABLE       = BLE_SIG_ADDR_TYPE_RANDOM_PRIVATE_RESOLVABLE,
    RANDOM_PRIVATE_NON_RESOLVABLE   = BLE_SIG_ADDR_TYPE_RANDOM_PRIVATE_NON_RESOLVABLE
};

typedef hal_ble_conn_handle_t BleConnectionHandle;
typedef hal_ble_attr_handle_t BleAttributeHandle;

typedef void (*BleOnDataReceivedCallback)(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);
typedef void (*BleOnScanResultCallback)(const BleScanResult* device, void* context);
typedef void (*BleOnConnectedCallback)(const BlePeerDevice& peer, void* context);
typedef void (*BleOnDisconnectedCallback)(const BlePeerDevice& peer, void* context);

class BleAdvertisingParams : public hal_ble_adv_params_t {
};
static_assert(std::is_pod<BleAdvertisingParams>::value, "BleAdvertisingParams is not a POD struct");

class BleConnectionParams : public hal_ble_conn_params_t {
};
static_assert(std::is_pod<BleConnectionParams>::value, "BleConnectionParams is not a POD struct");

class BleScanParams : public hal_ble_scan_params_t {
};
static_assert(std::is_pod<BleScanParams>::value, "BleScanParams is not a POD struct");

class BleCharacteristicHandles : public hal_ble_char_handles_t {
public:
    BleCharacteristicHandles& operator=(const hal_ble_char_handles_t& halHandles) {
        this->size = halHandles.size;
        this->decl_handle = halHandles.decl_handle;
        this->value_handle = halHandles.value_handle;
        this->user_desc_handle = halHandles.user_desc_handle;
        this->cccd_handle = halHandles.cccd_handle;
        this->sccd_handle = halHandles.sccd_handle;
        return *this;
    }
};
static_assert(std::is_pod<BleCharacteristicHandles>::value, "BleCharacteristicHandles is not a POD struct");


class BleAddress {
public:
    BleAddress();
    BleAddress(const hal_ble_addr_t& addr);
    BleAddress(const uint8_t addr[BLE_SIG_ADDR_LEN], BleAddressType type = BleAddressType::PUBLIC);
    BleAddress(const char* address, BleAddressType type = BleAddressType::PUBLIC);
    BleAddress(const String& address, BleAddressType type = BleAddressType::PUBLIC);
    ~BleAddress() = default;

    // Setters
    int type(BleAddressType type);
    int set(const uint8_t addr[BLE_SIG_ADDR_LEN], BleAddressType type = BleAddressType::PUBLIC);
    int set(const char* address, BleAddressType type = BleAddressType::PUBLIC);
    int set(const String& address, BleAddressType type = BleAddressType::PUBLIC);
    BleAddress& operator=(const hal_ble_addr_t& addr);
    BleAddress& operator=(const uint8_t addr[BLE_SIG_ADDR_LEN]);

    // Getters
    BleAddressType type() const;
    void octets(uint8_t addr[BLE_SIG_ADDR_LEN]) const;
    String toString(bool stripped = false) const;
    size_t toString(char* buf, size_t len, bool stripped = false) const;
    hal_ble_addr_t halAddress() const;
    uint8_t operator[](uint8_t i) const;

    bool operator==(const BleAddress& addr) const;

private:
    void toBigEndian(uint8_t buf[BLE_SIG_ADDR_LEN]) const;

    hal_ble_addr_t address_;
};


class BleUuid {
public:
    BleUuid();
    BleUuid(const hal_ble_uuid_t& uuid);
    BleUuid(const BleUuid& uuid);
    BleUuid(const uint8_t* uuid128, BleUuidOrder order = BleUuidOrder::LSB);
    BleUuid(uint16_t uuid16);
    BleUuid(const uint8_t* uuid128, uint16_t uuid16, BleUuidOrder order = BleUuidOrder::LSB);
    BleUuid(const String& uuid);
    BleUuid(const char* uuid);
    ~BleUuid() = default;

    bool isValid() const;

    BleUuidType type() const;

    hal_ble_uuid_t& halUUID();

    uint16_t shorted() const;

    void rawBytes(uint8_t uuid128[BLE_SIG_UUID_128BIT_LEN]) const;
    const uint8_t* rawBytes() const;

    String toString(bool stripped = false) const;
    size_t toString(char* buf, size_t len, bool stripped = false) const;

    BleUuid& operator=(const BleUuid& uuid);
    BleUuid& operator=(const uint8_t* uuid128);
    BleUuid& operator=(uint16_t uuid16);
    BleUuid& operator=(const String& uuid);
    BleUuid& operator=(const char* uuid);
    BleUuid& operator=(const hal_ble_uuid_t& uuid);

    bool operator==(const BleUuid& uuid) const;
    bool operator==(const char* uuid) const;
    bool operator==(const String& uuid) const;
    bool operator==(uint16_t uuid) const;
    bool operator==(const uint8_t* uuid128) const;

private:
    void construct(const char* uuid);
    void toBigEndian(uint8_t buf[BLE_SIG_UUID_128BIT_LEN]) const;

    hal_ble_uuid_t uuid_;
};


class iBeacon {
public:
    iBeacon()
            : major(0),
              minor(0),
              measurePower(0) {
    }

    template<typename T>
    iBeacon(uint16_t major, uint16_t minor, T uuid, int8_t measurePower)
            : major(major),
              minor(minor),
              uuid(uuid),
              measurePower(measurePower) {
    }

    ~iBeacon() = default;

    uint16_t major;
    uint16_t minor;
    BleUuid uuid;
    int8_t measurePower;

    static const uint16_t APPLE_COMPANY_ID = 0x004C;
    static const uint8_t BEACON_TYPE_IBEACON = 0x02;
};


class BleAdvertisingData {
public:
    BleAdvertisingData();
    BleAdvertisingData(const iBeacon& beacon);
    ~BleAdvertisingData() = default;

    size_t set(const uint8_t* buf, size_t len);
    size_t set(const iBeacon& beacon);

    size_t append(BleAdvertisingDataType type, const uint8_t* buf, size_t len, bool force = false);
    size_t appendCustomData(const uint8_t* buf, size_t len, bool force = false);
    // According to the Bluetooth CSS, Local Name shall not appear more than once in a block.
    size_t appendLocalName(const char* name);
    size_t appendLocalName(const String& name);

    template<typename T>
    size_t appendServiceUUID(T uuid, bool force = false) {
        BleUuid tempUUID(uuid);
        if (tempUUID.type() == BleUuidType::SHORT) {
            uint16_t uuid16 = tempUUID.shorted();
            return append(BleAdvertisingDataType::SERVICE_UUID_16BIT_COMPLETE, reinterpret_cast<const uint8_t*>(&uuid16), sizeof(uint16_t), force);
        }
        else {
            return append(BleAdvertisingDataType::SERVICE_UUID_128BIT_COMPLETE, tempUUID.rawBytes(), BLE_SIG_UUID_128BIT_LEN, force);
        }
    }

    void clear();
    void remove(BleAdvertisingDataType type);

    size_t get(uint8_t* buf, size_t len) const;
    size_t get(BleAdvertisingDataType type, uint8_t* buf, size_t len) const;

    uint8_t* data();
    size_t length() const;

    String deviceName() const;
    size_t deviceName(char* buf, size_t len) const;
    size_t serviceUUID(BleUuid* uuids, size_t count) const;
    size_t customData(uint8_t* buf, size_t len) const;

    size_t operator()(uint8_t* buf, size_t len) const {
        return get(buf, len);
    }

    bool contains(BleAdvertisingDataType type) const;

private:
    size_t serviceUUID(BleAdvertisingDataType type, BleUuid* uuids, size_t count) const;
    static size_t locate(const uint8_t* buf, size_t len, BleAdvertisingDataType type, size_t* offset);

    uint8_t selfData_[BLE_MAX_ADV_DATA_LEN];
    size_t selfLen_;
};


class BleCharacteristic {
public:
    BleCharacteristic();
    BleCharacteristic(const BleCharacteristic& characteristic);
    BleCharacteristic(const char* desc, BleCharacteristicProperty properties, BleOnDataReceivedCallback callback = nullptr, void* context = nullptr);
    BleCharacteristic(const String& desc, BleCharacteristicProperty properties, BleOnDataReceivedCallback callback = nullptr, void* context = nullptr)
            : BleCharacteristic(desc.c_str(), properties, callback, context) {
    }

    template<typename T>
    BleCharacteristic(const char* desc, BleCharacteristicProperty properties, T charUuid, T svcUuid, BleOnDataReceivedCallback callback = nullptr, void* context = nullptr) {
        BleUuid cUuid(charUuid);
        BleUuid sUuid(svcUuid);
        construct(desc, properties, cUuid, sUuid, callback, context);
    }

    template<typename T>
    BleCharacteristic(const String& desc, BleCharacteristicProperty properties, T charUuid, T svcUuid, BleOnDataReceivedCallback callback = nullptr, void* context = nullptr)
            : BleCharacteristic(desc.c_str(), properties, charUuid, svcUuid, callback, context) {
    }
    ~BleCharacteristic();

    BleCharacteristic& operator=(const BleCharacteristic& characteristic);

    bool valid() const;

    BleUuid UUID() const;
    BleCharacteristicProperty properties() const;

    // Get characteristic value
    ssize_t getValue(uint8_t* buf, size_t len) const;
    ssize_t getValue(String& str) const;

    template<typename T>
    ssize_t getValue(T* val) const {
        size_t len = sizeof(T);
        return getValue(reinterpret_cast<uint8_t*>(val), len);
    }

    // Set characteristic value
    ssize_t setValue(const uint8_t* buf, size_t len);
    ssize_t setValue(const String& str);
    ssize_t setValue(const char* str);

    template<typename T>
    ssize_t setValue(T val) {
        uint8_t buf[BLE_MAX_ATTR_VALUE_PACKET_SIZE];
        size_t len = std::min(sizeof(T), (unsigned)BLE_MAX_ATTR_VALUE_PACKET_SIZE);
        for (size_t i = 0, j = len - 1; i < len; i++, j--) {
            buf[i] = reinterpret_cast<const uint8_t*>(&val)[j];
        }
        return setValue(buf, len);
    }

    // Valid for peer characteristic only. Manually enable the characteristic notification or indication.
    int subscribe(bool enable) const;

    void onDataReceived(BleOnDataReceivedCallback callback, void* context);

    BleCharacteristicImpl* impl() const {
        return impl_.get();
    }

private:
    void construct(const char* desc, BleCharacteristicProperty properties, BleUuid& charUuid, BleUuid& svcUuid, BleOnDataReceivedCallback callback, void* context);

    std::shared_ptr<BleCharacteristicImpl> impl_;
};


class BleService {
public:
    BleService();
    BleService(const BleUuid& uuid);
    ~BleService() = default;

    BleUuid UUID() const;

    BleService& operator=(const BleService& service);

    bool operator==(const BleService& service) const;

    BleServiceImpl* impl() const {
        return impl_.get();
    }

private:
    std::shared_ptr<BleServiceImpl> impl_;
};


class BleScanResult {
public:
    BleAddress address;
    BleAdvertisingData advertisingData;
    BleAdvertisingData scanResponse;
    int8_t rssi;
};


class BlePeerDevice {
public:
    BlePeerDevice();
    ~BlePeerDevice();

    Vector<BleService>& discoverAllServices();
    Vector<BleCharacteristic>& discoverAllCharacteristics();

    bool getCharacteristicByDescription(BleCharacteristic* characteristic, const char* desc) const;
    bool getCharacteristicByDescription(BleCharacteristic* characteristic, const String& desc) const;
    bool getCharacteristicByUUID(BleCharacteristic* characteristic, const BleUuid& uuid) const;

    template <typename T>
    BleCharacteristic getCharacteristicByUUID(T uuid) const {
        BleUuid charUuid(uuid);
        return getCharacteristicByUUID(charUuid);
    }

    bool connected() const;

    BleAddress address() const;

    bool operator==(const BlePeerDevice& device) const;

    BlePeerDevice& operator=(const BlePeerDevice& peer);

    BlePeerDeviceImpl* impl() const {
        return impl_.get();
    }

private:
    std::shared_ptr<BlePeerDeviceImpl> impl_;
};


class BleLocalDevice {
public:
    // Local device identifies.
    int setAddress(const BleAddress& address) const;
    int setAddress(const char* address, BleAddressType type = BleAddressType::PUBLIC) const;
    int setAddress(const String& address, BleAddressType type = BleAddressType::PUBLIC) const;
    BleAddress address() const;

    int setDeviceName(const char* name, size_t len) const;
    int setDeviceName(const char* name) const;
    int setDeviceName(const String& name) const;
    ssize_t getDeviceName(char* name, size_t len) const;
    String getDeviceName() const;

    // Access radio performance
    int on();
    int off();
    int setTxPower(int8_t txPower) const;
    int txPower(int8_t* txPower) const;
    int selectAntenna(BleAntennaType antenna) const;

    // Access advertising parameters
    int setAdvertisingInterval(uint16_t interval) const;
    int setAdvertisingTimeout(uint16_t timeout) const;
    int setAdvertisingType(BleAdvertisingEventType type) const;
    int setAdvertisingParameters(const BleAdvertisingParams* params) const;
    int setAdvertisingParameters(uint16_t interval, uint16_t timeout, BleAdvertisingEventType type) const;
    int getAdvertisingParameters(BleAdvertisingParams* params) const;

    // Access advertising data and scan response data
    int setAdvertisingData(BleAdvertisingData* advertisingData) const;
    int setScanResponseData(BleAdvertisingData* scanResponse) const;
    ssize_t getAdvertisingData(BleAdvertisingData* advertisingData) const;
    ssize_t getScanResponseData(BleAdvertisingData* scanResponse) const;

    // Advertising control
    int advertise() const;
    int advertise(BleAdvertisingData* advertisingData, BleAdvertisingData* scanResponse = nullptr) const;
    int advertise(const iBeacon& beacon) const;
    int stopAdvertising() const;
    bool advertising() const;

    // Access scanning parameters
    int setScanTimeout(uint16_t timeout) const;
    int setScanParameters(const BleScanParams* params) const;
    int getScanParameters(BleScanParams* params) const;

    // Scanning control
    int scan(BleOnScanResultCallback callback, void* context) const;
    int scan(BleScanResult* results, size_t resultCount) const;
    Vector<BleScanResult> scan() const;
    int stopScanning() const;

    // Access local characteristics
    BleCharacteristic addCharacteristic(BleCharacteristic& characteristic);
    BleCharacteristic addCharacteristic(const char* desc, BleCharacteristicProperty properties, BleOnDataReceivedCallback callback = nullptr, void* context = nullptr);
    BleCharacteristic addCharacteristic(const String& desc, BleCharacteristicProperty properties, BleOnDataReceivedCallback callback = nullptr, void* context = nullptr);

    template<typename T>
    BleCharacteristic addCharacteristic(const char* desc, BleCharacteristicProperty properties, T charUuid, T svcUuid, BleOnDataReceivedCallback callback = nullptr, void* context = nullptr) {
        BleCharacteristic characteristic(desc, properties, charUuid, svcUuid, callback, context);
        addCharacteristic(characteristic);
        return characteristic;
    }

    template<typename T>
    BleCharacteristic addCharacteristic(const String& desc, BleCharacteristicProperty properties, T charUuid, T svcUuid, BleOnDataReceivedCallback callback = nullptr, void* context = nullptr) {
        BleCharacteristic characteristic(desc.c_str(), properties, charUuid, svcUuid, callback, context);
        addCharacteristic(characteristic);
        return characteristic;
    }

    // Access connection parameters
    int setPPCP(uint16_t minInterval, uint16_t maxInterval, uint16_t latency, uint16_t timeout) const;

    // Connection control
    BlePeerDevice connect(const BleAddress& addr, const BleConnectionParams* params, bool automatic = true) const;
    BlePeerDevice connect(const BleAddress& addr, uint16_t interval, uint16_t latency, uint16_t timeout, bool automatic = true) const;
    BlePeerDevice connect(const BleAddress& addr, bool automatic = true) const;
    int disconnect() const;
    int disconnect(const BlePeerDevice& peer) const;
    bool connected() const;
    void onConnected(BleOnConnectedCallback callback, void* context) const;
    void onDisconnected(BleOnDisconnectedCallback callback, void* context) const;

    static BleLocalDevice& getInstance();

    BleLocalDeviceImpl* impl() const {
        return impl_.get();
    }

private:
    BleLocalDevice();
    ~BleLocalDevice() = default;

    std::unique_ptr<BleLocalDeviceImpl> impl_;
};

#define BLE BleLocalDevice::getInstance()

#ifndef BLE_WIRING_DEBUG_ENABLED
#define BLE_WIRING_DEBUG_ENABLED 0
#endif

} /* namespace particle */

#endif /* Wiring_BLE */

#endif /* SPARK_WIRING_BLE_H */
