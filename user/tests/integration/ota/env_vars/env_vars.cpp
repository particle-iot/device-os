#include "application.h"

#include "system_env.h"
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
	assertMoreOrEqual(system_clear_env(nullptr /* reserved */), 0);
	expectSystemReset();
	System.reset();
}

test(02_start_local_env_update) {
	// Validate that no env vars are defined
	auto vars = System.listEnv();
	assertEqual(vars.size(), 0);
}

test(03_check_local_env_update) {
	// Original string values
	assertTrue(System.getEnv("APP_VAR1") == "abcdef");
	assertTrue(System.getEnv("APP_VAR2") == "123");
	assertTrue(System.getEnv("APP_VAR3") == "0");
	assertTrue(System.getEnv("APP_VAR4") == "true");
	assertTrue(System.getEnv("APP_VAR5") == "false");

	auto randVar1 = System.getEnv("RAND_VAR1");
	assertEqual(randVar1.length(), 30);
	assertTrue(System.getEnv("RAND_VAR2") == randVar1);

#if 0
	// Int conversion with default value
	assertTrue(System.getEnv("APP_VAR1", -1) == -1); // "abcdef" -> invalid int, return default
	assertTrue(System.getEnv("APP_VAR2", -2) == 123); // "123" -> valid int
	assertTrue(System.getEnv("APP_VAR3", -3) == 0);   // "0" -> valid int
	assertTrue(System.getEnv("APP_VAR4", -4) == -4);  // "true" -> invalid int, return default
	assertTrue(System.getEnv("APP_VAR5", -5) == -5);  // "false" -> invalid int, return default

	// Bool conversion with default value
	// Only "true" and "false" (case-sensitive, lowercase) are valid bools
	assertTrue(System.getEnv("APP_VAR1", true) == true);   // "abcdef" -> invalid, return default
	assertTrue(System.getEnv("APP_VAR1", false) == false); // "abcdef" -> invalid, return default
	assertTrue(System.getEnv("APP_VAR2", false) == false); // "123" -> invalid bool (not "true"/"false"), return default
	assertTrue(System.getEnv("APP_VAR3", true) == true);   // "0" -> invalid bool (not "true"/"false"), return default
	assertTrue(System.getEnv("APP_VAR4", false) == true);  // "true" -> valid bool
	assertTrue(System.getEnv("APP_VAR5", true) == false);  // "false" -> valid bool
#endif

	// New reference-based API: bool getEnv(name, bool& value)
	// Returns true if found AND valid, only modifies value if valid
	{
		bool val = true;
		assertTrue(System.getEnv("APP_VAR4", val));  // "true" -> valid
		assertTrue(val == true);

		val = true;
		assertTrue(System.getEnv("APP_VAR5", val));  // "false" -> valid
		assertTrue(val == false);

		val = true;
		assertFalse(System.getEnv("APP_VAR1", val)); // "abcdef" -> invalid
		assertTrue(val == true); // Unchanged

		val = false;
		assertFalse(System.getEnv("APP_VAR2", val)); // "123" -> invalid (not "true"/"false")
		assertTrue(val == false); // Unchanged

		val = true;
		assertFalse(System.getEnv("NONEXISTENT", val)); // Not found
		assertTrue(val == true); // Unchanged
	}

	// New reference-based API: bool getEnv(name, int& value)
	// Returns true if found AND valid, only modifies value if valid
	{
		int val = 999;
		assertTrue(System.getEnv("APP_VAR2", val));  // "123" -> valid
		assertTrue(val == 123);

		val = 999;
		assertTrue(System.getEnv("APP_VAR3", val));  // "0" -> valid
		assertTrue(val == 0);

		val = 999;
		assertFalse(System.getEnv("APP_VAR1", val)); // "abcdef" -> invalid
		assertTrue(val == 999); // Unchanged

		val = 999;
		assertFalse(System.getEnv("APP_VAR4", val)); // "true" -> invalid int
		assertTrue(val == 999); // Unchanged

		val = 999;
		assertFalse(System.getEnv("NONEXISTENT", val)); // Not found
		assertTrue(val == 999); // Unchanged
	}

	// New reference-based API: bool getEnv(name, String& value)
	// Returns true if found, only modifies value if found
	{
		String val = "default";
		assertTrue(System.getEnv("APP_VAR1", val));
		assertTrue(val == "abcdef");

		val = "default";
		assertTrue(System.getEnv("APP_VAR4", val));
		assertTrue(val == "true");

		val = "default";
		assertFalse(System.getEnv("NONEXISTENT", val)); // Not found
		assertTrue(val == "default"); // Unchanged
	}

	// Bool case sensitivity tests - only lowercase "true"/"false" are valid
	{
		bool val = false;
		assertFalse(System.getEnv("BOOL_UPPER_TRUE", val));  // "TRUE" -> invalid
		assertTrue(val == false); // Unchanged

		val = true;
		assertFalse(System.getEnv("BOOL_UPPER_FALSE", val)); // "FALSE" -> invalid
		assertTrue(val == true); // Unchanged

		val = false;
		assertFalse(System.getEnv("BOOL_MIXED_TRUE", val));  // "True" -> invalid
		assertTrue(val == false); // Unchanged

		val = true;
		assertFalse(System.getEnv("BOOL_MIXED_FALSE", val)); // "False" -> invalid
		assertTrue(val == true); // Unchanged
	}

	// Bool invalid value tests - numeric and other common alternatives not valid
	{
		bool val = false;
		assertFalse(System.getEnv("BOOL_ONE", val));  // "1" -> invalid
		assertTrue(val == false); // Unchanged

		val = true;
		assertFalse(System.getEnv("BOOL_ZERO", val)); // "0" -> invalid
		assertTrue(val == true); // Unchanged

		val = false;
		assertFalse(System.getEnv("BOOL_YES", val));  // "yes" -> invalid
		assertTrue(val == false); // Unchanged

		val = true;
		assertFalse(System.getEnv("BOOL_NO", val));   // "no" -> invalid
		assertTrue(val == true); // Unchanged

		val = false;
		assertFalse(System.getEnv("BOOL_LEADING_SPACE", val));  // " true" -> invalid
		assertTrue(val == false); // Unchanged

		val = false;
		assertFalse(System.getEnv("BOOL_TRAILING_SPACE", val)); // "true " -> invalid
		assertTrue(val == false); // Unchanged
	}

	// Int edge case tests
	{
		int val = 999;

		// Valid negative
		assertTrue(System.getEnv("INT_NEGATIVE", val));  // "-456" -> valid
		assertTrue(val == -456);

		// Valid INT_MAX
		val = 999;
		assertTrue(System.getEnv("INT_MAX", val));  // "2147483647" -> valid
		assertTrue(val == 2147483647);

		// Valid INT_MIN
		val = 999;
		assertTrue(System.getEnv("INT_MIN", val));  // "-2147483648" -> valid
		assertTrue(val == (-2147483647 - 1)); // INT_MIN

		// Invalid: overflow
		val = 999;
		assertFalse(System.getEnv("INT_OVERFLOW", val));  // "2147483648" -> overflow
		assertTrue(val == 999); // Unchanged

		// Invalid: underflow
		val = 999;
		assertFalse(System.getEnv("INT_UNDERFLOW", val)); // "-2147483649" -> underflow
		assertTrue(val == 999); // Unchanged

		// Invalid: float
		val = 999;
		assertFalse(System.getEnv("INT_FLOAT", val));  // "12.34" -> invalid
		assertTrue(val == 999); // Unchanged

		// Invalid: leading space
		val = 999;
		assertFalse(System.getEnv("INT_LEADING_SPACE", val)); // " 123" -> invalid
		assertTrue(val == 999); // Unchanged

		// Invalid: trailing space
		val = 999;
		assertFalse(System.getEnv("INT_TRAILING_SPACE", val)); // "123 " -> invalid
		assertTrue(val == 999); // Unchanged

		// Invalid: hex prefix
		val = 999;
		assertFalse(System.getEnv("INT_HEX", val)); // "0x1F" -> invalid
		assertTrue(val == 999); // Unchanged

		// Invalid: trailing characters
		val = 999;
		assertFalse(System.getEnv("INT_WITH_CHARS", val)); // "123abc" -> invalid
		assertTrue(val == 999); // Unchanged

		// Valid: leading zeros (parsed as decimal)
		val = 999;
		assertTrue(System.getEnv("INT_LEADING_ZEROS", val)); // "00123" -> valid, parsed as 123
		assertTrue(val == 123);

		// Valid: negative zero
		val = 999;
		assertTrue(System.getEnv("INT_NEGATIVE_ZERO", val)); // "-0" -> valid, parsed as 0
		assertTrue(val == 0);

		// Invalid: explicit plus sign
		val = 999;
		assertFalse(System.getEnv("INT_PLUS_SIGN", val)); // "+123" -> invalid
		assertTrue(val == 999); // Unchanged
	}

	// Empty string tests
	{
		String strVal = "default";
		assertTrue(System.getEnv("EMPTY_VAR", strVal)); // Empty string is still "found"
		assertTrue(strVal == ""); // Value is empty string

		bool boolVal = true;
		assertFalse(System.getEnv("EMPTY_VAR", boolVal)); // Empty is not valid bool
		assertTrue(boolVal == true); // Unchanged

		int intVal = 999;
		assertFalse(System.getEnv("EMPTY_VAR", intVal)); // Empty is not valid int
		assertTrue(intVal == 999); // Unchanged
	}

	// Variable names
	Vector<String> names;
	for (const auto& name: System.listEnv()) {
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

	// hasEnv tests
	assertTrue(System.hasEnv("APP_VAR1"));
	assertTrue(System.hasEnv("APP_VAR2"));
	assertTrue(System.hasEnv("APP_VAR3"));
	assertTrue(System.hasEnv("APP_VAR4"));
	assertTrue(System.hasEnv("APP_VAR5"));
	assertFalse(System.hasEnv("APP_VAR6")); // Not defined
	assertFalse(System.hasEnv("NONEXISTENT")); // Not defined
	assertTrue(System.hasEnv("RAND_VAR1"));
	assertTrue(System.hasEnv("RAND_VAR2"));
	assertTrue(System.hasEnv("EMPTY_VAR")); // Empty value still exists

	// Clear the env vars and reset to apply the changes
	assertEqual(system_clear_env(nullptr /* reserved */), SYSTEM_ENV_NEED_RESET);
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
	assertTrue(System.getEnv("DVOS_CI_ORG_VAR1") == String("dev org 1") + nonce); // Overridden
	assertTrue(System.getEnv("DVOS_CI_PROD_VAR1") == "prod default 1 wkWStqATwW");
}
