#include "Particle.h"
#include "unit-test/unit-test.h"

#if Wiring_BLE == 1

const char* peerServiceUuid = "6E400000-B5A3-F393-E0A9-E50E24DCCA9E";
const char* peerCharacteristicUuid = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

BleCharacteristic charWriteWoRsp("", BleCharacteristicProperty::WRITE_WO_RSP,
                                 peerCharacteristicUuid, peerServiceUuid, onDataReceived, nullptr);

typedef enum {
    CMD_UNKNOWN,
    CMD_RESTART_ADV,
    CMD_ADV_DEVICE_NAME,
    CMD_ADV_APPEARANCE,
    CMD_ADV_CUSTOM_DATA
} TestCommand;
TestCommand cmd = CMD_UNKNOWN;

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
    cmd = (TestCommand)(*data);
}

test(BLE_Broadcaster_01_Connect_By_Scanner) {
    Serial.println("This is BLE broadcaster.");

    BleCharacteristic temp;
    temp = BLE.addCharacteristic(charWriteWoRsp);
    assertTrue(temp.isValid());

    BleAdvertisingData data;
    data.appendServiceUUID(peerServiceUuid);
    int ret = BLE.advertise(&data);
    assertEqual(ret, 0);
    assertTrue(BLE.advertising());

    Serial.println("BLE starts advertising...");
    assertTrue(waitFor(BLE.connected, 20000));
}

test(BLE_Broadcaster_02_Restart_Advertising) {
    assertTrue(BLE.connected());
    assertTrue(waitFor([&]{ return cmd == CMD_RESTART_ADV; }, 60000));

    int ret = BLE.advertise();
    assertEqual(ret, 0);
    assertTrue(BLE.advertising());
}

test(BLE_Broadcaster_03_Advertise_Device_Name) {
    assertTrue(BLE.connected());
    assertTrue(waitFor([&]{ return cmd == CMD_ADV_DEVICE_NAME; }, 60000));

    BleAdvertisingData data;
    data.appendLocalName("PARTICLE_TEST");
    assertEqual(data.deviceName(), String("PARTICLE_TEST"));
    int ret = BLE.advertise(&data);
    assertEqual(ret, 0);
    assertTrue(BLE.advertising());
}

test(BLE_Broadcaster_04_Advertise_Appearance) {
    assertTrue(BLE.connected());
    assertTrue(waitFor([&]{ return cmd == CMD_ADV_APPEARANCE; }, 60000));

    BleAdvertisingData data;
    data.appendAppearance(BLE_SIG_APPEARANCE_GENERIC_DISPLAY);
    assertEqual(data.appearance(), BLE_SIG_APPEARANCE_GENERIC_DISPLAY);
    int ret = BLE.advertise(&data);
    assertEqual(ret, 0);
    assertTrue(BLE.advertising());
}

test(BLE_Broadcaster_05_Advertise_Custom_Data) {
    assertTrue(BLE.connected());
    assertTrue(waitFor([&]{ return cmd == CMD_ADV_CUSTOM_DATA; }, 60000));

    BleAdvertisingData data;
    constexpr uint8_t buf[] = {0xde, 0xad, 0xbe, 0xef};
    uint8_t read[4];
    data.appendCustomData(buf, sizeof(buf));
    data.customData(read, sizeof(read));
    assertEqual(memcmp(buf, read, sizeof(buf)), 0);
    int ret = BLE.advertise(&data);
    assertEqual(ret, 0);
    assertTrue(BLE.advertising());
}

#endif // #if Wiring_BLE == 1
