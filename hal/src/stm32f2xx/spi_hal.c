/**
 ******************************************************************************
 * @file    spi_hal.c
 * @author  Satish Nair
 * @version V1.0.0
 * @date    24-Dec-2014
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

/* Private define ------------------------------------------------------------*/
#if PLATFORM_ID == 10 // Electron
#define TOTAL_SPI   3
#else
#define TOTAL_SPI   2
#endif

/* Private typedef -----------------------------------------------------------*/
typedef enum SPI_Num_Def {
    SPI1_A3_A4_A5 = 0,
    SPI3_D4_D3_D2 = 1
#if PLATFORM_ID == 10 // Electron
   ,SPI3_C3_C2_C1 = 2
#endif
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

    uint8_t SPI_TX_DMA_Stream_IRQn;
    uint32_t SPI_TX_DMA_Stream_TC_Event;

    uint16_t SPI_SCK_Pin;
    uint16_t SPI_MISO_Pin;
    uint16_t SPI_MOSI_Pin;

    uint8_t SPI_AF_Mapping;

    SPI_InitTypeDef SPI_InitStructure;

    bool SPI_Bit_Order_Set;
    bool SPI_Data_Mode_Set;
    bool SPI_Clock_Divider_Set;
    bool SPI_Enabled;

    HAL_SPI_DMA_UserCallback SPI_DMA_UserCallback;
} STM32_SPI_Info;

/*
 * SPI mapping
 */
STM32_SPI_Info SPI_MAP[TOTAL_SPI] =
{
        { SPI1, &RCC->APB2ENR, RCC_APB2Periph_SPI1, &RCC->AHB1ENR, RCC_AHB1Periph_DMA2, DMA_Channel_3,
          DMA2_Stream5, DMA2_Stream2, DMA2_Stream5_IRQn, DMA_IT_TCIF5, SCK, MISO, MOSI, GPIO_AF_SPI1 },
        { SPI3, &RCC->APB1ENR, RCC_APB1Periph_SPI3, &RCC->AHB1ENR, RCC_AHB1Periph_DMA1, DMA_Channel_0,
          DMA1_Stream7, DMA1_Stream2, DMA1_Stream7_IRQn, DMA_IT_TCIF7, D4, D3, D2, GPIO_AF_SPI3 }
#if PLATFORM_ID == 10 // Electron
        ,{ SPI3, &RCC->APB1ENR, RCC_APB1Periph_SPI3, &RCC->AHB1ENR, RCC_AHB1Periph_DMA1, DMA_Channel_0,
          DMA1_Stream7, DMA1_Stream2, DMA1_Stream7_IRQn, DMA_IT_TCIF7, C3, C2, C1, GPIO_AF_SPI3 }
#endif
};

static STM32_SPI_Info *spiMap[TOTAL_SPI]; // pointer to SPI_MAP[] containing SPI peripheral info

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

static void HAL_SPI_DMA_Config(HAL_SPI_Interface spi, void* tx_buffer, void* rx_buffer, uint32_t length)
{
    DMA_InitTypeDef DMA_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Enable DMA Clock */
    *spiMap[spi]->SPI_RCC_AHBRegister |= spiMap[spi]->SPI_RCC_AHBClockEnable;

    /* Deinitialize DMA Streams */
    DMA_DeInit(spiMap[spi]->SPI_TX_DMA_Stream);
    DMA_DeInit(spiMap[spi]->SPI_RX_DMA_Stream);

    DMA_InitStructure.DMA_BufferSize = length;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;

    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&spiMap[spi]->SPI_Peripheral->DR;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;

    DMA_InitStructure.DMA_Channel = spiMap[spi]->SPI_DMA_Channel;

    uint8_t tempMemory = 0xFF;

    /* Configure Tx DMA Stream */
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
    if (tx_buffer)
    {
        DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)tx_buffer;
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    }
    else
    {
        DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)(&tempMemory);
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
    }
    DMA_Init(spiMap[spi]->SPI_TX_DMA_Stream, &DMA_InitStructure);

    /* Configure Rx DMA Stream */
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    if (rx_buffer)
    {
        DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)rx_buffer;
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    }
    else
    {
        DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&tempMemory;
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
    }
    DMA_Init(spiMap[spi]->SPI_RX_DMA_Stream, &DMA_InitStructure);

    /* Enable SPI TX DMA Stream Interrupt */
    DMA_ITConfig(spiMap[spi]->SPI_TX_DMA_Stream, DMA_IT_TC, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = spiMap[spi]->SPI_TX_DMA_Stream_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 12;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* Enable the DMA Tx/Rx Stream */
    DMA_Cmd(spiMap[spi]->SPI_TX_DMA_Stream, ENABLE);
    DMA_Cmd(spiMap[spi]->SPI_RX_DMA_Stream, ENABLE);

    /* Enable the SPI Rx/Tx DMA request */
    SPI_I2S_DMACmd(spiMap[spi]->SPI_Peripheral, SPI_I2S_DMAReq_Rx, ENABLE);
    SPI_I2S_DMACmd(spiMap[spi]->SPI_Peripheral, SPI_I2S_DMAReq_Tx, ENABLE);
}

static void HAL_SPI_TX_DMA_Stream_InterruptHandler(HAL_SPI_Interface spi)
{
    if (DMA_GetITStatus(spiMap[spi]->SPI_TX_DMA_Stream, spiMap[spi]->SPI_TX_DMA_Stream_TC_Event) == SET)
    {
        DMA_ClearITPendingBit(spiMap[spi]->SPI_TX_DMA_Stream, spiMap[spi]->SPI_TX_DMA_Stream_TC_Event);
        SPI_I2S_DMACmd(spiMap[spi]->SPI_Peripheral, SPI_I2S_DMAReq_Tx, DISABLE);
        DMA_Cmd(spiMap[spi]->SPI_TX_DMA_Stream, DISABLE);

        if (spiMap[spi]->SPI_DMA_UserCallback)
        {
            // notify user program about transfer completion
            spiMap[spi]->SPI_DMA_UserCallback();
        }
    }
}

void HAL_SPI_Init(HAL_SPI_Interface spi)
{
    if(spi == HAL_SPI_INTERFACE1)
    {
        spiMap[spi] = &SPI_MAP[SPI1_A3_A4_A5];
    }
    else if(spi == HAL_SPI_INTERFACE2)
    {
        spiMap[spi] = &SPI_MAP[SPI3_D4_D3_D2];
    }
#if PLATFORM_ID == 10 // Electron
    else if(spi == HAL_SPI_INTERFACE3)
    {
        spiMap[spi] = &SPI_MAP[SPI3_C3_C2_C1];
    }
#endif

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
    spiMap[spi]->SPI_DMA_UserCallback = userCallback;
    /* Config and initiate DMA transfer */
    HAL_SPI_DMA_Config(spi, tx_buffer, rx_buffer, length);
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

/**
 * @brief  This function handles DMA1 Stream 7 interrupt request.
 * @param  None
 * @retval None
 */
void DMA1_Stream7_irq(void)
{
    //HAL_SPI_INTERFACE2 and HAL_SPI_INTERFACE3 shares same DMA peripheral and stream
    HAL_SPI_TX_DMA_Stream_InterruptHandler(HAL_SPI_INTERFACE2);
}

/**
 * @brief  This function handles DMA2 Stream 5 interrupt request.
 * @param  None
 * @retval None
 */
void DMA2_Stream5_irq(void)
{
    HAL_SPI_TX_DMA_Stream_InterruptHandler(HAL_SPI_INTERFACE1);
}
