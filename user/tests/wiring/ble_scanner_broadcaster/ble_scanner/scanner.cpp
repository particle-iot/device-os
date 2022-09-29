#include "Particle.h"
#include "unit-test/unit-test.h"
#include "util.h"

#if Wiring_BLE == 1

#ifdef INFO
#undef INFO
#endif
#define INFO(msg, ...) \
    do { \
        Serial.printf(msg, ##__VA_ARGS__); \
    } while(false)

typedef enum {
    CMD_UNKNOWN,
    CMD_RESTART_ADV,
    CMD_ADV_DEVICE_NAME,
    CMD_ADV_APPEARANCE,
    CMD_ADV_CUSTOM_DATA,
    CMD_ADV_CODED_PHY,
    CMD_ADV_EXTENDED
} TestCommand;

const char* peerServiceUuid = "6E400000-B5A3-F393-E0A9-E50E24DCCA9E";
const char* peerCharacteristicUuid = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
BleAddress peerAddress;

static void bleOnScanResultCallback(const BleScanResult* result, void* context) {
    INFO("  > On BLE device scanned callback.\r\n");
    INFO("  > Stop scanning...\r\n");
    int ret = BLE.stopScanning();
    assertEqual(ret, 0);
}

BleCharacteristic peerCharWriteWoRsp;

using namespace particle::test;

test(BLE_000_Scanner_Cloud_Connect) {
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
    Serial.println("This is BLE scanner.");
    
    int ret;

    BleScanParams setScanParams = {};
    setScanParams.size = sizeof(BleScanParams);
    setScanParams.interval = 50; // In units of 0.625ms
    setScanParams.window = 25; // In units of 0.625ms
    setScanParams.timeout = 300; // In units of 10ms, 3s
    setScanParams.active = true; // Send scan request
    setScanParams.filter_policy = BLE_SCAN_FP_ACCEPT_ALL;
    ret = BLE.setScanParameters(&setScanParams);
    assertEqual(ret, 0);

#if HAL_PLATFORM_NRF52840
    NVIC_DisableIRQ(SD_EVT_IRQn);
#elif HAL_PLATFORM_RTL872X
    // TODO: double check
    NVIC_DisableIRQ(BT2WL_STS_IRQ);
#endif

    system_tick_t start = millis();
    Vector<BleScanResult> result = BLE.scan();
    assertEqual(result.size(), 0);
    system_tick_t now = millis();

    assertMoreOrEqual(now - start, 3000);
    assertLessOrEqual(now - start, 4500);

#if HAL_PLATFORM_NRF52840
    NVIC_EnableIRQ(SD_EVT_IRQn);
#elif HAL_PLATFORM_RTL872X
    // TODO: double check
    NVIC_DisableIRQ(BT2WL_STS_IRQ);
#endif
}
#endif // HAL_PLATFORM_NRF52840

test(BLE_02_Broadcaster_Prepare) {
}

test(BLE_03_Scanner_Connects_To_Broadcaster) {
    int ret;
    BleScanParams setScanParams = {};
    setScanParams.size = sizeof(BleScanParams);
    setScanParams.interval = 50; // In units of 0.625ms
    setScanParams.window = 25; // In units of 0.625ms
    setScanParams.timeout = 500; // In units of 10ms
    setScanParams.active = true; // Send scan request
    setScanParams.filter_policy = BLE_SCAN_FP_ACCEPT_ALL;
    ret = BLE.setScanParameters(&setScanParams);
    assertEqual(ret, 0);

    Serial.println("BLE starts scanning...");
    size_t wait = 10; // Try 10 times
    while (!BLE.connected() && wait > 0) {
        auto results = BLE.scan();
        if (results.size() > 0) {
            for (const auto& result : results) {
                BleUuid foundServiceUUID;
                size_t svcCount = result.advertisingData().serviceUUID(&foundServiceUUID, 1);
#if defined(PARTICLE_TEST_RUNNER)
                if (result.address() != BleAddress(getBleTestPeer().address, result.address().type())) {
                    continue;
                }
#endif // PARTICLE_TEST_RUNNER
                if (svcCount > 0 && foundServiceUUID == peerServiceUuid) {
                    BlePeerDevice peer = BLE.connect(result.address());
                    if (peer.connected()) {
                        assertTrue(peer.getCharacteristicByUUID(peerCharWriteWoRsp, peerCharacteristicUuid));
                        peerAddress = peer.address();
                    }
                    break;
                }
            }
        }
        wait--;
    }
    assertTrue(wait > 0);
}

test(BLE_04_Restart_Advertising) {
    assertTrue(BLE.connected());

    int ret;
    TestCommand cmd = CMD_RESTART_ADV;
    ret = peerCharWriteWoRsp.setValue((const uint8_t*)&cmd, 1);
    assertEqual(ret, 1);

    delay(1s);

    ret = BLE.scan(bleOnScanResultCallback, nullptr);
    assertTrue(ret > 0);
}

test(BLE_05_Scanner_Scan_Array) {
    assertTrue(BLE.connected());

    BleScanResult results[10];
    int ret = BLE.scan(results, sizeof(results)/sizeof(BleScanResult));
    assertTrue(ret > 0);
}

test(BLE_06_Scanner_Scan_Vector) {
    assertTrue(BLE.connected());

    Vector<BleScanResult> results = BLE.scan();
    assertTrue(results.size() > 0);
}

test(BLE_07_Scanner_Scan_With_Filter_Rssi) {
    assertTrue(BLE.connected());

    constexpr int8_t MIN_RSSI = -60;
    constexpr int8_t MAX_RSSI = -40;

    Vector<BleScanResult> results = BLE.scanWithFilter(BleScanFilter().minRssi(MIN_RSSI).maxRssi(MAX_RSSI));
    // We may very well not find any devices within [MIN_RSSI, MAX_RSSI]
    if (results.size() > 0) {
        for (const auto& result : results) {
            assertMoreOrEqual(result.rssi(), MIN_RSSI);
            assertLessOrEqual(result.rssi(), MAX_RSSI);
        }
    }
}

test(BLE_08_Scanner_Scan_With_Filter_Address) {
    assertTrue(BLE.connected());

    Vector<BleScanResult> results = BLE.scanWithFilter(BleScanFilter().address(peerAddress));
    assertEqual(results.size(), 1);

    // We assume that there is no device nearby with this particular address
    results = BLE.scanWithFilter(BleScanFilter().address("11:11:11:11:11:11"));
    assertEqual(results.size(), 0);
}

test(BLE_09_Scanner_Scan_With_Filter_Service_UUID) {
    assertTrue(BLE.connected());

    // The connected peer device is advertising service UUID by default
    Vector<BleScanResult> results = BLE.scanWithFilter(BleScanFilter().serviceUUID(peerServiceUuid));
    assertEqual(results.size(), 1);

    // We assume that there is no device nearby advertising this service UUID
    results = BLE.scanWithFilter(BleScanFilter().serviceUUID(0x1111));
    assertEqual(results.size(), 0);
}

test(BLE_10_Scanner_Scan_With_Device_Name) {
    assertTrue(BLE.connected());

    int ret;
    TestCommand cmd = CMD_ADV_DEVICE_NAME;
    ret = peerCharWriteWoRsp.setValue((const uint8_t*)&cmd, 1);
    assertEqual(ret, 1);

    delay(1s);

    Vector<BleScanResult> results = BLE.scanWithFilter(BleScanFilter().deviceName("PARTICLE_TEST"));
    assertEqual(results.size(), 1);
    
    // We assume that there is no device nearby advertising this device name
    results = BLE.scanWithFilter(BleScanFilter().deviceName("TESTTESTTEST"));
    assertEqual(results.size(), 0);
}

test(BLE_11_Scanner_Scan_With_Appearance) {
    assertTrue(BLE.connected());

    int ret;
    TestCommand cmd = CMD_ADV_APPEARANCE;
    ret = peerCharWriteWoRsp.setValue((const uint8_t*)&cmd, 1);
    assertEqual(ret, 1);

    delay(1s);

    Vector<BleScanResult> results = BLE.scanWithFilter(BleScanFilter().appearance(BLE_SIG_APPEARANCE_GENERIC_DISPLAY));
    assertEqual(results.size(), 1);
    
    // We assume that there is no device nearby advertising this appearance
    results = BLE.scanWithFilter(BleScanFilter().appearance(BLE_SIG_APPEARANCE_GENERIC_EYE_GLASSES));
    assertEqual(results.size(), 0);
}

test(BLE_12_Scanner_Scan_With_Custom_Data) {
    assertTrue(BLE.connected());

    int ret;
    TestCommand cmd = CMD_ADV_CUSTOM_DATA;
    ret = peerCharWriteWoRsp.setValue((const uint8_t*)&cmd, 1);
    assertEqual(ret, 1);

    delay(1s);

    constexpr uint8_t buf[] = {0xde, 0xad, 0xbe, 0xef};
    Vector<BleScanResult> results = BLE.scanWithFilter(BleScanFilter().customData(buf, sizeof(buf)));
    assertEqual(results.size(), 1);
    
    // We assume that there is no device nearby advertising this custom data
    constexpr uint8_t magic[] = {0x12, 0x34, 0x56, 0x78};
    results = BLE.scanWithFilter(BleScanFilter().customData(magic, sizeof(magic)));
    assertEqual(results.size(), 0);
}

#if HAL_PLATFORM_NRF52840
test(BLE_13_Scanner_Scan_On_Coded_Phy) {
    assertTrue(BLE.connected());

    int ret;
    TestCommand cmd = CMD_ADV_CODED_PHY;
    ret = peerCharWriteWoRsp.setValue((const uint8_t*)&cmd, 1);
    assertEqual(ret, 1);

    delay(1s);

    assertEqual(BLE.setScanPhy(BlePhy::BLE_PHYS_CODED), 0);
    Vector<BleScanResult> results = BLE.scanWithFilter(BleScanFilter().deviceName("CODED_PHY"));
    assertEqual(results.size(), 1);
    
    // Scan on 1MBPS PHY
    assertEqual(BLE.setScanPhy(BlePhy::BLE_PHYS_1MBPS), 0);
    results = BLE.scanWithFilter(BleScanFilter().deviceName("CODED_PHY"));
    assertEqual(results.size(), 0);
}

test(BLE_14_Scanner_Scan_Extended) {
    assertTrue(BLE.connected());

    int ret;
    TestCommand cmd = CMD_ADV_EXTENDED;
    ret = peerCharWriteWoRsp.setValue((const uint8_t*)&cmd, 1);
    assertEqual(ret, 1);

    delay(1s);

    assertEqual(BLE.setScanPhy(BlePhy::BLE_PHYS_CODED), 0);
    uint8_t buf[BLE_MAX_ADV_DATA_LEN_EXT_CONNECTABLE - 5];
    memset(buf, 0x35, BLE_MAX_ADV_DATA_LEN_EXT_CONNECTABLE - 5);
    Vector<BleScanResult> results = BLE.scanWithFilter(BleScanFilter().customData(buf, BLE_MAX_ADV_DATA_LEN_EXT_CONNECTABLE - 5));
    assertEqual(results.size(), 1);
}
#endif // HAL_PLATFORM_NRF52840

#endif // #if Wiring_BLE == 1
