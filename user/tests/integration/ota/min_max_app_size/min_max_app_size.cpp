#include "application.h"
#include "test.h"

// TODO: remove after done debugging
Serial1LogHandler g_logHandler(115200, LOG_LEVEL_WARN, {
    { "app", LOG_LEVEL_ALL }
});

namespace {

volatile int updateResult = firmware_update_failed;
bool slowAppLoop = false;

void prepareOta(bool connect = true) {
    System.disableReset();

    System.on(firmware_update, [](system_event_t ev, int data, void* context) {
        updateResult = data;
    });

    updateResult = SYSTEM_ERROR_OTA;

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

    for (auto start = millis(); millis() - start <= 5 * 60 * 1000;) {
        if (updateResult == firmware_update_complete || updateResult == firmware_update_failed || updateResult == firmware_update_pending) {
            break;
        } else if (updateResult == SYSTEM_ERROR_OTA && millis() - start >= 1 * 60 * 1000) {
            break;
        }
        Particle.process();
        delay(100);
    }

    assertNotEqual((int)updateResult, (int)firmware_update_failed);
    assertNotEqual((int)updateResult, (int)SYSTEM_ERROR_OTA);
    
    assertEqual(0, TestRunner::instance()->pushMailbox(Test::MailboxEntry().type(Test::MailboxEntry::Type::RESET_PENDING), 5000));
}

void highPriorityFunc() {
    while (1) {
        //Log.info("High priority thread");
        delay(1);
    }
}

void bleScanResultCallBack(const BleScanResult *result, void *context) {
    Log.info("BLE adv: %s", result->address().toString().c_str());
}

void lowPriorityFunc() {
    while (1) {
#if HAL_PLATFORM_BLE
        static bool bleOn = false;

        if (!bleOn) {
            BLE.on();
            bleOn = true;
        }

        BleScanParams params = {};
        params.version = BLE_API_VERSION;
        params.size = sizeof(BleScanParams);
        params.timeout = 50; // *10ms = 500ms overall duration
        params.interval = 800; // *0.625ms = 500ms
        params.window = 800; // *0.625 = 500s
        params.active = false;
        
        BLE.setScanParameters(params);
        BLE.scan(bleScanResultCallBack, nullptr);
#endif

        // TODO: Wifi scanning?

        //Log.info("Low priority thread");
        delay(1000);
    }
}

} // anonymous

STARTUP({
    testAppInit();
});

void setup() {
    testAppSetup();
}

void loop() {
    if (slowAppLoop) {
        auto delayMillisMax = 50;
        delay(random(delayMillisMax));
    }
    testAppLoop();
}


test(01_check_current_application) {
}

test(02_ota_max_application_start) {
    prepareOta();
}

test(03_ota_max_application_wait) {
    waitOta();
}

test(04_check_max_application) {

}

test(05_ota_original_application_start) {
    prepareOta();
}

test(06_ota_original_application_wait) {
    waitOta();
}

test(07_check_original_application) {
}

test(08_usb_flash_max_application_start) {
    prepareOta(false);
}

test(09_usb_flash_max_application_wait) {
    waitOta();
}

test(10_check_max_application) {
}

test(11_usb_flash_original_application_start) {
    prepareOta(false);
}

test(12_usb_flash_original_application_wait) {
    waitOta();
}

test(13_check_original_application) {
}

test(14_usb_flash_max_application_compressed_start) {
    prepareOta(false);
}

test(15_usb_flash_max_application_compressed_wait) {
    waitOta();
}

test(16_check_max_application) {
}

test(17_usb_flash_original_application_compressed_start) {
    prepareOta(false);
}

test(18_usb_flash_original_application_compressed_wait) {
    waitOta();
}

test(19_check_original_application) {
}

test(20_ota_max_busy_application_start) {
    Thread *highPriorityThread  = nullptr;
    Thread *lowPriorityThread = nullptr;

    Log.info("20_ota_max_busy_application_start");

    // Start some high priority threads
    highPriorityThread = new Thread("high_prio", highPriorityFunc, OS_THREAD_PRIORITY_CRITICAL);
    lowPriorityThread = new Thread("low_prio", lowPriorityFunc, OS_THREAD_PRIORITY_DEFAULT);

    slowAppLoop = true;

    prepareOta();
}

test(21_ota_max_busy_application_wait) {
    waitOta();
}

test(22_check_max_application) {

}

test(23_ota_original_application_start) {
    prepareOta();
}

test(24_ota_original_application_wait) {
    waitOta();
}

test(25_check_original_application) {

}
