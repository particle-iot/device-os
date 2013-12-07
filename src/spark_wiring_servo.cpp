/**
 ******************************************************************************
 * @file    spark_wiring_spi.h
 * @author  Zach Supalla
 * @version V1.0.0
 * @date    06-December-2013
 * @brief   Header for spark_wiring_servo.cpp module
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.
  Copyright (c) 2010 LeafLabs, LLC.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

#include "spark_wiring_servo.h"

// 20 millisecond period config.  For a 1-based prescaler,
//
//    (prescaler * overflow / CYC_MSEC) msec = 1 timer cycle = 20 msec
// => prescaler * overflow = 20 * CYC_MSEC
//
// This picks the smallest prescaler that allows an overflow < 2^16.
#define MAX_OVERFLOW    ((1 << 16) - 1)
#define CYC_MSEC        (1000 * CYCLES_PER_MICROSECOND)
#define TAU_MSEC        20
#define TAU_USEC        (TAU_MSEC * 1000)
#define TAU_CYC         (TAU_MSEC * CYC_MSEC)
#define SERVO_PRESCALER (TAU_CYC / MAX_OVERFLOW + 1)
#define SERVO_OVERFLOW  ((uint16)round((double)TAU_CYC / SERVO_PRESCALER))

// Unit conversions
#define US_TO_COMPARE(us) ((uint16)map((us), 0, TAU_USEC, 0, SERVO_OVERFLOW))
#define COMPARE_TO_US(c)  ((uint32)map((c), 0, SERVO_OVERFLOW, 0, TAU_USEC))
#define ANGLE_TO_US(a)    ((uint16)(map((a), this->minAngle, this->maxAngle, \
                                        this->minPW, this->maxPW)))
#define US_TO_ANGLE(us)   ((int16)(map((us), this->minPW, this->maxPW,  \
                                       this->minAngle, this->maxAngle)))

Servo::Servo() {
    this->resetFields();
}

bool Servo::attach(uint8_t pin,
                   uint16_t minPW,
                   uint16_t maxPW,
                   int16_t minAngle,
                   int16_t maxAngle) {
    timer_dev *tdev = PIN_MAP[pin].timer_device;

    if (tdev == NULL) {
        // don't reset any fields or ASSERT(0), to keep driving any
        // previously attach()ed servo.
        return false;
    }

    if (this->attached()) {
        this->detach();
    }

    this->pin = pin;
    this->minPW = minPW;
    this->maxPW = maxPW;
    this->minAngle = minAngle;
    this->maxAngle = maxAngle;

    pinMode(pin, PWM);

    timer_pause(tdev);
    timer_set_prescaler(tdev, SERVO_PRESCALER - 1); // prescaler is 1-based
    timer_set_reload(tdev, SERVO_OVERFLOW);
    timer_generate_update(tdev);
    timer_resume(tdev);

    return true;
}

bool Servo::detach() {
    if (!this->attached()) {
        return false;
    }

    timer_dev *tdev = PIN_MAP[this->pin].timer_device;
    uint8 tchan = PIN_MAP[this->pin].timer_channel;
    timer_set_mode(tdev, tchan, TIMER_DISABLED);

    this->resetFields();

    return true;
}

void Servo::write(int degrees) {
    degrees = constrain(degrees, this->minAngle, this->maxAngle);
    this->writeMicroseconds(ANGLE_TO_US(degrees));
}

int Servo::read() const {
    int a = US_TO_ANGLE(this->readMicroseconds());
    // map() round-trips in a weird way we mostly correct for here;
    // the round-trip is still sometimes off-by-one for write(1) and
    // write(179).
    return a == this->minAngle || a == this->maxAngle ? a : a + 1;
}

void Servo::writeMicroseconds(uint16 pulseWidth) {
    if (!this->attached()) {
        ASSERT(0);
        return;
    }

    pulseWidth = constrain(pulseWidth, this->minPW, this->maxPW);
    pwmWrite(this->pin, US_TO_COMPARE(pulseWidth));
}

uint16_t Servo::readMicroseconds() const {
    if (!this->attached()) {
        ASSERT(0);
        return 0;
    }

    stm32_pin_info pin_info = PIN_MAP[this->pin];
    uint16_t compare = timer_get_compare(pin_info.timer_device,
                                       pin_info.timer_channel);

    return COMPARE_TO_US(compare);
}

void Servo::resetFields(void) {
    this->pin = NOT_ATTACHED;
    this->minAngle = SERVO_DEFAULT_MIN_ANGLE;
    this->maxAngle = SERVO_DEFAULT_MAX_ANGLE;
    this->minPW = SERVO_DEFAULT_MIN_PW;
    this->maxPW = SERVO_DEFAULT_MAX_PW;
}
