#include "application.h"
#include "test.h"

// Log to USB serial as part of stress test
SerialLogHandler g_logHandler(115200, LOG_LEVEL_WARN, {
    { "app", LOG_LEVEL_ALL }
});

namespace {

volatile int updateResult = firmware_update_failed;
bool busyMode = false;
Vector<WiFiAccessPoint> apList;

void prepareOta(bool connect = true, bool compressedOta = true) {
    System.disableReset();

    System.on(firmware_update, [](system_event_t ev, int data, void* context) {
        updateResult = data;

        if (busyMode) {
            auto progress = (FileTransfer::Descriptor*)(context);

            auto bytesRx = progress->chunk_address - progress->file_address;
            auto chunksRx = bytesRx / progress->chunk_size;
            auto totalChunks = progress->chunk_count(progress->chunk_size);

            Log.info("bytes Rx/total %lu/%lu chunks Rx/total %lu/%u",
                bytesRx,
                progress->file_length,
                chunksRx,
                totalChunks);
        }
    });

    updateResult = SYSTEM_ERROR_OTA;

#if HAL_PLATFORM_COMPRESSED_OTA
    spark_protocol_set_connection_property(spark_protocol_instance(), protocol::Connection::COMPRESSED_OTA, compressedOta, nullptr, nullptr);
#endif

    if (connect) {
        Particle.connect();
        waitFor(Particle.connected, 5 * 60 * 1000);
    }
}

void waitOta() {
    SCOPE_GUARD({
        System.off(all_events);
        System.enableReset();
    });

    for (auto start = millis(); millis() - start <= 20 * 60 * 1000;) {
        if (updateResult == firmware_update_complete || updateResult == firmware_update_failed || updateResult == firmware_update_pending) {
            break;
        } else if (updateResult == SYSTEM_ERROR_OTA && millis() - start >= 1 * 60 * 1000) {
            break;
        }
        Particle.process();
        delay(100);
    }

    if (Particle.connected() && updateResult == firmware_update_complete) {
        // Just in case
        Particle.publish("test/ota", "success", WITH_ACK).wait();
    }

    assertNotEqual((int)updateResult, (int)firmware_update_failed);
    assertNotEqual((int)updateResult, (int)SYSTEM_ERROR_OTA);
    
    assertEqual(0, TestRunner::instance()->pushMailbox(Test::MailboxEntry().type(Test::MailboxEntry::Type::RESET_PENDING), 5000));
}

void highPriorityFunc() {
    while (1) {
        HAL_Delay_Microseconds(random(100));
        // Force frequent reschedule
        delay(1);
    }
}

void bleScanResultCallBack(const BleScanResult *result, void *context) {
    static int count = 0;
    if (count++ == 30) {
        Log.info("BLE adv: %s", result->address().toString().c_str());
        count = 0;
    }
}

void wifiScanResultCallBack(WiFiAccessPoint* wap, void* context) {
  if (apList.size() < 20)
    apList.append(*wap);
}

void wifiScanThread() {
    unsigned int wifiSeconds = 0;
    const int WIFI_SCAN_INTERVAL_SECONDS = 10;

    while (1) {
#if HAL_PLATFORM_WIFI
        if(System.uptime() >= wifiSeconds) {
            apList.clear();

            Log.info("WiFi scan start");
            auto wifiResult = WiFi.scan(wifiScanResultCallBack, nullptr);
            for (auto ap: apList) {
                String bssid = String::format("%02X:%02X:%02X:%02X:%02X:%02X",
                    ap.bssid[0], ap.bssid[1], ap.bssid[2], ap.bssid[3], ap.bssid[4], ap.bssid[5]);
                Log.info("SSID=%s, BSSID=%s, CHAN=%u, SIGNAL=%d",
                                ap.ssid, bssid.c_str(), (unsigned int)ap.channel, ap.rssi);
            }
            if (apList.size() == 0) {
                Log.info("WiFi scan failed: %d", wifiResult);
            }
            wifiSeconds = System.uptime() + WIFI_SCAN_INTERVAL_SECONDS;
        }
#endif
        delay(100);
    }
}

void bleScanThread() {
    while (1) {
#if HAL_PLATFORM_BLE

#if HAL_PLATFORM_RTL872X
        auto bleScanTimeout = BLE_SCAN_TIMEOUT_UNLIMITED;
#else 
        auto bleScanTimeout = 50;
#endif
        BleScanParams params = {};
        params.version = BLE_API_VERSION;
        params.size = sizeof(BleScanParams);
        params.timeout = bleScanTimeout; // *10ms = 500ms overall duration
        params.interval = 800; // *0.625ms = 500ms
        params.window = 800; // *0.625 = 500s
        params.active = false;
        
        BLE.setScanParameters(params);
        BLE.scanWithFilter(BleScanFilter().allowDuplicates(true), bleScanResultCallBack, nullptr);
#endif
        delay(500);
    }
}

void enableBusyMode() {
    if (busyMode) {
        // Already enabled
        return;
    }
#if HAL_PLATFORM_BLE
    BLE.on();
#endif

#if HAL_PLATFORM_WIFI
    WiFi.on();
#endif

    busyMode = true;
    new Thread("high_prio", highPriorityFunc, OS_THREAD_PRIORITY_CRITICAL);
    new Thread("wifi_scan", wifiScanThread, OS_THREAD_PRIORITY_DEFAULT);
    new Thread("ble_scan", bleScanThread, OS_THREAD_PRIORITY_DEFAULT);
}

} // anonymous

STARTUP({
    testAppInit();
});

void setup() {
    testAppSetup();
}

void loop() {
    if (busyMode) {
        delay(random(100));
    }
    testAppLoop();
}


test(01_check_current_application) {
}

test(02_ota_max_application_start) {
    prepareOta(true, false);
}

test(03_ota_max_application_wait) {
    waitOta();
}

test(04_check_max_application) {
}

test(05_ota_original_application_start) {
    prepareOta(true, false);
}

test(06_ota_original_application_wait) {
    waitOta();
}

test(07_check_original_application) {
}

test(08_ota_max_application_compressed_start) {
    prepareOta();
}

test(09_ota_max_application_compressed_wait) {
    waitOta();
}

test(10_check_max_application) {
}

test(11_ota_original_application_compressed_start) {
    prepareOta();
}

test(12_ota_original_application_compressed_wait) {
    waitOta();
}

test(13_check_original_application) {
}

test(14_usb_flash_max_application_start) {
    prepareOta(false);
}

test(15_usb_flash_max_application_wait) {
    waitOta();
}

test(16_check_max_application) {
}

test(17_usb_flash_original_application_start) {
    prepareOta(false);
}

test(18_usb_flash_original_application_wait) {
    waitOta();
}

test(19_check_original_application) {
}

test(20_usb_flash_max_application_compressed_start) {
    prepareOta(false);
}

test(21_usb_flash_max_application_compressed_wait) {
    waitOta();
}

test(22_check_max_application) {
}

test(23_usb_flash_original_application_compressed_start) {
    prepareOta(false);
}

test(24_usb_flash_original_application_compressed_wait) {
    waitOta();
}

test(25_check_original_application) {
}

test(26_ota_max_application_busy_start) {
    enableBusyMode();
    prepareOta(true, false);
}

test(27_ota_max_application_busy_wait) {
    waitOta();
}

test(28_check_max_application) {
}

test(29_ota_original_application_busy_start) {
    enableBusyMode();
    prepareOta(true, false);
}

test(30_ota_original_application_busy_wait) {
    waitOta();
}

test(31_check_original_application) {
}

test(32_usb_flash_max_application_busy_start) {
    enableBusyMode();
    prepareOta(false);
}

test(33_usb_flash_max_application_busy_wait) {
    waitOta();
}

test(34_check_max_application) {
}

test(35_usb_flash_original_application_busy_start) {
    enableBusyMode();
    prepareOta(false);
}

test(36_usb_flash_original_application_busy_wait) {
    waitOta();
}

test(37_check_original_application) {
}
