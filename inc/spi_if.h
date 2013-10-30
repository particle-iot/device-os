/**
  ******************************************************************************
  * @file    spi_if.h
  * @author  Spark Application Team
  * @version V1.0.0
  * @date    20-May-2013
  * @brief   Header for spi_if.c file.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPI_IF_MAL_H
#define __SPI_IF_MAL_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

uint16_t SPI_If_Init(void);
uint16_t SPI_If_Erase (uint32_t SectorAddress);
uint16_t SPI_If_Write (uint32_t SectorAddress, uint32_t DataLength);
uint8_t *SPI_If_Read (uint32_t SectorAddress, uint32_t DataLength);

#endif /* __SPI_IF_MAL_H */

