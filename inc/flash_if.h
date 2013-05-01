/**
  ******************************************************************************
  * @file    flash_if.h
  * @author  Spark Application Team
  * @version V1.0.0
  * @date    30-April-2013
  * @brief   Header for flash_if.c file.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FLASH_IF_MAL_H
#define __FLASH_IF_MAL_H

/* Includes ------------------------------------------------------------------*/
#include "platform_config.h"
 
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

uint16_t FLASH_If_Init(void);
uint16_t FLASH_If_Erase (uint32_t SectorAddress);
uint16_t FLASH_If_Write(uint32_t SectorAddress, uint32_t DataLength);
uint8_t *FLASH_If_Read (uint32_t SectorAddress, uint32_t DataLength);

#endif /* __FLASH_IF_MAL_H */

/*********************** (C) COPYRIGHT STMicroelectronics *********************/
