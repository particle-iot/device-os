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

#include "kg200z.h"

using namespace particle;

Kg200z::Kg200z(TwoWire* wire)
        : initialized_(false),
          address_(KG200Z_SLAVE_ADDR),
          wire_(wire) {
}

Kg200z::~Kg200z() {

}

Kg200z& Kg200z::getInstance() {
    static Kg200z lora(&Wire);
    return lora;
}

int Kg200z::begin() {
    CHECK_FALSE(initialized_, SYSTEM_ERROR_NONE);

    wire_->begin();

    // FIXME: I/O glitch when there is a pull-up on a pin that is configured as OUTPUT

    Mcp23s17::getInstance().setPinMode(intPin_.first, intPin_.second, INPUT_PULLUP);
    Mcp23s17::getInstance().setPinMode(rstPin_.first, rstPin_.second, OUTPUT);
    // HIGH: connect the module's I2C to MCU's I2C, disconnect the module's UART from MCU's UART
    // LOW: Disconnect the module's I2C from MCU's I2C, connect the module's UART to MCU's UART
    Mcp23s17::getInstance().setPinMode(busSelPin_.first, busSelPin_.second, OUTPUT);
    // HIGH: enter the BOOT mode for UART download, otherwise, enter the normal mode
    Mcp23s17::getInstance().setPinMode(bootPin_.first, bootPin_.second, OUTPUT);

    initialized_ = true;

    uartDownloadMode(false, true);

    return SYSTEM_ERROR_NONE;
}

int Kg200z::end() {
    CHECK_TRUE(initialized_, SYSTEM_ERROR_NONE);
    initialized_ = false;
    return SYSTEM_ERROR_NONE;
}

int Kg200z::getVersion() {
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    sendAtComand("AT+QVER=?");
    delay(1000); // TODO: In debug mode, KG200Z is very slow due to printing the verbosed log
    receiveAtResponse();
    return SYSTEM_ERROR_NONE;
}

int Kg200z::uartDownloadMode(bool enable, bool reset) {
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    if (enable) {
        Mcp23s17::getInstance().writePinValue(busSelPin_.first, busSelPin_.second, LOW);
        Mcp23s17::getInstance().writePinValue(bootPin_.first, bootPin_.second, HIGH);
    } else {
        Mcp23s17::getInstance().writePinValue(busSelPin_.first, busSelPin_.second, HIGH);
        Mcp23s17::getInstance().writePinValue(bootPin_.first, bootPin_.second, LOW);
    }
    if (reset) {
        // Perform hardware reset, active HIGH
        Mcp23s17::getInstance().writePinValue(rstPin_.first, rstPin_.second, HIGH);
        delay(100);
        Mcp23s17::getInstance().writePinValue(rstPin_.first, rstPin_.second, LOW);
        delay(500);
    }
    return SYSTEM_ERROR_NONE;
}

void Kg200z::sendAtComand(const char *command) {
    wire_->beginTransmission(address_);
    wire_->write(CMD_AT);
    wire_->write(command);
    wire_->write('\r');
    wire_->endTransmission();
}

void Kg200z::receiveAtResponse() {
    /// Get the length of the response
    wire_->beginTransmission(address_);
    wire_->write(CMD_AT_RSP_LEN);
    wire_->endTransmission(false); // TODO: false: no stop
    wire_->requestFrom(address_, 2);
    if (wire_->available() <= 0) {
        Log.info("No data available");
        return;
    }
    uint16_t length = wire_->read();

    delay(10);

    /// Get the AT response
    wire_->beginTransmission(address_);
    wire_->write(CMD_AT_RSP_DATA);
    wire_->endTransmission(false); // TODO: false: no stop

    static uint8_t buf[512];
    static int buf_index = 0;
    memset(buf, 0, sizeof(buf));
    buf_index = 0;

    // for loop to get all the data, each time 32 bytes at max
    for (uint16_t i = 0; i < length; i += 32) {
        uint16_t len = length - i;
        if (len > 32) {
            len = 32;
        }
        wire_->requestFrom(address_, len);
        while (wire_->available() > 0) {
            buf[buf_index++] = wire_->read();
        }
    }

    for (int i = 0; i < buf_index; i++) {
        Log.printf("%c", buf[i]);
    }
    Log.printf("\r\n");
}
