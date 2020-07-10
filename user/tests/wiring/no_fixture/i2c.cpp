
#include "application.h"
#include "unit-test/unit-test.h"
#include "scope_guard.h"

#if PLATFORM_ID == PLATFORM_TRACKER

test(I2C_01_Wire3_Cannot_Be_Enabled_While_Wire_Is_Enabled) {
    Wire.begin();
    assertTrue(Wire.isEnabled());
    // Wire3 cannot be enabled while Wire is enabled.
    Wire3.begin();
    assertFalse(Wire3.isEnabled());

    Wire.end();
    assertFalse(Wire.isEnabled());
    Wire3.begin();
    assertTrue(Wire3.isEnabled());
    Wire3.end();
    assertFalse(Wire3.isEnabled());
}

test(I2C_02_Wire_Cannot_Be_Enabled_While_Wire3_Is_Enabled) {
    Wire3.begin();
    assertTrue(Wire3.isEnabled());
    // Wire cannot be enabled while Wire3 is enabled.
    Wire.begin();
    assertFalse(Wire.isEnabled());

    Wire3.end();
    assertFalse(Wire3.isEnabled());
    Wire.begin();
    assertTrue(Wire.isEnabled());
    Wire.end();
    assertFalse(Wire.isEnabled());
}

test(I2C_03_Wire3_Cannot_Be_Enabled_While_Serial1_Is_Enabled) {
    Serial1.begin(115200);
    assertTrue(Serial1.isEnabled());
    // Wire3 cannot be enabled while Serial1 is enabled.
    Wire3.begin();
    assertFalse(Wire3.isEnabled());

    Serial1.end();
    assertFalse(Serial1.isEnabled());
    Wire3.begin();
    assertTrue(Wire3.isEnabled());
    Wire3.end();
    assertFalse(Wire3.isEnabled());
}

test(I2C_04_Serial1_Cannot_Be_Enabled_While_Wire3_Is_Enabled) {
    Wire3.begin();
    assertTrue(Wire3.isEnabled());
    // Serial1 cannot be enabled while Wire3 is enabled.
    Serial1.begin(115200);
    assertFalse(Serial1.isEnabled());

    Wire3.end();
    assertFalse(Wire3.isEnabled());
    Serial1.begin(115200);
    assertTrue(Serial1.isEnabled());
    Serial1.end();
    assertFalse(Serial1.isEnabled());
}

#endif // PLATFORM_ID == PLATFORM_TRACKER

test(I2C_05_Hal_Sleep_API_Test) {
    hal_i2c_config_t config = acquireWireBuffer();
    assertEqual(hal_i2c_init(HAL_I2C_INTERFACE1, &config), (int)SYSTEM_ERROR_NONE);

    // Suspend and resotre I2C
    hal_i2c_begin(HAL_I2C_INTERFACE1, I2C_MODE_MASTER, 0x00, nullptr);
    assertEqual(hal_i2c_sleep(HAL_I2C_INTERFACE1, true, nullptr), (int)SYSTEM_ERROR_NONE);  // Suspend
    assertFalse(hal_i2c_is_enabled(HAL_I2C_INTERFACE1, nullptr));
    assertEqual(hal_i2c_sleep(HAL_I2C_INTERFACE1, false, nullptr), (int)SYSTEM_ERROR_NONE); // Restore
    assertTrue(hal_i2c_is_enabled(HAL_I2C_INTERFACE1, nullptr));

    // Retore API should not re-initialize the disabled I2C
    hal_i2c_begin(HAL_I2C_INTERFACE1, I2C_MODE_MASTER, 0x00, nullptr);
    hal_i2c_end(HAL_I2C_INTERFACE1, nullptr);
    assertEqual(hal_i2c_sleep(HAL_I2C_INTERFACE1, false, nullptr), (int)SYSTEM_ERROR_INVALID_STATE);
}

#if HAL_PLATFORM_FUELGAUGE_MAX17043
test(I2C_06_I2c_Sleep_FuelGauge) {
    FuelGauge fuel(true);
    auto ver = fuel.getVersion();
    assertNotEqual(ver, 0x0000);
    assertNotEqual(ver, 0xffff);
    assertEqual(hal_i2c_sleep(HAL_PLATFORM_FUELGAUGE_MAX17043_I2C, true, nullptr), 0);
    SCOPE_GUARD({
        if (!hal_i2c_is_enabled(HAL_PLATFORM_FUELGAUGE_MAX17043_I2C, nullptr)) {
            hal_i2c_sleep(HAL_PLATFORM_FUELGAUGE_MAX17043_I2C, false, nullptr);
        }
    });
    assertFalse(hal_i2c_is_enabled(HAL_PLATFORM_FUELGAUGE_MAX17043_I2C, nullptr));
    assertEqual(hal_i2c_sleep(HAL_PLATFORM_FUELGAUGE_MAX17043_I2C, false, nullptr), 0);
    assertTrue(hal_i2c_is_enabled(HAL_PLATFORM_FUELGAUGE_MAX17043_I2C, nullptr));
    auto ver2 = fuel.getVersion();
    assertEqual(ver, ver2);
}
#endif // HAL_PLATFORM_FUELGAUGE_MAX17043