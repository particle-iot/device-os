/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#include "demux.h"

#if HAL_PLATFORM_DEMUX

#include "check.h"
#include "gpio_hal.h"
#include "system_error.h"

using namespace particle;

Demux::Demux()
        : initialized_(false),
          pinValue_(DEFAULT_PINS_VALUE) {
    init();
}

Demux::~Demux() {

}

int Demux::write(uint8_t pin, uint8_t value) {
    DemuxLock lock();
    CHECK_TRUE(pin < DEMUX_MAX_PIN_COUNT && pin != 0, SYSTEM_ERROR_INVALID_ARGUMENT); // Y0 is not available for user's usage.
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);

    uint32_t currOut = (hal_gpio_read(DEMUX_C) << 2) | (hal_gpio_read(DEMUX_B) << 1) | hal_gpio_read(DEMUX_A);
    if ((currOut == pin && value == 0) || (currOut != pin && value == 1)) {
        return SYSTEM_ERROR_NONE;
    }
    if (value) {
        // Select Y0 by default, so that all other pins output high.
        hal_gpio_write(DEMUX_C, 0);
        hal_gpio_write(DEMUX_B, 0);
        hal_gpio_write(DEMUX_A, 0);

        setPinValue(0, 0);
    } else {
        hal_gpio_write(DEMUX_C, pin >> 2);
        hal_gpio_write(DEMUX_B, (pin >> 1) & 1);
        hal_gpio_write(DEMUX_A, pin & 1);

        setPinValue(pin, 0);
    }
    return SYSTEM_ERROR_NONE;
}

uint8_t Demux::read(uint8_t pin) const {
    return getPinValue(pin);
}

Demux& Demux::getInstance() {
    static Demux demux;
    return demux;
}

int Demux::lock() {
    return mutex_.lock();
}

int Demux::unlock() {
    return mutex_.unlock();
}

void Demux::init() {
    hal_gpio_mode(DEMUX_A, OUTPUT);
    hal_gpio_mode(DEMUX_B, OUTPUT);
    hal_gpio_mode(DEMUX_C, OUTPUT);
    // Select Y0 by default.
    hal_gpio_write(DEMUX_C, 0);
    hal_gpio_write(DEMUX_B, 0);
    hal_gpio_write(DEMUX_A, 0);

    initialized_ = true;
}

uint8_t Demux::getPinValue(uint8_t pin) const {
    return (pinValue_ >> pin) & 0x01;
}

void Demux::setPinValue(uint8_t pin, uint8_t value) {
    if (value) {
        pinValue_ |= (0x01 << pin);
    } else {
        // Only one pin is active 0 at a time
        pinValue_ = ~(0x01 << pin) & DEFAULT_PINS_VALUE;
    }
}

constexpr uint8_t Demux::DEFAULT_PINS_VALUE;

#endif // HAL_PLATFORM_DEMUX
