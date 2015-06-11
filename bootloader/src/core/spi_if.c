/**
  ******************************************************************************
  * @file    spi_if.c
  * @author  Satish Nair
  * @version V1.0.0
  * @date    20-May-2013
  * @brief   specific media access Layer for SPI flash
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
#include "hw_config.h"
#include "spi_if.h"
#include "dfu_mal.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : SPI_If_Init
* Description    : Initializes the Media on the STM32
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t SPI_If_Init(void)
{
  sFLASH_Init();
  return MAL_OK;
}

/*******************************************************************************
* Function Name  : SPI_If_Erase
* Description    : Erase sector
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t SPI_If_Erase(uint32_t SectorAddress)
{
  sFLASH_EraseSector(SectorAddress);
  return MAL_OK;
}

/*******************************************************************************
* Function Name  : SPI_If_Write
* Description    : Write sectors
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t SPI_If_Write(uint32_t SectorAddress, uint32_t DataLength)
{
  sFLASH_WriteBuffer(MAL_Buffer, SectorAddress, (uint16_t)DataLength);
  return MAL_OK;
}

/*******************************************************************************
* Function Name  : SPI_If_Read
* Description    : Read sectors
* Input          : None
* Output         : None
* Return         : buffer address pointer
*******************************************************************************/
uint8_t *SPI_If_Read(uint32_t SectorAddress, uint32_t DataLength)
{
  sFLASH_ReadBuffer(MAL_Buffer, SectorAddress, (uint16_t)DataLength);
  return MAL_Buffer;
}

