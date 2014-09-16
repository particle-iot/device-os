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
#include "spark_wiring_usartserial.h"
#include "spark_wiring_spi.h"
#include "spark_wiring_i2c.h"

#define SERVO_TIM_PWM_FREQ	50											//20ms = 50Hz
#define SERVO_TIM_PRESCALER	(uint16_t)(SystemCoreClock / 1000000) - 1		//To get TIM counter clock = 1MHz
#define SERVO_TIM_ARR		(uint16_t)(1000000 / SERVO_TIM_PWM_FREQ) - 1	//To get PWM period = 20ms

#define ANGLE_TO_US(a)    ((uint16_t)(map((a), this->minAngle, this->maxAngle, \
                                        this->minPW, this->maxPW)))
#define US_TO_ANGLE(us)   ((int16_t)(map((us), this->minPW, this->maxPW,  \
                                       this->minAngle, this->maxAngle)))

Servo::Servo() {
    this->resetFields();
}

bool Servo::attach(uint16_t pin,
                   uint16_t minPW,
                   uint16_t maxPW,
                   int16_t minAngle,
                   int16_t maxAngle) {

    if (pin >= TOTAL_PINS || PIN_MAP[pin].timer_peripheral == NULL)
    {
        return false;
    }

    // SPI safety check
    if (SPI.isEnabled() == true && (pin == SCK || pin == MOSI || pin == MISO))
    {
        return false;
    }

    // I2C safety check
    if (Wire.isEnabled() == true && (pin == SCL || pin == SDA))
    {
        return false;
    }

    // Serial1 safety check
    if (Serial1.isEnabled() == true && (pin == RX || pin == TX))
    {
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

	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;

	// AFIO clock enable
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	pinMode(pin, AF_OUTPUT_PUSHPULL);

	// TIM clock enable
	if(PIN_MAP[pin].timer_peripheral == TIM2)
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	else if(PIN_MAP[pin].timer_peripheral == TIM3)
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	else if(PIN_MAP[pin].timer_peripheral == TIM4)
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	// Time base configuration
	TIM_TimeBaseStructure.TIM_Period = SERVO_TIM_ARR;
	TIM_TimeBaseStructure.TIM_Prescaler = SERVO_TIM_PRESCALER;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(PIN_MAP[pin].timer_peripheral, &TIM_TimeBaseStructure);

    // PWM1 Mode configuration
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_Pulse = 0x0000;

    if(PIN_MAP[this->pin].timer_ch == TIM_Channel_1)
    {
        // PWM1 Mode configuration: Channel1
        TIM_OC1Init(PIN_MAP[this->pin].timer_peripheral, &TIM_OCInitStructure);
        TIM_OC1PreloadConfig(PIN_MAP[this->pin].timer_peripheral, TIM_OCPreload_Enable);
    }
    else if(PIN_MAP[this->pin].timer_ch == TIM_Channel_2)
    {
        // PWM1 Mode configuration: Channel2
        TIM_OC2Init(PIN_MAP[this->pin].timer_peripheral, &TIM_OCInitStructure);
        TIM_OC2PreloadConfig(PIN_MAP[this->pin].timer_peripheral, TIM_OCPreload_Enable);
    }
    else if(PIN_MAP[this->pin].timer_ch == TIM_Channel_3)
    {
        // PWM1 Mode configuration: Channel3
        TIM_OC3Init(PIN_MAP[this->pin].timer_peripheral, &TIM_OCInitStructure);
        TIM_OC3PreloadConfig(PIN_MAP[this->pin].timer_peripheral, TIM_OCPreload_Enable);
    }
    else if(PIN_MAP[this->pin].timer_ch == TIM_Channel_4)
    {
        // PWM1 Mode configuration: Channel4
        TIM_OC4Init(PIN_MAP[this->pin].timer_peripheral, &TIM_OCInitStructure);
        TIM_OC4PreloadConfig(PIN_MAP[this->pin].timer_peripheral, TIM_OCPreload_Enable);
    }

    TIM_ARRPreloadConfig(PIN_MAP[this->pin].timer_peripheral, ENABLE);

    // TIM enable counter
    TIM_Cmd(PIN_MAP[this->pin].timer_peripheral, ENABLE);

	// Main Output Enable
	TIM_CtrlPWMOutputs(PIN_MAP[this->pin].timer_peripheral, ENABLE);

    return true;
}

bool Servo::detach() {
    if (!this->attached()) {
        return false;
    }
    
	// TIM disable counter
	TIM_Cmd(PIN_MAP[this->pin].timer_peripheral, DISABLE);

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

void Servo::writeMicroseconds(uint16_t pulseWidth) {

    if (!this->attached()) {
         return;
    }

    pulseWidth = constrain(pulseWidth, this->minPW, this->maxPW);

    //SERVO_TIM_CCR = pulseWidth * (SERVO_TIM_ARR + 1) * SERVO_TIM_PWM_FREQ / 1000000;
    uint16_t SERVO_TIM_CCR = pulseWidth;

    if(PIN_MAP[this->pin].timer_ch == TIM_Channel_1)
    {
        TIM_SetCompare1(PIN_MAP[this->pin].timer_peripheral, SERVO_TIM_CCR);
    }
    else if(PIN_MAP[this->pin].timer_ch == TIM_Channel_2)
    {
        TIM_SetCompare2(PIN_MAP[this->pin].timer_peripheral, SERVO_TIM_CCR);
    }
    else if(PIN_MAP[this->pin].timer_ch == TIM_Channel_3)
    {
        TIM_SetCompare3(PIN_MAP[this->pin].timer_peripheral, SERVO_TIM_CCR);
    }
    else if(PIN_MAP[this->pin].timer_ch == TIM_Channel_4)
    {
        TIM_SetCompare4(PIN_MAP[this->pin].timer_peripheral, SERVO_TIM_CCR);
    }
}

uint16_t Servo::readMicroseconds() const {
    if (!this->attached()) {
        return 0;
    }

    uint16_t SERVO_TIM_CCR = 0x0000;

    if(PIN_MAP[this->pin].timer_ch == TIM_Channel_1)
    {
    	SERVO_TIM_CCR = TIM_GetCapture1(PIN_MAP[this->pin].timer_peripheral);
    }
    else if(PIN_MAP[this->pin].timer_ch == TIM_Channel_2)
    {
    	SERVO_TIM_CCR = TIM_GetCapture2(PIN_MAP[this->pin].timer_peripheral);
    }
    else if(PIN_MAP[this->pin].timer_ch == TIM_Channel_3)
    {
    	SERVO_TIM_CCR = TIM_GetCapture3(PIN_MAP[this->pin].timer_peripheral);
    }
    else if(PIN_MAP[this->pin].timer_ch == TIM_Channel_4)
    {
    	SERVO_TIM_CCR = TIM_GetCapture4(PIN_MAP[this->pin].timer_peripheral);
    }

    //pulseWidth = (SERVO_TIM_CCR * 1000000) / ((SERVO_TIM_ARR + 1) * SERVO_TIM_PWM_FREQ);
    return SERVO_TIM_CCR;
}

void Servo::resetFields(void) {
    this->pin = NOT_ATTACHED;
    this->minAngle = SERVO_DEFAULT_MIN_ANGLE;
    this->maxAngle = SERVO_DEFAULT_MAX_ANGLE;
    this->minPW = SERVO_DEFAULT_MIN_PW;
    this->maxPW = SERVO_DEFAULT_MAX_PW;
}
