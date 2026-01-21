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
	// Original string values
	assertTrue(System.envVar("APP_VAR1") == "abcdef");
	assertTrue(System.envVar("APP_VAR2") == "123");
	assertTrue(System.envVar("APP_VAR3") == "0");
	assertTrue(System.envVar("APP_VAR4") == "true");
	assertTrue(System.envVar("APP_VAR5") == "false");

	auto randVar1 = System.envVar("RAND_VAR1");
	assertEqual(randVar1.length(), 30);
	assertTrue(System.envVar("RAND_VAR2") == randVar1);

	// Int conversion with default value
	assertTrue(System.envVar("APP_VAR1", -1) == -1); // "abcdef" -> invalid int, return default
	assertTrue(System.envVar("APP_VAR2", -2) == 123); // "123" -> valid int
	assertTrue(System.envVar("APP_VAR3", -3) == 0);   // "0" -> valid int
	assertTrue(System.envVar("APP_VAR4", -4) == -4);  // "true" -> invalid int, return default
	assertTrue(System.envVar("APP_VAR5", -5) == -5);  // "false" -> invalid int, return default

	// Bool conversion with default value
	// Only "true" and "false" (case-sensitive, lowercase) are valid bools
	assertTrue(System.envVar("APP_VAR1", true) == true);   // "abcdef" -> invalid, return default
	assertTrue(System.envVar("APP_VAR1", false) == false); // "abcdef" -> invalid, return default
	assertTrue(System.envVar("APP_VAR2", false) == false); // "123" -> invalid bool (not "true"/"false"), return default
	assertTrue(System.envVar("APP_VAR3", true) == true);   // "0" -> invalid bool (not "true"/"false"), return default
	assertTrue(System.envVar("APP_VAR4", false) == true);  // "true" -> valid bool
	assertTrue(System.envVar("APP_VAR5", true) == false);  // "false" -> valid bool

	// New reference-based API: bool envVar(name, bool& value)
	// Returns true if found AND valid, only modifies value if valid
	{
		bool val = true;
		assertTrue(System.envVar("APP_VAR4", val));  // "true" -> valid
		assertTrue(val == true);

		val = true;
		assertTrue(System.envVar("APP_VAR5", val));  // "false" -> valid
		assertTrue(val == false);

		val = true;
		assertFalse(System.envVar("APP_VAR1", val)); // "abcdef" -> invalid
		assertTrue(val == true); // Unchanged

		val = false;
		assertFalse(System.envVar("APP_VAR2", val)); // "123" -> invalid (not "true"/"false")
		assertTrue(val == false); // Unchanged

		val = true;
		assertFalse(System.envVar("NONEXISTENT", val)); // Not found
		assertTrue(val == true); // Unchanged
	}

	// New reference-based API: bool envVar(name, int& value)
	// Returns true if found AND valid, only modifies value if valid
	{
		int val = 999;
		assertTrue(System.envVar("APP_VAR2", val));  // "123" -> valid
		assertTrue(val == 123);

		val = 999;
		assertTrue(System.envVar("APP_VAR3", val));  // "0" -> valid
		assertTrue(val == 0);

		val = 999;
		assertFalse(System.envVar("APP_VAR1", val)); // "abcdef" -> invalid
		assertTrue(val == 999); // Unchanged

		val = 999;
		assertFalse(System.envVar("APP_VAR4", val)); // "true" -> invalid int
		assertTrue(val == 999); // Unchanged

		val = 999;
		assertFalse(System.envVar("NONEXISTENT", val)); // Not found
		assertTrue(val == 999); // Unchanged
	}

	// New reference-based API: bool envVar(name, String& value)
	// Returns true if found, only modifies value if found
	{
		String val = "default";
		assertTrue(System.envVar("APP_VAR1", val));
		assertTrue(val == "abcdef");

		val = "default";
		assertTrue(System.envVar("APP_VAR4", val));
		assertTrue(val == "true");

		val = "default";
		assertFalse(System.envVar("NONEXISTENT", val)); // Not found
		assertTrue(val == "default"); // Unchanged
	}

	// Bool case sensitivity tests - only lowercase "true"/"false" are valid
	{
		bool val = false;
		assertFalse(System.envVar("BOOL_UPPER_TRUE", val));  // "TRUE" -> invalid
		assertTrue(val == false); // Unchanged

		val = true;
		assertFalse(System.envVar("BOOL_UPPER_FALSE", val)); // "FALSE" -> invalid
		assertTrue(val == true); // Unchanged

		val = false;
		assertFalse(System.envVar("BOOL_MIXED_TRUE", val));  // "True" -> invalid
		assertTrue(val == false); // Unchanged

		val = true;
		assertFalse(System.envVar("BOOL_MIXED_FALSE", val)); // "False" -> invalid
		assertTrue(val == true); // Unchanged
	}

	// Bool invalid value tests - numeric and other common alternatives not valid
	{
		bool val = false;
		assertFalse(System.envVar("BOOL_ONE", val));  // "1" -> invalid
		assertTrue(val == false); // Unchanged

		val = true;
		assertFalse(System.envVar("BOOL_ZERO", val)); // "0" -> invalid
		assertTrue(val == true); // Unchanged

		val = false;
		assertFalse(System.envVar("BOOL_YES", val));  // "yes" -> invalid
		assertTrue(val == false); // Unchanged

		val = true;
		assertFalse(System.envVar("BOOL_NO", val));   // "no" -> invalid
		assertTrue(val == true); // Unchanged

		val = false;
		assertFalse(System.envVar("BOOL_LEADING_SPACE", val));  // " true" -> invalid
		assertTrue(val == false); // Unchanged

		val = false;
		assertFalse(System.envVar("BOOL_TRAILING_SPACE", val)); // "true " -> invalid
		assertTrue(val == false); // Unchanged
	}

	// Int edge case tests
	{
		int val = 999;

		// Valid negative
		assertTrue(System.envVar("INT_NEGATIVE", val));  // "-456" -> valid
		assertTrue(val == -456);

		// Valid INT_MAX
		val = 999;
		assertTrue(System.envVar("INT_MAX", val));  // "2147483647" -> valid
		assertTrue(val == 2147483647);

		// Valid INT_MIN
		val = 999;
		assertTrue(System.envVar("INT_MIN", val));  // "-2147483648" -> valid
		assertTrue(val == (-2147483647 - 1)); // INT_MIN

		// Invalid: overflow
		val = 999;
		assertFalse(System.envVar("INT_OVERFLOW", val));  // "2147483648" -> overflow
		assertTrue(val == 999); // Unchanged

		// Invalid: underflow
		val = 999;
		assertFalse(System.envVar("INT_UNDERFLOW", val)); // "-2147483649" -> underflow
		assertTrue(val == 999); // Unchanged

		// Invalid: float
		val = 999;
		assertFalse(System.envVar("INT_FLOAT", val));  // "12.34" -> invalid
		assertTrue(val == 999); // Unchanged

		// Invalid: leading space
		val = 999;
		assertFalse(System.envVar("INT_LEADING_SPACE", val)); // " 123" -> invalid
		assertTrue(val == 999); // Unchanged

		// Invalid: trailing space
		val = 999;
		assertFalse(System.envVar("INT_TRAILING_SPACE", val)); // "123 " -> invalid
		assertTrue(val == 999); // Unchanged

		// Invalid: hex prefix
		val = 999;
		assertFalse(System.envVar("INT_HEX", val)); // "0x1F" -> invalid
		assertTrue(val == 999); // Unchanged

		// Invalid: trailing characters
		val = 999;
		assertFalse(System.envVar("INT_WITH_CHARS", val)); // "123abc" -> invalid
		assertTrue(val == 999); // Unchanged

		// Valid: leading zeros (parsed as decimal)
		val = 999;
		assertTrue(System.envVar("INT_LEADING_ZEROS", val)); // "00123" -> valid, parsed as 123
		assertTrue(val == 123);

		// Valid: negative zero
		val = 999;
		assertTrue(System.envVar("INT_NEGATIVE_ZERO", val)); // "-0" -> valid, parsed as 0
		assertTrue(val == 0);

		// Invalid: explicit plus sign
		val = 999;
		assertFalse(System.envVar("INT_PLUS_SIGN", val)); // "+123" -> invalid
		assertTrue(val == 999); // Unchanged
	}

	// Empty string tests
	{
		String strVal = "default";
		assertTrue(System.envVar("EMPTY_VAR", strVal)); // Empty string is still "found"
		assertTrue(strVal == ""); // Value is empty string

		bool boolVal = true;
		assertFalse(System.envVar("EMPTY_VAR", boolVal)); // Empty is not valid bool
		assertTrue(boolVal == true); // Unchanged

		int intVal = 999;
		assertFalse(System.envVar("EMPTY_VAR", intVal)); // Empty is not valid int
		assertTrue(intVal == 999); // Unchanged
	}

	// Test listEnvVars with Vector& parameter (returns error code)
	{
		Vector<const char*> namesVec;
		int result = System.listEnvVars(namesVec);
		assertEqual(result, 0); // Should return 0 on success
		// Count: 5 APP_VAR + 2 RAND_VAR + 4 BOOL_CASE + 6 BOOL_INVALID + 13 INT + 1 EMPTY = 31 total
		assertEqual(namesVec.size(), 31);
	}

	// Variable names - using convenience method
	Vector<String> names;
	for (const auto& name: System.listEnvVars()) {
		names.append(name); // const char* -> String
	}
	// Count: 5 APP_VAR + 2 RAND_VAR + 4 BOOL_CASE + 6 BOOL_INVALID + 13 INT + 1 EMPTY = 31 total
	assertEqual(names.size(), 31);
	// Original vars
	assertTrue(names.contains("APP_VAR1"));
	assertTrue(names.contains("APP_VAR2"));
	assertTrue(names.contains("APP_VAR3"));
	assertTrue(names.contains("APP_VAR4"));
	assertTrue(names.contains("APP_VAR5"));
	assertTrue(names.contains("RAND_VAR1"));
	assertTrue(names.contains("RAND_VAR2"));
	// Bool case sensitivity vars
	assertTrue(names.contains("BOOL_UPPER_TRUE"));
	assertTrue(names.contains("BOOL_UPPER_FALSE"));
	assertTrue(names.contains("BOOL_MIXED_TRUE"));
	assertTrue(names.contains("BOOL_MIXED_FALSE"));
	// Bool invalid value vars
	assertTrue(names.contains("BOOL_ONE"));
	assertTrue(names.contains("BOOL_ZERO"));
	assertTrue(names.contains("BOOL_YES"));
	assertTrue(names.contains("BOOL_NO"));
	assertTrue(names.contains("BOOL_LEADING_SPACE"));
	assertTrue(names.contains("BOOL_TRAILING_SPACE"));
	// Int edge case vars
	assertTrue(names.contains("INT_NEGATIVE"));
	assertTrue(names.contains("INT_MAX"));
	assertTrue(names.contains("INT_MIN"));
	assertTrue(names.contains("INT_OVERFLOW"));
	assertTrue(names.contains("INT_UNDERFLOW"));
	assertTrue(names.contains("INT_FLOAT"));
	assertTrue(names.contains("INT_LEADING_SPACE"));
	assertTrue(names.contains("INT_TRAILING_SPACE"));
	assertTrue(names.contains("INT_HEX"));
	assertTrue(names.contains("INT_WITH_CHARS"));
	assertTrue(names.contains("INT_LEADING_ZEROS"));
	assertTrue(names.contains("INT_NEGATIVE_ZERO"));
	assertTrue(names.contains("INT_PLUS_SIGN"));
	// Empty var
	assertTrue(names.contains("EMPTY_VAR"));

	// hasEnvVar tests
	assertTrue(System.hasEnvVar("APP_VAR1"));
	assertTrue(System.hasEnvVar("APP_VAR2"));
	assertTrue(System.hasEnvVar("APP_VAR3"));
	assertTrue(System.hasEnvVar("APP_VAR4"));
	assertTrue(System.hasEnvVar("APP_VAR5"));
	assertFalse(System.hasEnvVar("APP_VAR6")); // Not defined
	assertFalse(System.hasEnvVar("NONEXISTENT")); // Not defined
	assertTrue(System.hasEnvVar("RAND_VAR1"));
	assertTrue(System.hasEnvVar("RAND_VAR2"));
	assertTrue(System.hasEnvVar("EMPTY_VAR")); // Empty value still exists
}
