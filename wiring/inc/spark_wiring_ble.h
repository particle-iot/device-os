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
#include <functional>
#include "spark_wiring_platform.h"

#if Wiring_BLE

#include "system_error.h"
#include "spark_wiring_string.h"
#include "spark_wiring_vector.h"
#include "spark_wiring_flags.h"
#include "ble_hal.h"
#include <memory>
#include "enumflags.h"
#include "system_ble_prov.h"

using namespace std::placeholders;

namespace particle {

constexpr int8_t BLE_RSSI_INVALID = 0x7F;
constexpr int8_t BLE_TX_POWER_INVALID = 0x7F;

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
ENABLE_ENUM_CLASS_BITWISE(BleCharacteristicProperty);

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

enum class BlePhy : uint8_t {
    BLE_PHYS_AUTO        = hal_ble_phys_t::BLE_PHYS_AUTO,
    BLE_PHYS_1MBPS       = hal_ble_phys_t::BLE_PHYS_1MBPS,
    BLE_PHYS_CODED       = hal_ble_phys_t::BLE_PHYS_CODED
};

ENABLE_ENUM_CLASS_BITWISE(BlePhy);

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

enum class BleTxRxType : uint8_t {
    AUTO = 0,
    ACK  = 1,
    NACK = 2
};

enum class BlePairingIoCaps : uint8_t {
    NONE = BLE_IO_CAPS_NONE,
    DISPLAY_ONLY = BLE_IO_CAPS_DISPLAY_ONLY,
    DISPLAY_YESNO = BLE_IO_CAPS_DISPLAY_YESNO,
    KEYBOARD_ONLY = BLE_IO_CAPS_KEYBOARD_ONLY,
    KEYBOARD_DISPLAY = BLE_IO_CAPS_KEYBOARD_DISPLAY
};

enum class BlePairingAlgorithm : uint8_t {
    AUTO = BLE_PAIRING_ALGORITHM_AUTO,
    LEGACY_ONLY = BLE_PAIRING_ALGORITHM_LEGACY_ONLY,
    LESC_ONLY = BLE_PAIRING_ALGORITHM_LESC_ONLY  // nRF52840-based platforms only
};

enum class BlePairingEventType : uint8_t {
    NONE = BLE_EVT_UNKNOWN,
    REQUEST_RECEIVED = BLE_EVT_PAIRING_REQUEST_RECEIVED,
    PASSKEY_DISPLAY = BLE_EVT_PAIRING_PASSKEY_DISPLAY,
    PASSKEY_INPUT = BLE_EVT_PAIRING_PASSKEY_INPUT,
    STATUS_UPDATED = BLE_EVT_PAIRING_STATUS_UPDATED,
    NUMERIC_COMPARISON = BLE_EVT_PAIRING_NUMERIC_COMPARISON
};

struct BlePairingStatus {
    int status;
    bool bonded;
    bool lesc;
};

union BlePairingEventPayload {
    const uint8_t* passkey;
    BlePairingStatus status;
};

struct BlePairingEvent {
    BlePeerDevice& peer;
    BlePairingEventType type;
    size_t payloadLen;
    BlePairingEventPayload payload;
};

typedef hal_ble_conn_handle_t BleConnectionHandle;
typedef hal_ble_attr_handle_t BleAttributeHandle;

typedef void (*BleOnDataReceivedCallback)(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);
typedef void (*BleOnScanResultCallback)(const BleScanResult* result, void* context);
typedef void (*BleOnScanResultCallbackRef)(const BleScanResult& result, void* context);
typedef void (*BleOnConnectedCallback)(const BlePeerDevice& peer, void* context);
typedef void (*BleOnDisconnectedCallback)(const BlePeerDevice& peer, void* context);
typedef void (*BleOnPairingEventCallback)(const BlePairingEvent& event, void* context);
typedef void (*BleOnAttMtutExchangedCallback)(const BlePeerDevice& peer, size_t attMtu, void* context);

typedef std::function<void(const uint8_t*, size_t, const BlePeerDevice& peer)> BleOnDataReceivedStdFunction;
typedef std::function<void(const BleScanResult& result)> BleOnScanResultStdFunction;
typedef std::function<void(const BlePeerDevice& peer)> BleOnConnectedStdFunction;
typedef std::function<void(const BlePeerDevice& peer)> BleOnDisconnectedStdFunction;
typedef std::function<void(const BlePairingEvent& event)> BleOnPairingEventStdFunction;
typedef std::function<void(const BlePeerDevice& peer, size_t attMtu)> BleOnAttMtuExchangedStdFunction;

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
    BleAddress(const BleAddress& addr);
    BleAddress(const hal_ble_addr_t& addr);
    BleAddress(const uint8_t addr[BLE_SIG_ADDR_LEN], BleAddressType type = BleAddressType::PUBLIC);
    BleAddress(const char* address, BleAddressType type = BleAddressType::PUBLIC);
    BleAddress(const String& address, BleAddressType type = BleAddressType::PUBLIC);
    ~BleAddress() = default;

    // Setters
    int type(BleAddressType type);
    int set(const hal_ble_addr_t& addr);
    int set(const uint8_t addr[BLE_SIG_ADDR_LEN], BleAddressType type = BleAddressType::PUBLIC);
    int set(const char* address, BleAddressType type = BleAddressType::PUBLIC);
    int set(const String& address, BleAddressType type = BleAddressType::PUBLIC);

    BleAddress& operator=(const BleAddress& addr);
    BleAddress& operator=(const hal_ble_addr_t& addr);
    BleAddress& operator=(const uint8_t addr[BLE_SIG_ADDR_LEN]);
    BleAddress& operator=(const char* address);
    BleAddress& operator=(const String& address);

    // Getters
    BleAddressType type() const;
    void octets(uint8_t addr[BLE_SIG_ADDR_LEN]) const;
    String toString(bool stripped = false) const;
    size_t toString(char* buf, size_t len, bool stripped = false) const;
    hal_ble_addr_t halAddress() const;
    uint8_t operator[](uint8_t i) const;

    bool operator==(const BleAddress& addr) const;
    bool operator==(const hal_ble_addr_t& addr) const;
    bool operator==(const uint8_t addr[BLE_SIG_ADDR_LEN]) const;
    bool operator==(const char* address) const;
    bool operator==(const String& address) const;

    bool operator!=(const BleAddress& addr) const;
    bool operator!=(const hal_ble_addr_t& addr) const;
    bool operator!=(const uint8_t addr[BLE_SIG_ADDR_LEN]) const;
    bool operator!=(const char* address) const;
    bool operator!=(const String& address) const;

    operator bool() const {
        return isValid();
    }

    bool isValid() const;
    int clear();

private:
    void toBigEndian(uint8_t buf[BLE_SIG_ADDR_LEN]) const;

    hal_ble_addr_t address_;
};


class BleUuid {
public:
    BleUuid();
    BleUuid(const hal_ble_uuid_t& uuid);
    BleUuid(const BleUuid& uuid) = default;
    BleUuid(const uint8_t* uuid128, BleUuidOrder order = BleUuidOrder::LSB);
    BleUuid(const uint8_t* uuid128, uint16_t uuid16, BleUuidOrder order = BleUuidOrder::LSB);
    BleUuid(uint16_t uuid16);
    BleUuid(const String& uuid);
    BleUuid(const char* uuid);
    ~BleUuid() = default;

    bool isValid() const;

    BleUuidType type() const;

    hal_ble_uuid_t halUUID();

    uint16_t shorted() const;

    size_t rawBytes(uint8_t uuid128[BLE_SIG_UUID_128BIT_LEN]) const;
    const uint8_t* rawBytes() const;

    String toString(bool stripped = false) const;
    size_t toString(char* buf, size_t len, bool stripped = false) const;

    template<typename T>
    BleUuid& operator=(T uuid) {
        return *this = BleUuid(uuid);
    }

    bool operator==(const BleUuid& uuid) const;

    template<typename T>
    bool operator==(T uuid) const {
        return *this == BleUuid(uuid);
    }

    template<typename T>
    bool operator!=(T uuid) const {
        return !(*this == BleUuid(uuid));
    }

    uint8_t operator[](uint8_t i) const;

    operator bool() const {
        return isValid();
    }

private:
    void construct(const char* uuid);
    void toBigEndian(uint8_t buf[BLE_SIG_UUID_128BIT_LEN]) const;

    static constexpr uint8_t BASE_UUID[BLE_SIG_UUID_128BIT_LEN] = {0xFB,0x34,0x9B,0x5F,0x80,0x00, 0x00,0x80, 0x00,0x10, 0x00,0x00, 0x00,0x00,0x00,0x00};
    static constexpr uint8_t UUID16_LO = 12;
    static constexpr uint8_t UUID16_HI = 13;

    uint8_t uuid128_[BLE_SIG_UUID_128BIT_LEN];
    BleUuidType type_;
};


class iBeacon {
public:
    iBeacon()
            : major_(0),
              minor_(0),
              measurePower_(0) {
    }

    template<typename T>
    iBeacon(uint16_t major, uint16_t minor, T uuid, int8_t measurePower)
            : major_(major),
              minor_(minor),
              uuid_(uuid),
              measurePower_(measurePower) {
    }

    ~iBeacon() = default;

    iBeacon& major(uint16_t major) {
        major_ = major;
        return *this;
    }

    iBeacon& minor(uint16_t minor) {
        minor_ = minor;
        return *this;
    }

    template<typename T>
    iBeacon& UUID(T uuid) {
        uuid_ = BleUuid(uuid);
        return *this;
    }

    iBeacon& measurePower(int8_t power) {
        measurePower_ = power;
        return *this;
    }

    uint16_t major() const {
        return major_;
    }

    uint16_t minor() const {
        return minor_;
    }

    const BleUuid& UUID() const {
        return uuid_;
    }

    int8_t measurePower() const {
        return measurePower_;
    }

    static const uint16_t APPLE_COMPANY_ID = 0x004C;
    static const uint8_t BEACON_TYPE_IBEACON = 0x02;

private:
    uint16_t major_;
    uint16_t minor_;
    BleUuid uuid_;
    int8_t measurePower_;
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
    size_t appendAppearance(ble_sig_appearance_t appearance);

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

    size_t resize(size_t size);

    void clear();
    void remove(BleAdvertisingDataType type);

    size_t get(uint8_t* buf, size_t len) const;
    size_t get(BleAdvertisingDataType type, uint8_t* buf, size_t len) const;

    uint8_t* data();
    size_t length() const;

    String deviceName() const;
    size_t deviceName(char* buf, size_t len) const;

    size_t serviceUUID(BleUuid* uuids, size_t count) const;
    Vector<BleUuid> serviceUUID() const;

    size_t customData(uint8_t* buf, size_t len) const;

    ble_sig_appearance_t appearance() const;

    size_t operator()(uint8_t* buf, size_t len) const {
        return get(buf, len);
    }

    uint8_t operator[](uint8_t i) const;

    bool contains(BleAdvertisingDataType type) const;

private:
    Vector<BleUuid> serviceUUID(BleAdvertisingDataType type) const;
    static size_t locate(const uint8_t* buf, size_t len, BleAdvertisingDataType type, size_t* offset);

    Vector<uint8_t> selfData_;
};


class BleCharacteristic {
public:
    BleCharacteristic();
    BleCharacteristic(const BleCharacteristic& characteristic);

    __attribute__((deprecated("The order of the first argument and second argument need to be swapped")))
    BleCharacteristic(const char* desc, EnumFlags<BleCharacteristicProperty> properties, BleOnDataReceivedCallback callback = nullptr, void* context = nullptr)
            : BleCharacteristic(properties, desc, callback, context) {
    }
    
    __attribute__((deprecated("The order of the first argument and second argument need to be swapped")))
    BleCharacteristic(const String& desc, EnumFlags<BleCharacteristicProperty> properties, BleOnDataReceivedCallback callback = nullptr, void* context = nullptr)
            : BleCharacteristic(properties, desc, callback, context) {
    }

    BleCharacteristic(EnumFlags<BleCharacteristicProperty> properties,  const char* desc, BleOnDataReceivedCallback callback = nullptr, void* context = nullptr);
    BleCharacteristic(EnumFlags<BleCharacteristicProperty> properties, const String& desc, BleOnDataReceivedCallback callback = nullptr, void* context = nullptr)
            : BleCharacteristic(properties, desc.c_str(), callback, context) {
    }
    BleCharacteristic(EnumFlags<BleCharacteristicProperty> properties, const char* desc, const BleOnDataReceivedStdFunction& callback);
    BleCharacteristic(EnumFlags<BleCharacteristicProperty> properties, const String& desc, const BleOnDataReceivedStdFunction& callback)
            : BleCharacteristic(properties, desc.c_str(), callback) {
    }

    template<typename T>
    BleCharacteristic(EnumFlags<BleCharacteristicProperty> properties, const String& desc, void(T::*callback)(const uint8_t*, size_t, const BlePeerDevice& peer), T* instance)
            : BleCharacteristic(properties, desc, (callback && instance) ? std::bind(callback, instance, _1, _2, _3) : (BleOnDataReceivedStdFunction)nullptr) {
    }

    template<typename T>
    BleCharacteristic(EnumFlags<BleCharacteristicProperty> properties, const char* desc, void(T::*callback)(const uint8_t*, size_t, const BlePeerDevice& peer), T* instance)
            : BleCharacteristic(properties, desc, (callback && instance) ? std::bind(callback, instance, _1, _2, _3) : (BleOnDataReceivedStdFunction)nullptr) {
    }

    template<typename T1, typename T2>
    BleCharacteristic(const char* desc, EnumFlags<BleCharacteristicProperty> properties, T1 charUuid, T2 svcUuid, BleOnDataReceivedCallback callback = nullptr, void* context = nullptr) {
        BleUuid cUuid(charUuid);
        BleUuid sUuid(svcUuid);
        construct(desc, properties, cUuid, sUuid, callback, context);
    }

    template<typename T1, typename T2>
    BleCharacteristic(const String& desc, EnumFlags<BleCharacteristicProperty> properties, T1 charUuid, T2 svcUuid, BleOnDataReceivedCallback callback = nullptr, void* context = nullptr)
            : BleCharacteristic(desc.c_str(), properties, charUuid, svcUuid, callback, context) {
    }

    template<typename T1, typename T2>
    BleCharacteristic(const char* desc, EnumFlags<BleCharacteristicProperty> properties, T1 charUuid, T2 svcUuid, const BleOnDataReceivedStdFunction& callback) {
        BleUuid cUuid(charUuid);
        BleUuid sUuid(svcUuid);
        construct(desc, properties, cUuid, sUuid, callback);
    }

    template<typename T1, typename T2>
    BleCharacteristic(const String& desc, EnumFlags<BleCharacteristicProperty> properties, T1 charUuid, T2 svcUuid, const BleOnDataReceivedStdFunction& callback)
            : BleCharacteristic(desc.c_str(), properties, charUuid, svcUuid, callback) {
    }

    template<typename T1, typename T2, typename T3>
    BleCharacteristic(const char* desc, EnumFlags<BleCharacteristicProperty> properties, T1 charUuid, T2 svcUuid, void(T3::*callback)(const uint8_t*, size_t, const BlePeerDevice& peer), T3* instance)
            : BleCharacteristic(desc, properties, charUuid, svcUuid, (callback && instance) ? std::bind(callback, instance, _1, _2, _3) : (BleOnDataReceivedStdFunction)nullptr) {
    }

    template<typename T1, typename T2, typename T3>
    BleCharacteristic(const String& desc, EnumFlags<BleCharacteristicProperty> properties, T1 charUuid, T2 svcUuid, void(T3::*callback)(const uint8_t*, size_t, const BlePeerDevice& peer), T3* instance)
            : BleCharacteristic(desc, properties, charUuid, svcUuid, (callback && instance) ? std::bind(callback, instance, _1, _2, _3) : (BleOnDataReceivedStdFunction)nullptr) {
    }

    ~BleCharacteristic();

    BleCharacteristic& operator=(const BleCharacteristic& characteristic);

    bool __attribute__((deprecated("Use BleCharacteristic::isValid() instead"))) valid() const;
    bool isValid() const;

    BleUuid UUID() const;
    EnumFlags<BleCharacteristicProperty> properties() const;
    String description() const;
    size_t description(char* buf, size_t len) const;

    // Get characteristic value
    ssize_t getValue(uint8_t* buf, size_t len) const;
    ssize_t getValue(String& str) const;

    template<typename T>
    typename std::enable_if_t<std::is_standard_layout<T>::value, ssize_t>
    getValue(T* val) const {
        return getValue(reinterpret_cast<uint8_t*>(val), sizeof(T));
    }

    // Set characteristic value
    ssize_t setValue(const uint8_t* buf, size_t len, BleTxRxType type = BleTxRxType::AUTO);
    ssize_t setValue(const String& str, BleTxRxType type = BleTxRxType::AUTO);
    ssize_t setValue(const char* str, BleTxRxType type = BleTxRxType::AUTO);

    template<typename T>
    typename std::enable_if_t<std::is_standard_layout<T>::value, ssize_t>
    setValue(const T& val, BleTxRxType type = BleTxRxType::AUTO) {
        return setValue(reinterpret_cast<const uint8_t*>(&val), sizeof(T), type);
    }

    // Valid for peer characteristic only. Manually enable the characteristic notification or indication.
    int subscribe(bool enable) const;

    operator bool() const {
        return isValid();
    }

    void onDataReceived(BleOnDataReceivedCallback callback, void* context = nullptr);
    void onDataReceived(const BleOnDataReceivedStdFunction& callback);

    template<typename T>
    void onDataReceived(void(T::*callback)(const uint8_t*, size_t, const BlePeerDevice& peer), T* instance) {
        onDataReceived((callback && instance) ? std::bind(callback, instance, _1, _2, _3) :(BleOnDataReceivedStdFunction) nullptr);
    }

    BleCharacteristicImpl* impl() const {
        return impl_.get();
    }

private:
    void construct(const char* desc, EnumFlags<BleCharacteristicProperty> properties,
            BleUuid& charUuid, BleUuid& svcUuid,
            BleOnDataReceivedCallback callback, void* context);

    void construct(const char* desc, EnumFlags<BleCharacteristicProperty> properties,
            BleUuid& charUuid, BleUuid& svcUuid, const BleOnDataReceivedStdFunction& callback);

    std::shared_ptr<BleCharacteristicImpl> impl_;
};


class BleService {
public:
    BleService();
    BleService(const BleUuid& uuid);
    BleService(const BleService&) = default;
    ~BleService() = default;

    BleUuid UUID() const;

    bool operator==(const BleService& service) const;
    bool operator!=(const BleService& service) const;

    BleService& operator=(const BleService&) = default;

    BleServiceImpl* impl() const {
        return impl_.get();
    }

private:
    std::shared_ptr<BleServiceImpl> impl_;
};


class BleScanResult {
public:
    BleScanResult()
            : rssi_(BLE_RSSI_INVALID) {
    }

    BleScanResult& address(const BleAddress& addr) {
        address_ = addr;
        return *this;
    }

    BleScanResult& advertisingData(const BleAdvertisingData& advData) {
        advertisingData_ = advData;
        return *this;
    }

    BleScanResult& advertisingData(const uint8_t* buf, size_t len) {
        advertisingData_.set(buf, len);
        return *this;
    }

    BleScanResult& scanResponse(const BleAdvertisingData& srData) {
        scanResponse_ = srData;
        return *this;
    }

    BleScanResult& scanResponse(const uint8_t* buf, size_t len) {
        scanResponse_.set(buf, len);
        return *this;
    }

    BleScanResult& rssi(int8_t value) {
        rssi_ = value;
        return *this;
    }

    const BleAddress& address() const {
        return address_;
    }

    const BleAdvertisingData& advertisingData() const {
        return advertisingData_;
    }

    const BleAdvertisingData& scanResponse() const {
        return scanResponse_;
    }

    int8_t rssi() const {
        return rssi_;
    }

private:
    BleAddress address_;
    BleAdvertisingData advertisingData_;
    BleAdvertisingData scanResponse_;
    int8_t rssi_;
};


/* AND-filter */
class BleScanFilter {
public:
    BleScanFilter()
            : minRssi_(BLE_RSSI_INVALID),
              maxRssi_(BLE_RSSI_INVALID),
              customData_(nullptr),
              customDataLen_(0) {
    }
    ~BleScanFilter() = default;

    // Device name
    template<typename T>
    BleScanFilter& deviceName(T name) {
        deviceNames_.append(String(name));
        return *this;
    }
    BleScanFilter& deviceNames(const Vector<String>& names) {
        deviceNames_.append(names);
        return *this;
    }
    const Vector<String>& deviceNames() const {
        return deviceNames_;
    }

    // Service UUID
    template<typename T>
    BleScanFilter& serviceUUID(T uuid) {
        serviceUuids_.append(BleUuid(uuid));
        return *this;
    }
    BleScanFilter& serviceUUIDs(const Vector<BleUuid>& uuids) {
        serviceUuids_.append(uuids);
        return *this;
    }
    const Vector<BleUuid>& serviceUUIDs() const {
        return serviceUuids_;
    }

    // Device address
    template<typename T>
    BleScanFilter& address(T addr) {
        addresses_.append(BleAddress(addr));
        return *this;
    }
    BleScanFilter& addresses(const Vector<BleAddress>& addrs) {
        addresses_.append(addrs);
        return *this;
    }
    const Vector<BleAddress>& addresses() const {
        return addresses_;
    }

    // Appearance
    BleScanFilter& appearance(const ble_sig_appearance_t& appearance) {
        appearances_.append(appearance);
        return *this;
    }
    BleScanFilter& appearances(const Vector<ble_sig_appearance_t>&  appearances) {
        appearances_.append(appearances);
        return *this;
    }
    const Vector<ble_sig_appearance_t>& appearances() const {
        return appearances_;
    }

    // RSSI
    BleScanFilter& minRssi(int8_t minRssi) {
        minRssi_ = minRssi;
        return *this;
    }
    BleScanFilter& maxRssi(int8_t maxRssi) {
        maxRssi_ = maxRssi;
        return *this;
    }
    int8_t minRssi() const {
        return minRssi_;
    }
    int maxRssi() const {
        return maxRssi_;
    }

    // Custom data
    BleScanFilter& customData(const uint8_t* const data, size_t len) {
        customData_ = data;
        customDataLen_ = len;
        return *this;
    }
    const uint8_t* customData(size_t* len) const {
        *len = customDataLen_;
        return customData_;
    }

    BleScanFilter& clear() {
        deviceNames_.clear();
        serviceUuids_.clear();
        addresses_.clear();
        appearances_.clear();
        minRssi_ = maxRssi_ = BLE_RSSI_INVALID;
        customData_ = nullptr;
        customDataLen_ = 0;
        return *this;
    }

private:
    Vector<String> deviceNames_;
    Vector<BleUuid> serviceUuids_;
    Vector<BleAddress> addresses_;
    Vector<ble_sig_appearance_t> appearances_;
    int8_t minRssi_;
    int8_t maxRssi_;
    const uint8_t* customData_;
    size_t customDataLen_;
};


class BlePeerDevice {
public:
    BlePeerDevice();
    BlePeerDevice(const BlePeerDevice&) = default;
    ~BlePeerDevice();

    // Discover all services on peer device.
    Vector<BleService> discoverAllServices();
    ssize_t discoverAllServices(BleService* services, size_t count);

    // Discover all characteristics on peer device.
    Vector<BleCharacteristic> discoverAllCharacteristics();
    ssize_t discoverAllCharacteristics(BleCharacteristic* characteristics, size_t count);

    // Discover all characteristics of a service
    Vector<BleCharacteristic> discoverCharacteristicsOfService(const BleService& service);
    ssize_t discoverCharacteristicsOfService(const BleService& service, BleCharacteristic* characteristics, size_t count);

    // Fetch the discovered services on peer device.
    Vector<BleService> services() const;
    size_t services(BleService* services, size_t count) const;
    bool getServiceByUUID(BleService& service, const BleUuid& uuid) const;
    // In case that there are several services with the same UUID.
    Vector<BleService> getServiceByUUID(const BleUuid& uuid) const;
    size_t getServiceByUUID(BleService* services, size_t count, const BleUuid& uuid) const;

    // Fetch the discovered characteristics on peer device.
    Vector<BleCharacteristic> characteristics() const;
    size_t characteristics(BleCharacteristic* characteristics, size_t count) const;
    bool getCharacteristicByDescription(BleCharacteristic& characteristic, const char* desc) const;
    bool getCharacteristicByDescription(BleCharacteristic& characteristic, const String& desc) const;
    bool getCharacteristicByUUID(BleCharacteristic& characteristic, const BleUuid& uuid) const;
    // In case that there are several characteristics with the same UUID.
    Vector<BleCharacteristic> getCharacteristicByUUID(const BleUuid& uuid) const;
    size_t getCharacteristicByUUID(BleCharacteristic* characteristics, size_t count, const BleUuid& uuid) const;

    // Fetch the discovered characteristics under a service.
    Vector<BleCharacteristic> characteristics(const BleService& service) const;
    size_t characteristics(const BleService& service, BleCharacteristic* characteristics, size_t count) const;
    bool getCharacteristicByDescription(const BleService& service, BleCharacteristic& characteristic, const char* desc) const;
    bool getCharacteristicByDescription(const BleService& service, BleCharacteristic& characteristic, const String& desc) const;
    bool getCharacteristicByUUID(const BleService& service, BleCharacteristic& characteristic, const BleUuid& uuid) const;

    int connect(const BleAddress& addr, const BleConnectionParams* params, bool automatic = true);
    int connect(const BleAddress& addr, const BleConnectionParams& params, bool automatic = true);
    int connect(const BleAddress& addr, uint16_t interval, uint16_t latency, uint16_t timeout, bool automatic = true);
    int connect(const BleAddress& addr, bool automatic = true);
    // These methods should be called after the peer device has bound with an address using the bind() method.
    int connect(const BleConnectionParams* params, bool automatic = true);
    int connect(const BleConnectionParams& params, bool automatic = true);
    int connect(uint16_t interval, uint16_t latency, uint16_t timeout, bool automatic = true);
    int connect(bool automatic = true);

    int disconnect() const;

    bool connected() const;

    void bind(const BleAddress& address) const;
    BleAddress address() const;

    bool isValid() const;

    bool operator==(const BlePeerDevice& device) const;
    bool operator!=(const BlePeerDevice& device) const;

    operator bool() const {
        return isValid();
    }

    BlePeerDevice& operator=(const BlePeerDevice& peer) = default;

    BlePeerDeviceImpl* impl() const {
        return impl_.get();
    }

private:
    std::shared_ptr<BlePeerDeviceImpl> impl_;
};


class BleLocalDevice {
public:
    int begin() const;
    int end() const;

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
    int on() const;
    int off() const;
    int setTxPower(int8_t txPower) const;
    int txPower(int8_t* txPower) const;
    int8_t txPower() const;
    int selectAntenna(BleAntennaType antenna) const;

    // Access provisioning mode
    int provisioningMode(bool enabled) const;
    bool getProvisioningStatus() const;

    template<typename T>
    int setProvisioningSvcUuid(T svcUuid) const {
        BleUuid tempSvcUUID(svcUuid);
        auto svcHalUuid = tempSvcUUID.halUUID();
        if (tempSvcUUID.isValid()) {
            return system_ble_prov_set_custom_svc_uuid(&svcHalUuid, nullptr);
        }
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    template<typename T>
    int setProvisioningTxUuid(T txUuid) const {
        BleUuid tempTxUUID(txUuid);
        auto txHalUuid = tempTxUUID.halUUID();
        if (tempTxUUID.isValid()) {
            return system_ble_prov_set_custom_tx_uuid(&txHalUuid, nullptr);
        }
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    template<typename T>
    int setProvisioningRxUuid(T rxUuid) const {
        BleUuid tempRxUUID(rxUuid);
        auto rxHalUuid = tempRxUUID.halUUID();
        if (tempRxUUID.isValid()) {
            return system_ble_prov_set_custom_rx_uuid(&rxHalUuid, nullptr);
        }
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    template<typename T>
    int setProvisioningVerUuid(T verUuid) const {
        BleUuid tempVerUUID(verUuid);
        auto verHalUuid = tempVerUUID.halUUID();
        if (tempVerUUID.isValid()) {
            return system_ble_prov_set_custom_ver_uuid(&verHalUuid, nullptr);
        }
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    int setProvisioningCompanyId(uint16_t companyId) const {
        return system_ble_prov_set_company_id(companyId, nullptr);
    }

    // Access advertising parameters
    int setAdvertisingInterval(uint16_t interval) const;
    int setAdvertisingTimeout(uint16_t timeout) const;
    int setAdvertisingType(BleAdvertisingEventType type) const;
    int setAdvertisingPhy(BlePhy phy) const; // Only one of the enum values can be specified as the argument.
    int setAdvertisingParameters(const BleAdvertisingParams* params) const;
    int setAdvertisingParameters(const BleAdvertisingParams& params) const;
    int setAdvertisingParameters(uint16_t interval, uint16_t timeout, BleAdvertisingEventType type) const;
    int getAdvertisingParameters(BleAdvertisingParams* params) const;
    int getAdvertisingParameters(BleAdvertisingParams& params) const;

    // Access advertising data and scan response data
    int setAdvertisingData(BleAdvertisingData* advertisingData) const;
    int setAdvertisingData(BleAdvertisingData& advertisingData) const;
    int setScanResponseData(BleAdvertisingData* scanResponse) const;
    int setScanResponseData(BleAdvertisingData& scanResponse) const;
    ssize_t getAdvertisingData(BleAdvertisingData* advertisingData) const;
    ssize_t getAdvertisingData(BleAdvertisingData& advertisingData) const;
    ssize_t getScanResponseData(BleAdvertisingData* scanResponse) const;
    ssize_t getScanResponseData(BleAdvertisingData& scanResponse) const;

    // Advertising control
    int advertise() const;
    int advertise(BleAdvertisingData* advertisingData, BleAdvertisingData* scanResponse = nullptr) const;
    int advertise(BleAdvertisingData& advertisingData) const;
    int advertise(BleAdvertisingData& advertisingData, BleAdvertisingData& scanResponse) const;
    int advertise(const iBeacon& beacon) const;
    int stopAdvertising() const;
    bool advertising() const;

    // Access scanning parameters
    int setScanTimeout(uint16_t timeout) const;
    int setScanPhy(EnumFlags<BlePhy> phy) const;
    int setScanParameters(const BleScanParams* params) const;
    int setScanParameters(const BleScanParams& params) const;
    int getScanParameters(BleScanParams* params) const;
    int getScanParameters(BleScanParams& params) const;

    // Scanning control
    int scan(BleOnScanResultCallback callback, void* context = nullptr) const;
    int scan(BleOnScanResultCallbackRef callback, void* context = nullptr) const;
    int scan(const BleOnScanResultStdFunction& callback) const;
    int scan(BleScanResult* results, size_t resultCount) const;
    Vector<BleScanResult> scan() const;

    template<typename T>
    int scan(void(T::*callback)(const BleScanResult&), T* instance) const {
        return scan((callback && instance) ? std::bind(callback, instance, _1) : (BleOnScanResultStdFunction)nullptr);
    }

    int scanWithFilter(const BleScanFilter& filter, BleOnScanResultCallback callback, void* context = nullptr) const;
    int scanWithFilter(const BleScanFilter& filter, BleOnScanResultCallbackRef callback, void* context = nullptr) const;
    int scanWithFilter(const BleScanFilter& filter, const BleOnScanResultStdFunction& callback) const;
    int scanWithFilter(const BleScanFilter& filter, BleScanResult* results, size_t resultCount) const;
    Vector<BleScanResult> scanWithFilter(const BleScanFilter& filter) const;

    template<typename T>
    int scanWithFilter(const BleScanFilter& filter, void(T::*callback)(const BleScanResult&), T* instance) const {
        return scanWithFilter(filter, (callback && instance) ? std::bind(callback, instance, _1) : (BleOnScanResultStdFunction)nullptr);
    }

    int stopScanning() const;

    // Access local characteristics
    BleCharacteristic addCharacteristic(const BleCharacteristic& characteristic);
    
    BleCharacteristic __attribute__((deprecated("The order of the first argument and second argument need to be swapped")))
    addCharacteristic(const char* desc, EnumFlags<BleCharacteristicProperty> properties, BleOnDataReceivedCallback callback = nullptr, void* context = nullptr) {
        return addCharacteristic(properties, desc, callback, context);
    }

    BleCharacteristic __attribute__((deprecated("The order of the first argument and second argument need to be swapped")))
    addCharacteristic(const String& desc, EnumFlags<BleCharacteristicProperty> properties, BleOnDataReceivedCallback callback = nullptr, void* context = nullptr) {
        return addCharacteristic(properties, desc, callback, context);
    }

    BleCharacteristic addCharacteristic(EnumFlags<BleCharacteristicProperty> properties, const char* desc, BleOnDataReceivedCallback callback = nullptr, void* context = nullptr);
    BleCharacteristic addCharacteristic(EnumFlags<BleCharacteristicProperty> properties, const String& desc, BleOnDataReceivedCallback callback = nullptr, void* context = nullptr);
    BleCharacteristic addCharacteristic(EnumFlags<BleCharacteristicProperty> properties, const char* desc, const BleOnDataReceivedStdFunction& callback);
    BleCharacteristic addCharacteristic(EnumFlags<BleCharacteristicProperty> properties, const String& desc, const BleOnDataReceivedStdFunction& callback);

    template<typename T>
    BleCharacteristic addCharacteristic(EnumFlags<BleCharacteristicProperty> properties, const char* desc, void(T::*callback)(const uint8_t*, size_t, const BlePeerDevice& peer), T* instance) {
        return addCharacteristic(properties, desc, (callback && instance) ? std::bind(callback, instance, _1, _2, _3) : (BleOnDataReceivedStdFunction)nullptr);
    }

    template<typename T>
    BleCharacteristic addCharacteristic(EnumFlags<BleCharacteristicProperty> properties, const String& desc, void(T::*callback)(const uint8_t*, size_t, const BlePeerDevice& peer), T* instance) {
        return addCharacteristic(properties, desc, (callback && instance) ? std::bind(callback, instance, _1, _2, _3) : (BleOnDataReceivedStdFunction)nullptr);
    }

    template<typename T1, typename T2>
    BleCharacteristic addCharacteristic(const char* desc, EnumFlags<BleCharacteristicProperty> properties, T1 charUuid, T2 svcUuid, BleOnDataReceivedCallback callback = nullptr, void* context = nullptr) {
        BleCharacteristic characteristic(desc, properties, charUuid, svcUuid, callback, context);
        addCharacteristic(characteristic);
        return characteristic;
    }

    template<typename T1, typename T2>
    BleCharacteristic addCharacteristic(const String& desc, EnumFlags<BleCharacteristicProperty> properties, T1 charUuid, T2 svcUuid, BleOnDataReceivedCallback callback = nullptr, void* context = nullptr) {
        return addCharacteristic(desc.c_str(), properties, charUuid, svcUuid, callback, context);
    }

    template<typename T1, typename T2>
    BleCharacteristic addCharacteristic(const char* desc, EnumFlags<BleCharacteristicProperty> properties, T1 charUuid, T2 svcUuid, const BleOnDataReceivedStdFunction& callback) {
        BleCharacteristic characteristic(desc, properties, charUuid, svcUuid, callback);
        addCharacteristic(characteristic);
        return characteristic;
    }

    template<typename T1, typename T2>
    BleCharacteristic addCharacteristic(const String& desc, EnumFlags<BleCharacteristicProperty> properties, T1 charUuid, T2 svcUuid, const BleOnDataReceivedStdFunction& callback) {
        return addCharacteristic(desc.c_str(), properties, charUuid, svcUuid, callback);
    }

    template<typename T1, typename T2, typename T3>
    BleCharacteristic addCharacteristic(const char* desc, EnumFlags<BleCharacteristicProperty> properties, T1 charUuid, T2 svcUuid, void(T3::*callback)(const uint8_t*, size_t, const BlePeerDevice& peer), T3* instance) {
        return addCharacteristic(desc, properties, charUuid, svcUuid, (callback && instance) ? std::bind(callback, instance, _1, _2, _3) : (BleOnDataReceivedStdFunction)nullptr);
    }

    template<typename T1, typename T2, typename T3>
    BleCharacteristic addCharacteristic(const String& desc, EnumFlags<BleCharacteristicProperty> properties, T1 charUuid, T2 svcUuid, void(T3::*callback)(const uint8_t*, size_t, const BlePeerDevice& peer), T3* instance) {
        return addCharacteristic(desc, properties, charUuid, svcUuid, (callback && instance) ? std::bind(callback, instance, _1, _2, _3) : (BleOnDataReceivedStdFunction)nullptr);
    }

    // Access connection parameters
    int setPPCP(uint16_t minInterval, uint16_t maxInterval, uint16_t latency, uint16_t timeout) const;
    int setDesiredAttMtu(size_t mtu) const;
    int updateAttMtu(const BlePeerDevice& peer) const;
    ssize_t getCurrentAttMtu(const BlePeerDevice& peer) const;
    void onAttMtuExchanged(BleOnAttMtutExchangedCallback callback, void* context = nullptr) const;
    void onAttMtuExchanged(const BleOnAttMtuExchangedStdFunction& callback) const;

    template<typename T>
    void onAttMtuExchanged(void(T::*callback)(const BlePeerDevice& peer, size_t attMtu), T* instance) const {
        return onAttMtuExchanged((callback && instance) ? std::bind(callback, instance, _1, _2) : (BleOnAttMtuExchangedStdFunction)nullptr);
    }

    // Connection control
    BlePeerDevice connect(const BleAddress& addr, const BleConnectionParams* params, bool automatic = true) const;
    BlePeerDevice connect(const BleAddress& addr, const BleConnectionParams& params, bool automatic = true) const;
    BlePeerDevice connect(const BleAddress& addr, uint16_t interval, uint16_t latency, uint16_t timeout, bool automatic = true) const;
    BlePeerDevice connect(const BleAddress& addr, bool automatic = true) const;

    int setPairingIoCaps(BlePairingIoCaps ioCaps) const;
    int setPairingAlgorithm(BlePairingAlgorithm algorithm) const;
    int startPairing(const BlePeerDevice& peer) const;
    int rejectPairing(const BlePeerDevice& peer) const;
    int setPairingNumericComparison(const BlePeerDevice& peer, bool equal) const;
    int setPairingPasskey(const BlePeerDevice& peer, const uint8_t* passkey) const;
    bool isPairing(const BlePeerDevice& peer) const;
    bool isPaired(const BlePeerDevice& peer) const;

    void onPairingEvent(BleOnPairingEventCallback callback, void* context = nullptr) const;
    void onPairingEvent(const BleOnPairingEventStdFunction& callback) const;

    template<typename T>
    void onPairingEvent(void(T::*callback)(const BlePairingEvent& event), T* instance) const {
        return onPairingEvent((callback && instance) ? std::bind(callback, instance, _1) : (BleOnPairingEventStdFunction)nullptr);
    }

    // This only disconnect the peer Central device, i.e. when the local device is acting as BLE Peripheral.
    int disconnect() const;
    int disconnect(const BlePeerDevice& peer) const;
    int disconnectAll() const;

    bool connected() const;
    
    void onConnected(BleOnConnectedCallback callback, void* context = nullptr) const;
    void onConnected(const BleOnConnectedStdFunction& callback) const;

    template<typename T>
    void onConnected(void(T::*callback)(const BlePeerDevice& peer), T* instance) const {
        return onConnected((callback && instance) ? std::bind(callback, instance, _1) : (BleOnConnectedStdFunction)nullptr);
    }

    void onDisconnected(BleOnDisconnectedCallback callback, void* context = nullptr) const;
    void onDisconnected(const BleOnDisconnectedStdFunction& callback) const;

    template<typename T>
    void onDisconnected(void(T::*callback)(const BlePeerDevice& peer), T* instance) const {
        return onDisconnected((callback && instance) ? std::bind(callback, instance, _1) : (BleOnDisconnectedStdFunction)nullptr);
    }

    BlePeerDevice peerCentral() const;

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
