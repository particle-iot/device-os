/**
 ******************************************************************************
 * @file    platform_config.h
 * @author  Satish Nair, Zachary Crockett and Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Board specific configuration file.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PLATFORM_CONFIG_H
#define __PLATFORM_CONFIG_H

#include "platforms.h"

/* If serial flash is present on board uncomment this define for "bootloader" use */
#define SPARK_SFLASH_ENABLE

#define         ID1          (0x1FFFF7E8)
#define         ID2          (0x1FFFF7EC)
#define         ID3          (0x1FFFF7F0)

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

//LEDs
#define LEDn                           		4
#define LED1_GPIO_PIN                    	GPIO_Pin_13
#define LED1_GPIO_PORT                   	GPIOA
#define LED1_GPIO_CLK                    	RCC_APB2Periph_GPIOA
#define LED2_GPIO_PIN                   	GPIO_Pin_8
#define LED2_GPIO_PORT                   	GPIOA
#define LED2_GPIO_CLK                    	RCC_APB2Periph_GPIOA
#define LED3_GPIO_PIN                   	GPIO_Pin_9
#define LED3_GPIO_PORT                   	GPIOA
#define LED3_GPIO_CLK                    	RCC_APB2Periph_GPIOA
#define LED4_GPIO_PIN                    	GPIO_Pin_10
#define LED4_GPIO_PORT                   	GPIOA
#define LED4_GPIO_CLK                    	RCC_APB2Periph_GPIOA

//Push Buttons
#define BUTTONn                           	1
#define BUTTON1_GPIO_PIN                 	GPIO_Pin_2
#define BUTTON1_GPIO_PORT                	GPIOB
#define BUTTON1_GPIO_CLK                 	RCC_APB2Periph_GPIOB
#define BUTTON1_GPIO_MODE					GPIO_Mode_IPU
#define BUTTON1_PRESSED						0x00
#define BUTTON1_EXTI_LINE                	EXTI_Line2
#define BUTTON1_EXTI_PORT_SOURCE         	GPIO_PortSourceGPIOB
#define BUTTON1_EXTI_PIN_SOURCE          	GPIO_PinSource2
#define BUTTON1_EXTI_IRQn                	EXTI2_IRQn
#define	BUTTON1_EXTI_TRIGGER				EXTI_Trigger_Falling
#define BUTTON2_GPIO_PIN                 	0
#define BUTTON2_GPIO_PORT               	0
#define BUTTON2_GPIO_CLK                	0
#define BUTTON2_GPIO_MODE					0
#define BUTTON2_PRESSED						0
#define BUTTON2_EXTI_LINE               	0
#define BUTTON2_EXTI_PORT_SOURCE        	0
#define BUTTON2_EXTI_PIN_SOURCE         	0
#define BUTTON2_EXTI_IRQn               	0
#define	BUTTON2_EXTI_TRIGGER				0

//Header IOs
#define Dn                           		8
#define D0_GPIO_PIN                       	GPIO_Pin_7
#define D0_GPIO_PORT                   		GPIOB
#define D0_GPIO_CLK                    		RCC_APB2Periph_GPIOB
#define D1_GPIO_PIN                       	GPIO_Pin_6
#define D1_GPIO_PORT                   		GPIOB
#define D1_GPIO_CLK                    		RCC_APB2Periph_GPIOB
#define D2_GPIO_PIN                     	GPIO_Pin_5
#define D2_GPIO_PORT                   		GPIOB
#define D2_GPIO_CLK                    		RCC_APB2Periph_GPIOB
#define D3_GPIO_PIN                      	GPIO_Pin_4
#define D3_GPIO_PORT                   		GPIOB
#define D3_GPIO_CLK                    		RCC_APB2Periph_GPIOB
#define D4_GPIO_PIN                      	GPIO_Pin_3
#define D4_GPIO_PORT                   		GPIOB
#define D4_GPIO_CLK                    		RCC_APB2Periph_GPIOB
#define D5_GPIO_PIN                     	GPIO_Pin_15
#define D5_GPIO_PORT                   		GPIOA
#define D5_GPIO_CLK                    		RCC_APB2Periph_GPIOA
#define D6_GPIO_PIN                      	GPIO_Pin_14
#define D6_GPIO_PORT                   		GPIOA
#define D6_GPIO_CLK                    		RCC_APB2Periph_GPIOA
#define D7_GPIO_PIN                      	GPIO_Pin_13
#define D7_GPIO_PORT                   		GPIOA
#define D7_GPIO_CLK                    		RCC_APB2Periph_GPIOA

//CC3000 Interface pins
#define CC3000_SPI							SPI2
#define CC3000_SPI_CLK						RCC_APB1Periph_SPI2
#define CC3000_SPI_CLK_CMD					RCC_APB1PeriphClockCmd
#define CC3000_SPI_SCK_GPIO_PIN				GPIO_Pin_13					/* PB.13 */
#define CC3000_SPI_SCK_GPIO_PORT			GPIOB						/* GPIOB */
#define CC3000_SPI_SCK_GPIO_CLK				RCC_APB2Periph_GPIOB
#define CC3000_SPI_MISO_GPIO_PIN			GPIO_Pin_14					/* PB.14 */
#define CC3000_SPI_MISO_GPIO_PORT			GPIOB						/* GPIOB */
#define CC3000_SPI_MISO_GPIO_CLK			RCC_APB2Periph_GPIOB
#define CC3000_SPI_MOSI_GPIO_PIN			GPIO_Pin_15					/* PB.15 */
#define CC3000_SPI_MOSI_GPIO_PORT			GPIOB						/* GPIOB */
#define CC3000_SPI_MOSI_GPIO_CLK			RCC_APB2Periph_GPIOB
#define CC3000_WIFI_CS_GPIO_PIN				GPIO_Pin_12					/* PB.12 */
#define CC3000_WIFI_CS_GPIO_PORT			GPIOB						/* GPIOB */
#define CC3000_WIFI_CS_GPIO_CLK				RCC_APB2Periph_GPIOB
#define CC3000_WIFI_EN_GPIO_PIN				GPIO_Pin_8					/* PB.08 */
#define CC3000_WIFI_EN_GPIO_PORT			GPIOB						/* GPIOB */
#define CC3000_WIFI_EN_GPIO_CLK				RCC_APB2Periph_GPIOB
#define CC3000_WIFI_INT_GPIO_PIN			GPIO_Pin_11					/* PB.11 */
#define CC3000_WIFI_INT_GPIO_PORT			GPIOB						/* GPIOB */
#define CC3000_WIFI_INT_GPIO_CLK			RCC_APB2Periph_GPIOB

#define CC3000_WIFI_INT_EXTI_LINE           EXTI_Line11
#define CC3000_WIFI_INT_EXTI_PORT_SOURCE    GPIO_PortSourceGPIOB
#define CC3000_WIFI_INT_EXTI_PIN_SOURCE     GPIO_PinSource11
#define CC3000_WIFI_INT_EXTI_IRQn           EXTI15_10_IRQn

#define CC3000_SPI_DMA_CLK                  RCC_AHBPeriph_DMA1
#define CC3000_SPI_RX_DMA_CHANNEL           DMA1_Channel4
#define CC3000_SPI_TX_DMA_CHANNEL           DMA1_Channel5
#define CC3000_SPI_RX_DMA_TCFLAG            DMA1_FLAG_TC4
#define CC3000_SPI_TX_DMA_TCFLAG            DMA1_FLAG_TC5
#define CC3000_SPI_RX_DMA_IRQn           	DMA1_Channel4_IRQn
#define CC3000_SPI_TX_DMA_IRQn           	DMA1_Channel5_IRQn

#define CC3000_SPI_DR_BASE                  ((uint32_t)0x4000380C)	/* SPI2_BASE | 0x0C */

#define CC3000_SPI_BAUDRATE_PRESCALER		SPI_BaudRatePrescaler_4

//SST25 FLASH Interface pins
#define sFLASH_SPI							SPI2
#define sFLASH_SPI_CLK						RCC_APB1Periph_SPI2
#define sFLASH_SPI_CLK_CMD					RCC_APB1PeriphClockCmd
#define sFLASH_SPI_SCK_GPIO_PIN				GPIO_Pin_13					/* PB.13 */
#define sFLASH_SPI_SCK_GPIO_PORT			GPIOB						/* GPIOB */
#define sFLASH_SPI_SCK_GPIO_CLK				RCC_APB2Periph_GPIOB
#define sFLASH_SPI_MISO_GPIO_PIN			GPIO_Pin_14					/* PB.14 */
#define sFLASH_SPI_MISO_GPIO_PORT			GPIOB						/* GPIOB */
#define sFLASH_SPI_MISO_GPIO_CLK			RCC_APB2Periph_GPIOB
#define sFLASH_SPI_MOSI_GPIO_PIN			GPIO_Pin_15					/* PB.15 */
#define sFLASH_SPI_MOSI_GPIO_PORT			GPIOB						/* GPIOB */
#define sFLASH_SPI_MOSI_GPIO_CLK			RCC_APB2Periph_GPIOB
#define sFLASH_MEM_CS_GPIO_PIN				GPIO_Pin_9					/* PB.09 */
#define sFLASH_MEM_CS_GPIO_PORT				GPIOB						/* GPIOB */
#define sFLASH_MEM_CS_GPIO_CLK				RCC_APB2Periph_GPIOB

#define sFLASH_SPI_BAUDRATE_PRESCALER		SPI_BaudRatePrescaler_2

#define USB_DISCONNECT_GPIO_PIN           	GPIO_Pin_10
#define USB_DISCONNECT_GPIO_PORT       		GPIOB
#define USB_DISCONNECT_GPIO_CLK		  		RCC_APB2Periph_GPIOB

#define UI_TIMER_FREQUENCY					100							/* 100Hz -> 10ms */
#define BUTTON_DEBOUNCE_INTERVAL			1000 / UI_TIMER_FREQUENCY

//NVIC Priorities based on NVIC_PriorityGroup_4
#define DMA1_CHANNEL5_IRQ_PRIORITY			0	//CC3000_SPI_TX_DMA Interrupt
#define EXTI15_10_IRQ_PRIORITY				1	//CC3000_WIFI_INT_EXTI & User Interrupt
#define USB_LP_IRQ_PRIORITY					2	//USB_LP_CAN1_RX0 Interrupt
#define RTCALARM_IRQ_PRIORITY				3	//RTC Alarm Interrupt
#define RTC_IRQ_PRIORITY					4	//RTC Seconds Interrupt
#define TIM1_CC_IRQ_PRIORITY				5	//TIM1_CC4 Interrupt
#define EXTI2_IRQ_PRIORITY					6	//BUTTON1_EXTI Interrupt
#define USART2_IRQ_PRIORITY					7	//USART2 Interrupt
#define EXTI0_IRQ_PRIORITY					11	//User Interrupt
#define EXTI1_IRQ_PRIORITY					11	//User Interrupt
#define EXTI3_IRQ_PRIORITY					11	//User Interrupt
#define EXTI4_IRQ_PRIORITY					11	//User Interrupt
#define EXTI9_5_IRQ_PRIORITY				12	//User Interrupt
#define SYSTICK_IRQ_PRIORITY				13	//CORTEX_M3 Systick Interrupt
#define SVCALL_IRQ_PRIORITY					14	//CORTEX_M3 SVCall Interrupt
#define PENDSV_IRQ_PRIORITY					15	//CORTEX_M3 PendSV Interrupt


#ifndef PLATFORM_ID
#define PLATFORM_ID PLATFORM_SPARK_CORE
#warning "PLATFORM_ID not defined, assuming 0"
#endif

#define PREPSTRING2(x) #x
#define PREPSTRING(x) PREPSTRING2(x)

#if PLATFORM_ID == PLATFORM_SPARK_CORE
#define INTERNAL_FLASH_SIZE     (0x20000)
#elif PLATFORM_ID == PLATFORM_SPARK_CORE_HD
    #define INTERNAL_FLASH_SIZE (0x40000)
#else
    #pragma message "PLATFORM_ID is " PREPSTRING(PLATFORM_ID)
    #error "Unknown PLATFORM_ID"
#endif

/* Exported functions ------------------------------------------------------- */

#endif /* __PLATFORM_CONFIG_H */
