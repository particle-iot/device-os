/**
 ******************************************************************************
 * @file    interrupts_hal.c
 * @author  Satish Nair, Mohit Bhoite, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

/* Includes ------------------------------------------------------------------*/
#include "interrupts_hal.h"
#include "gpio_hal.h"
#include "pinmap_impl.h"
#include "stm32f10x.h"
#include <stddef.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
//Interrupts
const uint8_t GPIO_IRQn[] = {
    EXTI0_IRQn,     //0
    EXTI1_IRQn,     //1
    EXTI2_IRQn,     //2
    EXTI3_IRQn,     //3
    EXTI4_IRQn,     //4
    EXTI9_5_IRQn,   //5
    EXTI9_5_IRQn,   //6
    EXTI9_5_IRQn,   //7
    EXTI9_5_IRQn,   //8
    EXTI9_5_IRQn,   //9
    EXTI15_10_IRQn, //10
    EXTI15_10_IRQn, //11
    EXTI15_10_IRQn, //12
    EXTI15_10_IRQn, //13
    EXTI15_10_IRQn, //14
    EXTI15_10_IRQn  //15
};

// Create a structure for user ISR function pointers
typedef struct exti_channel {
    HAL_InterruptHandler fn;
    void* data;
} exti_channel;

//Array to hold user ISR function pointers
static exti_channel exti_channels[16];
/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/



void HAL_Interrupts_Attach(uint16_t pin, HAL_InterruptHandler handler, void* data, InterruptMode mode, HAL_InterruptExtraConfiguration* config)
{
  uint8_t GPIO_PortSource = 0;    //variable to hold the port number

  //EXTI structure to init EXT
  EXTI_InitTypeDef EXTI_InitStructure = {0};
  //NVIC structure to set up NVIC controller
  NVIC_InitTypeDef NVIC_InitStructure = {0};

  //Map the Spark pin to the appropriate port and pin on the STM32
  GPIO_TypeDef *gpio_port = PIN_MAP[pin].gpio_peripheral;
  uint16_t gpio_pin = PIN_MAP[pin].gpio_pin;
  uint8_t GPIO_PinSource = PIN_MAP[pin].gpio_pin_source;

  //Clear pending EXTI interrupt flag for the selected pin
  EXTI_ClearITPendingBit(gpio_pin);

  //Select the port source
  if (gpio_port == GPIOA )
  {
    GPIO_PortSource = 0;
  }
  else if (gpio_port == GPIOB )
  {
    GPIO_PortSource = 1;
  }

  // Register the handler for the user function name
  exti_channels[GPIO_PinSource].fn = handler;
  exti_channels[GPIO_PinSource].data = data;

  //Connect EXTI Line to appropriate Pin
  GPIO_EXTILineConfig(GPIO_PortSource, GPIO_PinSource);

  //Configure GPIO EXTI line
  EXTI_InitStructure.EXTI_Line = gpio_pin;//EXTI_Line;

  //select the interrupt mode
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  switch (mode)
  {
    //case LOW:
    //There is no LOW mode in STM32, so using falling edge as default
    //EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    //break;
    case CHANGE:
      //generate interrupt on rising or falling edge
      EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
      break;
    case RISING:
      //generate interrupt on rising edge
      EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
      break;
    case FALLING:
      //generate interrupt on falling edge
      EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
      break;
  }

  //enable EXTI line
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  //send values to registers
  EXTI_Init(&EXTI_InitStructure);

  if(GPIO_PinSource < 10)//Should not try changing the priority of EXTI15_10_IRQn
  {
    //configure NVIC
    //select NVIC channel to configure
    NVIC_InitStructure.NVIC_IRQChannel = GPIO_IRQn[GPIO_PinSource];
    if (config == NULL) {
      if(GPIO_PinSource > 4)
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 14;
      else
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 13;
      NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    } else {
      NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = config->IRQChannelPreemptionPriority;
      NVIC_InitStructure.NVIC_IRQChannelSubPriority = config->IRQChannelSubPriority;
    }
    //enable IRQ channel
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    //update NVIC registers
    NVIC_Init(&NVIC_InitStructure);
  }
}

void HAL_Interrupts_Detach(uint16_t pin)
{
  //Map the Spark Core pin to the appropriate pin on the STM32
  uint16_t gpio_pin = PIN_MAP[pin].gpio_pin;
  uint8_t GPIO_PinSource = PIN_MAP[pin].gpio_pin_source;

  //Clear the pending interrupt flag for that interrupt pin
  EXTI_ClearITPendingBit(gpio_pin);

  //EXTI structure to init EXT
  EXTI_InitTypeDef EXTI_InitStructure = {0};

  if(gpio_pin != EXTI_Line2 || gpio_pin != EXTI_Line11)
  {
    //Select the appropriate EXTI line
    EXTI_InitStructure.EXTI_Line = gpio_pin;
    //disable that EXTI line
    EXTI_InitStructure.EXTI_LineCmd = DISABLE;
    //send values to registers
    EXTI_Init(&EXTI_InitStructure);
  }

  //unregister the user's handler
  exti_channels[GPIO_PinSource].fn = NULL;
  exti_channels[GPIO_PinSource].data = NULL;
}

void HAL_Interrupts_Enable_All(void)
{
  NVIC_EnableIRQ(EXTI0_IRQn);
  NVIC_EnableIRQ(EXTI1_IRQn);
  NVIC_EnableIRQ(EXTI3_IRQn);
  NVIC_EnableIRQ(EXTI4_IRQn);
  NVIC_EnableIRQ(EXTI9_5_IRQn);
}

void HAL_Interrupts_Disable_All(void)
{
  NVIC_DisableIRQ(EXTI0_IRQn);
  NVIC_DisableIRQ(EXTI1_IRQn);
  NVIC_DisableIRQ(EXTI3_IRQn);
  NVIC_DisableIRQ(EXTI4_IRQn);
  NVIC_DisableIRQ(EXTI9_5_IRQn);
}

void HAL_Interrupts_Suspend(void)
{
  // Untested/Unsupported
}

void HAL_Interrupts_Restore(void)
{
  // Untested/Unsupported
}

/*******************************************************************************
 * Function Name  : HAL_EXTI_Handler (Declared as weak in stm32_it.cpp)
 * Description    : This function is called by any of the interrupt handlers. It
                                                 essentially fetches the user function pointer from the array
                                                 and calls it.
 * Input          : EXTI_Line (Supported: 0, 1, 3, 4, 5, 6, 7, 13, 14 and 15)
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_EXTI_Handler(uint8_t EXTI_Line)
{
    HAL_Interrupts_Trigger(EXTI_Line, NULL);
}

void HAL_Interrupts_Trigger(uint16_t EXTI_Line, void* reserved)
{
    void* data = exti_channels[EXTI_Line].data;
    HAL_InterruptHandler userISR_Handle = exti_channels[EXTI_Line].fn;

    if (userISR_Handle)
        userISR_Handle(data);
}

int HAL_disable_irq()
{
  int is = __get_PRIMASK();
  __disable_irq();
  return is;
}

void HAL_enable_irq(int is) {
    if ((is & 1) == 0) {
        __enable_irq();
    }
}


inline bool isISR()
{
	return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
}

uint8_t HAL_IsISR()
{
	return isISR();
}



