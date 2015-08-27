/**
 ******************************************************************************
 * @file    spark_wiring_spi.h
 * @author  Zach Supalla
 * @version V1.0.0
 * @date    06-December-2013
 * @brief   Header for spark_wiring_servo.cpp module
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.
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
#include "servo_hal.h"

#define ANGLE_TO_US(a)    ((uint16_t)(map((a), this->minAngle, this->maxAngle, \
                                        this->minPW, this->maxPW)))
#define US_TO_ANGLE(us)   ((int16_t)(map((us), this->minPW, this->maxPW,  \
                                       this->minAngle, this->maxAngle)))

Servo::Servo()
{
  this->resetFields();
}

bool Servo::attach(uint16_t pin,
    uint16_t minPW,
    uint16_t maxPW,
    int16_t minAngle,
    int16_t maxAngle)
{

  if (HAL_Validate_Pin_Function(pin, PF_TIMER)!=PF_TIMER)
  {
    return false;
  }

  // Safety check
  if (!pinAvailable(pin))
  {
    return false;
  }

  if (this->attached())
  {
    this->detach();
  }

  this->pin = pin;
  this->minPW = minPW;
  this->maxPW = maxPW;
  this->minAngle = minAngle;
  this->maxAngle = maxAngle;

  HAL_Servo_Attach(this->pin);

  return true;
}

bool Servo::detach()
{
  if (!this->attached())
  {
    return false;
  }

  HAL_Servo_Detach(this->pin);

  this->resetFields();

  return true;
}

void Servo::write(int degrees)
{
  degrees = constrain(degrees, this->minAngle, this->maxAngle);
  this->writeMicroseconds(ANGLE_TO_US(degrees)+trim);
}

int Servo::read() const
{
  int a = US_TO_ANGLE(this->readMicroseconds()-trim);
  // map() round-trips in a weird way we mostly correct for here;
  // the round-trip is still sometimes off-by-one for write(1) and
  // write(179).
  return a == this->minAngle || a == this->maxAngle ? a : a + 1;
}

void Servo::writeMicroseconds(uint16_t pulseWidth)
{

  if (!this->attached())
  {
    return;
  }

  pulseWidth = constrain(pulseWidth, this->minPW, this->maxPW);

  HAL_Servo_Write_Pulse_Width(this->pin, pulseWidth);
}

uint16_t Servo::readMicroseconds() const
{
  if (!this->attached())
  {
    return 0;
  }

  return HAL_Servo_Read_Pulse_Width(this->pin);
}

void Servo::resetFields(void)
{
  this->pin = NOT_ATTACHED;
  this->minAngle = SERVO_DEFAULT_MIN_ANGLE;
  this->maxAngle = SERVO_DEFAULT_MAX_ANGLE;
  this->minPW = SERVO_DEFAULT_MIN_PW;
  this->maxPW = SERVO_DEFAULT_MAX_PW;
  this->trim = 0;
}
