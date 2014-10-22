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
//#define LEDn                           		4
//#define LED1_GPIO_PIN                    	GPIO_Pin_13
//#define LED1_GPIO_PORT                   	GPIOA
//#define LED1_GPIO_CLK                    	RCC_APB2Periph_GPIOA
//#define LED2_GPIO_PIN                   	GPIO_Pin_8
//#define LED2_GPIO_PORT                   	GPIOA
//#define LED2_GPIO_CLK                    	RCC_APB2Periph_GPIOA
//#define LED3_GPIO_PIN                   	GPIO_Pin_9
//#define LED3_GPIO_PORT                   	GPIOA
//#define LED3_GPIO_CLK                    	RCC_APB2Periph_GPIOA
//#define LED4_GPIO_PIN                    	GPIO_Pin_10
//#define LED4_GPIO_PORT                   	GPIOA
//#define LED4_GPIO_CLK                    	RCC_APB2Periph_GPIOA

//Push Buttons
//#define BUTTONn                           	1
//#define BUTTON1_GPIO_PIN                 	GPIO_Pin_2
//#define BUTTON1_GPIO_PORT                	GPIOB
//#define BUTTON1_GPIO_CLK                 	RCC_APB2Periph_GPIOB
//#define BUTTON1_GPIO_MODE					GPIO_Mode_IPU
#define BUTTON1_PRESSED						0x00
//#define BUTTON1_EXTI_LINE                	EXTI_Line2
//#define BUTTON1_EXTI_PORT_SOURCE         	GPIO_PortSourceGPIOB
//#define BUTTON1_EXTI_PIN_SOURCE          	GPIO_PinSource2
//#define BUTTON1_EXTI_IRQn                	EXTI2_IRQn
//#define	BUTTON1_EXTI_TRIGGER				EXTI_Trigger_Falling
//#define BUTTON2_GPIO_PIN                 	0
//#define BUTTON2_GPIO_PORT               	0
//#define BUTTON2_GPIO_CLK                	0
//#define BUTTON2_GPIO_MODE					0
//#define BUTTON2_PRESSED						0
//#define BUTTON2_EXTI_LINE               	0
//#define BUTTON2_EXTI_PORT_SOURCE        	0
//#define BUTTON2_EXTI_PIN_SOURCE         	0
//#define BUTTON2_EXTI_IRQn               	0
//#define	BUTTON2_EXTI_TRIGGER				0

//#define USB_DISCONNECT_GPIO_PIN           	GPIO_Pin_10
//#define USB_DISCONNECT_GPIO_PORT       		GPIOB
//#define USB_DISCONNECT_GPIO_CLK		  		RCC_APB2Periph_GPIOB

//#define UI_TIMER_FREQUENCY					100							/* 100Hz -> 10ms */
//#define BUTTON_DEBOUNCE_INTERVAL			1000 / UI_TIMER_FREQUENCY

//NVIC Priorities based on NVIC_PriorityGroup_4
//#define DMA1_CHANNEL5_IRQ_PRIORITY			0	//CC3000_SPI_TX_DMA Interrupt
//#define EXTI15_10_IRQ_PRIORITY				1	//CC3000_WIFI_INT_EXTI & User Interrupt
//#define USB_LP_IRQ_PRIORITY					2	//USB_LP_CAN1_RX0 Interrupt
//#define RTCALARM_IRQ_PRIORITY				3	//RTC Alarm Interrupt
//#define RTC_IRQ_PRIORITY					4	//RTC Seconds Interrupt
//#define TIM1_CC_IRQ_PRIORITY				5	//TIM1_CC4 Interrupt
//#define EXTI2_IRQ_PRIORITY					6	//BUTTON1_EXTI Interrupt
//#define USART2_IRQ_PRIORITY					7	//USART2 Interrupt
//#define EXTI0_IRQ_PRIORITY					11	//User Interrupt
//#define EXTI1_IRQ_PRIORITY					11	//User Interrupt
//#define EXTI3_IRQ_PRIORITY					11	//User Interrupt
//#define EXTI4_IRQ_PRIORITY					11	//User Interrupt
//#define EXTI9_5_IRQ_PRIORITY				12	//User Interrupt
//#define SYSTICK_IRQ_PRIORITY				13	//CORTEX_M3 Systick Interrupt
//#define SVCALL_IRQ_PRIORITY					14	//CORTEX_M3 SVCall Interrupt
//#define PENDSV_IRQ_PRIORITY					15	//CORTEX_M3 PendSV Interrupt


#ifndef SPARK_PRODUCT_ID
#define SPARK_PRODUCT_ID 0
#warning "SPARK_PRODUCT_ID not defined, assuming 0"
#endif

#define PREPSTRING2(x) #x
#define PREPSTRING(x) PREPSTRING2(x)

#if SPARK_PRODUCT_ID < 2
#define INTERNAL_FLASH_SIZE     (0x20000)
#elif SPARK_PRODUCT_ID == 2
    #define INTERNAL_FLASH_SIZE (0x40000)
#elif SPARK_PRODUCT_ID == 4
    #define INTERNAL_FLASH_SIZE (0x100000)
#else
    #pragma message "SPARK_PRODUCT_ID is " PREPSTRING(SPARK_PRODUCT_ID)
    #error "Unknown SPARK_PRODUCT_ID"
#endif

/* Exported functions ------------------------------------------------------- */

#endif /* __PLATFORM_CONFIG_H */
