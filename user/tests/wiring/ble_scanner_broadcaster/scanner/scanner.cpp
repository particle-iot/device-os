#include "Particle.h"
#include "unit-test/unit-test.h"

#ifdef INFO
#undef INFO
#endif
#define INFO(msg, ...) \
    do { \
        Serial.printf(msg, ##__VA_ARGS__); \
    } while(false)

static void bleOnScanResultCallback(const BleScanResult* result, void* context) {
    INFO("  > On BLE device scanned callback.\r\n");
    INFO("  > Stop scanning...\r\n");
    int ret = BLE.stopScanning();
    assertEqual(ret, 0);
}

test(BLE_Scanning_Control) {
    INFO("  > Please make sure that there is at least one BLE Peripheral being advertising nearby.\r\n");

    int ret;

    BleScanParams setScanParams = {};
    setScanParams.size = sizeof(BleScanParams);
    setScanParams.interval = 50; // In units of 0.625ms
    setScanParams.window = 25; // In units of 0.625ms
    setScanParams.timeout = 300; // In units of 10ms
    setScanParams.active = true; // Send scan request
    setScanParams.filter_policy = BLE_SCAN_FP_ACCEPT_ALL;
    ret = BLE.setScanParameters(&setScanParams);
    assertEqual(ret, 0);

    INFO("  > Testing BLE scanning for 3 seconds...\r\n");
    ret = BLE.scan(bleOnScanResultCallback, nullptr);
    assertTrue(ret > 0);

    INFO("  > Testing BLE scanning for 3 seconds...\r\n");
    BleScanResult results[10];
    ret = BLE.scan(results, sizeof(results)/sizeof(BleScanResult));
    assertTrue(ret > 0);
    INFO("  > Found %d BLE devices\r\n", ret);

    INFO("  > Testing BLE scanning for 3 seconds...\r\n");
    Vector<BleScanResult> result = BLE.scan();
    assertTrue(result.size() > 0);
    INFO("  > Found %d BLE devices\r\n", result.size());
}
