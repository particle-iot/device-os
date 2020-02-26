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

#include "check.h"
#include "system_error.h"
#include "demux.h"
#include "gpio_hal.h"

using namespace particle;

Demux::Demux()
        : initialized_(false),
          pinValue_(DEFAULT_PINS_VALUE) {
}

Demux::~Demux() {

}

int Demux::write(uint8_t pin, uint8_t value) {
    DemuxLock lock();
    CHECK_TRUE(pin < DEMUX_MAX_PIN_COUNT, SYSTEM_ERROR_INVALID_ARGUMENT);
    if (!initialized_) {
        init();
    }
    if (getPinValue(pin) == value) {
        return SYSTEM_ERROR_NONE;
    }
    // Warning: there is a short glitch on other pins, since we don't have control pin to
    // disable all outputs before selecting target pin.
    if (value == 0) {
        if (pin < 4) {
            HAL_GPIO_Write(DEMUX_C, 0);
        } else {
            HAL_GPIO_Write(DEMUX_C, 1);
        }
        if ((pin % 4) < 2) {
            HAL_GPIO_Write(DEMUX_B, 0);
        } else {
            HAL_GPIO_Write(DEMUX_B, 1);
        }
        if ((pin % 2) == 0) {
            HAL_GPIO_Write(DEMUX_A, 0);
        } else {
            HAL_GPIO_Write(DEMUX_A, 1);
        }
        setPinValue(pin, value);
    } else {
        // FIXME: We don't connect EN pin and Y7 is not connected, just select Y7 to make other pins output 1.
        HAL_GPIO_Write(DEMUX_C, 1);
        HAL_GPIO_Write(DEMUX_B, 1);
        HAL_GPIO_Write(DEMUX_A, 1);
        setPinValue(7, 0);
    }
    return SYSTEM_ERROR_NONE;
}

Demux& Demux::getInstance() {
    static Demux demux;
    return demux;
}

int Demux::lock() {
    return !mutex_.lock();
}

int Demux::unlock() {
    return !mutex_.unlock();
}

void Demux::init() {
    HAL_Pin_Mode(DEMUX_A, OUTPUT);
    HAL_Pin_Mode(DEMUX_B, OUTPUT);
    HAL_Pin_Mode(DEMUX_C, OUTPUT);
    HAL_GPIO_Write(DEMUX_A, 1);
    HAL_GPIO_Write(DEMUX_B, 1);
    HAL_GPIO_Write(DEMUX_C, 1);
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
        pinValue_ = DEFAULT_PINS_VALUE;
        pinValue_ &= ~(0x01 << pin);
    }
}

StaticRecursiveMutex Demux::mutex_;
