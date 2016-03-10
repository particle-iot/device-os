#include "application.h"
#include "unit-test/unit-test.h"

#define MCP23017_ADDRESS 0x40
#define BASE_REGISTER 0x00

static int doRequest(uint8_t addr, uint8_t reg, int len, bool ctrlStop, bool recvStop) {
    Wire.beginTransmission(addr >> 1);
    Wire.write(BASE_REGISTER + reg);
    Wire.endTransmission(ctrlStop);
    return Wire.requestFrom(addr >> 1, len, recvStop);
}

test(I2C_MCP23017_doRequestControlNoStopReadNoStop) {
    // The test ends all transactions with repeated START condition
    // and only ends the last transaction with STOP condition
    int totalCount = 0;

    Wire.setSpeed(400000);
    Wire.begin();

    SINGLE_THREADED_SECTION();
    // Perform 4 consecutive 1-byte reads without STOPs
    totalCount += doRequest(MCP23017_ADDRESS, 0x00, 1, false, false);
    totalCount += doRequest(MCP23017_ADDRESS, 0x01, 1, false, false);
    totalCount += doRequest(MCP23017_ADDRESS, 0x02, 1, false, false);
    totalCount += doRequest(MCP23017_ADDRESS, 0x03, 1, false, false);

    // Perform 2 consecutive 2-byte reads without STOPs
    totalCount += doRequest(MCP23017_ADDRESS, 0x00, 2, false, false);
    totalCount += doRequest(MCP23017_ADDRESS, 0x02, 2, false, false);

    // Perform single 4-byte read and end it with STOP
    totalCount += doRequest(MCP23017_ADDRESS, 0x00, 4, false, true);

    Wire.end();

    assertEqual(totalCount, 4 * 3);
}

test(I2C_MCP23017_doRequestControlStopReadStop) {
    // The test ends all transactions with STOP condition
    int totalCount = 0;

    Wire.setSpeed(400000);
    Wire.begin();

    SINGLE_THREADED_SECTION();
    // Perform 4 consecutive 1-byte reads with STOP after control sequence
    // and STOP after read
    totalCount += doRequest(MCP23017_ADDRESS, 0x00, 1, true, true);
    totalCount += doRequest(MCP23017_ADDRESS, 0x01, 1, true, true);
    totalCount += doRequest(MCP23017_ADDRESS, 0x02, 1, true, true);
    totalCount += doRequest(MCP23017_ADDRESS, 0x03, 1, true, true);

    // Perform 2 consecutive 2-byte reads with STOP after control sequence
    // and STOP after read
    totalCount += doRequest(MCP23017_ADDRESS, 0x00, 2, true, true);
    totalCount += doRequest(MCP23017_ADDRESS, 0x02, 2, true, true);

    // Perform single 4-byte read with STOP after control sequence
    // and STOP after read
    totalCount += doRequest(MCP23017_ADDRESS, 0x00, 4, true, true);

    Wire.end();

    assertEqual(totalCount, 4 * 3);
}

test(I2C_MCP23017_doRequestControlNoStopReadStop) {
    // The test performs all transactions with repeated START condition
    // after control sequence and STOP condition after read
    int totalCount = 0;

    Wire.setSpeed(400000);
    Wire.begin();

    SINGLE_THREADED_SECTION();
    // Perform 4 consecutive 1-byte reads with repeated START after control sequence
    // and STOP after read
    totalCount += doRequest(MCP23017_ADDRESS, 0x00, 1, false, true);
    totalCount += doRequest(MCP23017_ADDRESS, 0x01, 1, false, true);
    totalCount += doRequest(MCP23017_ADDRESS, 0x02, 1, false, true);
    totalCount += doRequest(MCP23017_ADDRESS, 0x03, 1, false, true);

    // Perform 2 consecutive 2-byte reads with repeated START after control sequence
    // and STOP after read
    totalCount += doRequest(MCP23017_ADDRESS, 0x00, 2, false, true);
    totalCount += doRequest(MCP23017_ADDRESS, 0x02, 2, false, true);

    // Perform single 4-byte read with repeated START after control sequence
    // and STOP after read
    totalCount += doRequest(MCP23017_ADDRESS, 0x00, 4, false, true);

    Wire.end();

    assertEqual(totalCount, 4 * 3);
}

test(I2C_MCP23017_doRequestControlStopReadNoStop) {
    // The test performs all transactions with STOP condition
    // after control sequence and repeated START condition after read
    // Last transaction is ended with STOP condition
    int totalCount = 0;

    Wire.setSpeed(400000);
    Wire.begin();

    SINGLE_THREADED_SECTION();
    // Perform 4 consecutive 1-byte reads with repeated START after control sequence
    // and STOP after read
    totalCount += doRequest(MCP23017_ADDRESS, 0x00, 1, true, false);
    totalCount += doRequest(MCP23017_ADDRESS, 0x01, 1, true, false);
    totalCount += doRequest(MCP23017_ADDRESS, 0x02, 1, true, false);
    totalCount += doRequest(MCP23017_ADDRESS, 0x03, 1, true, false);

    // Perform 2 consecutive 2-byte reads with repeated START after control sequence
    // and STOP after read
    totalCount += doRequest(MCP23017_ADDRESS, 0x00, 2, true, false);
    totalCount += doRequest(MCP23017_ADDRESS, 0x02, 2, true, false);

    // Perform single 4-byte read with repeated START after control sequence
    // and STOP after read
    totalCount += doRequest(MCP23017_ADDRESS, 0x00, 4, true, true);

    Wire.end();

    assertEqual(totalCount, 4 * 3);
}
