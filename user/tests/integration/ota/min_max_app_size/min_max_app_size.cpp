#include "application.h"
#include "test.h"

namespace {

volatile int updateResult = firmware_update_failed;

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

} // anonymous


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

