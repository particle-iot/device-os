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

retained char testNonce[32] = {};
auto firmwareUpdateStatus = FirmwareUpdateStatus::NONE;

bool hasEnvVarWithValue(const String& val) {
	for (const auto& name: System.listEnvVars()) {
		if (System.envVar(name) == val) {
			return true;
		}
	}
	return false;
}

void firmwareUpdateEventHandler(system_event_t, int data, void*) {
	switch (data) {
    case firmware_update_begin:
    	firmwareUpdateStatus = FirmwareUpdateStatus::STARTED;
    	break;
    case firmware_update_complete:
    	firmwareUpdateStatus = FirmwareUpdateStatus::SUCCESS;
    	break;
    case (int)firmware_update_failed: // Defined as (uint32_t)-1 in system_event.h
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
	while (millis() - t1 < 5000) {
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
	rand.genBase32(testNonce, sizeof(testNonce) - 1);
	testNonce[sizeof(testNonce) - 1] = '\0';
	pushMailboxMsg(testNonce, 5000 /* wait */);

	// Clear the env vars and reset to apply the changes
	assertMoreOrEqual(System.clearEnvVars(), 0);
	expectSystemReset();
	System.reset();
}

test(02_start_env_vars_local_update) {
	// Validate that no env vars are defined
	Vector<const char*> vars;
	assertEqual(System.listEnvVars(vars), 0);
	assertEqual(vars.size(), 0);
}

test(03_check_env_vars_local_update) {
	assertTrue(System.envVar("APP_VAR1") == "abcdef");
	// TODO: Add the API tests from the Nick's branch here

	// Clear the env vars and reset to apply the changes
	assertMoreOrEqual(System.clearEnvVars(), 0);
	expectSystemReset();
	System.reset();
}

test(04_start_env_vars_on_connect_update) {
	prepareForFirmwareUpdate();
	connect();
}

test(05_complete_env_vars_on_connect_update) {
	completeFirmwareUpdate();
}

test(06_check_env_vars_on_connect_update) {
	assertTrue(System.envVar("DVOS_CI_ORG_VAR1") == "pcT3RG9xr4");
	assertTrue(System.envVar("DVOS_CI_PROD_VAR1") == "wkWStqATwW");
}

test(07_start_env_vars_ad_hoc_update) {
    prepareForFirmwareUpdate();
    connect();
}

test(08_complete_env_vars_ad_hoc_update) {
	completeFirmwareUpdate(true /* expectSafeMode */);
}

test(09_check_env_vars_ad_hoc_update) {
	assertTrue(System.envVar("APP_VAR1") == "abcde");
	assertTrue(System.envVar("APP_VAR2") == "123");
	assertTrue(System.envVar("APP_VAR3") == testNonce);
}

test(10_start_product_env_vars_immediate_update) {
	prepareForFirmwareUpdate();
	connect();
}

test(11_complete_product_env_vars_immediate_update) {
	completeFirmwareUpdate();
}

test(12_check_product_env_vars_immediate_update) {
	assertTrue(hasEnvVarWithValue(String(testNonce) + "_prod"));
}

test(13_start_device_env_vars_immediate_update) {
	prepareForFirmwareUpdate();
	connect();
}

test(14_complete_device_env_vars_immediate_update) {
	completeFirmwareUpdate();
}

test(15_check_device_env_vars_immediate_update) {
	assertTrue(System.envVar("DEV_VAR1") == String(testNonce) + "_dev");
}
