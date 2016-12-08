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
#include <stddef.h>
#include "spi_hal.h"
#include "gpio_hal.h"
#include "pinmap_impl.h"
#include "interrupts_hal.h"
#include "debug.h"

/* Private define ------------------------------------------------------------*/
#if PLATFORM_ID == 10 // Electron
#define TOTAL_SPI   3
#else
#define TOTAL_SPI   2
#endif

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
    uint8_t SPI_RX_DMA_Stream_IRQn;
    uint32_t SPI_TX_DMA_Stream_TC_Event;
    uint32_t SPI_RX_DMA_Stream_TC_Event;

    uint8_t SPI_SCK_Pin;
    uint8_t SPI_MISO_Pin;
    uint8_t SPI_MOSI_Pin;
    uint8_t SPI_SS_Pin;

    uint8_t SPI_AF_Mapping;


} STM32_SPI_Info;

typedef struct SPI_State
{
    SPI_InitTypeDef SPI_InitStructure;

    bool SPI_Bit_Order_Set;
    bool SPI_Data_Mode_Set;
    bool SPI_Clock_Divider_Set;
    bool SPI_Enabled;

    HAL_SPI_DMA_UserCallback SPI_DMA_UserCallback;

    uint16_t SPI_SS_Pin;
    SPI_Mode mode;
    HAL_SPI_Select_UserCallback SPI_Select_UserCallback;
    uint32_t SPI_DMA_Last_Transfer_Length;
    uint32_t SPI_DMA_Current_Transfer_Length;
    volatile uint8_t SPI_SS_State;
    uint8_t SPI_DMA_Configured;
    volatile uint8_t SPI_DMA_Aborted;

    __attribute__((aligned(4))) uint8_t tempMemoryRx;
    __attribute__((aligned(4))) uint8_t tempMemoryTx;
} SPI_State;

static void HAL_SPI_SS_Handler(void *data);

/*
 * SPI mapping
 */
const STM32_SPI_Info spiMap[TOTAL_SPI] =
{
        { SPI1, &RCC->APB2ENR, RCC_APB2Periph_SPI1, &RCC->AHB1ENR, RCC_AHB1Periph_DMA2, DMA_Channel_3,
          DMA2_Stream5, DMA2_Stream2, DMA2_Stream5_IRQn, DMA2_Stream2_IRQn, DMA_IT_TCIF5, DMA_IT_TCIF2, SCK, MISO, MOSI, SS, GPIO_AF_SPI1 },
        { SPI3, &RCC->APB1ENR, RCC_APB1Periph_SPI3, &RCC->AHB1ENR, RCC_AHB1Periph_DMA1, DMA_Channel_0,
          DMA1_Stream7, DMA1_Stream2, DMA1_Stream7_IRQn, DMA1_Stream2_IRQn, DMA_IT_TCIF7, DMA_IT_TCIF2, D4, D3, D2, D5, GPIO_AF_SPI3  }
#if PLATFORM_ID == 10 // Electron
        ,{ SPI3, &RCC->APB1ENR, RCC_APB1Periph_SPI3, &RCC->AHB1ENR, RCC_AHB1Periph_DMA1, DMA_Channel_0,
          DMA1_Stream7, DMA1_Stream2, DMA1_Stream7_IRQn, DMA1_Stream2_IRQn, DMA_IT_TCIF7, DMA_IT_TCIF2, C3, C2, C1, C0, GPIO_AF_SPI3  }
#endif
};

/* Private typedef -----------------------------------------------------------*/
typedef enum SPI_Num_Def {
    SPI1_A3_A4_A5 = 0,
    SPI3_D4_D3_D2 = 1
#if PLATFORM_ID == 10 // Electron
   ,SPI3_C3_C2_C1 = 2
#endif
} SPI_Num_Def;


SPI_State spiState[TOTAL_SPI];

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

static void HAL_SPI_DMA_Config(HAL_SPI_Interface spi, void* tx_buffer, void* rx_buffer, uint32_t length)
{
    DMA_InitTypeDef DMA_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Deinitialize DMA Streams */
    DMA_DeInit(spiMap[spi].SPI_TX_DMA_Stream);
    DMA_DeInit(spiMap[spi].SPI_RX_DMA_Stream);

    DMA_InitStructure.DMA_BufferSize = length;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;

    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&spiMap[spi].SPI_Peripheral->DR;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;

    DMA_InitStructure.DMA_Channel = spiMap[spi].SPI_DMA_Channel;

    spiState[spi].tempMemoryTx = 0xFF;
    spiState[spi].tempMemoryRx = 0xFF;

    /* Configure Tx DMA Stream */
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
    if (tx_buffer)
    {
        DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)tx_buffer;
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    }
    else
    {
        DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)(&spiState[spi].tempMemoryTx);
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
    }
    DMA_Init(spiMap[spi].SPI_TX_DMA_Stream, &DMA_InitStructure);

    /* Configure Rx DMA Stream */
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    if (rx_buffer)
    {
        DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)rx_buffer;
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    }
    else
    {
        DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&spiState[spi].tempMemoryRx;
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
    }
    DMA_Init(spiMap[spi].SPI_RX_DMA_Stream, &DMA_InitStructure);

    /* Enable SPI TX DMA Stream Interrupt */
    DMA_ITConfig(spiMap[spi].SPI_TX_DMA_Stream, DMA_IT_TC, ENABLE);
    /* Enable SPI RX DMA Stream Interrupt */
    DMA_ITConfig(spiMap[spi].SPI_RX_DMA_Stream, DMA_IT_TC, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = spiMap[spi].SPI_TX_DMA_Stream_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = spiState[spi].mode == SPI_MODE_MASTER ? 12 : 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = spiMap[spi].SPI_RX_DMA_Stream_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = spiState[spi].mode == SPI_MODE_MASTER ? 12 : 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* Enable the DMA Tx/Rx Stream */
    DMA_Cmd(spiMap[spi].SPI_TX_DMA_Stream, ENABLE);
    DMA_Cmd(spiMap[spi].SPI_RX_DMA_Stream, ENABLE);

    /* Enable the SPI Rx/Tx DMA request */
    SPI_I2S_DMACmd(spiMap[spi].SPI_Peripheral, SPI_I2S_DMAReq_Rx, ENABLE);
    SPI_I2S_DMACmd(spiMap[spi].SPI_Peripheral, SPI_I2S_DMAReq_Tx, ENABLE);

    spiState[spi].SPI_DMA_Current_Transfer_Length = length;
    spiState[spi].SPI_DMA_Last_Transfer_Length = 0;
    spiState[spi].SPI_DMA_Aborted = 0;
    spiState[spi].SPI_DMA_Configured = 1;
}

static void HAL_SPI_TX_DMA_Stream_InterruptHandler(HAL_SPI_Interface spi)
{
    if (DMA_GetITStatus(spiMap[spi].SPI_TX_DMA_Stream, spiMap[spi].SPI_TX_DMA_Stream_TC_Event) == SET)
    {
        DMA_ClearITPendingBit(spiMap[spi].SPI_TX_DMA_Stream, spiMap[spi].SPI_TX_DMA_Stream_TC_Event);
        SPI_I2S_DMACmd(spiMap[spi].SPI_Peripheral, SPI_I2S_DMAReq_Tx, DISABLE);
        DMA_Cmd(spiMap[spi].SPI_TX_DMA_Stream, DISABLE);
    }
}

static void HAL_SPI_RX_DMA_Stream_InterruptHandler(HAL_SPI_Interface spi)
{
    if (DMA_GetITStatus(spiMap[spi].SPI_RX_DMA_Stream, spiMap[spi].SPI_RX_DMA_Stream_TC_Event) == SET)
    {
        uint32_t remainingCount = DMA_GetCurrDataCounter(spiMap[spi].SPI_RX_DMA_Stream);

        if (!spiState[spi].SPI_DMA_Aborted)
        {
            while (SPI_I2S_GetFlagStatus(spiMap[spi].SPI_Peripheral, SPI_I2S_FLAG_TXE) == RESET);
            while (SPI_I2S_GetFlagStatus(spiMap[spi].SPI_Peripheral, SPI_I2S_FLAG_BSY) == SET);
        }

        if (spiState[spi].mode == SPI_MODE_SLAVE)
        {
            SPI_Cmd(spiMap[spi].SPI_Peripheral, DISABLE);

            if (spiState[spi].SPI_DMA_Aborted)
            {
                /* Reset SPI peripheral completely, othewise we might end up with a case where
                 * SPI peripheral has already loaded some data from aborted transfer into its circular register,
                 * but haven't transferred it out yet. The next transfer then will start with this remainder data.
                 * The only way to clear it out is by completely disabling SPI peripheral (stopping its clock).
                 */
                spiState[spi].SPI_DMA_Aborted = 0;
                SPI_DeInit(spiMap[spi].SPI_Peripheral);
                HAL_SPI_Begin_Ext(spi, SPI_MODE_SLAVE, spiState[spi].SPI_SS_Pin, NULL);
            }
        }
        DMA_ClearITPendingBit(spiMap[spi].SPI_RX_DMA_Stream, spiMap[spi].SPI_RX_DMA_Stream_TC_Event);
        SPI_I2S_DMACmd(spiMap[spi].SPI_Peripheral, SPI_I2S_DMAReq_Rx, DISABLE);
        DMA_Cmd(spiMap[spi].SPI_RX_DMA_Stream, DISABLE);

        spiState[spi].SPI_DMA_Last_Transfer_Length = spiState[spi].SPI_DMA_Current_Transfer_Length - remainingCount;
        spiState[spi].SPI_DMA_Configured = 0;

        HAL_SPI_DMA_UserCallback callback = spiState[spi].SPI_DMA_UserCallback;
        if (callback) {
            // notify user program about transfer completion
            callback();
        }
    }
}

void HAL_SPI_Init(HAL_SPI_Interface spi)
{
    spiState[spi].SPI_Bit_Order_Set = false;
    spiState[spi].SPI_Data_Mode_Set = false;
    spiState[spi].SPI_Clock_Divider_Set = false;
    spiState[spi].SPI_Enabled = false;

    spiState[spi].SPI_SS_Pin = spiMap[spi].SPI_SS_Pin;
    spiState[spi].mode = SPI_MODE_MASTER;
    spiState[spi].SPI_SS_State = 0;
    spiState[spi].SPI_DMA_Current_Transfer_Length = 0;
    spiState[spi].SPI_DMA_Last_Transfer_Length = 0;
    spiState[spi].SPI_DMA_Configured = 0;
    spiState[spi].SPI_DMA_Aborted = 0;
}

void HAL_SPI_Begin(HAL_SPI_Interface spi, uint16_t pin)
{
    // Default to Master mode
    HAL_SPI_Begin_Ext(spi, SPI_MODE_MASTER, pin, NULL);
}

void HAL_SPI_Begin_Ext(HAL_SPI_Interface spi, SPI_Mode mode, uint16_t pin, void* reserved)
{
    if (pin == SPI_DEFAULT_SS)
        pin = spiMap[spi].SPI_SS_Pin;

    spiState[spi].SPI_SS_Pin = pin;
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();

    spiState[spi].mode = mode;

    /* Enable SPI Clock */
    *spiMap[spi].SPI_RCC_APBRegister |= spiMap[spi].SPI_RCC_APBClockEnable;
    *spiMap[spi].SPI_RCC_AHBRegister |= spiMap[spi].SPI_RCC_AHBClockEnable;

    /* Connect SPI pins to AF */
    GPIO_PinAFConfig(PIN_MAP[spiMap[spi].SPI_SCK_Pin].gpio_peripheral, PIN_MAP[spiMap[spi].SPI_SCK_Pin].gpio_pin_source, spiMap[spi].SPI_AF_Mapping);
    GPIO_PinAFConfig(PIN_MAP[spiMap[spi].SPI_MISO_Pin].gpio_peripheral, PIN_MAP[spiMap[spi].SPI_MISO_Pin].gpio_pin_source, spiMap[spi].SPI_AF_Mapping);
    GPIO_PinAFConfig(PIN_MAP[spiMap[spi].SPI_MOSI_Pin].gpio_peripheral, PIN_MAP[spiMap[spi].SPI_MOSI_Pin].gpio_pin_source, spiMap[spi].SPI_AF_Mapping);

    HAL_Pin_Mode(spiMap[spi].SPI_SCK_Pin, AF_OUTPUT_PUSHPULL);
    HAL_Pin_Mode(spiMap[spi].SPI_MISO_Pin, AF_OUTPUT_PUSHPULL);
    HAL_Pin_Mode(spiMap[spi].SPI_MOSI_Pin, AF_OUTPUT_PUSHPULL);

    if (mode == SPI_MODE_MASTER)
    {
        // Ensure that there is no glitch on SS pin
        PIN_MAP[pin].gpio_peripheral->BSRRL = PIN_MAP[pin].gpio_pin;
        HAL_Pin_Mode(pin, OUTPUT);
    }
    else
    {
        HAL_Pin_Mode(pin, INPUT);
    }

    /* SPI configuration */
    spiState[spi].SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spiState[spi].SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    spiState[spi].SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    if(spiState[spi].SPI_Data_Mode_Set != true)
    {
        //Default: SPI_MODE3
        spiState[spi].SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
        spiState[spi].SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    }
    spiState[spi].SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;

    if(spiState[spi].SPI_Clock_Divider_Set != true)
    {
        /* Defaults to 15Mbit/s on SPI1, SPI2 and SPI3 */
        if(spi == HAL_SPI_INTERFACE1)
        {
            spiState[spi].SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;//60/4=15
        }
        else
        {
            spiState[spi].SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;//30/2=15
        }
    }
    if(spiState[spi].SPI_Bit_Order_Set != true)
    {
        //Default: MSBFIRST
        spiState[spi].SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    }
    spiState[spi].SPI_InitStructure.SPI_CRCPolynomial = 7;

    SPI_Init(spiMap[spi].SPI_Peripheral, &spiState[spi].SPI_InitStructure);

    if (mode == SPI_MODE_SLAVE)
    {
        /* Attach interrupt to slave select pin */
        HAL_InterruptExtraConfiguration irqConf = {0};
        irqConf.version = HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION_1;
        irqConf.IRQChannelPreemptionPriority = 1;
        irqConf.IRQChannelSubPriority = 0;

        HAL_Interrupts_Attach(pin, &HAL_SPI_SS_Handler, (void*)(spi), CHANGE, &irqConf);

        /* Switch to slave mode */
        spiState[spi].SPI_InitStructure.SPI_Mode = SPI_Mode_Slave;
        SPI_Init(spiMap[spi].SPI_Peripheral, &spiState[spi].SPI_InitStructure);
    }

    if (mode == SPI_MODE_MASTER)
    {
        /* SPI peripheral should not be enabled in Slave mode until the device is selected */
        SPI_Cmd(spiMap[spi].SPI_Peripheral, ENABLE);
    }

    spiState[spi].SPI_Enabled = true;
}

void HAL_SPI_End(HAL_SPI_Interface spi)
{
    if(spiState[spi].SPI_Enabled != false)
    {
        if (spiState[spi].mode == SPI_MODE_SLAVE)
        {
            HAL_Interrupts_Detach(spiState[spi].SPI_SS_Pin);
        }
        SPI_Cmd(spiMap[spi].SPI_Peripheral, DISABLE);
        HAL_SPI_DMA_Transfer_Cancel(spi);
        SPI_DeInit(spiMap[spi].SPI_Peripheral);
        spiState[spi].SPI_Enabled = false;
    }
}

static inline void HAL_SPI_Set_Bit_Order_Impl(HAL_SPI_Interface spi, uint8_t order)
{
    if(order == LSBFIRST)
    {
        spiState[spi].SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_LSB;
    }
    else
    {
        spiState[spi].SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    }
}

void HAL_SPI_Set_Bit_Order(HAL_SPI_Interface spi, uint8_t order)
{
    HAL_SPI_Set_Bit_Order_Impl(spi, order);

    if(spiState[spi].SPI_Enabled != false) {
        SPI_Cmd(spiMap[spi].SPI_Peripheral, DISABLE);
        SPI_Init(spiMap[spi].SPI_Peripheral, &spiState[spi].SPI_InitStructure);
        SPI_Cmd(spiMap[spi].SPI_Peripheral, ENABLE);
    }

    spiState[spi].SPI_Bit_Order_Set = true;
}

static inline void HAL_SPI_Set_Data_Mode_Impl(HAL_SPI_Interface spi, uint8_t mode)
{
    switch(mode)
    {
        case SPI_MODE0:
            spiState[spi].SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
            spiState[spi].SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
            break;

        case SPI_MODE1:
            spiState[spi].SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
            spiState[spi].SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
            break;

        case SPI_MODE2:
            spiState[spi].SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
            spiState[spi].SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
            break;

        case SPI_MODE3:
            spiState[spi].SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
            spiState[spi].SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
            break;
    }
}

void HAL_SPI_Set_Data_Mode(HAL_SPI_Interface spi, uint8_t mode)
{
    HAL_SPI_Set_Data_Mode_Impl(spi, mode);

    if(spiState[spi].SPI_Enabled != false)
    {
        SPI_Cmd(spiMap[spi].SPI_Peripheral, DISABLE);
        SPI_Init(spiMap[spi].SPI_Peripheral, &spiState[spi].SPI_InitStructure);
        SPI_Cmd(spiMap[spi].SPI_Peripheral, ENABLE);
    }

    spiState[spi].SPI_Data_Mode_Set = true;
}

static inline void HAL_SPI_Set_Clock_Divider_Impl(HAL_SPI_Interface spi, uint8_t rate)
{
    spiState[spi].SPI_InitStructure.SPI_BaudRatePrescaler = rate;
}

void HAL_SPI_Set_Clock_Divider(HAL_SPI_Interface spi, uint8_t rate)
{
    HAL_SPI_Set_Clock_Divider_Impl(spi, rate);

    SPI_Init(spiMap[spi].SPI_Peripheral, &spiState[spi].SPI_InitStructure);

    spiState[spi].SPI_Clock_Divider_Set = true;
}

int32_t HAL_SPI_Set_Settings(HAL_SPI_Interface spi, uint8_t set_default, uint8_t clockdiv, uint8_t order, uint8_t mode, void* reserved)
{
    if (!set_default)
    {
        HAL_SPI_Set_Clock_Divider_Impl(spi, clockdiv);
        HAL_SPI_Set_Bit_Order_Impl(spi, order);
        HAL_SPI_Set_Data_Mode_Impl(spi, mode);
    }

    spiState[spi].SPI_Clock_Divider_Set = !set_default;
    spiState[spi].SPI_Data_Mode_Set = !set_default;
    spiState[spi].SPI_Bit_Order_Set = !set_default;

    if (set_default) {
        // HAL_SPI_End(spi);
        HAL_SPI_Begin_Ext(spi, spiState[spi].mode, spiState[spi].SPI_SS_Pin, NULL);
    } else if (spiState[spi].SPI_Enabled != false) {
        SPI_Cmd(spiMap[spi].SPI_Peripheral, DISABLE);
        SPI_Init(spiMap[spi].SPI_Peripheral, &spiState[spi].SPI_InitStructure);
        SPI_Cmd(spiMap[spi].SPI_Peripheral, ENABLE);
    }

    return 0;
}

uint16_t HAL_SPI_Send_Receive_Data(HAL_SPI_Interface spi, uint16_t data)
{
    if (spiState[spi].mode == SPI_MODE_SLAVE)
        return 0;
    /* Wait for SPI Tx buffer empty */
    while (SPI_I2S_GetFlagStatus(spiMap[spi].SPI_Peripheral, SPI_I2S_FLAG_TXE) == RESET);
    /* Send SPI1 data */
    SPI_I2S_SendData(spiMap[spi].SPI_Peripheral, data);

    /* Wait for SPI data reception */
    while (SPI_I2S_GetFlagStatus(spiMap[spi].SPI_Peripheral, SPI_I2S_FLAG_RXNE) == RESET);
    /* Read and return SPI received data */
    return SPI_I2S_ReceiveData(spiMap[spi].SPI_Peripheral);
}

void HAL_SPI_DMA_Transfer(HAL_SPI_Interface spi, void* tx_buffer, void* rx_buffer, uint32_t length, HAL_SPI_DMA_UserCallback userCallback)
{
    int32_t state = HAL_disable_irq();
    spiState[spi].SPI_DMA_UserCallback = userCallback;
    /* Config and initiate DMA transfer */
    HAL_SPI_DMA_Config(spi, tx_buffer, rx_buffer, length);
    if (spiState[spi].mode == SPI_MODE_SLAVE && spiState[spi].SPI_SS_State == 1)
    {
        SPI_Cmd(spiMap[spi].SPI_Peripheral, ENABLE);
    }
    HAL_enable_irq(state);
}

void HAL_SPI_DMA_Transfer_Cancel(HAL_SPI_Interface spi)
{
    if (spiState[spi].SPI_DMA_Configured)
    {
        spiState[spi].SPI_DMA_Aborted = 1;
        DMA_Cmd(spiMap[spi].SPI_TX_DMA_Stream, DISABLE);
        DMA_Cmd(spiMap[spi].SPI_RX_DMA_Stream, DISABLE);
    }
}

int32_t HAL_SPI_DMA_Transfer_Status(HAL_SPI_Interface spi, HAL_SPI_TransferStatus* st)
{
    int32_t transferLength = 0;
    if (spiState[spi].SPI_DMA_Configured == 0)
    {
        transferLength = spiState[spi].SPI_DMA_Last_Transfer_Length;
    }
    else
    {
        transferLength = spiState[spi].SPI_DMA_Current_Transfer_Length - DMA_GetCurrDataCounter(spiMap[spi].SPI_RX_DMA_Stream);
    }

    if (st != NULL)
    {
        st->configured_transfer_length = spiState[spi].SPI_DMA_Current_Transfer_Length;
        st->transfer_length = (uint32_t)transferLength;
        st->ss_state = spiState[spi].SPI_SS_State;
        st->transfer_ongoing = spiState[spi].SPI_DMA_Configured;
    }

    return transferLength;
}

void HAL_SPI_Set_Callback_On_Select(HAL_SPI_Interface spi, HAL_SPI_Select_UserCallback cb, void* reserved)
{
    spiState[spi].SPI_Select_UserCallback = cb;
}

bool HAL_SPI_Is_Enabled(HAL_SPI_Interface spi)
{
    return spiState[spi].SPI_Enabled;
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
#if TOTAL_SPI==3
    if (spiState[HAL_SPI_INTERFACE3].SPI_DMA_Configured)
        HAL_SPI_TX_DMA_Stream_InterruptHandler(HAL_SPI_INTERFACE3);
    else
#endif
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

/**
 * @brief  This function handles DMA1 Stream 2 interrupt request.
 * @param  None
 * @retval None
 */
void DMA1_Stream2_irq(void)
{
    //HAL_SPI_INTERFACE2 and HAL_SPI_INTERFACE3 shares same DMA peripheral and stream
#if TOTAL_SPI==3
    if (spiState[HAL_SPI_INTERFACE3].SPI_DMA_Configured)
        HAL_SPI_RX_DMA_Stream_InterruptHandler(HAL_SPI_INTERFACE3);
    else
#endif
        HAL_SPI_RX_DMA_Stream_InterruptHandler(HAL_SPI_INTERFACE2);
}

/**
 * @brief  This function handles DMA2 Stream 2 interrupt request.
 * @param  None
 * @retval None
 */
void DMA2_Stream2_irq_override(void)
{
    HAL_SPI_RX_DMA_Stream_InterruptHandler(HAL_SPI_INTERFACE1);
}

void HAL_SPI_Info(HAL_SPI_Interface spi, hal_spi_info_t* info, void* reserved)
{
    info->system_clock = spi==HAL_SPI_INTERFACE1 ? 60000000 : 30000000;
    if (info->version >= HAL_SPI_INFO_VERSION_1) {
        int32_t state = HAL_disable_irq();
        if (spiState[spi].SPI_Enabled) {
            uint32_t prescaler = (1 << ((spiState[spi].SPI_InitStructure.SPI_BaudRatePrescaler / 8) + 1));
            info->clock = info->system_clock / prescaler;
        } else {
            info->clock = 0;
        }
        info->default_settings = !(spiState[spi].SPI_Clock_Divider_Set || spiState[spi].SPI_Data_Mode_Set || spiState[spi].SPI_Bit_Order_Set);
        info->enabled = spiState[spi].SPI_Enabled;
        info->mode = spiState[spi].mode;
        info->bit_order = spiState[spi].SPI_InitStructure.SPI_FirstBit == SPI_FirstBit_MSB ? MSBFIRST : LSBFIRST;
        info->data_mode = spiState[spi].SPI_InitStructure.SPI_CPOL | spiState[spi].SPI_InitStructure.SPI_CPHA;
        HAL_enable_irq(state);
    }
}

void HAL_SPI_SS_Handler(void *data)
{
    HAL_SPI_Interface spi = (HAL_SPI_Interface)data;
    uint8_t state = !HAL_GPIO_Read(spiState[spi].SPI_SS_Pin);
    spiState[spi].SPI_SS_State = state;
    if (state)
    {
        /* Selected */
        if (spiState[spi].SPI_DMA_Configured)
            SPI_Cmd(spiMap[spi].SPI_Peripheral, ENABLE);
    }
    else
    {
        /* Deselected
         * If there is a pending DMA transfer, it needs to be canceled.
         */
        SPI_Cmd(spiMap[spi].SPI_Peripheral, DISABLE);
        if (spiState[spi].SPI_DMA_Configured)
        {
            HAL_SPI_DMA_Transfer_Cancel(spi);
        }
        else
        {
            /* Just in case clear DR */
            while (SPI_I2S_GetFlagStatus(spiMap[spi].SPI_Peripheral, SPI_I2S_FLAG_RXNE) == SET)
            {
                (void)SPI_I2S_ReceiveData(spiMap[spi].SPI_Peripheral);
            }
        }
    }

    if (spiState[spi].SPI_Select_UserCallback)
        spiState[spi].SPI_Select_UserCallback(state);
}
