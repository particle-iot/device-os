#include "Particle.h"
#include "unit-test/unit-test.h"
#include "util.h"

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
    CMD_ADV_CUSTOM_DATA,
    CMD_ADV_CODED_PHY,
    CMD_ADV_EXTENDED
} TestCommand;
TestCommand cmd = CMD_UNKNOWN;

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
    cmd = (TestCommand)(*data);
}

using namespace particle::test;

test(BLE_000_Broacaster_Cloud_Connect) {
    subscribeEvents(BLE_ROLE_PERIPHERAL);
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertTrue(publishBlePeerInfo());
}

test(BLE_00_Prepare) {
#ifndef PARTICLE_TEST_RUNNER
    for (int i = 0; i < 60; i++) {
        assertTrue(publishBlePeerInfo());
        if (getBleTestPeer().isValid()) {
            break;
        }
        delay(1000);
    }
    assertTrue(getBleTestPeer().isValid());
#else
    assertTrue(waitFor(getBleTestPeer().isValid, 60 * 1000));
#endif // PARTICLE_TEST_RUNNER
}

#if HAL_PLATFORM_NRF52840
test(BLE_01_Scanner_Blocked_Timeout_Simulate) {
}
#endif // HAL_PLATFORM_NRF52840

test(BLE_02_Broadcaster_Prepare) {
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
}

test(BLE_03_Scanner_Connects_To_Broadcaster) {
    assertTrue(waitFor(BLE.connected, 20000));
}

test(BLE_04_Restart_Advertising) {
    assertTrue(BLE.connected());
    assertTrue(waitFor([&]{ return cmd == CMD_RESTART_ADV; }, 60000));

    int ret = BLE.advertise();
    assertEqual(ret, 0);
    assertTrue(BLE.advertising());
}

test(BLE_05_Scanner_Scan_Array) {
}

test(BLE_06_Scanner_Scan_Vector) {
}

test(BLE_07_Scanner_Scan_With_Filter_Rssi) {
}

test(BLE_08_Scanner_Scan_With_Filter_Address) {
}

test(BLE_09_Scanner_Scan_With_Filter_Service_UUID) {
}

test(BLE_10_Scanner_Scan_With_Device_Name) {
    assertTrue(BLE.connected());
    assertTrue(waitFor([&]{ return cmd == CMD_ADV_DEVICE_NAME; }, 60000));

    BleAdvertisingData data;
    data.appendLocalName("PARTICLE_TEST");
    assertEqual(data.deviceName(), String("PARTICLE_TEST"));
    int ret = BLE.advertise(&data);
    assertEqual(ret, 0);
    assertTrue(BLE.advertising());
}

test(BLE_11_Scanner_Scan_With_Appearance) {
    assertTrue(BLE.connected());
    assertTrue(waitFor([&]{ return cmd == CMD_ADV_APPEARANCE; }, 60000));

    BleAdvertisingData data;
    data.appendAppearance(BLE_SIG_APPEARANCE_GENERIC_DISPLAY);
    assertEqual(data.appearance(), BLE_SIG_APPEARANCE_GENERIC_DISPLAY);
    int ret = BLE.advertise(&data);
    assertEqual(ret, 0);
    assertTrue(BLE.advertising());
}

test(BLE_12_Scanner_Scan_With_Custom_Data) {
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

#if HAL_PLATFORM_NRF52840
test(BLE_13_Scanner_Scan_On_Coded_Phy) {
    assertTrue(BLE.connected());
    assertTrue(waitFor([&]{ return cmd == CMD_ADV_CODED_PHY; }, 60000));

    assertEqual(BLE.setAdvertisingPhy(BlePhy::BLE_PHYS_CODED), 0);

    BleAdvertisingData data;
    data.appendLocalName("CODED_PHY");
    assertEqual(data.deviceName(), String("CODED_PHY"));
    int ret = BLE.advertise(&data);
    assertEqual(ret, 0);
    assertTrue(BLE.advertising());
}

test(BLE_14_Scanner_Scan_Extended) {
    assertTrue(BLE.connected());
    assertTrue(waitFor([&]{ return cmd == CMD_ADV_EXTENDED; }, 60000));

    assertEqual(BLE.setAdvertisingPhy(BlePhy::BLE_PHYS_CODED), 0);
    // Make the advert 5 less than maximum to account for flags, type, and size
    uint8_t buf[BLE_MAX_ADV_DATA_LEN_EXT_CONNECTABLE - 5];
    memset(buf, 0x35, BLE_MAX_ADV_DATA_LEN_EXT_CONNECTABLE - 5);
    BleAdvertisingData data;
    data.appendCustomData(buf, BLE_MAX_ADV_DATA_LEN_EXT_CONNECTABLE - 5);
    int ret = BLE.advertise(&data);
    assertEqual(ret, 0);
    assertTrue(BLE.advertising());
}
#endif // HAL_PLATFORM_NRF52840

#endif // #if Wiring_BLE == 1
