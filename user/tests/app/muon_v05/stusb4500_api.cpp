/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */


#include "stusb4500_api.h"
#include "mcp23s17.h"

namespace {
STUSB4500 usb;

std::pair<uint8_t, uint8_t> pdRstPin    = {MCP23S17_PORT_B, 4};
std::pair<uint8_t, uint8_t> pdAttachPin = {MCP23S17_PORT_A, 4};
std::pair<uint8_t, uint8_t> pdIntPin    = {MCP23S17_PORT_A, 3};

constexpr uint8_t STUSB4500_ADDR = 0x28;
constexpr uint8_t STUSB4500_DEV_ID_REG = 0x2F;
constexpr uint8_t STUSB4500_DEVICE_ID = 0x25;

bool initialized = false;

TwoWire& wire = Wire;
}

int stusb4500Init(void) {
    CHECK_FALSE(initialized, SYSTEM_ERROR_NONE);

    Mcp23s17::getInstance().setPinMode(pdAttachPin.first, pdAttachPin.second, INPUT_PULLUP);
    Mcp23s17::getInstance().setPinMode(pdIntPin.first, pdIntPin.second, INPUT_PULLUP);
    Mcp23s17::getInstance().setPinMode(pdRstPin.first, pdRstPin.second, OUTPUT);

    Mcp23s17::getInstance().writePinValue(pdRstPin.first, pdRstPin.second, HIGH); // Reset
    delay(100);
    Mcp23s17::getInstance().writePinValue(pdRstPin.first, pdRstPin.second, LOW);
    delay(100);

    if (!usb.begin()) {
        Log.info("Cannot connect to STUSB4500.");
        Log.info("Is the board connected? Is the device ID correct?");
        return SYSTEM_ERROR_INTERNAL;
    }

    initialized = true;
    return SYSTEM_ERROR_NONE;
}

int stusb4500ReadDeviceId(uint8_t* deviceId) {
    CHECK_TRUE(initialized, SYSTEM_ERROR_INVALID_STATE);
    wire.beginTransmission(STUSB4500_ADDR);
    wire.write(STUSB4500_DEV_ID_REG);
    int ret = wire.endTransmission(false);
    if (ret) {
        Log.info("[stusb4500ReadDeviceId] endTransmission: %d", ret);
        return ret;
    }
    ret = wire.requestFrom(STUSB4500_ADDR, 1);
    if (ret != 1) {
        Log.info("[stusb4500ReadDeviceId] requestFrom: %d", ret);
        return ret;
    }
    *deviceId = wire.read();
    Log.info("[stusb4500ReadDeviceId] Device_ID: 0x%02x, expected: 0x%02x", *deviceId, STUSB4500_DEVICE_ID);
    return SYSTEM_ERROR_NONE;
}

void stusb4500SetDefaultParams(void) {
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

void stusb4500LogParams(void) {
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

void stusb4500LogStatus(void) {
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
