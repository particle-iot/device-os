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

String findVarWithValue(const String& val) {
	for (const auto& name: System.listEnvVars()) {
		if (System.envVar(name) == val) {
			return name;
		}
	}
	return String();
}

void firmwareUpdateEventHandler(system_event_t, int data, void*) {
	switch (data) {
    case firmware_update_begin:
    	firmwareUpdateStatus = FirmwareUpdateStatus::STARTED;
    	break;
    case firmware_update_complete:
    	firmwareUpdateStatus = FirmwareUpdateStatus::SUCCESS;
    	break;
    case firmware_update_progress:
    	break;
    default:
    	firmwareUpdateStatus = FirmwareUpdateStatus::ERROR;
    	break;
	}
}

void prepareForFirmwareUpdate() {
	firmwareUpdateStatus = FirmwareUpdateStatus::NONE;
    System.on(firmware_update, firmwareUpdateEventHandler);
    System.disableReset();
}

void completeFirmwareUpdate(bool expectSafeMode = false) {
	bool ok = false;
	auto t1 = millis();
	// The JS part of the test waits until the OTA completes so the timeout here is just for
	// finalizing the update on the device
	while (millis() - t1 < 10000) {
		if (firmwareUpdateStatus == FirmwareUpdateStatus::SUCCESS) {
			ok = true;
			break;
		}
		if (firmwareUpdateStatus == FirmwareUpdateStatus::ERROR) {
			break;
		}
	}
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
	assertMoreOrEqual(System.clearEnvVars(), 0);
	expectSystemReset();
	System.reset();
}

test(02_start_local_env_update) {
	// Validate that no env vars are defined
	Vector<const char*> vars;
	assertEqual(System.listEnvVars(vars), 0);
	assertEqual(vars.size(), 0);
}

test(03_check_local_env_update) {
	assertTrue(System.envVar("VAR1") == "abcdef");
	// TODO: Add the API tests from the Nick's branch here

	// Clear the env vars and reset to apply the changes
	assertMoreOrEqual(System.clearEnvVars(), 0);
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
	assertTrue(System.envVar("DVOS_CI_ORG_VAR1") == "org default 1 pcT3RG9xr4");
	assertTrue(System.envVar("DVOS_CI_PROD_VAR1") == "prod default 1 wkWStqATwW");
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
	assertTrue(System.envVar("APP_VAR1") == String("app 1 ") + nonce);
	assertTrue(System.envVar("APP_VAR2") == String("app 2 ") + nonce);

	// Predefined org/product variables
	assertTrue(System.envVar("DVOS_CI_ORG_VAR1") == "org default 1 pcT3RG9xr4");
	assertTrue(System.envVar("DVOS_CI_PROD_VAR1") == "prod default 1 wkWStqATwW");
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
	assertTrue(System.envVar(var2.c_str()) == String("prod 2 ") + nonce);
	assertTrue(var2.length() < sizeof(deviceVar2));
	std::memcpy(deviceVar2, var2.c_str(), var2.length() + 1);

	// Application variables
	assertTrue(System.envVar("APP_VAR1") == String("app 1 ") + nonce);
	assertTrue(System.envVar("APP_VAR2") == String("app 2 ") + nonce);

	// Predefined org/product variables
	assertTrue(System.envVar("DVOS_CI_ORG_VAR1") == "org default 1 pcT3RG9xr4");
	assertTrue(System.envVar("DVOS_CI_PROD_VAR1") == "prod default 1 wkWStqATwW");
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
	assertTrue(System.envVar(deviceVar1) == String("prod 1 ") + nonce);
	assertTrue(System.envVar(deviceVar2) == String("dev prod 2 ") + nonce); // Overridden

	// Application variables
	assertTrue(System.envVar("APP_VAR1") == String("app 1 ") + nonce);
	assertTrue(System.envVar("APP_VAR2") == String("dev app 2 ") + nonce); // Overridden

	// Predefined org/product variables
	assertTrue(System.envVar("DVOS_CI_ORG_VAR1") == String("dev org 1") + nonce); // Overridden
	assertTrue(System.envVar("DVOS_CI_PROD_VAR1") == "prod default 1 wkWStqATwW");
}
