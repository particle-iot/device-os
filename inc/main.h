/**
  ******************************************************************************
  * @file    main.h
  * @author  Spark Application Team
  * @version V1.0.0
  * @date    30-April-2013
  * @brief   Header for main.c module
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/

#include "platform_config.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void Timing_Decrement(void);
void Delay(__IO uint32_t nTime);

#endif /* __MAIN_H */
