/**
 ******************************************************************************
 * @file    platform_config.h
 * @author  Spark Application Team
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Board specific configuration file.
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PLATFORM_CONFIG_H
#define __PLATFORM_CONFIG_H

/* Uncomment the line corresponding to the STM32 board used */
#if !defined (USE_SPARK_TV1) &&  !defined (USE_SPARK_TV2) &&  !defined (USE_SPARKFUN_H103) &&  !defined (USE_ST_VLDISCOVERY)
//#define USE_SPARK_TV1
#define USE_SPARK_TV2
//#define USE_SPARKFUN_H103
//#define USE_ST_VLDISCOVERY
#endif

/*Unique Devices IDs register set*/
#define         ID1          (0x1FFFF7E8)
#define         ID2          (0x1FFFF7EC)
#define         ID3          (0x1FFFF7F0)

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#if defined (USE_SPARK_TV1)
//
#elif defined (USE_SPARK_TV2)
//
#elif defined (USE_SPARKFUN_H103)
//
#elif defined (USE_ST_VLDISCOVERY)
//
#endif

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Define the STM32F10x hardware depending on the used board */
#if defined (USE_SPARK_TV1)
//
#elif defined (USE_SPARK_TV2)
/**
 * @brief  CC3000 Interface pins
 */
#define CC3000_SPI							SPI1
#define CC3000_SPI_CLK						RCC_APB2Periph_SPI1
#define CC3000_SPI_SCK_PIN					GPIO_Pin_5                  /* PA.05 */
#define CC3000_SPI_SCK_GPIO_PORT			GPIOA                       /* GPIOA */
#define CC3000_SPI_SCK_GPIO_CLK				RCC_APB2Periph_GPIOA
#define CC3000_SPI_MISO_PIN					GPIO_Pin_6                  /* PA.06 */
#define CC3000_SPI_MISO_GPIO_PORT			GPIOA                       /* GPIOA */
#define CC3000_SPI_MISO_GPIO_CLK			RCC_APB2Periph_GPIOA
#define CC3000_SPI_MOSI_PIN					GPIO_Pin_7                  /* PA.07 */
#define CC3000_SPI_MOSI_GPIO_PORT			GPIOA                       /* GPIOA */
#define CC3000_SPI_MOSI_GPIO_CLK			RCC_APB2Periph_GPIOA
#define CC3000_SPI_CS_PIN					GPIO_Pin_8          	    /* PA.08 */
#define CC3000_SPI_CS_GPIO_PORT				GPIOA                       /* GPIOA */
#define CC3000_SPI_CS_GPIO_CLK				RCC_APB2Periph_GPIOA
#define CC3000_WIFI_EN_PIN					GPIO_Pin_14                 /* PC.14 */
#define CC3000_WIFI_EN_GPIO_PORT			GPIOC                       /* GPIOC */
#define CC3000_WIFI_EN_GPIO_CLK				RCC_APB2Periph_GPIOC
#define CC3000_WIFI_INT_PIN					GPIO_Pin_15                 /* PC.15 */
#define CC3000_WIFI_INT_GPIO_PORT			GPIOC                       /* GPIOC */
#define CC3000_WIFI_INT_GPIO_CLK			RCC_APB2Periph_GPIOC

#define CC3000_WIFI_INT_EXTI_LINE           EXTI_Line15
#define CC3000_WIFI_INT_EXTI_PORT_SOURCE    GPIO_PortSourceGPIOC
#define CC3000_WIFI_INT_EXTI_PIN_SOURCE     GPIO_PinSource15
#define CC3000_WIFI_INT_EXTI_IRQn           EXTI15_10_IRQn

#define CC3000_SPI_DMA_CLK                  RCC_AHBPeriph_DMA1
#define CC3000_SPI_RX_DMA_CHANNEL           DMA1_Channel2
#define CC3000_SPI_TX_DMA_CHANNEL           DMA1_Channel3
#define CC3000_SPI_RX_DMA_TCFLAG            DMA1_FLAG_TC2
#define CC3000_SPI_TX_DMA_TCFLAG            DMA1_FLAG_TC3
#define CC3000_SPI_RX_DMA_IRQn           	DMA1_Channel2_IRQn
#define CC3000_SPI_TX_DMA_IRQn           	DMA1_Channel3_IRQn

#define CC3000_SPI_DR_BASE                  ((uint32_t)0x4001300C)	/* SPI1_BASE | 0x0C */

#elif defined (USE_SPARKFUN_H103)
//
#elif defined (USE_ST_VLDISCOVERY)
//
#endif

/* Exported macro ------------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

#endif /* __PLATFORM_CONFIG_H */
