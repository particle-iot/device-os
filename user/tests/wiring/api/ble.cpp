
#include "testapi.h"

#if Wiring_BLE == 1

using namespace std::placeholders;

class Handlers {
public:
    void dataHandler(const uint8_t*, size_t, const BlePeerDevice& peer) {
    }

    void scanHandler(const BleScanResult& result) {
    }

    void connectedHandler(const BlePeerDevice& peer) {
    }

    void disconnectedHandler(const BlePeerDevice& peer) {
    }

    void pairingEventHandler(const BlePairingEvent& event) {
    }

    void attMtuExchangedHandler(const BlePeerDevice& peer, size_t attMtu) {
    }
};

Handlers bleHandlerInstance;

void dataHandlerFunc(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
}

void scanHandlerFunc(const BleScanResult* result, void* context) {
}

void scanHandlerFuncRef(const BleScanResult& result, void* context) {
}

void connectedHandlerFunc(const BlePeerDevice& peer, void* context) {
}

void disconnectedHandlerFunc(const BlePeerDevice& peer, void* context) {
}

void pairingEventHandlerFunc(const BlePairingEvent& event, void* context) {
    if (event.type == BlePairingEventType::REQUEST_RECEIVED) {
        BlePeerDevice peer = event.peer;
    } else if (event.type == BlePairingEventType::STATUS_UPDATED) {
        int status = event.payload.status.status;
        bool bonded = event.payload.status.bonded;
        bool lesc = event.payload.status.lesc;
        (void)status;
        (void)bonded;
        (void)lesc;
    } else if (event.type == BlePairingEventType::PASSKEY_DISPLAY || event.type == BlePairingEventType::NUMERIC_COMPARISON) {
        uint8_t key = event.payload.passkey[0];
        (void)key;
    } else if (event.type == BlePairingEventType::PASSKEY_INPUT) {

    }
}

void attMtuExchangedHandlerFunc(const BlePeerDevice& peer, size_t attMtu, void* context) {
}

test(ble_characteristic_property) {
    API_COMPILE({
        EnumFlags<BleCharacteristicProperty> props = BleCharacteristicProperty::BROADCAST |
                                                     BleCharacteristicProperty::READ |
                                                     BleCharacteristicProperty::WRITE_WO_RSP |
                                                     BleCharacteristicProperty::WRITE |
                                                     BleCharacteristicProperty::NOTIFY |
                                                     BleCharacteristicProperty::INDICATE |
                                                     BleCharacteristicProperty::AUTH_SIGN_WRITES |
                                                     BleCharacteristicProperty::EXTENDED_PROP;
        (void)props;
    });
}

test(ble_advertising_data_type) {
    BleAdvertisingDataType type;
    API_COMPILE({ type = BleAdvertisingDataType::FLAGS; });
    API_COMPILE({ type = BleAdvertisingDataType::SERVICE_UUID_16BIT_MORE_AVAILABLE; });
    API_COMPILE({ type = BleAdvertisingDataType::SERVICE_UUID_16BIT_COMPLETE; });
    API_COMPILE({ type = BleAdvertisingDataType::SERVICE_UUID_32BIT_MORE_AVAILABLE; });
    API_COMPILE({ type = BleAdvertisingDataType::SERVICE_UUID_32BIT_COMPLETE; });
    API_COMPILE({ type = BleAdvertisingDataType::SERVICE_UUID_128BIT_MORE_AVAILABLE; });
    API_COMPILE({ type = BleAdvertisingDataType::SERVICE_UUID_128BIT_COMPLETE; });
    API_COMPILE({ type = BleAdvertisingDataType::SHORT_LOCAL_NAME; });
    API_COMPILE({ type = BleAdvertisingDataType::COMPLETE_LOCAL_NAME; });
    API_COMPILE({ type = BleAdvertisingDataType::TX_POWER_LEVEL; });
    API_COMPILE({ type = BleAdvertisingDataType::CLASS_OF_DEVICE; });
    API_COMPILE({ type = BleAdvertisingDataType::SIMPLE_PAIRING_HASH_C; });
    API_COMPILE({ type = BleAdvertisingDataType::SIMPLE_PAIRING_RANDOMIZER_R; });
    API_COMPILE({ type = BleAdvertisingDataType::SECURITY_MANAGER_TK_VALUE; });
    API_COMPILE({ type = BleAdvertisingDataType::SECURITY_MANAGER_OOB_FLAGS; });
    API_COMPILE({ type = BleAdvertisingDataType::SLAVE_CONNECTION_INTERVAL_RANGE; });
    API_COMPILE({ type = BleAdvertisingDataType::SOLICITED_SERVICE_UUIDS_16BIT; });
    API_COMPILE({ type = BleAdvertisingDataType::SOLICITED_SERVICE_UUIDS_128BIT; });
    API_COMPILE({ type = BleAdvertisingDataType::SERVICE_DATA; });
    API_COMPILE({ type = BleAdvertisingDataType::PUBLIC_TARGET_ADDRESS; });
    API_COMPILE({ type = BleAdvertisingDataType::RANDOM_TARGET_ADDRESS; });
    API_COMPILE({ type = BleAdvertisingDataType::APPEARANCE; });
    API_COMPILE({ type = BleAdvertisingDataType::ADVERTISING_INTERVAL; });
    API_COMPILE({ type = BleAdvertisingDataType::LE_BLUETOOTH_DEVICE_ADDRESS; });
    API_COMPILE({ type = BleAdvertisingDataType::LE_ROLE; });
    API_COMPILE({ type = BleAdvertisingDataType::SIMPLE_PAIRING_HASH_C256; });
    API_COMPILE({ type = BleAdvertisingDataType::SIMPLE_PAIRING_RANDOMIZER_R256; });
    API_COMPILE({ type = BleAdvertisingDataType::SERVICE_SOLICITATION_32BIT_UUID; });
    API_COMPILE({ type = BleAdvertisingDataType::SERVICE_DATA_32BIT_UUID; });
    API_COMPILE({ type = BleAdvertisingDataType::SERVICE_DATA_128BIT_UUID; });
    API_COMPILE({ type = BleAdvertisingDataType::LESC_CONFIRMATION_VALUE; });
    API_COMPILE({ type = BleAdvertisingDataType::LESC_RANDOM_VALUE; });
    API_COMPILE({ type = BleAdvertisingDataType::URI; });
    API_COMPILE({ type = BleAdvertisingDataType::INDOOR_POSITIONING; });
    API_COMPILE({ type = BleAdvertisingDataType::TRANSPORT_DISCOVERY_DATA; });
    API_COMPILE({ type = BleAdvertisingDataType::LE_SUPPORTED_FEATURES; });
    API_COMPILE({ type = BleAdvertisingDataType::CHANNEL_MAP_UPDATE_INDICATION; });
    API_COMPILE({ type = BleAdvertisingDataType::PB_ADV; });
    API_COMPILE({ type = BleAdvertisingDataType::MESH_MESSAGE; });
    API_COMPILE({ type = BleAdvertisingDataType::MESH_BEACON; });
    API_COMPILE({ type = BleAdvertisingDataType::THREE_D_INFORMATION_DATA; });
    API_COMPILE({ type = BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA; });
    (void)type;
}

test(ble_advertising_event_type) {
    BleAdvertisingEventType type;
    API_COMPILE({ type = BleAdvertisingEventType::CONNECTABLE_SCANNABLE_UNDIRECRED; });
    API_COMPILE({ type = BleAdvertisingEventType::CONNECTABLE_UNDIRECTED; });
    API_COMPILE({ type = BleAdvertisingEventType::CONNECTABLE_DIRECTED; });
    API_COMPILE({ type = BleAdvertisingEventType::NON_CONNECTABLE_NON_SCANABLE_UNDIRECTED; });
    API_COMPILE({ type = BleAdvertisingEventType::NON_CONNECTABLE_NON_SCANABLE_DIRECTED; });
    API_COMPILE({ type = BleAdvertisingEventType::SCANABLE_UNDIRECTED; });
    API_COMPILE({ type = BleAdvertisingEventType::SCANABLE_DIRECTED; });
    (void)type;
}

test(ble_antenna_type) {
    BleAntennaType type;
    API_COMPILE({ type = BleAntennaType::DEFAULT; });
    API_COMPILE({ type = BleAntennaType::INTERNAL; });
    API_COMPILE({ type = BleAntennaType::EXTERNAL; });
    (void)type;
}

test(ble_address_type) {
    BleAddressType type;
    API_COMPILE({ type = BleAddressType::PUBLIC; });
    API_COMPILE({ type = BleAddressType::RANDOM_STATIC; });
    API_COMPILE({ type = BleAddressType::RANDOM_PRIVATE_RESOLVABLE; });
    API_COMPILE({ type = BleAddressType::RANDOM_PRIVATE_NON_RESOLVABLE; });
    (void)type;
}

test(ble_tx_rx_type) {
    BleTxRxType type;
    API_COMPILE({ type = BleTxRxType::AUTO; });
    API_COMPILE({ type = BleTxRxType::ACK; });
    API_COMPILE({ type = BleTxRxType::NACK; });
    (void)type;
}

test(ble_uuid_type) {
    BleUuidType type;
    API_COMPILE({ type = BleUuidType::SHORT; });
    API_COMPILE({ type = BleUuidType::LONG; });
    (void)type;
}

test(ble_uuid_order) {
    BleUuidOrder order;
    API_COMPILE({ order = BleUuidOrder::LSB; });
    API_COMPILE({ order = BleUuidOrder::MSB; });
    (void)order;
}

test(ble_pairing_algorithm) {
    BlePairingAlgorithm algorithm;
    API_COMPILE({ algorithm = BlePairingAlgorithm::LEGACY_ONLY; });
    API_COMPILE({ algorithm = BlePairingAlgorithm::LESC_ONLY; });
    API_COMPILE({ algorithm = BlePairingAlgorithm::AUTO; });
    (void)algorithm;
}

test(ble_pairing_io_caps) {
    BlePairingIoCaps ioCaps;
    API_COMPILE({ ioCaps = BlePairingIoCaps::NONE; });
    API_COMPILE({ ioCaps = BlePairingIoCaps::DISPLAY_ONLY; });
    API_COMPILE({ ioCaps = BlePairingIoCaps::DISPLAY_YESNO; });
    API_COMPILE({ ioCaps = BlePairingIoCaps::KEYBOARD_ONLY; });
    API_COMPILE({ ioCaps = BlePairingIoCaps::KEYBOARD_DISPLAY; });
    (void)ioCaps;
}

test(ble_pairing_event_type) {
    BlePairingEventType type;
    API_COMPILE({ type = BlePairingEventType::REQUEST_RECEIVED; });
    API_COMPILE({ type = BlePairingEventType::PASSKEY_DISPLAY; });
    API_COMPILE({ type = BlePairingEventType::PASSKEY_INPUT; });
    API_COMPILE({ type = BlePairingEventType::STATUS_UPDATED; });
    API_COMPILE({ type = BlePairingEventType::NUMERIC_COMPARISON; });
    (void)type;
}

test(ble_phys_type) {
    BlePhy phy;
    API_COMPILE({ phy = BlePhy::BLE_PHYS_AUTO; });
    API_COMPILE({ phy = BlePhy::BLE_PHYS_1MBPS; });
    API_COMPILE({ phy = BlePhy::BLE_PHYS_CODED; });
    (void)phy;
}

test(ble_address_class) {
    hal_ble_addr_t halAddr;
    uint8_t addrArray[BLE_SIG_ADDR_LEN];
    BleAddress addr;

    API_COMPILE(BleAddress a(addr));
    API_COMPILE(BleAddress a(halAddr));
    API_COMPILE(BleAddress a(addrArray));
    API_COMPILE(BleAddress a(addrArray, BleAddressType::PUBLIC));
    API_COMPILE(BleAddress a("123"));
    API_COMPILE(BleAddress a("123", BleAddressType::PUBLIC));
    API_COMPILE(BleAddress a(String("123")));
    API_COMPILE(BleAddress a(String("123"), BleAddressType::PUBLIC));

    API_COMPILE({ int ret = addr.type(BleAddressType::PUBLIC); (void)ret; });
    API_COMPILE({ BleAddressType type = addr.type(); (void)type; });

    API_COMPILE({ int ret = addr.set(halAddr); (void)ret; });
    API_COMPILE({ int ret = addr.set(addrArray); (void)ret; });
    API_COMPILE({ int ret = addr.set(addrArray, BleAddressType::PUBLIC); (void)ret; });
    API_COMPILE({ int ret = addr.set("123"); (void)ret; });
    API_COMPILE({ int ret = addr.set("123", BleAddressType::PUBLIC); (void)ret; });
    API_COMPILE({ int ret = addr.set(String("123")); (void)ret; });
    API_COMPILE({ int ret = addr.set(String("123"), BleAddressType::PUBLIC); (void)ret; });

    API_COMPILE({ BleAddress a = addr; });
    API_COMPILE({ BleAddress a = halAddr; });
    API_COMPILE({ BleAddress a = addrArray; });
    API_COMPILE({ BleAddress a = "123"; });
    API_COMPILE({ BleAddress a = String("123"); });

    API_COMPILE({ uint8_t array[1]; addr.octets(array); });

    API_COMPILE({ String str = addr.toString(); });
    API_COMPILE({ String str = addr.toString(true); });
    API_COMPILE({ char getStr[1]; size_t size = addr.toString(getStr, 0); (void)size; });
    API_COMPILE({ char getStr[1]; size_t size = addr.toString(getStr, 0, true); (void)size; });

    API_COMPILE({ hal_ble_addr_t a = addr.halAddress(); (void)a; });

    API_COMPILE({ uint8_t byte = addr[0]; (void)byte; });

    API_COMPILE({ bool ret = addr == BleAddress(); (void)ret; });
    API_COMPILE({ bool ret = addr == halAddr; (void)ret; });
    API_COMPILE({ bool ret = addr == "123"; (void)ret; });
    API_COMPILE({ bool ret = addr == String("123"); (void)ret; });
    API_COMPILE({ bool ret = addr == addrArray; (void)ret; });

    API_COMPILE({ bool ret = addr != BleAddress(); (void)ret; });
    API_COMPILE({ bool ret = addr != halAddr; (void)ret; });
    API_COMPILE({ bool ret = addr != "123"; (void)ret; });
    API_COMPILE({ bool ret = addr != String("123"); (void)ret; });
    API_COMPILE({ bool ret = addr != addrArray; (void)ret; });

    API_COMPILE({ bool ret = addr; (void)ret; });

    API_COMPILE({ bool ret = addr.isValid(); (void)ret; });
    API_COMPILE({ int ret = addr.clear(); (void)ret; });
}

test(ble_uuid_class) {
    hal_ble_uuid_t halUuid;
    uint8_t uuidArray[1];
    BleUuid uuid;

    API_COMPILE(BleUuid u(uuid));
    API_COMPILE(BleUuid u(halUuid));
    API_COMPILE(BleUuid u(uuidArray));
    API_COMPILE(BleUuid u(uuidArray, BleUuidOrder::MSB));
    API_COMPILE(BleUuid u(uuidArray, 0x1234));
    API_COMPILE(BleUuid u(uuidArray, 0x1234, BleUuidOrder::MSB));
    API_COMPILE(BleUuid u(0x1234));
    API_COMPILE(BleUuid u(String("1234")));
    API_COMPILE(BleUuid u("1234"));

    API_COMPILE({ bool ret = uuid.isValid(); (void)ret; });

    API_COMPILE({ BleUuidType type = uuid.type(); (void)type; });

    API_COMPILE({ hal_ble_uuid_t u = uuid.halUUID(); (void)u; });

    API_COMPILE({ uint16_t u = uuid.shorted(); (void)u; });

    API_COMPILE({ uint8_t u[1]; size_t ret = uuid.rawBytes(u); (void)ret; });
    API_COMPILE({ const uint8_t* ptr = uuid.rawBytes(); (void)ptr; });

    API_COMPILE({ String str = uuid.toString(); });
    API_COMPILE({ String str = uuid.toString(true); });
    API_COMPILE({ char buf[1]; size_t size = uuid.toString(buf, 0); (void)size; });
    API_COMPILE({ char buf[1]; size_t size = uuid.toString(buf, 0, true); (void)size; });

    API_COMPILE({ BleUuid u = uuid; });
    API_COMPILE({ BleUuid u = halUuid; });
    API_COMPILE({ BleUuid u = uuidArray; });
    API_COMPILE({ BleUuid u = 0x1234; });
    API_COMPILE({ BleUuid u = "1234"; });
    API_COMPILE({ BleUuid u_int16_t = String("1234"); });

    API_COMPILE({ bool ret = uuid == BleUuid(); (void)ret; });
    API_COMPILE({ bool ret = uuid == halUuid; (void)ret; });
    API_COMPILE({ bool ret = uuid == "1234"; (void)ret; });
    API_COMPILE({ bool ret = uuid == String("1234"); (void)ret; });
    API_COMPILE({ bool ret = uuid == 0x1234; (void)ret; });
    API_COMPILE({ bool ret = uuid == uuidArray; (void)ret; });

    API_COMPILE({ bool ret = uuid != BleUuid(); (void)ret; });
    API_COMPILE({ bool ret = uuid != halUuid; (void)ret; });
    API_COMPILE({ bool ret = uuid != "1234"; (void)ret; });
    API_COMPILE({ bool ret = uuid != String("1234"); (void)ret; });
    API_COMPILE({ bool ret = uuid != 0x1234; (void)ret; });
    API_COMPILE({ bool ret = uuid != uuidArray; (void)ret; });

    API_COMPILE({ bool ret = uuid; (void)ret; });

    API_COMPILE({ uint8_t ret = uuid[0]; (void)ret; });
}

test(ble_ibeacon_class) {
    BleUuid uuid;
    iBeacon beacon;

    API_COMPILE(iBeacon beacon(0, 0, uuid, 0));

    API_COMPILE({ iBeacon b = beacon.major(0); });
    API_COMPILE({ iBeacon b = beacon.minor(0); });
    API_COMPILE({ iBeacon b = beacon.UUID(uuid); });
    API_COMPILE({ iBeacon b = beacon.measurePower(-1); });

    API_COMPILE({ uint16_t val = beacon.major(); (void)val; });
    API_COMPILE({ uint16_t val = beacon.minor(); (void)val; });
    API_COMPILE({ BleUuid u = beacon.UUID(); (void)u; });
    API_COMPILE({ const BleUuid& u = beacon.UUID(); (void)u; });
    API_COMPILE({ int8_t val = beacon.measurePower(); (void)val; });

    API_COMPILE({ uint16_t id = iBeacon::APPLE_COMPANY_ID; (void)id; });
    API_COMPILE({ uint16_t type = iBeacon::BEACON_TYPE_IBEACON; (void)type; });
}

test(ble_advertising_data_class) {
    BleUuid uuid;
    BleAdvertisingData data;
    iBeacon beacon;

    API_COMPILE(BleAdvertisingData data(beacon));

    API_COMPILE({ uint8_t buf[1]; size_t ret = data.set(buf, 0); (void)ret; });
    API_COMPILE({ size_t ret = data.set(beacon); (void)ret; });

    API_COMPILE({ uint8_t buf[1]; size_t ret = data.append(BleAdvertisingDataType::FLAGS, buf, 0); (void)ret; });

    API_COMPILE({ uint8_t buf[1]; size_t ret = data.appendCustomData(buf, 0); (void)ret; });
    API_COMPILE({ uint8_t buf[1]; size_t ret = data.appendCustomData(buf, 0, true); (void)ret; });

    API_COMPILE({ size_t ret = data.appendLocalName("123"); (void)ret; });
    API_COMPILE({ size_t ret = data.appendLocalName(String("123")); (void)ret; });

    API_COMPILE({ size_t ret = data.appendServiceUUID(uuid); (void)ret; });
    API_COMPILE({ size_t ret = data.appendServiceUUID(uuid, true); (void)ret; });

    API_COMPILE({ size_t ret = data.resize(0); (void)ret; });

    API_COMPILE(data.clear());
    API_COMPILE(data.remove(BleAdvertisingDataType::FLAGS));

    API_COMPILE({ uint8_t buf[1]; size_t ret = data.get(buf, 0); (void)ret; });
    API_COMPILE({ uint8_t buf[1]; size_t ret = data.get(BleAdvertisingDataType::FLAGS, buf, 0); (void)ret; });

    API_COMPILE({ uint8_t* ptr = data.data(); (void)ptr; });

    API_COMPILE({ size_t ret = data.length(); (void)ret; });

    API_COMPILE({ String str = data.deviceName(); });
    API_COMPILE({ char buf[1]; size_t ret = data.deviceName(buf, 0); (void)ret; });

    API_COMPILE({ BleUuid u[1]; size_t ret = data.serviceUUID(u, 0); (void)ret; });
    API_COMPILE({ Vector<BleUuid> u = data.serviceUUID(); });
    API_COMPILE({ const Vector<BleUuid>& u = data.serviceUUID(); });

    API_COMPILE({ uint8_t buf[1]; size_t ret = data.customData(buf, 0); (void)ret; });

    API_COMPILE({ uint8_t buf[1]; size_t ret = data(buf, 0); (void)ret; });

    API_COMPILE({ bool ret = data.contains(BleAdvertisingDataType::FLAGS); (void)ret; });

    API_COMPILE({ uint8_t ret = data[0]; (void)ret; });
}

test(ble_characteristic_class) {
    BleUuid charUuid;
    BleUuid svcUuid;
    BleCharacteristic characteristic;
    EnumFlags<BleCharacteristicProperty> props = BleCharacteristicProperty::BROADCAST;

    API_COMPILE(BleCharacteristic c(characteristic));
    API_COMPILE(BleCharacteristic c(props, "1234"));
    API_COMPILE(BleCharacteristic c(props, "1234", dataHandlerFunc));
    API_COMPILE(BleCharacteristic c(props, "1234", dataHandlerFunc, nullptr));
    API_COMPILE(BleCharacteristic c(props, String("1234")));
    API_COMPILE(BleCharacteristic c(props, String("1234"), dataHandlerFunc));
    API_COMPILE(BleCharacteristic c(props, String("1234"), dataHandlerFunc, nullptr));
    API_COMPILE(BleCharacteristic c(props, "1234", [](const uint8_t*, size_t, const BlePeerDevice& peer) {}));
    API_COMPILE(BleCharacteristic c(props, String("1234"), [](const uint8_t*, size_t, const BlePeerDevice& peer) {}));
    API_COMPILE(BleCharacteristic c(props, "1234", &Handlers::dataHandler, &bleHandlerInstance));
    API_COMPILE(BleCharacteristic c(props, String("1234"), &Handlers::dataHandler, &bleHandlerInstance));
    API_COMPILE(BleCharacteristic c(props, "1234", std::bind(&Handlers::dataHandler, &bleHandlerInstance, _1, _2, _3)));
    API_COMPILE(BleCharacteristic c(props, String("1234"), std::bind(&Handlers::dataHandler, &bleHandlerInstance, _1, _2, _3)));

    API_COMPILE(BleCharacteristic c("1234", props, charUuid, svcUuid));
    API_COMPILE(BleCharacteristic c(String("1234"), props, charUuid, svcUuid));
    API_COMPILE(BleCharacteristic c("1234", props, charUuid, svcUuid, dataHandlerFunc));
    API_COMPILE(BleCharacteristic c(String("1234"), props, charUuid, svcUuid, dataHandlerFunc));
    API_COMPILE(BleCharacteristic c("1234", props, charUuid, svcUuid, dataHandlerFunc, nullptr));
    API_COMPILE(BleCharacteristic c(String("1234"), props, charUuid, svcUuid, dataHandlerFunc, nullptr));
    API_COMPILE(BleCharacteristic c("1234", props, charUuid, svcUuid, [](const uint8_t*, size_t, const BlePeerDevice& peer) {}));
    API_COMPILE(BleCharacteristic c(String("1234"), props, charUuid, svcUuid, [](const uint8_t*, size_t, const BlePeerDevice& peer) {}));
    API_COMPILE(BleCharacteristic c("1234", props, charUuid, svcUuid, &Handlers::dataHandler, &bleHandlerInstance));
    API_COMPILE(BleCharacteristic c(String("1234"), props, charUuid, svcUuid, &Handlers::dataHandler, &bleHandlerInstance));
    API_COMPILE(BleCharacteristic c("1234", props, charUuid, svcUuid, std::bind(&Handlers::dataHandler, &bleHandlerInstance, _1, _2, _3)));
    API_COMPILE(BleCharacteristic c(String("1234"), props, charUuid, svcUuid, std::bind(&Handlers::dataHandler, &bleHandlerInstance, _1, _2, _3)));

    API_COMPILE({ BleCharacteristic c = characteristic; });

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    // These APIs are known deprecated APIs, we don't need to see this warning in tests
    API_COMPILE({ bool ret = characteristic.valid(); (void)ret; });
    API_COMPILE({ bool ret = characteristic.isValid(); (void)ret; });
#pragma GCC diagnostic pop

    API_COMPILE({ BleUuid uuid = characteristic.UUID(); });

    API_COMPILE({ EnumFlags<BleCharacteristicProperty> flag = characteristic.properties(); (void)flag; });

    API_COMPILE({ String str = characteristic.description(); });
    API_COMPILE({ char buf[1]; size_t ret = characteristic.description(buf, 0); (void)ret; });

    API_COMPILE({ uint8_t buf[1]; size_t ret = characteristic.getValue(buf, 0); (void)ret; });
    API_COMPILE({ String str; size_t ret = characteristic.getValue(str); (void)ret; });
    API_COMPILE({ uint8_t val; size_t ret = characteristic.getValue(&val); (void)ret; });
    API_COMPILE({ uint16_t val; size_t ret = characteristic.getValue(&val); (void)ret; });
    API_COMPILE({ uint32_t val; size_t ret = characteristic.getValue(&val); (void)ret; });
    API_COMPILE({ uint64_t val; size_t ret = characteristic.getValue(&val); (void)ret; });
    API_COMPILE({ int8_t val; size_t ret = characteristic.getValue(&val); (void)ret; });
    API_COMPILE({ int16_t val; size_t ret = characteristic.getValue(&val); (void)ret; });
    API_COMPILE({ int32_t val; size_t ret = characteristic.getValue(&val); (void)ret; });
    API_COMPILE({ int64_t val; size_t ret = characteristic.getValue(&val); (void)ret; });
    API_COMPILE({ float val; size_t ret = characteristic.getValue(&val); (void)ret; });
    API_COMPILE({ double val; size_t ret = characteristic.getValue(&val); (void)ret; });
    API_COMPILE({ char val; size_t ret = characteristic.getValue(&val); (void)ret; });
    API_COMPILE({
        struct stream {
            uint16_t val1;
            uint16_t val2;
        } val;
        size_t ret = characteristic.getValue(&val);
        (void)ret;
    });

    API_COMPILE({ uint8_t buf[1]; size_t ret = characteristic.setValue(buf, 0); (void)ret; });
    API_COMPILE({ uint8_t buf[1]; size_t ret = characteristic.setValue(buf, 0, BleTxRxType::AUTO); (void)ret; });
    API_COMPILE({ uint8_t buf[1]; size_t ret = characteristic.setValue(buf, 0, BleTxRxType::ACK); (void)ret; });
    API_COMPILE({ uint8_t buf[1]; size_t ret = characteristic.setValue(buf, 0, BleTxRxType::NACK); (void)ret; });
    API_COMPILE({ size_t ret = characteristic.setValue(String("1234")); (void)ret; });
    API_COMPILE({ size_t ret = characteristic.setValue(String("1234"), BleTxRxType::AUTO); (void)ret; });
    API_COMPILE({ size_t ret = characteristic.setValue("1234"); (void)ret; });
    API_COMPILE({ size_t ret = characteristic.setValue("1234", BleTxRxType::AUTO); (void)ret; });
    API_COMPILE({ uint8_t val; size_t ret = characteristic.setValue(val); (void)ret; });
    API_COMPILE({ uint8_t val; size_t ret = characteristic.setValue(val, BleTxRxType::AUTO); (void)ret; });
    API_COMPILE({ uint16_t val; size_t ret = characteristic.setValue(val); (void)ret; });
    API_COMPILE({ uint16_t val; size_t ret = characteristic.setValue(val, BleTxRxType::AUTO); (void)ret; });
    API_COMPILE({ uint32_t val; size_t ret = characteristic.setValue(val); (void)ret; });
    API_COMPILE({ uint32_t val; size_t ret = characteristic.setValue(val, BleTxRxType::AUTO); (void)ret; });
    API_COMPILE({ uint64_t val; size_t ret = characteristic.setValue(val); (void)ret; });
    API_COMPILE({ uint64_t val; size_t ret = characteristic.setValue(val, BleTxRxType::AUTO); (void)ret; });
    API_COMPILE({ int8_t val; size_t ret = characteristic.setValue(val); (void)ret; });
    API_COMPILE({ int8_t val; size_t ret = characteristic.setValue(val, BleTxRxType::AUTO); (void)ret; });
    API_COMPILE({ int16_t val; size_t ret = characteristic.setValue(val); (void)ret; });
    API_COMPILE({ int16_t val; size_t ret = characteristic.setValue(val, BleTxRxType::AUTO); (void)ret; });
    API_COMPILE({ int32_t val; size_t ret = characteristic.setValue(val); (void)ret; });
    API_COMPILE({ int32_t val; size_t ret = characteristic.setValue(val, BleTxRxType::AUTO); (void)ret; });
    API_COMPILE({ int64_t val; size_t ret = characteristic.setValue(val); (void)ret; });
    API_COMPILE({ int64_t val; size_t ret = characteristic.setValue(val, BleTxRxType::AUTO); (void)ret; });
    API_COMPILE({ float val; size_t ret = characteristic.setValue(val); (void)ret; });
    API_COMPILE({ float val; size_t ret = characteristic.setValue(val, BleTxRxType::AUTO); (void)ret; });
    API_COMPILE({ double val; size_t ret = characteristic.setValue(val); (void)ret; });
    API_COMPILE({ double val; size_t ret = characteristic.setValue(val, BleTxRxType::AUTO); (void)ret; });
    API_COMPILE({ char val; size_t ret = characteristic.setValue(val); (void)ret; });
    API_COMPILE({ char val; size_t ret = characteristic.setValue(val, BleTxRxType::AUTO); (void)ret; });
    API_COMPILE({
        struct stream {
            uint16_t val1;
            uint16_t val2;
        } val;
        size_t ret = characteristic.setValue(val);
        (void)ret;
    });
    API_COMPILE({
        struct stream {
            uint16_t val1;
            uint16_t val2;
        } val;
        size_t ret = characteristic.setValue(val, BleTxRxType::AUTO);
        (void)ret;
    });

    API_COMPILE({ int ret = characteristic.subscribe(true); (void)ret; });

    API_COMPILE({ characteristic.onDataReceived(dataHandlerFunc); });
    API_COMPILE({ characteristic.onDataReceived(dataHandlerFunc, nullptr); });
    API_COMPILE({ characteristic.onDataReceived([](const uint8_t*, size_t, const BlePeerDevice& peer) {}); });
    API_COMPILE({ characteristic.onDataReceived(&Handlers::dataHandler, &bleHandlerInstance); });
    API_COMPILE({ characteristic.onDataReceived(std::bind(&Handlers::dataHandler, &bleHandlerInstance, _1, _2, _3)); });

    API_COMPILE({ bool ret = characteristic; (void)ret; });
}

test(ble_service_class) {
    BleUuid uuid;
    BleService service;

    API_COMPILE(BleService service(uuid));

    API_COMPILE({ BleUuid u = service.UUID(); });

    API_COMPILE({ BleService s = service; });

    API_COMPILE({ bool ret = service == BleService(uuid); (void)ret; });
    API_COMPILE({ bool ret = service != BleService(uuid); (void)ret; });
}

test(ble_scan_result) {
    BleScanResult result;

    API_COMPILE({ BleScanResult r = result.address(BleAddress()); (void)r; });
    API_COMPILE({ BleScanResult r = result.advertisingData(BleAdvertisingData()); (void)r; });
    API_COMPILE({ uint8_t buf[1]; BleScanResult r = result.advertisingData(buf, 0); (void)r; });
    API_COMPILE({ BleScanResult r = result.scanResponse(BleAdvertisingData()); (void)r; });
    API_COMPILE({ uint8_t buf[1]; BleScanResult r = result.scanResponse(buf, 0); (void)r; });
    API_COMPILE({ BleScanResult r = result.rssi(-8); (void)r; });

    API_COMPILE({ BleAddress addr = result.address(); (void)addr; });
    API_COMPILE({ const BleAddress& addr = result.address(); (void)addr; });
    API_COMPILE({ BleAdvertisingData adv = result.advertisingData(); (void)adv; });
    API_COMPILE({ const BleAdvertisingData& adv = result.advertisingData(); (void)adv; });
    API_COMPILE({ BleAdvertisingData sr = result.scanResponse(); (void)sr; });
    API_COMPILE({ const BleAdvertisingData& sr = result.scanResponse(); (void)sr; });
    API_COMPILE({ int8_t ret = result.rssi(); (void)ret; });
}

test(ble_scan_filter) {
    BleScanFilter filter;

    API_COMPILE({ BleScanFilter f = filter.clear(); (void)f; });

    API_COMPILE({ BleScanFilter f = filter.deviceName("1234"); (void)f; });
    API_COMPILE({ BleScanFilter f = filter.deviceName(String("1234")); (void)f; });
    API_COMPILE({ BleScanFilter f = filter.deviceNames(Vector<String>()); (void)f; });
    API_COMPILE({ Vector<String> names = filter.deviceNames(); (void)names; });
    API_COMPILE({ const Vector<String>& names = filter.deviceNames(); (void)names; });

    API_COMPILE({ BleScanFilter f = filter.serviceUUID(BleUuid()); (void)f; });
    API_COMPILE({ BleScanFilter f = filter.serviceUUIDs(Vector<BleUuid>()); (void)f; });
    API_COMPILE({ Vector<BleUuid> uuids = filter.serviceUUIDs(); (void)uuids; });
    API_COMPILE({ const Vector<BleUuid>& uuids = filter.serviceUUIDs(); (void)uuids; });

    API_COMPILE({ BleScanFilter f = filter.address("1234"); (void)f; });
    API_COMPILE({ BleScanFilter f = filter.addresses(Vector<BleAddress>()); (void)f; });
    API_COMPILE({ Vector<BleAddress> addrs = filter.addresses(); (void)addrs; });
    API_COMPILE({ const Vector<BleAddress>& addrs = filter.addresses(); (void)addrs; });

    API_COMPILE({ BleScanFilter f = filter.appearance(BLE_SIG_APPEARANCE_GENERIC_PHONE); (void)f; });
    API_COMPILE({ BleScanFilter f = filter.appearances(Vector<ble_sig_appearance_t>()); (void)f; });
    API_COMPILE({ Vector<ble_sig_appearance_t> apprs = filter.appearances(); (void)apprs; });
    API_COMPILE({ const Vector<ble_sig_appearance_t>& apprs = filter.appearances(); (void)apprs; });

    API_COMPILE({ BleScanFilter f = filter.minRssi(0); (void)f; });
    API_COMPILE({ BleScanFilter f = filter.maxRssi(0); (void)f; });
    API_COMPILE({ int8_t ret = filter.minRssi(); (void)ret; });
    API_COMPILE({ int8_t ret = filter.maxRssi(); (void)ret; });

    API_COMPILE({ uint8_t buf[1]; BleScanFilter f = filter.customData(buf, 0); (void)f; });
    API_COMPILE({ size_t len; const uint8_t* buf = filter.customData(&len); (void)len; (void)buf; });
}

test(ble_peer_device) {
    BleUuid uuid;
    BlePeerDevice peer;
    BleAddress addr;
    BleConnectionParams params;

    API_COMPILE({ Vector<BleService> services = peer.discoverAllServices(); });
    API_COMPILE({ BleService services[1]; ssize_t ret = peer.discoverAllServices(services, 1); (void)ret; });

    API_COMPILE({ Vector<BleCharacteristic> characteristics = peer.discoverAllCharacteristics(); });
    API_COMPILE({ BleCharacteristic characteristics[1]; ssize_t ret = peer.discoverAllCharacteristics(characteristics, 1); (void)ret; });

    API_COMPILE({ Vector<BleCharacteristic> characteristics = peer.discoverCharacteristicsOfService(BleService()); });
    API_COMPILE({ BleCharacteristic characteristics[1]; ssize_t ret = peer.discoverCharacteristicsOfService(BleService(), characteristics, 1); (void)ret; });

    API_COMPILE({ Vector<BleService> services = peer.services(); });
    API_COMPILE({ BleService services[1]; size_t ret = peer.services(services, 1); (void)ret; });
    API_COMPILE({ BleService service; bool ret = peer.getServiceByUUID(service, uuid); (void)ret; });
    API_COMPILE({ Vector<BleService> services = peer.getServiceByUUID(uuid); });
    API_COMPILE({ BleService services[1]; size_t ret = peer.getServiceByUUID(services, 1, uuid); (void)ret; });

    API_COMPILE({ Vector<BleCharacteristic> characteristics = peer.characteristics(); });
    API_COMPILE({ BleCharacteristic characteristics[1]; size_t ret = peer.characteristics(characteristics, 1); (void)ret; });
    API_COMPILE({ BleCharacteristic characteristic; bool ret = peer.getCharacteristicByDescription(characteristic, "1234"); (void)ret; });
    API_COMPILE({ BleCharacteristic characteristic; bool ret = peer.getCharacteristicByDescription(characteristic, String("1234")); (void)ret; });
    API_COMPILE({ BleCharacteristic characteristic; bool ret = peer.getCharacteristicByUUID(characteristic, uuid); (void)ret; });
    API_COMPILE({ Vector<BleCharacteristic> characteristics = peer.getCharacteristicByUUID(uuid); });
    API_COMPILE({ BleCharacteristic characteristics[1]; size_t ret = peer.getCharacteristicByUUID(characteristics, 1, uuid); (void)ret; });

    API_COMPILE({ Vector<BleCharacteristic> characteristics = peer.characteristics(BleService()); });
    API_COMPILE({ BleCharacteristic characteristics[1]; size_t ret = peer.characteristics(BleService(), characteristics, 1); (void)ret; });
    API_COMPILE({ BleCharacteristic characteristic; bool ret = peer.getCharacteristicByDescription(BleService(), characteristic, "1234"); (void)ret; });
    API_COMPILE({ BleCharacteristic characteristic; bool ret = peer.getCharacteristicByDescription(BleService(), characteristic, String("1234")); (void)ret; });
    API_COMPILE({ BleCharacteristic characteristic; bool ret = peer.getCharacteristicByUUID(BleService(), characteristic, uuid); (void)ret; });

    API_COMPILE({ int ret = peer.connect(addr); (void)ret; });
    API_COMPILE({ int ret = peer.connect(addr, false); (void)ret; });
    API_COMPILE({ int ret = peer.connect(addr, &params); (void)ret; });
    API_COMPILE({ int ret = peer.connect(addr, &params, false); (void)ret; });
    API_COMPILE({ int ret = peer.connect(addr, params); (void)ret; });
    API_COMPILE({ int ret = peer.connect(addr, params, false); (void)ret; });
    API_COMPILE({ int ret = peer.connect(addr, 0, 0, 0); (void)ret; });
    API_COMPILE({ int ret = peer.connect(addr, 0, 0, 0, false); (void)ret; });
    API_COMPILE({ int ret = peer.connect(); (void)ret; });
    API_COMPILE({ int ret = peer.connect(false); (void)ret; });
    API_COMPILE({ int ret = peer.connect(&params); (void)ret; });
    API_COMPILE({ int ret = peer.connect(&params, false); (void)ret; });
    API_COMPILE({ int ret = peer.connect(params); (void)ret; });
    API_COMPILE({ int ret = peer.connect(params, false); (void)ret; });
    API_COMPILE({ int ret = peer.connect(0, 0, 0); (void)ret; });
    API_COMPILE({ int ret = peer.connect(0, 0, 0, false); (void)ret; });
    API_COMPILE({ int ret = peer.disconnect(); (void)ret; });
    API_COMPILE({ bool ret = peer.connected(); (void)ret; });

    API_COMPILE({ peer.bind(addr); });

    API_COMPILE({ BleAddress a = peer.address(); });

    API_COMPILE({ bool ret = peer.isValid(); (void)ret; });

    API_COMPILE({ bool ret = peer == BlePeerDevice(); (void)ret; });
    API_COMPILE({ bool ret = peer != BlePeerDevice(); (void)ret; });

    API_COMPILE({ bool ret = peer; (void)ret; });

    API_COMPILE({ BlePeerDevice p = peer; });
}

test(ble_local_device_class) {
    BleAddress addr;
    BleAdvertisingParams params;
    BleScanParams scanParams;
    BleConnectionParams connectParams;
    BleAdvertisingData advData;
    BleAdvertisingData srData;
    iBeacon beacon;
    BleCharacteristic characteristic;
    EnumFlags<BleCharacteristicProperty> props = BleCharacteristicProperty::BROADCAST;
    BleUuid charUuid;
    BleUuid svcUuid;
    BlePeerDevice peer;

    API_COMPILE({ int ret = BleLocalDevice::getInstance().on(); (void)ret; });

    API_COMPILE({ int ret = BLE.begin(); (void)ret; });
    API_COMPILE({ int ret = BLE.end(); (void)ret; });

    API_COMPILE({ int ret = BLE.on(); (void)ret; });
    API_COMPILE({ int ret = BLE.off(); (void)ret; });

    API_COMPILE({ int ret = BLE.setTxPower(-8); (void)ret; });
    API_COMPILE({ int8_t p; int ret = BLE.txPower(&p); (void)ret; });
    API_COMPILE({ int8_t p = BLE.txPower(); (void)p; });

    API_COMPILE({ int ret = BLE.selectAntenna(BleAntennaType::DEFAULT); (void)ret; });

    API_COMPILE({ int ret = BLE.setAddress(addr); (void)ret; });
    API_COMPILE({ int ret = BLE.setAddress("1234"); (void)ret; });
    API_COMPILE({ int ret = BLE.setAddress("1234", BleAddressType::PUBLIC); (void)ret; });
    API_COMPILE({ int ret = BLE.setAddress(String("1234")); (void)ret; });
    API_COMPILE({ int ret = BLE.setAddress(String("1234"), BleAddressType::PUBLIC); (void)ret; });
    API_COMPILE({ BleAddress a = BLE.address(); });

    API_COMPILE({ int ret = BLE.setDeviceName("1234"); (void)ret; });
    API_COMPILE({ int ret = BLE.setDeviceName("1234", 0); (void)ret; });
    API_COMPILE({ int ret = BLE.setDeviceName(String("1234")); (void)ret; });
    API_COMPILE({ char buf[1]; size_t ret = BLE.getDeviceName(buf, 1); (void)ret; });
    API_COMPILE({ String name = BLE.getDeviceName(); });

    API_COMPILE({ int ret = BLE.setAdvertisingInterval(0); (void)ret; });
    API_COMPILE({ int ret = BLE.setAdvertisingTimeout(0); (void)ret; });
    API_COMPILE({ int ret = BLE.setAdvertisingType(BleAdvertisingEventType::CONNECTABLE_SCANNABLE_UNDIRECRED); (void)ret; });
    API_COMPILE({ int ret = BLE.setAdvertisingPhy(BlePhy::BLE_PHYS_AUTO); (void)ret; });
    API_COMPILE({ int ret = BLE.setAdvertisingParameters(&params); (void)ret; });
    API_COMPILE({ int ret = BLE.setAdvertisingParameters(params); (void)ret; });
    API_COMPILE({ int ret = BLE.setAdvertisingParameters(0, 0, BleAdvertisingEventType::CONNECTABLE_SCANNABLE_UNDIRECRED); (void)ret; });
    API_COMPILE({ int ret = BLE.getAdvertisingParameters(&params); (void)ret; });
    API_COMPILE({ int ret = BLE.getAdvertisingParameters(params); (void)ret; });

    API_COMPILE({ int ret = BLE.setAdvertisingData(&advData); (void)ret; });
    API_COMPILE({ int ret = BLE.setAdvertisingData(advData); (void)ret; });
    API_COMPILE({ int ret = BLE.setScanResponseData(&srData); (void)ret; });
    API_COMPILE({ int ret = BLE.setScanResponseData(srData); (void)ret; });
    API_COMPILE({ size_t ret = BLE.getAdvertisingData(&advData); (void)ret; });
    API_COMPILE({ size_t ret = BLE.getAdvertisingData(advData); (void)ret; });
    API_COMPILE({ size_t ret = BLE.getScanResponseData(&srData); (void)ret; });
    API_COMPILE({ size_t ret = BLE.getScanResponseData(srData); (void)ret; });

    API_COMPILE({ int ret = BLE.advertise(); (void)ret; });
    API_COMPILE({ int ret = BLE.advertise(&advData); (void)ret; });
    API_COMPILE({ int ret = BLE.advertise(advData); (void)ret; });
    API_COMPILE({ int ret = BLE.advertise(&advData, &srData); (void)ret; });
    API_COMPILE({ int ret = BLE.advertise(advData, srData); (void)ret; });
    API_COMPILE({ int ret = BLE.advertise(beacon); (void)ret; });
    API_COMPILE({ int ret = BLE.stopAdvertising(); (void)ret; });
    API_COMPILE({ bool ret = BLE.advertising(); (void)ret; });

    API_COMPILE({ int ret = BLE.setScanTimeout(0); (void)ret; });
    API_COMPILE({ int ret = BLE.setScanPhy(BlePhy::BLE_PHYS_AUTO | BlePhy::BLE_PHYS_CODED); (void)ret; });
    API_COMPILE({ int ret = BLE.setScanParameters(&scanParams); (void)ret; });
    API_COMPILE({ int ret = BLE.setScanParameters(scanParams); (void)ret; });
    API_COMPILE({ int ret = BLE.getScanParameters(&scanParams); (void)ret; });
    API_COMPILE({ int ret = BLE.getScanParameters(scanParams); (void)ret; });

    API_COMPILE({ Vector<BleScanResult> results = BLE.scan(); });
    API_COMPILE({ BleScanResult results[1]; int ret = BLE.scan(results, 0); (void)ret; });
    API_COMPILE({ int ret = BLE.scan(scanHandlerFunc); (void)ret; });
    API_COMPILE({ int ret = BLE.scan(scanHandlerFunc, nullptr); (void)ret; });
    API_COMPILE({ int ret = BLE.scan(scanHandlerFuncRef); (void)ret; });
    API_COMPILE({ int ret = BLE.scan(scanHandlerFuncRef, nullptr); (void)ret; });
    API_COMPILE({ int ret = BLE.scan([](const BleScanResult& result) {}); (void)ret; });
    API_COMPILE({ int ret = BLE.scan(&Handlers::scanHandler, &bleHandlerInstance); (void)ret; });
    API_COMPILE({ int ret = BLE.scan(std::bind(&Handlers::scanHandler, &bleHandlerInstance, _1)); (void)ret; });
    API_COMPILE({ Vector<BleScanResult> results = BLE.scanWithFilter(BleScanFilter()); });
    API_COMPILE({ BleScanResult results[1]; int ret = BLE.scanWithFilter(BleScanFilter(), results, 0); (void)ret; });
    API_COMPILE({ int ret = BLE.scanWithFilter(BleScanFilter(), scanHandlerFunc); (void)ret; });
    API_COMPILE({ int ret = BLE.scanWithFilter(BleScanFilter(), scanHandlerFunc, nullptr); (void)ret; });
    API_COMPILE({ int ret = BLE.scanWithFilter(BleScanFilter(), scanHandlerFuncRef); (void)ret; });
    API_COMPILE({ int ret = BLE.scanWithFilter(BleScanFilter(), scanHandlerFuncRef, nullptr); (void)ret; });
    API_COMPILE({ int ret = BLE.scanWithFilter(BleScanFilter(), [](const BleScanResult& result) {}); (void)ret; });
    API_COMPILE({ int ret = BLE.scanWithFilter(BleScanFilter(), &Handlers::scanHandler, &bleHandlerInstance); (void)ret; });
    API_COMPILE({ int ret = BLE.scanWithFilter(BleScanFilter(), std::bind(&Handlers::scanHandler, &bleHandlerInstance, _1)); (void)ret; });
    API_COMPILE({ int ret = BLE.stopScanning(); (void)ret; });

    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic(characteristic); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic(props, "1234"); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic(props, "1234", dataHandlerFunc); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic(props, "1234", dataHandlerFunc, nullptr); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic(props, String("1234")); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic(props, String("1234"), dataHandlerFunc); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic(props, String("1234"), dataHandlerFunc, nullptr); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic(props, "1234", [](const uint8_t*, size_t, const BlePeerDevice& peer) {}); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic(props, String("1234"), [](const uint8_t*, size_t, const BlePeerDevice& peer) {}); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic(props, "1234", &Handlers::dataHandler, &bleHandlerInstance); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic(props, String("1234"), &Handlers::dataHandler, &bleHandlerInstance); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic(props, "1234", std::bind(&Handlers::dataHandler, &bleHandlerInstance, _1, _2, _3)); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic(props, String("1234"), std::bind(&Handlers::dataHandler, &bleHandlerInstance, _1, _2, _3)); });

    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic("1234", props, charUuid, svcUuid); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic(String("1234"), props, charUuid, svcUuid); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic("1234", props, charUuid, svcUuid, dataHandlerFunc); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic(String("1234"), props, charUuid, svcUuid, dataHandlerFunc); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic("1234", props, charUuid, svcUuid, dataHandlerFunc, nullptr); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic(String("1234"), props, charUuid, svcUuid, dataHandlerFunc, nullptr); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic("1234", props, charUuid, svcUuid, [](const uint8_t*, size_t, const BlePeerDevice& peer) {}); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic(String("1234"), props, charUuid, svcUuid, [](const uint8_t*, size_t, const BlePeerDevice& peer) {}); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic("1234", props, charUuid, svcUuid, &Handlers::dataHandler, &bleHandlerInstance); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic(String("1234"), props, charUuid, svcUuid, &Handlers::dataHandler, &bleHandlerInstance); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic("1234", props, charUuid, svcUuid, std::bind(&Handlers::dataHandler, &bleHandlerInstance, _1, _2, _3)); });
    API_COMPILE({ BleCharacteristic c = BLE.addCharacteristic(String("1234"), props, charUuid, svcUuid, std::bind(&Handlers::dataHandler, &bleHandlerInstance, _1, _2, _3)); });

    API_COMPILE({ int ret = BLE.setPPCP(0, 0, 0, 0); (void)ret; });

    API_COMPILE({ BlePeerDevice p = BLE.connect(addr); });
    API_COMPILE({ BlePeerDevice p = BLE.connect(addr, false); });
    API_COMPILE({ BlePeerDevice p = BLE.connect(addr, &connectParams); });
    API_COMPILE({ BlePeerDevice p = BLE.connect(addr, &connectParams, false); });
    API_COMPILE({ BlePeerDevice p = BLE.connect(addr, connectParams); });
    API_COMPILE({ BlePeerDevice p = BLE.connect(addr, connectParams, false); });
    API_COMPILE({ BlePeerDevice p = BLE.connect(addr, 0 , 0, 0); });
    API_COMPILE({ BlePeerDevice p = BLE.connect(addr, 0, 0, 0, false); });
    API_COMPILE({ int ret = BLE.disconnect(); (void)ret; });
    API_COMPILE({ int ret = BLE.disconnect(peer); (void)ret; });
    API_COMPILE({ int ret = BLE.disconnectAll(); (void)ret; });
    API_COMPILE({ bool ret = BLE.connected(); (void)ret; });

    API_COMPILE({ BlePeerDevice p = BLE.peerCentral(); });

    API_COMPILE({ BLE.onConnected(connectedHandlerFunc); });
    API_COMPILE({ BLE.onConnected(connectedHandlerFunc, nullptr); });
    API_COMPILE({ BLE.onConnected([](const BlePeerDevice&) {}); });
    API_COMPILE({ BLE.onConnected(&Handlers::connectedHandler, &bleHandlerInstance); });
    API_COMPILE({ BLE.onConnected(std::bind(&Handlers::connectedHandler, &bleHandlerInstance, _1)); });
    API_COMPILE({ BLE.onDisconnected(disconnectedHandlerFunc); });
    API_COMPILE({ BLE.onDisconnected(disconnectedHandlerFunc, nullptr); });
    API_COMPILE({ BLE.onDisconnected([](const BlePeerDevice&) {}); });
    API_COMPILE({ BLE.onDisconnected(&Handlers::disconnectedHandler, &bleHandlerInstance); });
    API_COMPILE({ BLE.onDisconnected(std::bind(&Handlers::disconnectedHandler, &bleHandlerInstance, _1)); });

    API_COMPILE({ int ret = BLE.setPairingIoCaps(BlePairingIoCaps::NONE); (void)ret; });
    API_COMPILE({ int ret = BLE.setPairingAlgorithm(BlePairingAlgorithm::AUTO); (void)ret; });
    API_COMPILE({ int ret = BLE.startPairing(BlePeerDevice()); (void)ret; });
    API_COMPILE({ int ret = BLE.rejectPairing(BlePeerDevice()); (void)ret; });
    API_COMPILE({ uint8_t passkey[6]; int ret = BLE.setPairingPasskey(BlePeerDevice(), passkey); (void)ret; });
    API_COMPILE({ int ret = BLE.setPairingNumericComparison(BlePeerDevice(), true); (void)ret; });
    API_COMPILE({ bool ret = BLE.isPairing(BlePeerDevice()); (void)ret; });
    API_COMPILE({ bool ret = BLE.isPaired(BlePeerDevice()); (void)ret; });
    API_COMPILE({ BLE.onPairingEvent(pairingEventHandlerFunc); });
    API_COMPILE({ BLE.onPairingEvent(pairingEventHandlerFunc, nullptr); });
    API_COMPILE({ BLE.onPairingEvent([](const BlePairingEvent&) {}); });
    API_COMPILE({ BLE.onPairingEvent(&Handlers::pairingEventHandler, &bleHandlerInstance); });
    API_COMPILE({ BLE.onPairingEvent(std::bind(&Handlers::pairingEventHandler, &bleHandlerInstance, _1)); });

    API_COMPILE({ int ret = BLE.setDesiredAttMtu(123); (void)ret; });
    API_COMPILE({ int ret = BLE.updateAttMtu(BlePeerDevice()); (void)ret; });
    API_COMPILE({ size_t ret = BLE.getCurrentAttMtu(BlePeerDevice()); (void)ret; });
    API_COMPILE({ BLE.onAttMtuExchanged(attMtuExchangedHandlerFunc); });
    API_COMPILE({ BLE.onAttMtuExchanged(attMtuExchangedHandlerFunc, nullptr); });
    API_COMPILE({ BLE.onAttMtuExchanged([](const BlePeerDevice&, size_t) {}); });
    API_COMPILE({ BLE.onAttMtuExchanged(&Handlers::attMtuExchangedHandler, &bleHandlerInstance); });
    API_COMPILE({ BLE.onAttMtuExchanged(std::bind(&Handlers::attMtuExchangedHandler, &bleHandlerInstance, _1, _2)); });
}

#endif
