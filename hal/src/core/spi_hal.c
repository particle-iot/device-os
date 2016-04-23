/**
 ******************************************************************************
 * @file    spi_hal.c
 * @author  Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
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
#include "spi_hal.h"
#include "gpio_hal.h"
#include "pinmap_impl.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static SPI_InitTypeDef SPI_InitStructure;
static bool SPI_Bit_Order_Set = false;
static bool SPI_Data_Mode_Set = false;
static bool SPI_Clock_Divider_Set = false;
static bool SPI_Enabled = false;

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

void HAL_SPI_Init(HAL_SPI_Interface spi)
{
    //There's only one SPI interface available on Core.
    //HAL_SPI_Interface as argument in Core HAL APIs has no effect.
}

void HAL_SPI_Begin(HAL_SPI_Interface spi, uint16_t pin)
{
    if (pin==SPI_DEFAULT_SS)
        pin = SS;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

  HAL_Pin_Mode(SCK, AF_OUTPUT_PUSHPULL);
  HAL_Pin_Mode(MOSI, AF_OUTPUT_PUSHPULL);
  HAL_Pin_Mode(MISO, INPUT);

  HAL_Pin_Mode(pin, OUTPUT);
  HAL_GPIO_Write(pin, Bit_SET);//HIGH

  /* SPI configuration */
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  if(SPI_Data_Mode_Set != true)
  {
    //Default: SPI_MODE3
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
  }
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  if(SPI_Clock_Divider_Set != true)
  {
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
  }
  if(SPI_Bit_Order_Set != true)
  {
    //Default: MSBFIRST
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  }
  SPI_InitStructure.SPI_CRCPolynomial = 7;

  SPI_Init(SPI1, &SPI_InitStructure);

  SPI_Cmd(SPI1, ENABLE);

  SPI_Enabled = true;
}

void HAL_SPI_Begin_Ext(HAL_SPI_Interface spi, SPI_Mode mode, uint16_t pin, void* reserved)
{
  HAL_SPI_Begin(spi, pin);
}

void HAL_SPI_End(HAL_SPI_Interface spi)
{
  if(SPI_Enabled != false)
  {
    SPI_Cmd(SPI1, DISABLE);

    SPI_Enabled = false;
  }
}

void HAL_SPI_Set_Bit_Order(HAL_SPI_Interface spi, uint8_t order)
{
  if(order == LSBFIRST)
  {
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_LSB;
  }
  else
  {
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  }

  SPI_Init(SPI1, &SPI_InitStructure);

  SPI_Bit_Order_Set = true;
}

void HAL_SPI_Set_Data_Mode(HAL_SPI_Interface spi, uint8_t mode)
{
  if(SPI_Enabled != false)
  {
    SPI_Cmd(SPI1, DISABLE);
  }

  switch(mode)
  {
    case SPI_MODE0:
      SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
      SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
      break;

    case SPI_MODE1:
      SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
      SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
      break;

    case SPI_MODE2:
      SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
      SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
      break;

    case SPI_MODE3:
      SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
      SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
      break;
  }

  SPI_Init(SPI1, &SPI_InitStructure);

  if(SPI_Enabled != false)
  {
    SPI_Cmd(SPI1, ENABLE);
  }

  SPI_Data_Mode_Set = true;
}

void HAL_SPI_Set_Clock_Divider(HAL_SPI_Interface spi, uint8_t rate)
{
  SPI_InitStructure.SPI_BaudRatePrescaler = rate;

  SPI_Init(SPI1, &SPI_InitStructure);

  SPI_Clock_Divider_Set = true;
}

uint16_t HAL_SPI_Send_Receive_Data(HAL_SPI_Interface spi, uint16_t data)
{
  /* Wait for SPI1 Tx buffer empty */
  while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
  /* Send SPI1 data */
  SPI_I2S_SendData(SPI1, data);

  /* Wait for SPI1 data reception */
  while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
  /* Read and return SPI1 received data */
  return SPI_I2S_ReceiveData(SPI1);
}

void HAL_SPI_DMA_Transfer(HAL_SPI_Interface spi, void* tx_buffer, void* rx_buffer, uint32_t length, HAL_SPI_DMA_UserCallback userCallback)
{
}

bool HAL_SPI_Is_Enabled(HAL_SPI_Interface spi)
{
  return SPI_Enabled;
}

void HAL_SPI_Info(HAL_SPI_Interface spi, hal_spi_info_t* info, void* reserved)
{
    info->system_clock = 36000000;
}

void HAL_SPI_Set_Callback_On_Select(HAL_SPI_Interface spi, HAL_SPI_Select_UserCallback cb, void* reserved)
{
}

void HAL_SPI_DMA_Transfer_Cancel(HAL_SPI_Interface spi)
{
}

int32_t HAL_SPI_DMA_Transfer_Status(HAL_SPI_Interface spi, HAL_SPI_TransferStatus* st)
{
    return -1;
}
