#include "application.h"
#include "unit-test/unit-test.h"
#include "i2c_helper.h"

test(0_I2C_scanBus) {
    i2c::reset();

    uint32_t ts1 = 0, ts2 = 0;
    // Scan I2C bus
    for (uint8_t addr = 0x01; addr < 0x7f; addr++) {
        ts1 = millis();
        USE_WIRE.beginTransmission(addr);
        int32_t err = USE_WIRE.endTransmission();
        ts2 = millis();

        if (err == 0) {
            i2c::devices.append(addr);
            // Test::out->printlnf("I2C device @ 0x%02x", addr);
            Log.trace("I2C device @ 0x%02x", addr);
        } else if (err == 0x03) {
            // No one ACKed the address
            // Check that this doesn't cause standard 100ms delay in I2C HAL
            assertMoreOrEqual(ts2, ts1);
            assertLessOrEqual(ts2 - ts1, 50);
            // Test::out->printlnf("err1 @ 0x%02x %ld", addr, err);
            Log.trace("err1 @ 0x%02x %ld", addr, err);
        } else {
            // Test::out->printlnf("err2 @ 0x%02x %ld", addr, err);
            Log.trace("err2 @ 0x%02x %ld", addr, err);
        }
        // Test::out->printlnf("addr 0x%02x", addr);
        Log.trace("addr 0x%02x", addr);
        // assertEqual(addr, 19);
    }

    assertEqual(i2c::errorCount, 0);
}
