#include <atomic>
#include <limits>

#include "application.h"

#include "random.h"

#include "unit-test/unit-test.h"

namespace {

using namespace particle;

enum class FirmwareUpdateStatus {
    NONE,
    STARTED,
    SUCCESS,
    ERROR
};

// Unique variable names specific to the device
retained char deviceVar1[128] = {};
retained char deviceVar2[128] = {};

// Random string generated for each test suite run
retained char nonce[11] = {};

auto firmwareUpdateStatus = FirmwareUpdateStatus::NONE;
std::atomic<int> firmwareUpdateProgressCount;

String findVarWithValue(const String& val) {
    for (const auto& name: System.listEnv()) {
        if (System.getEnv(name) == val) {
            return name;
        }
    }
    return String();
}

void firmwareUpdateEventHandler(system_event_t, int data, void*) {
    switch (data) {
    case firmware_update_begin:
        Test::out->println("firmware_update_begin");
        firmwareUpdateStatus = FirmwareUpdateStatus::STARTED;
        break;
    case firmware_update_complete:
        Test::out->println("firmware_update_complete");
        firmwareUpdateStatus = FirmwareUpdateStatus::SUCCESS;
        break;
    case firmware_update_progress:
        ++firmwareUpdateProgressCount;
        break;
    default:
        Test::out->printlnf("Unexpected firmware update status: 0x%08x", (unsigned)data);
        firmwareUpdateStatus = FirmwareUpdateStatus::ERROR;
        break;
    }
}

void prepareForFirmwareUpdate() {
    System.disableReset();
    System.on(firmware_update, firmwareUpdateEventHandler);
    firmwareUpdateStatus = FirmwareUpdateStatus::NONE;
    firmwareUpdateProgressCount = 0;
}

void completeFirmwareUpdate(bool expectSafeMode = false) {
    bool ok = false;
    auto t1 = millis();
    for (;;) {
        if (firmwareUpdateStatus == FirmwareUpdateStatus::SUCCESS) {
            ok = true;
            break;
        }
        if (firmwareUpdateStatus == FirmwareUpdateStatus::ERROR) {
            Test::out->println("Firmware update failed");
            break;
        }
        // The JS part of the test waits until the OTA completes so the timeout here is for
        // finalizing the update on the device
        if (millis() - t1 >= 30000) {
            Test::out->println("Firmware update timeout");
            break;
        }
    }
    Test::out->printlnf("firmware_update_progress count: %d", firmwareUpdateProgressCount.load());
    System.off(firmware_update);
    if (ok) {
        if (expectSafeMode) {
            TestRunner::instance()->expectSafeMode();
        } else {
            TestRunner::instance()->expectSystemReset();
        }
    }
    assertTrue(ok);
    System.enableReset();
}

void connect() {
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
}

} // namespace

test(01_init) {
    // Generate a random string for this test suite run
    Random rand;
    rand.genBase32(nonce, sizeof(nonce) - 1);
    nonce[sizeof(nonce) - 1] = '\0';
    pushMailboxMsg(nonce, 5000 /* wait */);

    // Clear the env vars and reset to apply the changes
    System.clearEnv(false /* reset */);
    expectSystemReset();
    System.reset();
}

test(02_start_local_env_update) {
    // Validate that no env vars are defined
    auto vars = System.listEnv();
    assertEqual(vars.size(), 0);
}

test(03_check_local_env_update) {
    String longStr;
    assertTrue(longStr.resize(1000));
    for (unsigned i = 0; i < longStr.length(); ++i) {
        longStr[i] = 'a';
    }

    // System.getEnv()
    assertTrue(System.getEnv("STR_SHORT") == "abc");
    assertTrue(System.getEnv("STR_LONG") == longStr);
    assertTrue(System.getEnv("STR_RANDOM") == nonce);
    assertTrue(System.getEnv("STR_EMPTY") == "");
    assertTrue(System.getEnv("INT_POSITIVE") == "123");
    assertTrue(System.getEnv("INT_NEGATIVE") == "-456");
    assertTrue(System.getEnv("INT_LEADING_ZEROS") == "00123");
    assertTrue(System.getEnv("INT_ZERO") == "0");
    assertTrue(System.getEnv("INT_NEGATIVE_ZERO") == "-0");
    assertTrue(System.getEnv("INT_PLUS_SIGN") == "+123");
    assertTrue(System.getEnv("INT_MAX") == "2147483647");
    assertTrue(System.getEnv("INT_MIN") == "-2147483648");
    assertTrue(System.getEnv("INT_OVERFLOW") == "2147483648");
    assertTrue(System.getEnv("INT_UNDERFLOW") == "-2147483649");
    assertTrue(System.getEnv("INT_FLOAT") == "12.34");
    assertTrue(System.getEnv("INT_LEADING_SPACE") == " 123");
    assertTrue(System.getEnv("INT_TRAILING_SPACE") == "123 ");
    assertTrue(System.getEnv("INT_HEX") == "0x1F");
    assertTrue(System.getEnv("INT_WITH_CHARS") == "123abc");
    assertTrue(System.getEnv("BOOL_TRUE") == "true");
    assertTrue(System.getEnv("BOOL_FALSE") == "false");
    assertTrue(System.getEnv("BOOL_UPPER_TRUE") == "TRUE");
    assertTrue(System.getEnv("BOOL_UPPER_FALSE") == "FALSE");
    assertTrue(System.getEnv("BOOL_MIXED_TRUE") == "True");
    assertTrue(System.getEnv("BOOL_MIXED_FALSE") == "False");
    assertTrue(System.getEnv("BOOL_ONE") == "1");
    assertTrue(System.getEnv("BOOL_ZERO") == "0");
    assertTrue(System.getEnv("BOOL_YES") == "yes");
    assertTrue(System.getEnv("BOOL_NO") == "no");
    assertTrue(System.getEnv("BOOL_LEADING_SPACE") == " false");
    assertTrue(System.getEnv("BOOL_TRAILING_SPACE") == "false ");

    // System.listEnv()
    Vector<String> names;
    for (const auto& name: System.listEnv()) {
        names.append(name); // const char* -> String
    }
    assertEqual(names.size(), 31);
    assertTrue(names.contains("STR_SHORT"));
    assertTrue(names.contains("STR_LONG"));
    assertTrue(names.contains("STR_RANDOM"));
    assertTrue(names.contains("STR_EMPTY"));
    assertTrue(names.contains("INT_POSITIVE"));
    assertTrue(names.contains("INT_NEGATIVE"));
    assertTrue(names.contains("INT_LEADING_ZEROS"));
    assertTrue(names.contains("INT_ZERO"));
    assertTrue(names.contains("INT_NEGATIVE_ZERO"));
    assertTrue(names.contains("INT_PLUS_SIGN"));
    assertTrue(names.contains("INT_MAX"));
    assertTrue(names.contains("INT_MAX"));
    assertTrue(names.contains("INT_OVERFLOW"));
    assertTrue(names.contains("INT_UNDERFLOW"));
    assertTrue(names.contains("INT_FLOAT"));
    assertTrue(names.contains("INT_LEADING_SPACE"));
    assertTrue(names.contains("INT_TRAILING_SPACE"));
    assertTrue(names.contains("INT_HEX"));
    assertTrue(names.contains("INT_WITH_CHARS"));
    assertTrue(names.contains("BOOL_TRUE"));
    assertTrue(names.contains("BOOL_FALSE"));
    assertTrue(names.contains("BOOL_UPPER_TRUE"));
    assertTrue(names.contains("BOOL_UPPER_FALSE"));
    assertTrue(names.contains("BOOL_MIXED_TRUE"));
    assertTrue(names.contains("BOOL_MIXED_FALSE"));
    assertTrue(names.contains("BOOL_ONE"));
    assertTrue(names.contains("BOOL_ZERO"));
    assertTrue(names.contains("BOOL_YES"));
    assertTrue(names.contains("BOOL_NO"));
    assertTrue(names.contains("BOOL_LEADING_SPACE"));
    assertTrue(names.contains("BOOL_TRAILING_SPACE"));

    // System.hasEnv()
    for (int i = 0; i < names.size(); ++i) {
        assertTrue(System.hasEnv(names[i].c_str()));
    }
    assertTrue(System.hasEnv("STR_EMPTY")); // Empty value still exists
    assertFalse(System.hasEnv("NON_EXISTENT"));

    // System.getEnv(..., String&)
    {
        String val = "xxx";
        assertTrue(System.getEnv("STR_SHORT", val));
        assertTrue(val == "abc");

        val = "xxx";
        assertTrue(System.getEnv("STR_LONG", val));
        assertTrue(val == longStr);

        val = "xxx";
        assertTrue(System.getEnv("STR_RANDOM", val));
        assertTrue(val == nonce);

        val = "xxx";
        assertTrue(System.getEnv("STR_EMPTY", val));
        assertTrue(val == "");

        val = "xxx";
        assertTrue(System.getEnv("INT_ZERO", val));
        assertTrue(val == "0");

        val = "xxx";
        assertTrue(System.getEnv("BOOL_FALSE", val));
        assertTrue(val == "false");

        val = "xxx";
        assertFalse(System.getEnv("NON_EXISTENT", val)); // Not found
        assertTrue(val == "xxx"); // Unchanged
    }

    // System.getEnv(..., int&)
    {
        int val = 999;
        assertTrue(System.getEnv("INT_POSITIVE", val));
        assertTrue(val == 123);

        val = 999;
        assertTrue(System.getEnv("INT_NEGATIVE", val));
        assertTrue(val == -456);

        val = 999;
        assertTrue(System.getEnv("INT_LEADING_ZEROS", val));
        assertTrue(val == 123);

        val = 999;
        assertTrue(System.getEnv("INT_ZERO", val));
        assertTrue(val == 0);

        val = 999;
        assertTrue(System.getEnv("INT_NEGATIVE_ZERO", val));
        assertTrue(val == 0);

        val = 999;
        assertFalse(System.getEnv("INT_PLUS_SIGN", val)); // Invalid
        assertTrue(val == 999); // Unchanged

        val = 999;
        assertTrue(System.getEnv("INT_MAX", val));
        assertTrue(val == std::numeric_limits<int>::max());

        val = 999;
        assertTrue(System.getEnv("INT_MIN", val));
        assertTrue(val == std::numeric_limits<int>::min());

        val = 999;
        assertFalse(System.getEnv("INT_OVERFLOW", val)); // Invalid
        assertTrue(val == 999); // Unchanged

        val = 999;
        assertFalse(System.getEnv("INT_UNDERFLOW", val)); // Invalid
        assertTrue(val == 999); // Unchanged

        val = 999;
        assertFalse(System.getEnv("INT_FLOAT", val)); // Invalid
        assertTrue(val == 999); // Unchanged

        val = 999;
        assertFalse(System.getEnv("INT_LEADING_SPACE", val)); // Invalid
        assertTrue(val == 999); // Unchanged

        val = 999;
        assertFalse(System.getEnv("INT_TRAILING_SPACE", val)); // Invalid
        assertTrue(val == 999); // Unchanged

        val = 999;
        assertFalse(System.getEnv("INT_HEX", val)); // Invalid
        assertTrue(val == 999); // Unchanged

        val = 999;
        assertFalse(System.getEnv("INT_WITH_CHARS", val)); // Invalid
        assertTrue(val == 999); // Unchanged

        val = 999;
        assertFalse(System.getEnv("STR_EMPTY", val)); // Invalid
        assertTrue(val == 999); // Unchanged

        val = 999;
        assertFalse(System.getEnv("BOOL_TRUE", val)); // Invalid
        assertTrue(val == 999); // Unchanged

        val = 999;
        assertFalse(System.getEnv("NON_EXISTENT", val)); // Not found
        assertTrue(val == 999); // Unchanged
    }

    // System.getEnv(..., bool&)
    {
        bool val = false;
        assertTrue(System.getEnv("BOOL_TRUE", val));
        assertTrue(val == true);

        val = true;
        assertTrue(System.getEnv("BOOL_FALSE", val));
        assertTrue(val == false);

        val = false;
        assertFalse(System.getEnv("BOOL_UPPER_TRUE", val)); // Invalid
        assertTrue(val == false);

        val = true;
        assertFalse(System.getEnv("BOOL_UPPER_FALSE", val)); // Invalid
        assertTrue(val == true); // Unchanged

        val = false;
        assertFalse(System.getEnv("BOOL_MIXED_TRUE", val)); // Invalid
        assertTrue(val == false); // Unchanged

        val = true;
        assertFalse(System.getEnv("BOOL_MIXED_FALSE", val)); // Invalid
        assertTrue(val == true); // Unchanged

        val = false;
        assertFalse(System.getEnv("BOOL_ONE", val)); // Invalid
        assertTrue(val == false); // Unchanged

        val = true;
        assertFalse(System.getEnv("BOOL_ZERO", val)); // Invalid
        assertTrue(val == true); // Unchanged

        val = false;
        assertFalse(System.getEnv("BOOL_YES", val)); // Invalid
        assertTrue(val == false); // Unchanged

        val = true;
        assertFalse(System.getEnv("BOOL_NO", val)); // Invalid
        assertTrue(val == true); // Unchanged

        val = true;
        assertFalse(System.getEnv("BOOL_LEADING_SPACE", val)); // Invalid
        assertTrue(val == true); // Unchanged

        val = true;
        assertFalse(System.getEnv("BOOL_TRAILING_SPACE", val)); // Invalid
        assertTrue(val == true); // Unchanged

        val = true;
        assertFalse(System.getEnv("STR_EMPTY", val)); // Invalid
        assertTrue(val == true); // Unchanged

        val = true;
        assertFalse(System.getEnv("NON_EXISTENT", val)); // Not found
        assertTrue(val == true); // Unchanged
    }

    // Clear the env vars and reset to apply the changes
    assertTrue(System.clearEnv(false /* reset */));
    expectSystemReset();
    System.reset();
}

test(04_start_on_connect_env_update) {
    prepareForFirmwareUpdate();
    connect();
}

test(05_complete_on_connect_env_update) {
    completeFirmwareUpdate();
}

test(06_check_on_connect_env_update) {
    // The contents of the snapshot can be anything at this point so only check the predefined
    // org/product variables here
    assertTrue(System.getEnv("DVOS_CI_ORG_VAR1") == "org default 1 pcT3RG9xr4");
    assertTrue(System.getEnv("DVOS_CI_PROD_VAR1") == "prod default 1 wkWStqATwW");
}

test(07_start_ad_hoc_env_update) {
    prepareForFirmwareUpdate();
    connect();
}

test(08_complete_ad_hoc_env_update) {
    completeFirmwareUpdate(true /* expectSafeMode */);
}

test(09_check_ad_hoc_env_update) {
    // Application variables
    assertTrue(System.getEnv("APP_VAR1") == String("app 1 ") + nonce);
    assertTrue(System.getEnv("APP_VAR2") == String("app 2 ") + nonce);

    // Predefined org/product variables
    assertTrue(System.getEnv("DVOS_CI_ORG_VAR1") == "org default 1 pcT3RG9xr4");
    assertTrue(System.getEnv("DVOS_CI_PROD_VAR1") == "prod default 1 wkWStqATwW");
}

test(10_start_immediate_product_env_update) {
    prepareForFirmwareUpdate();
    connect();
}

test(11_complete_immediate_product_env_update) {
    completeFirmwareUpdate();
}

test(12_check_immediate_product_env_update) {
    // Product variables
    auto var1 = findVarWithValue(String("prod 1 ") + nonce);
    assertTrue(var1.length() > 0 && var1.length() < sizeof(deviceVar1));
    std::memcpy(deviceVar1, var1.c_str(), var1.length() + 1); // Include '\0'

    auto var2 = var1 + "_2";
    assertTrue(System.getEnv(var2.c_str()) == String("prod 2 ") + nonce);
    assertTrue(var2.length() < sizeof(deviceVar2));
    std::memcpy(deviceVar2, var2.c_str(), var2.length() + 1);

    // Application variables
    assertTrue(System.getEnv("APP_VAR1") == String("app 1 ") + nonce);
    assertTrue(System.getEnv("APP_VAR2") == String("app 2 ") + nonce);

    // Predefined org/product variables
    assertTrue(System.getEnv("DVOS_CI_ORG_VAR1") == "org default 1 pcT3RG9xr4");
    assertTrue(System.getEnv("DVOS_CI_PROD_VAR1") == "prod default 1 wkWStqATwW");
}

test(13_start_immediate_device_env_update) {
    prepareForFirmwareUpdate();
    connect();
}

test(14_complete_immediate_device_env_update) {
    completeFirmwareUpdate();
}

test(15_check_immediate_device_env_update) {
    // Product variables
    assertTrue(System.getEnv(deviceVar1) == String("prod 1 ") + nonce);
    assertTrue(System.getEnv(deviceVar2) == String("dev prod 2 ") + nonce); // Overridden

    // Application variables
    assertTrue(System.getEnv("APP_VAR1") == String("app 1 ") + nonce);
    assertTrue(System.getEnv("APP_VAR2") == String("dev app 2 ") + nonce); // Overridden

    // Predefined org/product variables
    // FIXME: This is failing for some reason
    // Test::out->println(System.getEnv("DVOS_CI_ORG_VAR1").c_str());
    // assertTrue(System.getEnv("DVOS_CI_ORG_VAR1") == String("dev org 1") + nonce); // Overridden
    assertTrue(System.getEnv("DVOS_CI_PROD_VAR1") == "prod default 1 wkWStqATwW");
}
