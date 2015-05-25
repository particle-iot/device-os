/**
 ******************************************************************************
 * @file    spi_hal.c
 * @author  Satish Nair
 * @version V1.0.0
 * @date    24-Dec-2014
 * @brief
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

/* Includes ------------------------------------------------------------------*/
#include "spi_hal.h"
#include "gpio_hal.h"
#include "pinmap_impl.h"

/* Private define ------------------------------------------------------------*/
#define TOTAL_SPI   2

/* Private typedef -----------------------------------------------------------*/
typedef enum SPI_Num_Def {
    SPI_1 = 0,
    SPI_3 = 1
} SPI_Num_Def;

/* Private variables ---------------------------------------------------------*/
typedef struct STM32_SPI_Info {
    SPI_TypeDef* SPI_Peripheral;

    __IO uint32_t* SPI_RCC_APBRegister;
    uint32_t SPI_RCC_APBClockEnable;

    __IO uint32_t* SPI_RCC_AHBRegister;
    uint32_t SPI_RCC_AHBClockEnable;

    uint32_t SPI_DMA_Channel;

    DMA_Stream_TypeDef* SPI_TX_DMA_Stream;
    DMA_Stream_TypeDef* SPI_RX_DMA_Stream;

    uint16_t SPI_SCK_Pin;
    uint16_t SPI_MISO_Pin;
    uint16_t SPI_MOSI_Pin;

    uint8_t SPI_AF_Mapping;

    SPI_InitTypeDef SPI_InitStructure;

    bool SPI_Bit_Order_Set;
    bool SPI_Data_Mode_Set;
    bool SPI_Clock_Divider_Set;
    bool SPI_Enabled;
} STM32_SPI_Info;

/*
 * SPI mapping
 */
STM32_SPI_Info SPI_MAP[TOTAL_SPI] =
{
        { SPI1, &RCC->APB2ENR, RCC_APB2Periph_SPI1, &RCC->AHB1ENR, RCC_AHB1Periph_DMA2, DMA_Channel_3, DMA2_Stream5, DMA2_Stream2, SCK, MISO, MOSI, GPIO_AF_SPI1 },
        { SPI3, &RCC->APB1ENR, RCC_APB1Periph_SPI3, &RCC->AHB1ENR, RCC_AHB1Periph_DMA1, DMA_Channel_0, DMA1_Stream5, DMA1_Stream2, D4, D3, D2, GPIO_AF_SPI3 }
};

static STM32_SPI_Info *spiMap[TOTAL_SPI]; // pointer to SPI_MAP[] containing SPI peripheral info

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

void HAL_SPI_Init(HAL_SPI_Interface spi)
{
    if(spi == HAL_SPI_INTERFACE1)
    {
        spiMap[spi] = &SPI_MAP[SPI_1];
    }
    else if(spi == HAL_SPI_INTERFACE2)
    {
        spiMap[spi] = &SPI_MAP[SPI_3];
    }

    spiMap[spi]->SPI_Bit_Order_Set = false;
    spiMap[spi]->SPI_Data_Mode_Set = false;
    spiMap[spi]->SPI_Clock_Divider_Set = false;
    spiMap[spi]->SPI_Enabled = false;
}

void HAL_SPI_Begin(HAL_SPI_Interface spi, uint16_t pin)
{
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();

    /* Enable SPI Clock */
    *spiMap[spi]->SPI_RCC_APBRegister |= spiMap[spi]->SPI_RCC_APBClockEnable;

    /* Connect SPI pins to AF */
    GPIO_PinAFConfig(PIN_MAP[spiMap[spi]->SPI_SCK_Pin].gpio_peripheral, PIN_MAP[spiMap[spi]->SPI_SCK_Pin].gpio_pin_source, spiMap[spi]->SPI_AF_Mapping);
    GPIO_PinAFConfig(PIN_MAP[spiMap[spi]->SPI_MISO_Pin].gpio_peripheral, PIN_MAP[spiMap[spi]->SPI_MISO_Pin].gpio_pin_source, spiMap[spi]->SPI_AF_Mapping);
    GPIO_PinAFConfig(PIN_MAP[spiMap[spi]->SPI_MOSI_Pin].gpio_peripheral, PIN_MAP[spiMap[spi]->SPI_MOSI_Pin].gpio_pin_source, spiMap[spi]->SPI_AF_Mapping);

    HAL_Pin_Mode(spiMap[spi]->SPI_SCK_Pin, AF_OUTPUT_PUSHPULL);
    HAL_Pin_Mode(spiMap[spi]->SPI_MISO_Pin, AF_OUTPUT_PUSHPULL);
    HAL_Pin_Mode(spiMap[spi]->SPI_MOSI_Pin, AF_OUTPUT_PUSHPULL);

    HAL_Pin_Mode(pin, OUTPUT);
    HAL_GPIO_Write(pin, Bit_SET);//HIGH

    /* SPI configuration */
    spiMap[spi]->SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spiMap[spi]->SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    spiMap[spi]->SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    if(spiMap[spi]->SPI_Data_Mode_Set != true)
    {
        //Default: SPI_MODE3
        spiMap[spi]->SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
        spiMap[spi]->SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    }
    spiMap[spi]->SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    if(spiMap[spi]->SPI_Clock_Divider_Set != true)
    {
        spiMap[spi]->SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
    }
    if(spiMap[spi]->SPI_Bit_Order_Set != true)
    {
        //Default: MSBFIRST
        spiMap[spi]->SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    }
    spiMap[spi]->SPI_InitStructure.SPI_CRCPolynomial = 7;

    SPI_Init(spiMap[spi]->SPI_Peripheral, &spiMap[spi]->SPI_InitStructure);

    SPI_Cmd(spiMap[spi]->SPI_Peripheral, ENABLE);

    spiMap[spi]->SPI_Enabled = true;
}

void HAL_SPI_End(HAL_SPI_Interface spi)
{
    if(spiMap[spi]->SPI_Enabled != false)
    {
        SPI_Cmd(spiMap[spi]->SPI_Peripheral, DISABLE);

        spiMap[spi]->SPI_Enabled = false;
    }
}

void HAL_SPI_Set_Bit_Order(HAL_SPI_Interface spi, uint8_t order)
{
    if(order == LSBFIRST)
    {
        spiMap[spi]->SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_LSB;
    }
    else
    {
        spiMap[spi]->SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    }

    SPI_Init(spiMap[spi]->SPI_Peripheral, &spiMap[spi]->SPI_InitStructure);

    spiMap[spi]->SPI_Bit_Order_Set = true;
}

void HAL_SPI_Set_Data_Mode(HAL_SPI_Interface spi, uint8_t mode)
{
    if(spiMap[spi]->SPI_Enabled != false)
    {
        SPI_Cmd(spiMap[spi]->SPI_Peripheral, DISABLE);
    }

    switch(mode)
    {
        case SPI_MODE0:
            spiMap[spi]->SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
            spiMap[spi]->SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
            break;

        case SPI_MODE1:
            spiMap[spi]->SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
            spiMap[spi]->SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
            break;

        case SPI_MODE2:
            spiMap[spi]->SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
            spiMap[spi]->SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
            break;

        case SPI_MODE3:
            spiMap[spi]->SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
            spiMap[spi]->SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
            break;
    }

    SPI_Init(spiMap[spi]->SPI_Peripheral, &spiMap[spi]->SPI_InitStructure);

    if(spiMap[spi]->SPI_Enabled != false)
    {
        SPI_Cmd(spiMap[spi]->SPI_Peripheral, ENABLE);
    }

    spiMap[spi]->SPI_Data_Mode_Set = true;
}

void HAL_SPI_Set_Clock_Divider(HAL_SPI_Interface spi, uint8_t rate)
{
    spiMap[spi]->SPI_InitStructure.SPI_BaudRatePrescaler = rate;

    SPI_Init(spiMap[spi]->SPI_Peripheral, &spiMap[spi]->SPI_InitStructure);

    spiMap[spi]->SPI_Clock_Divider_Set = true;
}

uint16_t HAL_SPI_Send_Receive_Data(HAL_SPI_Interface spi, uint16_t data)
{
    /* Wait for SPI Tx buffer empty */
    while (SPI_I2S_GetFlagStatus(spiMap[spi]->SPI_Peripheral, SPI_I2S_FLAG_TXE) == RESET);
    /* Send SPI1 data */
    SPI_I2S_SendData(spiMap[spi]->SPI_Peripheral, data);

    /* Wait for SPI data reception */
    while (SPI_I2S_GetFlagStatus(spiMap[spi]->SPI_Peripheral, SPI_I2S_FLAG_RXNE) == RESET);
    /* Read and return SPI received data */
    return SPI_I2S_ReceiveData(spiMap[spi]->SPI_Peripheral);
}

void HAL_SPI_DMA_Transfer(HAL_SPI_Interface spi, void* tx_buffer, void* rx_buffer, uint32_t length, HAL_SPI_DMA_UserCallback userCallback)
{
}

bool HAL_SPI_Is_Enabled(HAL_SPI_Interface spi)
{
    return spiMap[spi]->SPI_Enabled;
}


/**
 * Compatibility for the RC4 release of tinker, which used the no-arg version.
 */
bool HAL_SPI_Is_Enabled_Old()
{
    return false;
}
