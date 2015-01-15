/**
 ******************************************************************************
 * @file    platform_config.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    22-Oct-2014
 * @brief   Board specific configuration file.
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PLATFORM_CONFIG_H
#define __PLATFORM_CONFIG_H

/* If serial flash is present on board uncomment this define for "bootloader" use */
//#define SPARK_SFLASH_ENABLE

#define         ID1          (0x1FFF7A10)
#define         ID2          (0x1FFF7A14)
#define         ID3          (0x1FFF7A18)

/* Includes ------------------------------------------------------------------*/
#include "stm32f2xx.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

//LEDs
#define LEDn                           		4
#define LED1_GPIO_AF_TIM                        0                       //To Decide
#define LED1_GPIO_PIN                    	0                       //To Decide
#define LED1_GPIO_PIN_SOURCE                    0                       //To Decide
#define LED1_GPIO_PORT                   	0                       //To Decide
#define LED1_GPIO_CLK                    	0                       //To Decide
#define LED2_GPIO_AF_TIM                        GPIO_AF_TIM2            //BLUE Led
#define LED2_GPIO_PIN                   	GPIO_Pin_3              //BLUE Led
#define LED2_GPIO_PIN_SOURCE                    GPIO_PinSource3         //BLUE Led
#define LED2_GPIO_PORT                   	GPIOA                   //BLUE Led
#define LED2_GPIO_CLK                    	RCC_AHB1Periph_GPIOA    //BLUE Led
#define LED3_GPIO_AF_TIM                        GPIO_AF_TIM2            //RED Led
#define LED3_GPIO_PIN                   	GPIO_Pin_1              //RED Led
#define LED3_GPIO_PIN_SOURCE                    GPIO_PinSource1         //RED Led
#define LED3_GPIO_PORT                   	GPIOA                   //RED Led
#define LED3_GPIO_CLK                    	RCC_AHB1Periph_GPIOA    //RED Led
#define LED4_GPIO_AF_TIM                        GPIO_AF_TIM2            //GREEN Led
#define LED4_GPIO_PIN                    	GPIO_Pin_2              //GREEN Led
#define LED4_GPIO_PIN_SOURCE                    GPIO_PinSource2         //GREEN Led
#define LED4_GPIO_PORT                   	GPIOA                   //GREEN Led
#define LED4_GPIO_CLK                    	RCC_AHB1Periph_GPIOA    //GREEN Led

//Push Buttons
#define BUTTONn                                 1
#define BUTTON1_GPIO_PIN                        GPIO_Pin_2
#define BUTTON1_GPIO_PORT                       GPIOC
#define BUTTON1_GPIO_CLK                        RCC_AHB1Periph_GPIOC
#define BUTTON1_GPIO_MODE		        GPIO_Mode_IN
#define BUTTON1_GPIO_PUPD                       GPIO_PuPd_UP
#define BUTTON1_PRESSED			        0x00
#define BUTTON1_EXTI_LINE                       EXTI_Line2
#define BUTTON1_EXTI_PORT_SOURCE                EXTI_PortSourceGPIOC
#define BUTTON1_EXTI_PIN_SOURCE                 EXTI_PinSource2
#define BUTTON1_EXTI_IRQn                       EXTI2_IRQn
#define	BUTTON1_EXTI_TRIGGER		        EXTI_Trigger_Falling

#define UI_TIMER_FREQUENCY                      100	/* 100Hz -> 10ms */
#define BUTTON_DEBOUNCE_INTERVAL		1000 / UI_TIMER_FREQUENCY

//NVIC Priorities based on NVIC_PriorityGroup_4
#define SDIO_IRQ_PRIORITY                       0       //??? BCM43362 SDIO Interrupt
#define OTG_HS_IRQ_PRIORITY                     2       //USB OTG HS Interrupt
#define OTG_HS_WKUP_IRQ_PRIORITY                2       //USB OTG HS Wakeup Interrupt
#define RTC_Alarm_IRQ_PRIORITY                  3       //RTC Alarm Interrupt
#define RTC_WKUP_IRQ_PRIORITY                   4       //RTC Seconds Interrupt
#define USART1_IRQ_PRIORITY                     5       //USART1 Interrupt
#define USART2_IRQ_PRIORITY                     5       //USART2 Interrupt
#define TIM2_IRQ_PRIORITY                       6       //TIM2 CC Interrupt(Button Use)
#define EXTI2_IRQ_PRIORITY                      7       //Mode Button
#define EXTI15_10_IRQ_PRIORITY                  8       //??? User Interrupt
#define EXTI9_5_IRQ_PRIORITY                    9       //??? User Interrupt
#define EXTI0_IRQ_PRIORITY                      10      //??? User Interrupt
#define EXTI1_IRQ_PRIORITY                      10      //??? User Interrupt
#define EXTI3_IRQ_PRIORITY                      10      //??? User Interrupt
#define EXTI4_IRQ_PRIORITY                      10      //??? User Interrupt
#define SYSTICK_IRQ_PRIORITY                    13      //CORTEX_M3 Systick Interrupt
#define SVCALL_IRQ_PRIORITY                     14      //CORTEX_M3 SVCall Interrupt
#define PENDSV_IRQ_PRIORITY                     15      //CORTEX_M3 PendSV Interrupt

#ifndef PLATFORM_ID
#define PLATFORM_ID 0
#warning "PLATFORM_ID not defined, assuming 0"
#endif

#define PREPSTRING2(x) #x
#define PREPSTRING(x) PREPSTRING2(x)

#if PLATFORM_ID < 2
#define INTERNAL_FLASH_SIZE     (0x20000)
#elif PLATFORM_ID == 2
    #define INTERNAL_FLASH_SIZE (0x40000)
#elif PLATFORM_ID == 4
    #define INTERNAL_FLASH_SIZE (0x100000)
#else
    #pragma message "PLATFORM_ID is " PREPSTRING(PLATFORM_ID)
    #error "Unknown PLATFORM_ID"
#endif

/* Exported functions ------------------------------------------------------- */

#endif /* __PLATFORM_CONFIG_H */
