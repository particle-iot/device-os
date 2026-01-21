#include "application.h"
#include "unit-test/unit-test.h"

namespace {

using namespace particle;

enum class FirmwareUpdateStatus {
	NONE,
	STARTED,
	SUCCESS,
	ERROR
};

auto firmwareUpdateStatus = FirmwareUpdateStatus::NONE;

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

} // namespace

test(01_clear_and_reset) {
	assertMoreOrEqual(System.clearEnvVars(), 0);

	expectSystemReset();
	System.reset();
}

test(02_init_and_connect) {
	Vector<const char*> vars;
	assertEqual(System.listEnvVars(vars), 0);
	assertEqual(vars.size(), 0);

	Particle.connect();
	assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
}

test(03_start_app_env_vars_update) {
    prepareForFirmwareUpdate();
}

test(04_complete_app_env_vars_update) {
	completeFirmwareUpdate(true /* expectSafeMode */);
}

test(05_check_app_env_vars_update) {
	// Original values
	assertTrue(System.envVar("APP_VAR1") == "abcdef");
	assertTrue(System.envVar("APP_VAR2") == "123");
	assertTrue(System.envVar("APP_VAR3") == "0");
	assertTrue(System.envVar("APP_VAR4") == "true");
	assertTrue(System.envVar("APP_VAR5") == "false");

	auto randVar1 = System.envVar("RAND_VAR1");
	assertEqual(randVar1.length(), 30);
	assertTrue(System.envVar("RAND_VAR2") == randVar1);

	// Type conversions
	assertTrue(System.envVar("APP_VAR1", -1) == -1); // Conversion error
	assertTrue(System.envVar("APP_VAR2", -2) == 123);
	assertTrue(System.envVar("APP_VAR3", -3) == 0);
	assertTrue(System.envVar("APP_VAR4", -4) == -4); // Error
	assertTrue(System.envVar("APP_VAR5", -5) == -5); // Error

	assertTrue(System.envVar("APP_VAR1", true) == true); // Error
	assertTrue(System.envVar("APP_VAR1", false) == false); // Error
	assertTrue(System.envVar("APP_VAR2", false) == true);
	assertTrue(System.envVar("APP_VAR3", true) == false);
	assertTrue(System.envVar("APP_VAR4", false) == true);
	assertTrue(System.envVar("APP_VAR5", true) == false);

	// Variable names
	Vector<String> names;
	for (const auto& name: System.listEnvVars()) {
		names.append(name); // const char* -> String
	}
	assertEqual(names.size(), 7);
	assertTrue(names.contains("APP_VAR1"));
	assertTrue(names.contains("APP_VAR2"));
	assertTrue(names.contains("APP_VAR3"));
	assertTrue(names.contains("APP_VAR4"));
	assertTrue(names.contains("APP_VAR5"));
	assertTrue(names.contains("RAND_VAR1"));
	assertTrue(names.contains("RAND_VAR2"));

	assertTrue(System.hasEnvVar("APP_VAR1"));
	assertTrue(System.hasEnvVar("APP_VAR2"));
	assertTrue(System.hasEnvVar("APP_VAR3"));
	assertTrue(System.hasEnvVar("APP_VAR4"));
	assertTrue(System.hasEnvVar("APP_VAR5"));
	assertFalse(System.hasEnvVar("APP_VAR6")); // Not defined
	assertTrue(System.hasEnvVar("RAND_VAR1"));
	assertTrue(System.hasEnvVar("RAND_VAR2"));
}
