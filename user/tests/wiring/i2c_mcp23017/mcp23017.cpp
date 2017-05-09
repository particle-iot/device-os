#include "application.h"
#include "unit-test/unit-test.h"
#include "i2c_helper.h"
#include "interferer.h"
#include <algorithm>

#define MCP23017_BASE_ADDRESS 0x20

static bool s_skip = false;
static void mcp23017_looped_test();
static uint8_t s_mcpAddress = 0x00;

#define LOOP_COUNT 10000

externTest(0_I2C_scanBus);

test(1_I2C_MCP23017_doRequestControlNoStopReadNoStop) {
    assertTestPass(0_I2C_scanBus);

    // Check whether we have found the MCP23017
    for (uint8_t addr = MCP23017_BASE_ADDRESS; addr <= MCP23017_BASE_ADDRESS + 0b111; addr++) {
        if (std::find(i2c::devices.begin(), i2c::devices.end(), addr) != i2c::devices.end()) {
            s_mcpAddress = addr;
            break;
        }
    }

    if (s_mcpAddress == 0x00) {
        // Skip the tests if not
        s_skip = true;
        skip();
        return;
    }

    // The test ends all transactions with repeated START condition
    // and only ends the last transaction with STOP condition

    i2c::reset();

    // Perform 4 consecutive 1-byte reads without STOPs
    i2c::readRegister(s_mcpAddress, 0x00, nullptr, 1, false, false);
    i2c::readRegister(s_mcpAddress, 0x01, nullptr, 1, false, false);
    i2c::readRegister(s_mcpAddress, 0x02, nullptr, 1, false, false);
    i2c::readRegister(s_mcpAddress, 0x03, nullptr, 1, false, false);

    // Perform 2 consecutive 2-byte reads without STOPs
    i2c::readRegister(s_mcpAddress, 0x00, nullptr, 2, false, false);
    i2c::readRegister(s_mcpAddress, 0x02, nullptr, 2, false, false);

    // Perform single 4-byte read and end it with STOP
    i2c::readRegister(s_mcpAddress, 0x00, nullptr, 4, true, false);

    USE_WIRE.end();

    assertEqual(i2c::errorCount, 0);
}

test(2_I2C_MCP23017_doRequestControlStopReadStop) {
    if (s_skip) {
        skip();
        return;
    }
    // The test ends all transactions with STOP condition
    i2c::reset();

    // Perform 4 consecutive 1-byte reads with STOP after control sequence
    // and STOP after read
    i2c::readRegister(s_mcpAddress, 0x00, nullptr, 1, true, true);
    i2c::readRegister(s_mcpAddress, 0x01, nullptr, 1, true, true);
    i2c::readRegister(s_mcpAddress, 0x02, nullptr, 1, true, true);
    i2c::readRegister(s_mcpAddress, 0x03, nullptr, 1, true, true);

    // Perform 2 consecutive 2-byte reads with STOP after control sequence
    // and STOP after read
    i2c::readRegister(s_mcpAddress, 0x00, nullptr, 2, true, true);
    i2c::readRegister(s_mcpAddress, 0x02, nullptr, 2, true, true);

    // Perform single 4-byte read with STOP after control sequence
    // and STOP after read
    i2c::readRegister(s_mcpAddress, 0x00, nullptr, 4, true, true);

    USE_WIRE.end();

    assertEqual(i2c::errorCount, 0);
}

test(3_I2C_MCP23017_doRequestControlNoStopReadStop) {
    if (s_skip) {
        skip();
        return;
    }
    // The test performs all transactions with repeated START condition
    // after control sequence and STOP condition after read
    i2c::reset();

    // Perform 4 consecutive 1-byte reads with repeated START after control sequence
    // and STOP after read
    i2c::readRegister(s_mcpAddress, 0x00, nullptr, 1, true, false);
    i2c::readRegister(s_mcpAddress, 0x01, nullptr, 1, true, false);
    i2c::readRegister(s_mcpAddress, 0x02, nullptr, 1, true, false);
    i2c::readRegister(s_mcpAddress, 0x03, nullptr, 1, true, false);

    // Perform 2 consecutive 2-byte reads with repeated START after control sequence
    // and STOP after read
    i2c::readRegister(s_mcpAddress, 0x00, nullptr, 2, true, false);
    i2c::readRegister(s_mcpAddress, 0x02, nullptr, 2, true, false);

    // Perform single 4-byte read with repeated START after control sequence
    // and STOP after read
    i2c::readRegister(s_mcpAddress, 0x00, nullptr, 4, true, false);

    USE_WIRE.end();

    assertEqual(i2c::errorCount, 0);
}

test(4_I2C_MCP23017_doRequestControlStopReadNoStop) {
    if (s_skip) {
        skip();
        return;
    }
    // The test performs all transactions with STOP condition
    // after control sequence and repeated START condition after read
    // Last transaction is ended with STOP condition
    i2c::reset();

    // Perform 4 consecutive 1-byte reads with repeated START after control sequence
    // and STOP after read
    i2c::readRegister(s_mcpAddress, 0x00, nullptr, 1, false, true);
    i2c::readRegister(s_mcpAddress, 0x01, nullptr, 1, false, true);
    i2c::readRegister(s_mcpAddress, 0x02, nullptr, 1, false, true);
    i2c::readRegister(s_mcpAddress, 0x03, nullptr, 1, false, true);

    // Perform 2 consecutive 2-byte reads with repeated START after control sequence
    // and STOP after read
    i2c::readRegister(s_mcpAddress, 0x00, nullptr, 2, false, true);
    i2c::readRegister(s_mcpAddress, 0x02, nullptr, 2, false, true);

    // Perform single 4-byte read with repeated START after control sequence
    // and STOP after read
    i2c::readRegister(s_mcpAddress, 0x00, nullptr, 4, true, true);

    USE_WIRE.end();

    assertEqual(i2c::errorCount, 0);
}

/*
 * Tests for https://github.com/spark/firmware/issues/1042
 */
test(5_I2C_MCP23017_HighPriorityInterruptsDoNotInterfere) {
    if (s_skip) {
        skip();
        return;
    }

    HighPriorityInterruptInterferer interferer;

    i2c::reset();

    // Switch MCP23017 GPIOs to OUTPUT
    i2c::writeRegister(s_mcpAddress, 0x00, 0x00, true);
    i2c::writeRegister(s_mcpAddress, 0x01, 0x00, true);

    assertEqual(i2c::errorCount, 0);

    for (int i = 0; i < LOOP_COUNT; i++) {
        mcp23017_looped_test();
        assertEqual(i2c::errorCount, 0);
    }

    assertEqual(i2c::errorCount, 0);
}

/*
 * Tests for https://github.com/spark/firmware/issues/1042
 */
test(6_I2C_MCP23017_NastyThreadDisablingContextSwitchingDoesNotInterfere) {
    if (s_skip) {
        skip();
        return;
    }

    ContextSwitchBlockingInteferer interferer;

    i2c::reset();

    // Switch MCP23017 GPIOs to OUTPUT
    i2c::writeRegister(s_mcpAddress, 0x00, 0x00, true);
    i2c::writeRegister(s_mcpAddress, 0x01, 0x00, true);

    assertEqual(i2c::errorCount, 0);

    for (int i = 0; i < LOOP_COUNT; i++) {
        mcp23017_looped_test();
        assertEqual(i2c::errorCount, 0);
    }

    assertEqual(i2c::errorCount, 0);
}

/*
 * Tests for https://github.com/spark/firmware/issues/1042
 */
test(7_I2C_MCP23017_HighPriorityInterruptsAndNastyThreadDisablingContextSwitchingDoNotInterfere) {
    if (s_skip) {
        skip();
        return;
    }

    ContextSwitchBlockingInteferer interferer;
    HighPriorityInterruptInterferer interferer1;

    i2c::reset();

    // Switch MCP23017 GPIOs to OUTPUT
    i2c::writeRegister(s_mcpAddress, 0x00, 0x00, true);
    i2c::writeRegister(s_mcpAddress, 0x01, 0x00, true);

    assertEqual(i2c::errorCount, 0);

    for (int i = 0; i < LOOP_COUNT; i++) {
        mcp23017_looped_test();
        assertEqual(i2c::errorCount, 0);
    }

    assertEqual(i2c::errorCount, 0);
}

void mcp23017_looped_test() {
    // With STOP conditions
    // Read GPIO state
    uint8_t gpioa = i2c::readRegister(s_mcpAddress, 0x12, true, true);
    uint8_t gpiob = i2c::readRegister(s_mcpAddress, 0x13, true, true);
    assertEqual(i2c::errorCount, 0);

    // Invert GPIO state
    i2c::writeRegister(s_mcpAddress, 0x12, ~gpioa, true);
    i2c::writeRegister(s_mcpAddress, 0x13, ~gpiob, true);
    assertEqual(i2c::errorCount, 0);

    // With repeated-START conditions
    // Read GPIO state
    gpioa = i2c::readRegister(s_mcpAddress, 0x12, false, false);
    gpiob = i2c::readRegister(s_mcpAddress, 0x13, false, false);
    assertEqual(i2c::errorCount, 0);

    // Invert GPIO state
    i2c::writeRegister(s_mcpAddress, 0x12, ~gpioa, false);
    i2c::writeRegister(s_mcpAddress, 0x13, ~gpiob, false);
    assertEqual(i2c::errorCount, 0);

    // With STOP conditions
    // Read out 0x15 consecutive registers starting from 0x00
    i2c::readRegister(s_mcpAddress, 0x00, nullptr, 0x15, true, true);

    // With repeated-START conditions
    // Read out 0x15 consecutive registers starting from 0x00
    i2c::readRegister(s_mcpAddress, 0x00, nullptr, 0x15, false, false);
    assertEqual(i2c::errorCount, 0);
}
