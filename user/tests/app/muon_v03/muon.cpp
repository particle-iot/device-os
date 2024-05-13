#include "application.h"
#include "mcp23s17.h"

SYSTEM_MODE(MANUAL);

Serial1LogHandler l1(115200, LOG_LEVEL_ALL);
SerialLogHandler l2(115200, LOG_LEVEL_ALL);

void uartRouteLora(bool isLora) {
    static bool configured = false;
    std::pair<uint8_t, uint8_t> loraBusSelPin = {MCP23S17_PORT_B, 1};
    if (!configured) {
        Mcp23s17::getInstance().setPinMode(loraBusSelPin.first, loraBusSelPin.second, OUTPUT);
        configured = true;
    }
    Mcp23s17::getInstance().writePinValue(loraBusSelPin.first, loraBusSelPin.second, isLora ? LOW : HIGH);
}

void testStusb4500() {
    static bool configured = false;
    std::pair<uint8_t, uint8_t> pdRstPin = {MCP23S17_PORT_A, 5};
    if (!configured) {
        Mcp23s17::getInstance().setPinMode(pdRstPin.first, pdRstPin.second, OUTPUT);
        Mcp23s17::getInstance().writePinValue(pdRstPin.first, pdRstPin.second, HIGH); // Reset
        delay(100);
        Mcp23s17::getInstance().writePinValue(pdRstPin.first, pdRstPin.second, LOW);
        delay(100);
        configured = true;
    }
    constexpr uint8_t STUSB4500_ADDR = 0x28;
    constexpr uint8_t DEV_ID_REG = 0x2F;
    constexpr uint8_t DEVICE_ID = 0x25;
    Wire.beginTransmission(STUSB4500_ADDR);
    Wire.write(DEV_ID_REG);
    int ret = Wire.endTransmission(false);
    if (ret) {
        Log.info("[testStusb4500] endTransmission: %d", ret);
        return;
    }
    ret = Wire.requestFrom(STUSB4500_ADDR, 1);
    if (ret != 1) {
        Log.info("[testStusb4500] requestFrom: %d", ret);
        return;
    }
    uint8_t val = Wire.read();
    Log.info("[testStusb4500] Device_ID: 0x%02x, expected: 0x%02x", val, DEVICE_ID);
}

void testFuelGauge() {
    FuelGauge fuel;
    fuel.wakeup();
    delay(1000);
    auto ret = fuel.getVersion();
    Log.info("[testFuelGauge] version: 0x%04x", ret);
}

void testPmic() {
    constexpr uint8_t BQ24195_VERSION = 0x23;
    PMIC power(true);
    power.begin();
    auto pVer = power.getVersion();
    if (pVer != BQ24195_VERSION) {
        LOG(ERROR, "[testPmic] PMIC not detected");
        return;
    }
    Log.info("[testPmic] version 0x%02x, expected: 0x%02x", pVer, BQ24195_VERSION);
}

void testExternalRtc() {
    constexpr uint8_t AM18X5_ADDR = 0x69;
    constexpr uint8_t PART_NUM_REG = 0x28;
    constexpr uint16_t PART_NUMBER = 0x1805;
    Wire.beginTransmission(AM18X5_ADDR);
    Wire.write(PART_NUM_REG);
    int ret = Wire.endTransmission(false);
    if (ret) {
        Log.info("[testExternalRtc] endTransmission: %d", ret);
        return;
    }
    ret = Wire.requestFrom(AM18X5_ADDR, 2);
    if (ret != 2) {
        Log.info("[testExternalRtc] requestFrom: %d", ret);
        return;
    }
    uint16_t val = Wire.read() << 8;
    val |= Wire.read();
    Log.info("[testExternalRtc] Part Number: 0x%04x, expected: 0x%04x", val, PART_NUMBER);
}

void testTemperatureSensor() {
    constexpr uint8_t TMP112A_ADDR = 0x48;
    constexpr uint8_t CONFIG_REG = 0x01;
    constexpr uint8_t TEMP_REG = 0x00;
    constexpr uint16_t DEFAULT_CONFIG = 0x60A0;
    Wire.beginTransmission(TMP112A_ADDR);
    Wire.write(CONFIG_REG);
    int ret = Wire.endTransmission(false);
    if (ret) {
        Log.info("[testTemperatureSensor] endTransmission: %d", ret);
        return;
    }
    ret = Wire.requestFrom(TMP112A_ADDR, 2);
    if (ret != 2) {
        Log.info("[testTemperatureSensor] requestFrom: %d", ret);
        return;
    }
    uint16_t val = (Wire.read() << 8) & 0x7FFF;
    val |= Wire.read();
    Log.info("[testTemperatureSensor] Config: 0x%04x, expected: 0x%04x", val, DEFAULT_CONFIG);
    if (val == DEFAULT_CONFIG) {
        Wire.beginTransmission(TMP112A_ADDR);
        Wire.write(TEMP_REG);
        Wire.endTransmission(false);
        Wire.requestFrom(TMP112A_ADDR, 2);
        val = Wire.read() << 8;
        val |= Wire.read();
        val >>= 4;
        bool neg = false;
        if (val & 0x800) {
            val = (~val) + 1;
            neg = true;
        }
        float temp = val * 0.0625;
        Log.info("[testTemperatureSensor] Temperature: (0x%04x) %c%.1f", val, (neg ? '-' : '+'), temp);
    }
}

void setup() {
    Log.info("<< Test program for Muon v0.3 >>\r\n");

    // Route Serial1 to header pins
    uartRouteLora(false);

    Wire.begin();
    
    testFuelGauge();
    testPmic();
    testStusb4500();
    testExternalRtc();
    testTemperatureSensor();
}

void loop() {

}
