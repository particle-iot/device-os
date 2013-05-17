/**
  ******************************************************************************
  * @file    platform_config.h
  * @author  Spark Application Team
  * @version V1.0.0
  * @date    30-April-2013
  * @brief   Board specific configuration file.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PLATFORM_CONFIG_H
#define __PLATFORM_CONFIG_H

/* Uncomment the line corresponding to the STM32 board used */
#if !defined (USE_SPARK_CORE) &&  !defined (USE_OLIMEX_H103)
#define USE_SPARK_CORE
//#define USE_OLIMEX_H103
#endif

/* Uncomment the line below to enable SFLASH functionality */
//#define SPARK_SFLASH_ENABLE

#define         ID1          (0x1FFFF7E8)
#define         ID2          (0x1FFFF7EC)
#define         ID3          (0x1FFFF7F0)

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Define the STM32F10x hardware depending on the used board */
#if defined (USE_SPARK_CORE)

//LEDs
#define LEDn                           		2
#define LED1_PIN                         	GPIO_Pin_8
#define LED1_GPIO_PORT                   	GPIOA
#define LED1_GPIO_CLK                    	RCC_APB2Periph_GPIOA
#define LED2_PIN                         	GPIO_Pin_9
#define LED2_GPIO_PORT                   	GPIOA
#define LED2_GPIO_CLK                    	RCC_APB2Periph_GPIOA

//Push Buttons
#define BUTTONn                           	2
#define BUTTON1_PIN                      	GPIO_Pin_10
#define BUTTON1_GPIO_PORT                	GPIOA
#define BUTTON1_GPIO_CLK                 	RCC_APB2Periph_GPIOA
#define BUTTON1_GPIO_MODE					GPIO_Mode_IPU
#define BUTTON1_PRESSED						0x00
#define BUTTON1_EXTI_LINE                	EXTI_Line10
#define BUTTON1_EXTI_PORT_SOURCE         	GPIO_PortSourceGPIOA
#define BUTTON1_EXTI_PIN_SOURCE          	GPIO_PinSource10
#define BUTTON1_EXTI_IRQn                	EXTI15_10_IRQn
#define	BUTTON1_EXTI_TRIGGER				EXTI_Trigger_Falling
#define BUTTON2_PIN                     	0//GPIO_Pin_y
#define BUTTON2_GPIO_PORT               	0//GPIOY
#define BUTTON2_GPIO_CLK                	0//RCC_APB2Periph_GPIOY
#define BUTTON2_GPIO_MODE					0//GPIO_Mode_IN_FLOATING
#define BUTTON2_PRESSED						0//0x00
#define BUTTON2_EXTI_LINE               	0//EXTI_Liney
#define BUTTON2_EXTI_PORT_SOURCE        	0//GPIO_PortSourceGPIOY
#define BUTTON2_EXTI_PIN_SOURCE         	0//GPIO_PinSourcey
#define BUTTON2_EXTI_IRQn               	0//EXTIy_IRQn
#define	BUTTON2_EXTI_TRIGGER				0//EXTI_Trigger_Falling

//Button Debounce Timer
#define DEBOUNCE_FREQ						100	//100 Hz => 10ms
#define DEBOUNCE_TIMER						TIM1
#define DEBOUNCE_TIMER_CLK					RCC_APB2Periph_TIM1
#define DEBOUNCE_TIMER_CLK_CMD				RCC_APB2PeriphClockCmd
#define DEBOUNCE_TIMER_FLAG            		TIM_IT_Update
#define DEBOUNCE_TIMER_IRQn           		TIM1_UP_IRQn

//SST25 FLASH Interface pins
#define sFLASH_SPI							SPI2
#define sFLASH_SPI_CLK						RCC_APB1Periph_SPI2
#define sFLASH_SPI_CLK_CMD					RCC_APB1PeriphClockCmd
#define sFLASH_SPI_SCK_PIN					GPIO_Pin_13					/* PB.13 */
#define sFLASH_SPI_SCK_GPIO_PORT			GPIOB						/* GPIOB */
#define sFLASH_SPI_SCK_GPIO_CLK				RCC_APB2Periph_GPIOB
#define sFLASH_SPI_MISO_PIN					GPIO_Pin_14					/* PB.14 */
#define sFLASH_SPI_MISO_GPIO_PORT			GPIOB						/* GPIOB */
#define sFLASH_SPI_MISO_GPIO_CLK			RCC_APB2Periph_GPIOB
#define sFLASH_SPI_MOSI_PIN					GPIO_Pin_15					/* PB.15 */
#define sFLASH_SPI_MOSI_GPIO_PORT			GPIOB						/* GPIOB */
#define sFLASH_SPI_MOSI_GPIO_CLK			RCC_APB2Periph_GPIOB
#define sFLASH_MEM_CS_PIN					GPIO_Pin_9					/* PB.09 */
#define sFLASH_MEM_CS_GPIO_PORT				GPIOB						/* GPIOB */
#define sFLASH_MEM_CS_GPIO_CLK				RCC_APB2Periph_GPIOB

#define sFLASH_SPI_BAUDRATE_PRESCALER		SPI_BaudRatePrescaler_32

#define USB_DISCONNECT_PIN               	GPIO_Pin_10
#define USB_DISCONNECT_GPIO_PORT       		GPIOB
#define USB_DISCONNECT_GPIO_CLK		  		RCC_APB2Periph_GPIOB

#elif defined (USE_OLIMEX_H103)

//LEDs
#define LEDn                           		2
#define LED1_PIN                         	GPIO_Pin_12
#define LED1_GPIO_PORT                   	GPIOC
#define LED1_GPIO_CLK                    	RCC_APB2Periph_GPIOC
#define LED2_PIN                         	GPIO_Pin_5
#define LED2_GPIO_PORT                   	GPIOB
#define LED2_GPIO_CLK                    	RCC_APB2Periph_GPIOB

//Push Buttons
#define BUTTONn                           	2
#define BUTTON1_PIN                      	GPIO_Pin_0
#define BUTTON1_GPIO_PORT                	GPIOA
#define BUTTON1_GPIO_CLK                 	RCC_APB2Periph_GPIOA
#define BUTTON1_GPIO_MODE					GPIO_Mode_IN_FLOATING
#define BUTTON1_PRESSED						0x00
#define BUTTON1_EXTI_LINE                	EXTI_Line0
#define BUTTON1_EXTI_PORT_SOURCE         	GPIO_PortSourceGPIOA
#define BUTTON1_EXTI_PIN_SOURCE          	GPIO_PinSource0
#define BUTTON1_EXTI_IRQn                	EXTI0_IRQn
#define	BUTTON1_EXTI_TRIGGER				EXTI_Trigger_Falling
#define BUTTON2_PIN                     	0//GPIO_Pin_y
#define BUTTON2_GPIO_PORT               	0//GPIOY
#define BUTTON2_GPIO_CLK                	0//RCC_APB2Periph_GPIOY
#define BUTTON2_GPIO_MODE					0//GPIO_Mode_IN_FLOATING
#define BUTTON2_PRESSED						0//0x00
#define BUTTON2_EXTI_LINE               	0//EXTI_Liney
#define BUTTON2_EXTI_PORT_SOURCE        	0//GPIO_PortSourceGPIOY
#define BUTTON2_EXTI_PIN_SOURCE         	0//GPIO_PinSourcey
#define BUTTON2_EXTI_IRQn               	0//EXTIy_IRQn
#define	BUTTON2_EXTI_TRIGGER				0//EXTI_Trigger_Falling

//Button Debounce Timer
#define DEBOUNCE_FREQ						100	//100 Hz => 10ms
#define DEBOUNCE_TIMER						TIM1
#define DEBOUNCE_TIMER_CLK					RCC_APB2Periph_TIM1
#define DEBOUNCE_TIMER_CLK_CMD				RCC_APB2PeriphClockCmd
#define DEBOUNCE_TIMER_FLAG            		TIM_IT_Update
#define DEBOUNCE_TIMER_IRQn           		TIM1_UP_IRQn

//SST25 FLASH Interface pins
#define sFLASH_SPI							SPI2
#define sFLASH_SPI_CLK						RCC_APB1Periph_SPI2
#define sFLASH_SPI_CLK_CMD					RCC_APB1PeriphClockCmd
#define sFLASH_SPI_SCK_PIN					GPIO_Pin_13					/* PB.13 */
#define sFLASH_SPI_SCK_GPIO_PORT			GPIOB						/* GPIOB */
#define sFLASH_SPI_SCK_GPIO_CLK				RCC_APB2Periph_GPIOB
#define sFLASH_SPI_MISO_PIN					GPIO_Pin_14					/* PB.14 */
#define sFLASH_SPI_MISO_GPIO_PORT			GPIOB						/* GPIOB */
#define sFLASH_SPI_MISO_GPIO_CLK			RCC_APB2Periph_GPIOB
#define sFLASH_SPI_MOSI_PIN					GPIO_Pin_15					/* PB.15 */
#define sFLASH_SPI_MOSI_GPIO_PORT			GPIOB						/* GPIOB */
#define sFLASH_SPI_MOSI_GPIO_CLK			RCC_APB2Periph_GPIOB
#define sFLASH_MEM_CS_PIN					GPIO_Pin_9					/* PB.09 */
#define sFLASH_MEM_CS_GPIO_PORT				GPIOB						/* GPIOB */
#define sFLASH_MEM_CS_GPIO_CLK				RCC_APB2Periph_GPIOB

#define sFLASH_SPI_BAUDRATE_PRESCALER		SPI_BaudRatePrescaler_32

#define USB_DISCONNECT_PIN              	GPIO_Pin_11
#define USB_DISCONNECT_GPIO_PORT        	GPIOC
#define USB_DISCONNECT_GPIO_CLK  			RCC_APB2Periph_GPIOC

#endif

/* Exported macro ------------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

#endif /* __PLATFORM_CONFIG_H */
