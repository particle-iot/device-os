
// Flash the Lora firmware through SWD with Daplink:
// openocd -f interface/cmsis-dap.cfg -c "transport select swd" -f target/stm32wlx.cfg  -c init -c "reset halt; wait_halt; flash write_image erase KG200Z_AT.bin 0x08000000" -c reset -c shutdown
// Flash the Lora firmware through SWD with Jlink:
// openocd -f interface/jlink.cfg -c "transport select swd" -f target/stm32wlx.cfg  -c init -c "reset halt; wait_halt; flash write_image erase KG200Z_AT.bin 0x08000000" -c reset -c shutdown

#include "application.h"
#include "mcp23s17.h"
#include "SparkFun_STUSB4500.h"
#include "am18x5.h"

#define WIRE Wire

SYSTEM_MODE(MANUAL);

STARTUP (
    System.setPowerConfiguration(SystemPowerConfiguration().feature(SystemPowerFeature::PMIC_DETECTION));
);

Serial1LogHandler l1(115200, LOG_LEVEL_ALL);
SerialLogHandler l2(115200, LOG_LEVEL_ALL);

STUSB4500 usb;

#define CMD_AT              0x01
#define CMD_AT_RSP_LEN      0x02
#define CMD_AT_RSP_DATA     0x03

volatile bool buttonClicked = false;
void onButtonClick(system_event_t ev, int button_data) {
    buttonClicked = true;
}

void sendAtComand(const char *command) {
    WIRE.beginTransmission(0x61);
    WIRE.write(CMD_AT);
    WIRE.write(command);
    WIRE.write('\r');
    WIRE.endTransmission();
}

void receiveAtResponse() {
    // uint16_t length = 4;

    /// Get the length of the response
    WIRE.beginTransmission(0x61);
    WIRE.write(CMD_AT_RSP_LEN);
    WIRE.endTransmission(false); // TODO: false: no stop
    WIRE.requestFrom(0x61, 2);
    if (WIRE.available() <= 0) {
        Log.info("No data available");
        return;
    }
    uint16_t length = WIRE.read();

    delay(10);

    /// Get the AT response
    WIRE.beginTransmission(0x61);
    WIRE.write(CMD_AT_RSP_DATA);
    WIRE.endTransmission(false); // TODO: false: no stop

    static uint8_t buf[512];
    static int buf_index = 0;
    memset(buf, 0, sizeof(buf));
    buf_index = 0;

    // for loop to get all the data, each time 32 bytes at max
    // Log.info("Read %d bytes from Lora", length);
    for (uint16_t i = 0; i < length; i += 32) {
        uint16_t len = length - i;
        if (len > 32) {
            len = 32;
        }
        WIRE.requestFrom(0x61, len);
        // Log.info("Avaliable: %d", WIRE.available());
        // delay(10);
        while (WIRE.available() > 0) {
            buf[buf_index++] = WIRE.read();
        }
    }

    // dump buf
    // Log.printf("\r\nRx Length: %d\r\n", length);
    // Log.printf("Rx Data, size: %d\r\n", buf_index);
    // for (int i = 0; i < buf_index; i++) {
    //     Log.printf("0x%02X ", buf[i]);
    // }
    // Log.printf("\r\n");
    for (int i = 0; i < buf_index; i++) {
        Log.printf("%c", buf[i]);
    }
    Log.printf("\r\n");

    Serial.write(buf, buf_index);
}

void writeParameters() {
    delay(100);

    /* Set Number of Power Data Objects (PDO) 1-3 */
    usb.setPdoNumber(3);

    /* PDO1
    - Voltage fixed at 5V
    - Current value for PDO1 0-5A, if 0 used, FLEX_I value is used
    - Under Voltage Lock Out (setUnderVoltageLimit) fixed at 3.3V
    - Over Voltage Lock Out (setUpperVoltageLimit) 5-20%
    */
    usb.setCurrent(1, 3); // 5V3A
    usb.setUpperVoltageLimit(1, 20);

    /* PDO2
    - Voltage 5-20V
    - Current value for PDO2 0-5A, if 0 used, FLEX_I value is used
    - Under Voltage Lock Out (setUnderVoltageLimit) 5-20%
    - Over Voltage Lock Out (setUpperVoltageLimit) 5-20%
    */
    usb.setVoltage(2, 9.0);
    usb.setCurrent(2, 1.5); // 9V1.5A
    usb.setLowerVoltageLimit(2, 20);
    usb.setUpperVoltageLimit(2, 20);

    /* PDO3
    - Voltage 5-20V
    - Current value for PDO3 0-5A, if 0 used, FLEX_I value is used
    - Under Voltage Lock Out (setUnderVoltageLimit) 5-20%
    - Over Voltage Lock Out (setUpperVoltageLimit) 5-20%
    */
    usb.setVoltage(3, 12.0);
    usb.setCurrent(3, 1); // 12V1A
    usb.setLowerVoltageLimit(3, 20);
    usb.setUpperVoltageLimit(3, 20);

    /* Flexible current value common to all PDOs */
    //  usb.setFlexCurrent(1.0);

    /* Unconstrained Power bit setting in capabilities message sent by the sink */
    //  usb.setExternalPower(false);

    /* USB 2.0 or 3.x data communication capability by sink system */
    usb.setUsbCommCapable(false);

    /* Selects POWER_OK pins configuration
        0 - Configuration 1
        1 - No applicable
        2 - Configuration 2 (default)
        3 - Configuration 3
    */
    //  usb.setConfigOkGpio(2);

    /* Selects GPIO pin configuration
        0 - SW_CTRL_GPIO
        1 - ERROR_RECOVERY
        2 - DEBUG
        3 - SINK_POWER
    */
     usb.setGpioCtrl(3);

    /* Selects VBUS_EN_SNK pin configuration */
     usb.setPowerAbove5vOnly(false);

    /* In case of match, selects which operating current from the sink or the
        source is to be requested in the RDO message */
    //  usb.setReqSrcCurrent(false);

    /* Write the new settings to the NVM */
    usb.write();
}

void readParamters() {
    delay(100);

    /* Read the NVM settings to verify the new settings are correct */
    usb.read();

    Log.info("New Parameters:\n");

    /* Read the Power Data Objects (PDO) highest priority */
    Log.info("PDO Number: %d", usb.getPdoNumber());

    /* Read settings for PDO1 */
    Log.info("");
    Log.info("Voltage1 (V): %.2f", usb.getVoltage(1));
    Log.info("Current1 (A): %.2f", usb.getCurrent(1));
    Log.info("Lower Voltage Tolerance1 (%%): %.2f", usb.getLowerVoltageLimit(1));
    Log.info("Upper Voltage Tolerance1 (%%): %.2f", usb.getUpperVoltageLimit(1));
    Log.info("");

    /* Read settings for PDO2 */
    Log.info("Voltage2 (V): %.2f", usb.getVoltage(2));
    Log.info("Current2 (A): %.2f", usb.getCurrent(2));
    Log.info("Lower Voltage Tolerance2 (%%): %.2f", usb.getLowerVoltageLimit(2));
    Log.info("Upper Voltage Tolerance2 (%%): %.2f", usb.getUpperVoltageLimit(2));
    Log.info("");

    /* Read settings for PDO3 */
    Log.info("Voltage3 (V): %.2f", usb.getVoltage(3));
    Log.info("Current3 (A): %.2f", usb.getCurrent(3));
    Log.info("Lower Voltage Tolerance3 (%%): %.2f", usb.getLowerVoltageLimit(3));
    Log.info("Upper Voltage Tolerance3 (%%): %.2f", usb.getUpperVoltageLimit(3));
    Log.info("");

    /* Read the flex current value */
    Log.info("Flex Current: %.2f", usb.getFlexCurrent());

    /* Read the External Power capable bit */
    Log.info("External Power: %d", usb.getExternalPower());

    /* Read the USB Communication capable bit */
    Log.info("USB Communication Capable: %d", usb.getUsbCommCapable());

    /* Read the POWER_OK pins configuration */
    Log.info("Configuration OK GPIO: %d", usb.getConfigOkGpio());

    /* Read the GPIO pin configuration */
    Log.info("GPIO Control: %d", usb.getGpioCtrl());

    /* Read the bit that enables VBUS_EN_SNK pin only when power is greater than 5V */
    Log.info("Enable Power Only Above 5V: %d", usb.getPowerAbove5vOnly());

    /* Read bit that controls if the Source or Sink device's
        operating current is used in the RDO message */
    Log.info("Request Source Current: %d", usb.getReqSrcCurrent());
}

void readStatus() {
    Log.info("");
    Log.info("");

    uint8_t buffer[32] = {};
    usb.I2C_Read_USB_PD(REG_DEVICE_ID, buffer, 1);
    Log.info("Device ID: 0x%02X", buffer[0]);
    Log.info("");

    // Read PORT_STATUS_1 Register
    memset(buffer, 0, sizeof(buffer));
    usb.I2C_Read_USB_PD(REG_PORT_STATUS, buffer, 1);
    Log.info("Port Status: 0x%02X", buffer[0]);
    Log.info("ATTACHED_DEVICE: %d", (buffer[0] >> 5) & 0x07);
    Log.info("POWER_MODE: %d", (buffer[0] >> 3) & 0x01);
    Log.info("DATA_MODE: %d", (buffer[0] >> 2) & 0x01);
    Log.info("ATTACH: %d", (buffer[0] >> 0) & 0x01);
    Log.info("");

    // Read CC_STATUS Register
    memset(buffer, 0, sizeof(buffer));
    usb.I2C_Read_USB_PD(CC_STATUS, buffer, 1);
    Log.info("CC Status: 0x%02X", buffer[0]);
    Log.info("LOOKING_4_CONNECTION: %d", (buffer[0] >> 5) & 0x01);
    Log.info("CONNECT_RESULT: %d", (buffer[0] >> 4) & 0x01);
    Log.info("CC2_STATE: %d", (buffer[0] >> 2) & 0x03);
    Log.info("CC1_STATE: %d", (buffer[0] >> 0) & 0x03);
    Log.info("");

    // Read RDO
    memset(buffer, 0, sizeof(buffer));
    usb.I2C_Read_USB_PD(DPM_REQ_RDO, buffer, 4);
    uint32_t ActiveRDO = (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
    Log.info("ActiveRDO: 0x%08x", ActiveRDO);
    uint8_t objectPosition = (ActiveRDO >> 28) & 0x07;
    uint8_t giveBackFlag = (ActiveRDO >> 27) & 0x01;
    uint8_t capabilityMismatch = (ActiveRDO >> 26) & 0x01;
    uint8_t usbCommunicationsCapable = (ActiveRDO >> 25) & 0x01;
    uint8_t noUSBSuspend = (ActiveRDO >> 24) & 0x01;
    uint8_t unchunkedSupported = (ActiveRDO >> 23) & 0x01;
    uint8_t operatingX = (ActiveRDO >> 10) & 0x03FF;
    uint8_t maxMinOperatingCurrent = ActiveRDO & 0x03FF;
    Log.info(" - objectPosition: 0x%02x", objectPosition);
    Log.info(" - giveBackFlag: 0x%02x", giveBackFlag);
    Log.info(" - capabilityMismatch: 0x%02x", capabilityMismatch);
    Log.info(" - usbCommunicationsCapable: 0x%02x", usbCommunicationsCapable);
    Log.info(" - noUSBSuspend: 0x%02x", noUSBSuspend);
    Log.info(" - unchunkedSupported: 0x%02x", unchunkedSupported);
    Log.info(" - operatingX: %d", operatingX);
    Log.info(" - maxMinOperatingCurrent: %d", maxMinOperatingCurrent);
}

void uartRouteLora(bool isLora) {
    static bool configured = false;
    std::pair<uint8_t, uint8_t> loraBusSelPin = {MCP23S17_PORT_B, 1};
    if (!configured) {
        Mcp23s17::getInstance().setPinMode(loraBusSelPin.first, loraBusSelPin.second, OUTPUT);
        configured = true;
    }
    // When the bus select pin is HIGH, the switch between Lora's I2C and MCU's I2C is on, otherwise off.
    Mcp23s17::getInstance().writePinValue(loraBusSelPin.first, loraBusSelPin.second, isLora ? LOW : HIGH);
}

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

void testStusb4500() {
    static bool configured = false;
    std::pair<uint8_t, uint8_t> pdRstPin = {MCP23S17_PORT_B, 4};
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
    WIRE.beginTransmission(STUSB4500_ADDR);
    WIRE.write(DEV_ID_REG);
    int ret = WIRE.endTransmission(false);
    if (ret) {
        Log.info("[testStusb4500] endTransmission: %d", ret);
        return;
    }
    ret = WIRE.requestFrom(STUSB4500_ADDR, 1);
    if (ret != 1) {
        Log.info("[testStusb4500] requestFrom: %d", ret);
        return;
    }
    uint8_t val = WIRE.read();
    Log.info("[testStusb4500] Device_ID: 0x%02x, expected: 0x%02x", val, DEVICE_ID);

    if (!usb.begin()) {
        Log.info("Cannot connect to STUSB4500.");
        Log.info("Is the board connected? Is the device ID correct?");
        while (1) {
            ;
        }
    }
}

void loopStusb4500() {
    static auto startTime = millis();
    if (buttonClicked) {
        buttonClicked = false;
        writeParameters();
        readParamters();
    }
    if (millis() - startTime > 3000) {
        startTime = millis();
        // readStatus();
        // usb.printRDO();
    }
}

void testFuelGauge() {
    FuelGauge fuel;
    fuel.wakeup();
    delay(1000);
    auto ret = fuel.getVersion();
    Log.info("[testFuelGauge] version: 0x%04x", ret);

    fuel.setAlertThreshold(25);
    fuel.clearAlert();
    Log.info("[testFuelGauge] Current percentage: %.2f%%", fuel.getSoC());
    pinMode(A7, INPUT_PULLUP);
}

void testPmic() {
    constexpr uint8_t BQ24195_VERSION = 0x23;
    PMIC power(true);
    power.begin();
    auto pVer = power.getVersion();
    if (pVer != BQ24195_VERSION) {
        Log.info("[testPmic] PMIC not detected");
        return;
    }
    Log.info("[testPmic] version 0x%02x, expected: 0x%02x", pVer, BQ24195_VERSION);
}

void testExternalRtc() {
    constexpr uint8_t AM18X5_ADDR = 0x69;
    constexpr uint8_t PART_NUM_REG = 0x28;
    constexpr uint16_t PART_NUMBER = 0x1805;
    WIRE.beginTransmission(AM18X5_ADDR);
    WIRE.write(PART_NUM_REG);
    int ret = WIRE.endTransmission(false);
    if (ret) {
        Log.info("[testExternalRtc] endTransmission: %d", ret);
        return;
    }
    ret = WIRE.requestFrom(AM18X5_ADDR, 2);
    if (ret != 2) {
        Log.info("[testExternalRtc] requestFrom: %d", ret);
        return;
    }
    uint16_t val = WIRE.read() << 8;
    val |= WIRE.read();
    Log.info("[testExternalRtc] Part Number: 0x%04x, expected: 0x%04x", val, PART_NUMBER);
}

bool temperatureDetected = false;
void testTemperatureSensor() {
    constexpr uint8_t TMP112A_ADDR = 0x48;
    constexpr uint8_t CONFIG_REG = 0x01;
    constexpr uint8_t TEMP_REG = 0x00;
    constexpr uint16_t DEFAULT_CONFIG = 0x60A0;
    WIRE.beginTransmission(TMP112A_ADDR);
    WIRE.write(CONFIG_REG);
    int ret = WIRE.endTransmission(false);
    if (ret) {
        Log.info("[testTemperatureSensor] endTransmission: %d", ret);
        return;
    }
    ret = WIRE.requestFrom(TMP112A_ADDR, 2);
    if (ret != 2) {
        Log.info("[testTemperatureSensor] requestFrom: %d", ret);
        return;
    }
    uint16_t val = (WIRE.read() << 8) & 0x7FFF;
    val |= WIRE.read();
    Log.info("[testTemperatureSensor] Config: 0x%04x, expected: 0x%04x", val, DEFAULT_CONFIG);
    if (val == DEFAULT_CONFIG) {
        temperatureDetected = true;
    }
}

void loopReadTemperature() {
    if (!temperatureDetected) {
        return;
    }
    constexpr uint8_t TMP112A_ADDR = 0x48;
    constexpr uint8_t TEMP_REG = 0x00;
    WIRE.beginTransmission(TMP112A_ADDR);
    WIRE.write(TEMP_REG);
    WIRE.endTransmission(false);
    WIRE.requestFrom(TMP112A_ADDR, 2);
    uint16_t val = WIRE.read() << 8;
    val |= WIRE.read();
    val >>= 4;
    bool neg = false;
    if (val & 0x800) {
        val = (~val) + 1;
        neg = true;
    }
    float temp = val * 0.0625;
    Log.info("Temperature: (0x%04x) %c%.1f", val, (neg ? '-' : '+'), temp);
}

void testLora() {
    static bool configured = false;
    std::pair<uint8_t, uint8_t> loraRstPin = {MCP23S17_PORT_B, 2};
    std::pair<uint8_t, uint8_t> loraBootPin = {MCP23S17_PORT_B, 0};
    if (!configured) {
        Mcp23s17::getInstance().setPinMode(loraBootPin.first, loraBootPin.second, OUTPUT);
        Mcp23s17::getInstance().writePinValue(loraBootPin.first, loraBootPin.second, LOW);
        Mcp23s17::getInstance().setPinMode(loraRstPin.first, loraRstPin.second, OUTPUT);
        Mcp23s17::getInstance().writePinValue(loraRstPin.first, loraRstPin.second, HIGH); // reset
        delay(100);
        Mcp23s17::getInstance().writePinValue(loraRstPin.first, loraRstPin.second, LOW); // Not reset
        delay(500);
        configured = true;
    }

    Log.info("[testLora] AT+QVER=?");
    sendAtComand("AT+QVER=?");
    delay(1000); // TODO: In debug mode, KG200Z is very slow due to printing the verbosed log
    receiveAtResponse();
}

void setup() {
    Log.info("<< Test program for Muon v0.3 >>\r\n");

    // Route Serial1 to header pins
    uartRouteLora(false);

    WIRE.begin();

    testFuelGauge();
    testPmic();
    testStusb4500();
    testExternalRtc();
    testTemperatureSensor();

    auxPowerControl(true);
    testLora();

    System.on(button_click, onButtonClick);

    uint16_t id;
    Am18x5::getInstance().getPartNumber(&id);
    Log.info("Am18x5 Part Number: 0x%04x", id);

    struct timeval tv = {};
    tv.tv_sec = 1716819790;
    Log.info("Set time: %ld", tv.tv_sec);
    Am18x5::getInstance().setTime(&tv);

    // Am18x5::getInstance().setPsw(1);
}

void loop() {
    static system_tick_t startTime = 0;
    if (millis() - startTime > 3000) {
        startTime = millis();
        loopStusb4500();
        loopReadTemperature();

        struct timeval tv = {};
        Am18x5::getInstance().getTime(&tv);
        struct tm calendar = {};
        gmtime_r(&tv.tv_sec, &calendar);
        Log.info("Time: %ld-%ld-%ld, %ld:%ld:%ld",
            calendar.tm_year + 1900, calendar.tm_mon + 1, calendar.tm_mday, calendar.tm_hour, calendar.tm_min, calendar.tm_sec);

        FuelGauge fuelGauge;
        Log.info("Current percentage: %.2f%%", fuelGauge.getSoC());
        int val = digitalRead(A7);
        if (val == 0) {
            Log.info("low battery alert, low than 25% !!!");
            fuelGauge.begin();
            fuelGauge.wakeup();
            fuelGauge.clearAlert();  // Ensure this is cleared, or interrupts will never occur
        }
    }
}
