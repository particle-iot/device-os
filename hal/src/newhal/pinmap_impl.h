/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#pragma once

const hal_pin_t TOTAL_PINS = 21;
const hal_pin_t TOTAL_ANALOG_PINS = 8;
const hal_pin_t FIRST_ANALOG_PIN = 10;
const hal_pin_t D0 = 0;
const hal_pin_t D1 = 1;
const hal_pin_t D2 = 2;
const hal_pin_t D3 = 3;
const hal_pin_t D4 = 4;
const hal_pin_t D5 = 5;
const hal_pin_t D6 = 6;
const hal_pin_t D7 = 7;

const hal_pin_t A0 = 10;
const hal_pin_t A1 = 11;
const hal_pin_t A2 = 12;
const hal_pin_t A3 = 13;
const hal_pin_t A4 = 14;
const hal_pin_t A5 = 15;
const hal_pin_t A6 = 16;

// WKP pin is also an ADC on Photon
const hal_pin_t A7 = 17;

// RX and TX pins are also ADCs on Photon
const hal_pin_t A8 = 18;
const hal_pin_t A9 = 19;

const hal_pin_t RX = 18;
const hal_pin_t TX = 19;

const hal_pin_t BTN = 20;

// WKP pin on Photon
const hal_pin_t WKP = 17;

// Timer pins

const hal_pin_t TIMER2_CH1 = 10;
const hal_pin_t TIMER2_CH2 = 11;
const hal_pin_t TIMER2_CH3 = 18;
const hal_pin_t TIMER2_CH4 = 19;

const hal_pin_t TIMER3_CH1 = 14;
const hal_pin_t TIMER3_CH2 = 15;
const hal_pin_t TIMER3_CH3 = 16;
const hal_pin_t TIMER3_CH4 = 17;

const hal_pin_t TIMER4_CH1 = 1;
const hal_pin_t TIMER4_CH2 = 0;

// SPI pins
const hal_pin_t SS   = 12;
const hal_pin_t SCK  = 13;
const hal_pin_t MISO = 14;
const hal_pin_t MOSI = 15;

// I2C pins
const hal_pin_t SDA  = 0;
const hal_pin_t SCL  = 1;

// DAC pins on Photon
const hal_pin_t DAC1 = 16;
const hal_pin_t DAC2 = 13;

const uint8_t LSBFIRST = 0;
const uint8_t MSBFIRST = 1;
