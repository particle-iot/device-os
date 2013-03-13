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
//
#elif defined (USE_SPARKFUN_H103)
//
#elif defined (USE_ST_VLDISCOVERY)
//
#endif

/* Exported macro ------------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

#endif /* __PLATFORM_CONFIG_H */
