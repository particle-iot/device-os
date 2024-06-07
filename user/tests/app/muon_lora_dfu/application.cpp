#include "application.h"
#include "mcp23s17.h"

#define WIRE Wire

SYSTEM_MODE(MANUAL);

STARTUP (
    System.setPowerConfiguration(SystemPowerConfiguration().feature(SystemPowerFeature::PMIC_DETECTION));
);

// Serial1LogHandler l1(115200, LOG_LEVEL_ALL);
// SerialLogHandler l2(115200, LOG_LEVEL_ALL);

void auxPowerControl(bool enable) {
    static bool configured = false;
    std::pair<uint8_t, uint8_t> auxEnPin = {MCP23S17_PORT_B, 3};
    if (!configured) {
        Mcp23s17::getInstance().setPinMode(auxEnPin.first, auxEnPin.second, OUTPUT);
        configured = true;
    }
    if (enable) {
        Mcp23s17::getInstance().writePinValue(auxEnPin.first, auxEnPin.second, HIGH);
    } else {
        Mcp23s17::getInstance().writePinValue(auxEnPin.first, auxEnPin.second, LOW);
    }
}

void enterLoraBootloader() {
    std::pair<uint8_t, uint8_t> loraBootPin = {MCP23S17_PORT_B, 0};
    std::pair<uint8_t, uint8_t> loraBusSelPin = {MCP23S17_PORT_B, 1};
    std::pair<uint8_t, uint8_t> loraRstPin = {MCP23S17_PORT_B, 2};

    // Lora Bus Select Pin
    //  - HIGH: MCU TX/RX external 2.54 TX/RX
    //  - LOW: MCU TX/RX -> Lora RX/TX
    Mcp23s17::getInstance().setPinMode(loraBusSelPin.first, loraBusSelPin.second, OUTPUT);
    Mcp23s17::getInstance().writePinValue(loraBusSelPin.first, loraBusSelPin.second, LOW);

    // Lora boot pin
    //  - HIGH: bootloader
    //  - LOW: user app
    Mcp23s17::getInstance().setPinMode(loraBootPin.first, loraBootPin.second, OUTPUT);
    Mcp23s17::getInstance().writePinValue(loraBootPin.first, loraBootPin.second, HIGH);

    // Lora reset pin
    //  - HIGH: reset
    Mcp23s17::getInstance().setPinMode(loraRstPin.first, loraRstPin.second, OUTPUT);
    Mcp23s17::getInstance().writePinValue(loraRstPin.first, loraRstPin.second, LOW);

    delay(10);

    Mcp23s17::getInstance().writePinValue(loraRstPin.first, loraRstPin.second, HIGH); // reset
    delay(500);
    Mcp23s17::getInstance().writePinValue(loraRstPin.first, loraRstPin.second, LOW); // Not reset

    delay(500);

    Log.info("KG200Z enters bootloader!");
}

void enterLoraApp() {
    std::pair<uint8_t, uint8_t> loraBootPin = {MCP23S17_PORT_B, 0};
    std::pair<uint8_t, uint8_t> loraBusSelPin = {MCP23S17_PORT_B, 1};
    std::pair<uint8_t, uint8_t> loraRstPin = {MCP23S17_PORT_B, 2};

    // Lora Bus Select Pin
    //  - HIGH: MCU TX/RX external 2.54 TX/RX
    //  - LOW: MCU TX/RX -> Lora RX/TX
    Mcp23s17::getInstance().setPinMode(loraBusSelPin.first, loraBusSelPin.second, OUTPUT);
    Mcp23s17::getInstance().writePinValue(loraBusSelPin.first, loraBusSelPin.second, LOW);

    // Lora boot pin
    //  - HIGH: bootloader
    //  - LOW: user app
    Mcp23s17::getInstance().setPinMode(loraBootPin.first, loraBootPin.second, OUTPUT);
    Mcp23s17::getInstance().writePinValue(loraBootPin.first, loraBootPin.second, LOW);

    // Lora reset pin
    //  - HIGH: reset
    Mcp23s17::getInstance().setPinMode(loraRstPin.first, loraRstPin.second, OUTPUT);
    Mcp23s17::getInstance().writePinValue(loraRstPin.first, loraRstPin.second, LOW);

    delay(10);

    Mcp23s17::getInstance().writePinValue(loraRstPin.first, loraRstPin.second, HIGH); // reset
    delay(500);
    Mcp23s17::getInstance().writePinValue(loraRstPin.first, loraRstPin.second, LOW); // Not reset

    delay(500);

    Log.info("KG200Z enters bootloader!");
}

void setup() {
    auxPowerControl(true);

    Serial1.begin(115200, SERIAL_PARITY_EVEN);
    Serial2.begin(115200, SERIAL_PARITY_EVEN);

    // pinMode(TX, INPUT_PULLUP);
    // pinMode(RX, INPUT_PULLUP);
    // enterLoraApp();
    enterLoraBootloader();
}

void loop() {

    while (1) {
        // at bridge using uart
        if (Serial2.available() > 0) {
            char c = Serial2.read();
            Serial1.write(c);
        }

        if (Serial1.available() > 0) {
            char c = Serial1.read();
            Serial2.write(c);
        }
    }

}
